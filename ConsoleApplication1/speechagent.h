// speechagent.h
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <mutex>

// speechagent
// 提供三套并行能力：
// 1 getText：一体式 语音→文本（独立存在）
// 2 push：只录制一句语音，放入队列
// 3 pop：取出一条语音并进行处理
class speechagent
{
public:
    struct SpeechParams
    {
        int device_index = 1;
        // 麦克风设备选择 0 表示系统默认 1 及以上表示其它输入设备序号

        int channels = 1;
        // 采集声道数 1 表示单声道 2 表示立体声 推荐 1

        int sample_rate = 48000;
        // 采集采样率 单位 Hz 例如 48000 Whisper 推理前会降采样到 16000

        float vad_threshold = 0.01f;
        // 静音判定阈值 RMS 小于该值就算静音 越大越不容易触发说话

        int start_hit = 1;
        // 开始说话确认次数 连续多少次满足开始条件才进入说话状态 单位 次

        float start_rms_min = 0.08f;
        // 开始说话最低强度 RMS 必须大于等于这个值才允许触发开始说话

        int end_silence_ms = 200;
        // 结束说话判定静音时长 单位 ms 进入说话状态后 连续静音达到该值就结束

        int check_ms = 10;
        // 检测间隔 单位 ms 越小反应越快 CPU 越高

        int max_record_ms = 8000;
        // 最长等待时间 单位 ms 一直等不到说话就触发超时结束

        int debug_rms_print_ms = 500;
        // 调试 RMS 打印间隔 单位 ms 只控制打印频率 不影响判定
    };
    // 生命周期
    bool start();
    void stop();

    // 一体调用接口（与 push / pop 无关）
    std::string getText();
    bool pushtext();
    std::string poptext();

    // 调试控制
    static void setDebug(bool enable);
    static bool getDebug();
};
