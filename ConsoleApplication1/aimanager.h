#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

// 前向声明（必须）
class AiManager;
class AiManagerUI;

// UI 线程入口（类外）
void aiuiThreadEntry(AiManager* mgr);

#include "plcclient.h"
#include "aiclient.h"
#include "aicontroller.h"
#include "aitrace.h"

#include "chatai.h"
#include "judgmentai.h"
#include "executeai.h"
#include "workspaceai.h"
#include "jsonstatewriter.h"
#include "speechagent.h"
#include "aimanagerui.h"

class AiManager
{
public:
    AiManager();
    ~AiManager();

    // 启动管理器
    void run();

    // 推送用户输入
    void pushUserInput(const std::string& text);

    // UI（附属）
    AiManagerUI* ui = nullptr;

private:
    // 初始化
    void iniwrite();
    void iniread();
    // 主逻辑线程
    void processLoop();
    void processWorkLoop();

    // 输入相关
    void inputLoop();
    void textInputLoop();
    void voiceInputLoop();
    void voicepushloop();
    void voicepoploop();

    // 处理逻辑
    void handleUserInput(const std::string& text);
    void handleResultInput(const std::string& text);

private:
    struct WorkItem
    {
        int type = 0;
        std::string text;
    };

private:
    // 核心模块
    PLCClient plc;
    AIClient ai;
    AIController aiController;
    AITrace aiTrace;

    ChatAI* chat = nullptr;
    Judgmentai* judgment = nullptr;
    ExecuteAI* execute = nullptr;
    WorkspaceAI* workspace = nullptr;

private:
    // 状态输出
    JsonStateWriter live2dWriter;

private:
    // 队列
    std::queue<std::string> userQueue;
    std::queue<WorkItem> workQueue;
    std::queue<std::string> resultQueue;

private:
    // 线程
    std::thread uiThread;
    std::thread inputThread;
    std::thread textThread;
    std::thread voiceThread;
    std::thread voicepush;
    std::thread voicepop;
    std::thread mainThread;
    std::thread workThread;

private:
    // 同步
    std::mutex userMutex;
    std::mutex workMutex;
    std::mutex resultMutex;
    std::condition_variable cv;
    std::condition_variable workcv;
private:
   //
    bool uvq = true;//选择语言模式，默认队列
	bool vie = true;//启用语音输入
	int ai_mode = 2;//ai模式，默认本地聊天
private:
    // 其他
    speechagent speech;
    std::atomic<bool> running{ true };
};
