// speechagent.cpp
#define NOMINMAX
#include <windows.h>
#include <portaudio.h>
#include <whisper.h>
#include "speechagent.h"
#include <vector>
#include <mutex>
#include <thread>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

// 控制台输出
static void printGBK1(const std::string& text)
{
    DWORD w;
    WriteConsoleA(
        GetStdHandle(STD_OUTPUT_HANDLE),
        text.c_str(),
        (DWORD)text.size(),
        &w,
        NULL
    );
}

static void printUTF81(const std::string& text)
{
    int wlen = MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        NULL,
        0
    );
    if (wlen <= 0) return;

    std::wstring wbuf((size_t)wlen, 0);
    MultiByteToWideChar(
        CP_UTF8,
        0,
        text.c_str(),
        -1,
        &wbuf[0],
        wlen
    );

    DWORD w;
    WriteConsoleW(
        GetStdHandle(STD_OUTPUT_HANDLE),
        wbuf.c_str(),
        (DWORD)wbuf.size(),
        &w,
        NULL
    );
}

// 调试开关
static bool g_debug = true;
void speechagent::setDebug(bool enable)
{
    g_debug = enable;
}
bool speechagent::getDebug()
{
    return g_debug;
}

// 全局状态
static speechagent::SpeechParams speechParams;
static std::vector<float> audioBuffer48k;
static std::mutex audioBufferMutex;
static std::deque<std::vector<float>> audioQueue;
static std::mutex audioQueueMutex;

static whisper_context* whisperContext = nullptr;
static PaStream* audioInputStream = nullptr;

// ini 配置读取
static void load_config(const char* path)
{
    std::ifstream in(path);
    if (!in.is_open())
        return;

    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty()) continue;
        if (line[0] == '[') continue;

        auto pos = line.find('=');
        if (pos == std::string::npos) continue;

        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);

        auto trim = [](std::string& s)
        {
            while (!s.empty() && (s.front() == ' ' || s.front() == '\t'))
                s.erase(s.begin());
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t'))
                s.pop_back();
        };

        trim(key);
        trim(val);

        if (key == "device_index") speechParams.device_index = std::stoi(val);
        else if (key == "channels") speechParams.channels = std::stoi(val);
        else if (key == "sample_rate") speechParams.sample_rate = std::stoi(val);
        else if (key == "vad_threshold") speechParams.vad_threshold = std::stof(val);
        else if (key == "start_hit") speechParams.start_hit = std::stoi(val);
        else if (key == "start_rms_min") speechParams.start_rms_min = std::stof(val);
        else if (key == "end_silence_ms") speechParams.end_silence_ms = std::stoi(val);
        else if (key == "check_ms") speechParams.check_ms = std::stoi(val);
        else if (key == "max_record_ms") speechParams.max_record_ms = std::stoi(val);
        else if (key == "debug_rms_print_ms") speechParams.debug_rms_print_ms = std::stoi(val);
        else if (key == "debug") g_debug = (std::stoi(val) != 0);
    }
}

// PortAudio 回调
static int audioCallback(
    const void* input,
    void*,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void*)
{
    if (!input) return paContinue;

    const float* in = (const float*)input;
    std::lock_guard<std::mutex> lock(audioBufferMutex);

    if (speechParams.channels == 1)
    {
        for (unsigned long i = 0; i < frameCount; ++i)
            audioBuffer48k.push_back(in[i]);
    }
    else
    {
        for (unsigned long i = 0; i < frameCount; ++i)
        {
            float mono = 0.5f * (in[i * 2] + in[i * 2 + 1]);
            audioBuffer48k.push_back(mono);
        }
    }
    return paContinue;
}

// 工具函数
static float calc_rms(const std::vector<float>& v)
{
    if (v.empty()) return 0.0f;

    double s = 0.0;
    for (float x : v) s += (double)x * (double)x;
    return (float)std::sqrt(s / (double)v.size());
}

static void downsample_48k_to_16k(
    const std::vector<float>& in,
    std::vector<float>& out)
{
    out.clear();
    for (size_t i = 0; i + 2 < in.size(); i += 3)
        out.push_back((in[i] + in[i + 1] + in[i + 2]) / 3.0f);
}

