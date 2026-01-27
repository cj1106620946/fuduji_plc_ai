#include "voicemanager.h"
#include "piperdemo.h"
#include <windows.h>
#include <mmsystem.h>
#include "LAppModel.hpp"
#pragma comment(lib, "winmm.lib")

// 构造
VoiceManager::VoiceManager()
{
}

// 析构
VoiceManager::~VoiceManager()
{
    PlaySoundA(NULL, NULL, 0);
    stop();
}

// 启动语音系统
void VoiceManager::start()
{
    std::lock_guard<std::mutex> lock(mutex);
    if (running)
        return;
    running = true;
    // 创建 piper
    piper = new PiperDemo();
    //piper->init("piper"); 
    piper->init("qwen3tts"); 

    worker = std::thread(&VoiceManager::threadLoop, this);
}

// 停止语音系统
void VoiceManager::stop()
{
    PlaySoundA(NULL, NULL, 0);
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (!running)
            return;
        running = false;
    }

    cv.notify_all();

    if (worker.joinable())
        worker.join();

    delete piper;
    piper = nullptr;

    playing = false;
    currentVolume = 0.0f;
    currentText.clear();


}

// 入队语音消息
void VoiceManager::enqueue(
    const std::string& text,
    const std::string& emotion,
    int priority)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        queue.push({ text, emotion, priority });
    }
    cv.notify_one();
}

// 查询当前音量
float VoiceManager::getVolume() const
{
    return currentVolume;
}

// 查询当前文本
std::string VoiceManager::getCurrentText() const
{
    return currentText;
}
void VoiceManager::setModel(LAppModel* m)
{
    model = m;
}

// 是否正在播放
bool VoiceManager::isPlaying() const
{
    return playing;
}
// 线程主循环
void VoiceManager::threadLoop()
{
    while (true)
    {
        VoiceItem item;

        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [&]()
            {
                return !queue.empty() || !running;
            });

            if (!running)
                break;

            item = queue.front();
            queue.pop();
        }

        currentText = item.text;
        playing = true;

        if (piper && piper->generate(item.text))
        {
            PlaySoundA(
                piper->getOutputWav().c_str(),
                NULL,
                SND_FILENAME | SND_ASYNC
            );

            if (model)
            {
                Sleep(200);
                model->StartLipSync(piper->getOutputWav());
            }
        }



        // 播放结束
        playing = false;
        currentVolume = 0.0f;
        currentText.clear();
    }
}
