#include "aimanager.h"
#include <thread>
#include <cstdlib>
// GBK 直接输出到控制台
void printGBK1(const std::string& text)
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
// UTF-8 转宽字符后输出到控制台
void printUTF81(const std::string& text)
{
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (wlen <= 0)
        return;

    std::wstring wbuf(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wbuf[0], wlen);

    DWORD w;
    WriteConsoleW(
        GetStdHandle(STD_OUTPUT_HANDLE),
        wbuf.c_str(),
        (DWORD)wbuf.size(),
        &w,
        NULL
    );
}
// 将控制台输入的 GBK 字符串转换为 UTF-8，供 AI 使用
std::string GBKtoUTF81(const std::string& gbk)
{
    // GBK → UTF-16
    int wlen = MultiByteToWideChar(936, 0, gbk.c_str(), -1, NULL, 0);
    std::wstring wbuf;
    wbuf.resize(wlen);
    MultiByteToWideChar(936, 0, gbk.c_str(), -1, &wbuf[0], wlen);

    // UTF-16 → UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, NULL, 0, NULL, NULL);
    std::string utf8;
    utf8.resize(u8len);
    WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, &utf8[0], u8len, NULL, NULL);

    return utf8;
}

// 构造函数：初始化各类 AI，对话线程与工作线程稍后启动
AiManager::AiManager()
    : aiController(ai)
{
    chat = new ChatAI(2, aiController, aiTrace);
    judgment = new Judgmentai(2, aiController, aiTrace);
    execute = new ExecuteAI(2, aiController, aiTrace, plc);
    workspace = new WorkspaceAI(2, aiController, aiTrace);
}

// 析构函数：安全关闭线程并释放资源
AiManager::~AiManager()
{
    // 通知所有线程退出
    running = false;
    cv.notify_all();
    workcv.notify_all();
    // 等待线程结束，避免访问已释放对象
    if (mainThread.joinable())
        mainThread.join();
    if (workThread.joinable())
        workThread.join();
    if (inputThread.joinable())
        inputThread.join();

    delete chat;
    delete judgment;
    delete execute;
    delete workspace;
}

// 将用户原始输入推入队列，仅负责入队，不做任何 AI 处理
void AiManager::pushUserInput(const std::string& text)
{
    {
        std::lock_guard<std::mutex> lock(userMutex);
        userQueue.push(text);
    }
    cv.notify_one();
}
void AiManager::ini()
{
    printGBK1("\n正在初始化，请等待");
    ai.setAPIKey("sk-358db553de8a4b1d867be");
    speech.setDebug(0);
    workspace->loadFromFile("workspace.json");
	chat->runOnce(workspace->getWorkspaceJson());
	// 输入线程：负责读取控制台输入
    inputThread = std::thread(&AiManager::inputLoop, this);
    // 对话线程：处理用户输入、Chat、Judgment
    mainThread = std::thread(&AiManager::processLoop, this);
    // 工作线程：专门执行 Workspace / Execute 等耗时任务
    workThread = std::thread(&AiManager::processWorkLoop, this);
    printGBK1("\n初始化完成\n");
    printGBK1("输入：");
}
// 主运行入口，启动对话线程与后台工作线程
void AiManager::run()
{  
    ini();
    while (running)
    {
        ;
    }
}
// 输入线程入口，启动文本与语音输入子线程
void AiManager::inputLoop()
{
     // true = push/pop，false = getText
    // 文本线程始终存在
    textThread = std::thread(&AiManager::textInputLoop, this);

    if (useVoiceQueue)
    {
        // 队列模式：两个语音线程
        voicepush = std::thread(&AiManager::voicepushloop, this);
        voicepop = std::thread(&AiManager::voicepoploop, this);
    }
    else
    {
        // 阻塞模式：一个语音线程
        voiceThread = std::thread(&AiManager::voiceInputLoop, this);
    }

    // ===== 等待退出 =====
    if (textThread.joinable())
        textThread.join();

    if (useVoiceQueue)
    {
        if (voicepush.joinable())
            voicepush.join();
        if (voicepop.joinable())
            voicepop.join();
    }
    else
    {
        if (voiceThread.joinable())
            voiceThread.join();
    }
}

