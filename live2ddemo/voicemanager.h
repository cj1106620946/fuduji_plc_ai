#pragma once

#include <string>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

class PiperDemo;
class LAppModel;

// 语音管理类
class VoiceManager
{
public:
    VoiceManager();
    ~VoiceManager();

    void start();
    void stop();

    void enqueue(
        const std::string& text,
        const std::string& emotion,
        int priority
    );

    float getVolume() const;
    std::string getCurrentText() const;
    bool isPlaying() const;
    void setModel(LAppModel* m);

private:
    VoiceManager(const VoiceManager&) = delete;
    VoiceManager& operator=(const VoiceManager&) = delete;

private:
    // 队列元素
    struct VoiceItem
    {
        std::string text;
        std::string emotion;
        int priority;
    };

    // 线程主循环
    void threadLoop();

private:
    // 队列与同步
    std::queue<VoiceItem> queue;
    mutable std::mutex mutex;
    std::condition_variable cv;
    LAppModel* model = nullptr;

    // 线程控制
    std::thread worker;
    bool running = false;

    // 当前播放状态
    bool playing = false;
    float currentVolume = 0.0f;
    std::string currentText;

    // 语音生成器
    PiperDemo* piper = nullptr;
};