// 启动
bool speechagent::start()
{
    load_config("speechagent.cfg");

    whisper_context_params cparams =
        whisper_context_default_params();

    whisperContext =
        whisper_init_from_file_with_params(
            "ggml-small.bin",
            cparams
        );

    if (!whisperContext)
        return false;

    if (Pa_Initialize() != paNoError)
        return false;

    PaStreamParameters in{};
    in.device = speechParams.device_index;
    in.channelCount = speechParams.channels;
    in.sampleFormat = paFloat32;

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(in.device);
    if (!devInfo)
        return false;

    in.suggestedLatency = devInfo->defaultLowInputLatency;

    if (Pa_OpenStream(
        &audioInputStream,
        &in,
        nullptr,
        speechParams.sample_rate,
        480,
        paClipOff,
        audioCallback,
        nullptr) != paNoError)
        return false;

    Pa_StartStream(audioInputStream);

    if (g_debug)
        printUTF81(u8"[语音] speechagent 已启动\n");

    return true;
}
// 录取音频进入队列（带 debug）
bool speechagent::pushtext()
{
    {
        std::lock_guard<std::mutex> lock(audioBufferMutex);
        audioBuffer48k.clear();
    }

    bool isRecordingSpeech = false;
    int speechStartConfirmCount = 0;
    int silenceDurationMs = 0;
    size_t speechStartSampleIndex = 0;

    DWORD t_base = GetTickCount();
    DWORD last_rms_print = 0;

    auto now_sec = [&]() -> double
    {
        return (GetTickCount() - t_base) / 1000.0;
    };

    if (g_debug)
    {
        std::ostringstream oss;
        oss << u8"[" << now_sec() << u8"s][语音] pushtext 等待说话\n";
        printUTF81(oss.str());
    }

    while (true)
    {
        Sleep(speechParams.check_ms);

        DWORD now = GetTickCount();

        if (!isRecordingSpeech &&
            now - t_base > (DWORD)speechParams.max_record_ms)
        {
            if (g_debug)
            {
                std::ostringstream oss;
                oss << u8"[" << now_sec() << u8"s][语音] pushtext 超时未检测到说话\n";
                printUTF81(oss.str());
            }
            return false;
        }

        std::vector<float> chunk;
        {
            std::lock_guard<std::mutex> lock(audioBufferMutex);
            size_t need =
                (size_t)speechParams.sample_rate *
                (size_t)speechParams.check_ms / 1000;

            if (audioBuffer48k.size() < need)
                continue;

            chunk.assign(
                audioBuffer48k.end() - need,
                audioBuffer48k.end()
            );
        }

        float rmsValue = calc_rms(chunk);

        if (g_debug && now - last_rms_print >= (DWORD)speechParams.debug_rms_print_ms)
        {
            std::ostringstream oss;
            oss << u8"[" << now_sec() << u8"s][语音] RMS="
                << rmsValue
                << u8" threshold=" << speechParams.vad_threshold
                << u8" start_min=" << speechParams.start_rms_min
                << u8"\n";
            printUTF81(oss.str());
            last_rms_print = now;
        }

        if (!isRecordingSpeech)
        {
            if (rmsValue > speechParams.vad_threshold &&
                rmsValue >= speechParams.start_rms_min)
            {
                speechStartConfirmCount++;

                if (g_debug)
                {
                    std::ostringstream oss;
                    oss << u8"[" << now_sec() << u8"s][语音] 开始命中 "
                        << speechStartConfirmCount
                        << u8"/"
                        << speechParams.start_hit
                        << u8"\n";
                    printUTF81(oss.str());
                }

                if (speechStartConfirmCount >= speechParams.start_hit)
                {
                    isRecordingSpeech = true;
                    silenceDurationMs = 0;

                    {
                        std::lock_guard<std::mutex> lock(audioBufferMutex);
                        speechStartSampleIndex = audioBuffer48k.size();
                    }

                    if (g_debug)
                    {
                        std::ostringstream oss;
                        oss << u8"[" << now_sec() << u8"s][语音] pushtext 判定开始说话\n";
                        printUTF81(oss.str());
                    }
                }
            }
            else
            {
                speechStartConfirmCount = 0;
            }
        }
        else
        {
            if (rmsValue < speechParams.vad_threshold)
                silenceDurationMs += speechParams.check_ms;
            else
                silenceDurationMs = 0;

            if (g_debug)
            {
                std::ostringstream oss;
                oss << u8"[" << now_sec() << u8"s][语音] 静音累计 "
                    << silenceDurationMs
                    << u8" ms\n";
                printUTF81(oss.str());
            }

            if (silenceDurationMs >= speechParams.end_silence_ms)
            {
                if (g_debug)
                {
                    std::ostringstream oss;
                    oss << u8"[" << now_sec() << u8"s][语音] pushtext 判定语音结束，入队\n";
                    printUTF81(oss.str());
                }

                std::vector<float> pcm48k;
                {
                    std::lock_guard<std::mutex> lock(audioBufferMutex);
                    if (speechStartSampleIndex < audioBuffer48k.size())
                        pcm48k.assign(
                            audioBuffer48k.begin() + speechStartSampleIndex,
                            audioBuffer48k.end()
                        );
                }

                std::vector<float> pcm16k;
                downsample_48k_to_16k(pcm48k, pcm16k);

                if (pcm16k.empty())
                    return false;

                {
                    std::lock_guard<std::mutex> qlock(audioQueueMutex);
                    audioQueue.push_back(std::move(pcm16k));
                }

                return true;
            }
        }
    }
}


