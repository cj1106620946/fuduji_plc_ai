#pragma once

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#include "plcclient.h"
#include "aiclient.h"
#include "aicontroller.h"
#include "aitrace.h"

#include "chatai.h"
#include "judgmentai.h"
#include "executeai.h"
#include "workspaceai.h"
#include"jsonstatewriter.h"
#include"speechagent.h"
// AiManager 负责统一调度对话 AI 与后台工作 AI
// 对话线程始终保持流畅，耗时任务全部放入后台线程执行
class AiManager
{
public:
    AiManager();
    ~AiManager();

    // 启动 AI 管理器，进入主循环
    void run();

    // 将用户原始输入推入队列，仅负责入队
    void pushUserInput(const std::string& text);

private:
	// 初始化各类 AI 模块与线程
    void ini();
    // 对话线程主循环，处理用户输入与 Chat / Judgment
    void processLoop();
    // 后台工作线程主循环，处理 Workspace / Execute 等耗时任务
    void processWorkLoop();
    // 处理单条用户输入，立即响应 Chat，并决定是否投递后台任务
    void handleUserInput(const std::string& text);
    // 后台任务完成后，将结果交由 Chat 进行自然回复
    void handleResultInput(const std::string& text);
    // 输入线程入口
    void inputLoop();

    // 文本输入线程
    void textInputLoop();

    // 语音输入线程
    void voiceInputLoop();
    void voicepoploop();
    void voicepushloop();
private:
    // 后台任务描述结构，仅包含最小必要信息
    struct WorkItem
    {
        // 任务类型：1 表示 Execute，2 表示 Workspace
        int type = 0;
        // 原始 UTF-8 文本，作为 AI 输入
        std::string text;
    };
private:
    // PLC 客户端实例
    PLCClient plc;
    // AI 网络或本地调用客户端
    AIClient ai;
    // AI 控制器，负责构建各类 Prompt
    AIController aiController;
    // AI 调用追踪与调试记录
    AITrace aiTrace;
    // 各 AI 模块实例
    ChatAI* chat = nullptr;
    Judgmentai* judgment = nullptr;
    ExecuteAI* execute = nullptr;
    WorkspaceAI* workspace = nullptr;

private:
    // 用户输入队列，仅由对话线程消费
    std::queue<std::string> userQueue;
    // 后台工作任务队列，仅由工作线程消费
    std::queue<WorkItem> workQueue;
    // 后台任务结果队列，用于结果回流
    std::queue<std::string> resultQueue;
    // 输入线程总控
    std::thread inputThread;
    // 文本 / 语音 子线程
    std::thread textThread;
    std::thread voiceThread;
    std::thread voicepush;
    std::thread voicepop;
    speechagent speech;
    bool useVoiceQueue = true;//切换
private:
    // 用户输入队列互斥锁
    std::mutex userMutex;

    // 后台工作队列互斥锁
    std::mutex workMutex;

    // 结果队列互斥锁
    std::mutex resultMutex;

    // 用户输入条件变量，用于唤醒对话线程
    std::condition_variable cv;

    // 后台工作条件变量，用于唤醒工作线程
    std::condition_variable workcv;

private:
    JsonStateWriter live2dWriter;
    // 对话线程对象
    std::thread mainThread;

    // 后台工作线程对象
    std::thread workThread;

    // 运行状态标志，用于安全退出所有线程
    std::atomic<bool> running{ true };
};