// 文本输入线程循环
void AiManager::textInputLoop()
{
    while (running)
    {
        std::string input;
        std::getline(std::cin, input);

        if (!running)
            break;
        if (input == "break0")
        {
            running = false;
            cv.notify_all();
            workcv.notify_all();
            break;
        }

        pushUserInput(GBKtoUTF81(input));
    }
}
// 语音输入线程循环
void AiManager::voiceInputLoop()
{
    // 启动语音识别
    if (!speech.start())
    {
        // 语音识别启动失败，只退出语音线程
        return;
    }

    speech.setDebug(0);

    while (running)
    {
        // 获取语音识别文本
        std::string text = speech.getText();
        // nosl：当前没有形成完整语句
        if (text == "nosl")
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        // 只要有一句完整文本，就交给 AI 系统
        printUTF81(u8"\n[Speech] 识别结果：");
        printUTF81(text);
        printUTF81("\n");
        pushUserInput(text);
    }

    // 线程结束时，关闭语音识别
    speech.stop();
}
//入队
void AiManager::voicepushloop()
{
    if (!speech.start())
    {
        return;
    }


    while (running)
    {
        // 阻塞等待一句完整语音
        bool ok = speech.pushtext();
        if (!ok)
        {
            // 没有形成完整语音，稍微让出 CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        // 成功入队后立刻继续下一次录制
    }

    speech.stop();
}
//出队
void AiManager::voicepoploop()
{
    // 注意：这里不需要 start()
    // poptext 只用 whisper，不碰麦克风
    while (running)
    {
        std::string text = speech.poptext();
        if (text.empty())
        {
            // 队列暂时没内容，避免空转
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        printUTF81(u8"\n[Speech] 识别结果：");
        printUTF81(text);
        printUTF81("\n");
        pushUserInput(text);
    }
}


// 对话线程循环：串行处理用户输入，保证 Chat 连续性
void AiManager::processLoop()
{
    while (running)
    {
        std::unique_lock<std::mutex> lock(userMutex);
        cv.wait(lock, [&]() {
            return !userQueue.empty() || !running;
        });
        if (!running)
            break;
        std::string rawText = userQueue.front();
        userQueue.pop();
        lock.unlock();
        handleUserInput(rawText);
    }
}
// 后台工作线程：专门处理耗时的 Workspace / Execute
void AiManager::processWorkLoop()
{
    // 工作线程主循环
    while (running)
    {
        // 等待工作队列中有任务，或系统退出
        std::unique_lock<std::mutex> lock(workMutex);
        workcv.wait(lock, [&]() {
            return !workQueue.empty() || !running;
        });
        if (!running)
            break;
        // 取出一个工作任务
        WorkItem item = workQueue.front();
        workQueue.pop();
        lock.unlock();
        std::string resultText;

        // 根据任务类型调用对应的功能 AI
        if (item.type == 1)
        {
            resultText = execute->runOnce(item.text);
        }
        /*
        else if (item.type == 2)
        {
            // 工作区建模任务
            bool ok = workspace->runOnce(item.text);

            if (ok)
            {
                // 工作区成功时，将 Workspace JSON 作为结果传递
                resultText = workspace->getWorkspaceJson();
            }
            else
            {
                // 工作区未完成时，将 AI 原始输出传递
                // 可能包含 WS:ERR / NEED 等结构化信息
                resultText = workspace->getAiRawOutput();
            }
        }*/
        else
        {
            // 未知任务类型，直接忽略
            continue;
        }

        // 将工作结果放入结果队列
        // 结果队列中的内容不是用户输入，而是系统状态文本
        {
            std::lock_guard<std::mutex> rlock(resultMutex);
            resultQueue.push(resultText);
        }
        handleResultInput(resultText);
    }
}
// 处理一条用户输入：Chat 立即回应，Judgment 决定是否投递后台任务
void AiManager::handleUserInput(const std::string& text)
{
    if (text.empty())
        return;
    // Chat 永远优先执行，保证用户即时反馈
    std::string chatReply = chat->runOnce(text);
    printGBK1("[Chat]\n");
    printUTF81(chatReply);
    printGBK1("\n\n");
    // 判决 AI 只负责分类，不执行任务
    std::string decisionText = judgment->runOnce(text);
    int decisionValue = std::atoi(decisionText.c_str());
    if (decisionValue == 0)
        return;
    // 将耗时任务投递到后台线程
    {
        std::lock_guard<std::mutex> lock(workMutex);
        WorkItem item;
        item.type = decisionValue;
        item.text = text;
        workQueue.push(item);
    }
    workcv.notify_one();
}
// 工作完成后，由 Chat 统一对用户进行自然回复
void AiManager::handleResultInput(const std::string& text)
{
    std::string reply = chat->runOnce(text);
    printGBK1("[Result]\n");
    printUTF81(reply);
    printGBK1("\n\n");
}