// 弹出并处理一条音频，返回文本
std::string speechagent::poptext()
{
    std::vector<float> pcm16k;

    // 从音频队列中取出一条音频
    {
        std::lock_guard<std::mutex> lock(audioQueueMutex);
        if (audioQueue.empty())
            return std::string();

        pcm16k = std::move(audioQueue.front());
        audioQueue.pop_front();
    }

    if (pcm16k.empty())
        return std::string();

    // whisper 参数
    whisper_full_params wparams =
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.language = "auto";
    wparams.temperature = 0.0f;
    wparams.suppress_blank = true;

    unsigned int hc = std::thread::hardware_concurrency();
    if (hc == 0) hc = 4;
    wparams.n_threads = (int)((std::max)(2u, hc / 2));

    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_timestamps = false;

    // 执行识别
    whisper_full(
        whisperContext,
        wparams,
        pcm16k.data(),
        (int)pcm16k.size()
    );

    // 只取第一个 segment
    int n = whisper_full_n_segments(whisperContext);
    if (n <= 0)
        return std::string();

    const char* t = whisper_full_get_segment_text(whisperContext, 0);
    if (!t)
        return std::string();

    return std::string(t);
}

// 阻塞获取文本
std::string speechagent::getText()
{
    {
        std::lock_guard<std::mutex> lock(audioBufferMutex);
        audioBuffer48k.clear();
    }

    bool isRecordingSpeech = false;
    bool hasDetectedSpeech = false;
    int speechStartConfirmCount = 0;
    int silenceDurationMs = 0;
    size_t speechStartSampleIndex = 0;

    DWORD t_base = GetTickCount();
    DWORD last_rms_print = 0;

    auto now_sec = [&]() -> double
    {
        return (GetTickCount() - t_base) / 1000.0;
    };

    whisper_full_params wparams =
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.language = "auto";
    wparams.temperature = 0.0f;
    wparams.suppress_blank = true;

    unsigned int hc = std::thread::hardware_concurrency();
    if (hc == 0) hc = 4;
    wparams.n_threads = (int)((std::max)(2u, hc / 2));

    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_timestamps = false;

    if (g_debug)
    {
        std::ostringstream oss;
        oss << "[" << std::fixed << std::setprecision(3)
            << now_sec()
            << "s][" << u8"语音" << "] " << u8"等待用户说话\n";
        printUTF81(oss.str());
    }

    while (true)
    {
        Sleep(speechParams.check_ms);

        DWORD now = GetTickCount();

        if (!isRecordingSpeech &&
            now - t_base > (DWORD)speechParams.max_record_ms)
        {
            if (g_debug)
            {
                std::ostringstream oss;
                oss << "[" << now_sec()
                    << "s][" << u8"语音" << "] " << u8"超时且未检测到说话，直接返回\n";
                printUTF81(oss.str());
            }

            return "nosl";
        }

        std::vector<float> chunk;
        {
            std::lock_guard<std::mutex> lock(audioBufferMutex);
            size_t need =
                (size_t)speechParams.sample_rate *
                (size_t)speechParams.check_ms / 1000;

            if (audioBuffer48k.size() < need)
                continue;

            chunk.assign(
                audioBuffer48k.end() - need,
                audioBuffer48k.end()
            );
        }

        float rmsValue = calc_rms(chunk);

        if (g_debug && (now - last_rms_print >= (DWORD)speechParams.debug_rms_print_ms))
        {
            std::ostringstream oss;
            oss << "[" << now_sec()
                << "s][" << u8"语音" << "] " << u8"当前RMS=" << rmsValue
                << " threshold=" << speechParams.vad_threshold
                << " start_min=" << speechParams.start_rms_min
                << "\n";
            printUTF81(oss.str());
            last_rms_print = now;
        }

        if (!isRecordingSpeech)
        {
            if (rmsValue > speechParams.vad_threshold &&
                rmsValue >= speechParams.start_rms_min)
            {
                speechStartConfirmCount++;

                if (g_debug)
                {
                    std::ostringstream oss;
                    oss << "[" << now_sec()
                        << "s][" << u8"语音" << "] " << u8"开始命中 "
                        << speechStartConfirmCount << "/"
                        << speechParams.start_hit
                        << " RMS=" << rmsValue
                        << "\n";
                    printUTF81(oss.str());
                }

                if (speechStartConfirmCount >= speechParams.start_hit)
                {
                    isRecordingSpeech = true;
                    hasDetectedSpeech = true;
                    silenceDurationMs = 0;

                    {
                        std::lock_guard<std::mutex> lock(audioBufferMutex);
                        speechStartSampleIndex = audioBuffer48k.size();
                    }

                    if (g_debug)
                    {
                        std::ostringstream oss;
                        oss << "[" << now_sec() << "s][" << u8"语音" << "] " << u8"检测到开始说话 ";
                        oss << "RMS=" << rmsValue << " ";
                        oss << "threshold=" << speechParams.vad_threshold << " ";
                        oss << "start_min=" << speechParams.start_rms_min << " ";
                        oss << "hit=" << speechParams.start_hit;
                        oss << "\n";
                        printUTF81(oss.str());
                    }
                }
            }
            else
            {
                if (speechStartConfirmCount != 0 && g_debug)
                {
                    std::ostringstream oss;
                    oss << "[" << now_sec()
                        << "s][" << u8"语音" << "] " << u8"开始命中清零 ";
                    oss << "RMS=" << rmsValue;
                    oss << "\n";
                    printUTF81(oss.str());
                }
                speechStartConfirmCount = 0;
            }
        }
        else
        {
            if (rmsValue < speechParams.vad_threshold)
            {
                silenceDurationMs += speechParams.check_ms;

                if (g_debug)
                {
                    std::ostringstream oss;
                    oss << "[" << now_sec()
                        << "s][" << u8"语音" << "] " << u8"静音累计 "
                        << silenceDurationMs
                        << u8" ms"
                        << " RMS=" << rmsValue
                        << "\n";
                    printUTF81(oss.str());
                }
            }
            else
            {
                silenceDurationMs = 0;
            }

            if (silenceDurationMs >= speechParams.end_silence_ms)
            {
                if (g_debug)
                {
                    std::ostringstream oss;
                    oss << "[" << now_sec()
                        << "s][" << u8"语音" << "] " << u8"判定说话结束，开始识别\n";
                    printUTF81(oss.str());
                }

                std::vector<float> pcm48k;
                {
                    std::lock_guard<std::mutex> lock(audioBufferMutex);
                    if (speechStartSampleIndex < audioBuffer48k.size())
                        pcm48k.assign(
                            audioBuffer48k.begin() + speechStartSampleIndex,
                            audioBuffer48k.end()
                        );
                }

                std::vector<float> pcm16k;
                downsample_48k_to_16k(pcm48k, pcm16k);

                if (g_debug)
                {
                    std::ostringstream oss;
                    oss << "[" << now_sec()
                        << "s][" << u8"语音" << "] " << u8"Whisper 开始处理\n";
                    printUTF81(oss.str());
                }

                DWORD t0 = GetTickCount();
                std::string result;

                if (!pcm16k.empty())
                {
                    whisper_full(
                        whisperContext,
                        wparams,
                        pcm16k.data(),
                        (int)pcm16k.size()
                    );

                    int n = whisper_full_n_segments(whisperContext);
                    for (int i = 0; i < n; ++i)
                    {
                        const char* t =
                            whisper_full_get_segment_text(
                                whisperContext, i);
                        if (t) result += t;
                    }
                }

                if (g_debug)
                {
                    double cost =
                        (GetTickCount() - t0) / 1000.0;

                    std::ostringstream oss;
                    oss << "[" << now_sec()
                        << "s][" << u8"语音" << "] " << u8"Whisper 结束，耗时 "
                        << cost << u8" 秒\n";
                    printUTF81(oss.str());
                }

                return result;
            }
        }
    }
}

// 停止
void speechagent::stop()
{
    if (audioInputStream)
    {
        Pa_StopStream(audioInputStream);
        Pa_CloseStream(audioInputStream);
        audioInputStream = nullptr;
    }

    Pa_Terminate();

    if (whisperContext)
    {
        whisper_free(whisperContext);
        whisperContext = nullptr;
    }
    if (g_debug)
        printUTF81(u8"[语音] speechagent 已停止\n");
}
