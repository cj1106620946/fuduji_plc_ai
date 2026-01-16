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
    ,   live2dWriter("live2dstate.json")
{
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
    speech.stop();
    delete ui;
    delete chat;
    delete judgment;
    delete execute;
    delete workspace;
}
void uiThread(AiManager* mgr)
{
    mgr->ui = new AiManagerUI();

    if (!mgr->ui->create(GetModuleHandleW(nullptr)))
        return;

    mgr->ui->show();
    mgr->ui->loop();
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

void AiManager::iniaiwrite()
{
    while (true)
    {
        printGBK1("正在初始化写入模块...\n");
        printGBK1(
            "请选择 AI 模式：\n"
            "  1 云端模式（DeepSeek API，需要 API Key）\n"
            "    - 稳定可用，无需本地部署\n"
            "  2 本地模式（Ollama + qwen2.5 模型）\n"
            "    - 需要提前部署并启动 Ollama\n"
        );

        std::cin >> ai_mode;
        std::cin.ignore();

        if (ai_mode == 1)
        {
            // 云端模式
            printGBK1("当前为云端模式，请输入 AI Key：\n");

            std::string key;
            std::getline(std::cin, key);
            ai.setAPIKey(key);

            judgment = new Judgmentai(1, aiController, aiTrace);

            std::string result = judgment->runOnce("");

            if (result == "0" || result == "1")
            {
                printUTF81(result);
                printGBK1("云端 AI 校验成功。\n");
                break;
            }
            else
            {
                printGBK1("云端 API 错误，请重新选择模式。\n");
                delete judgment;
                judgment = nullptr;
                continue;
            }
        }
        else
        {
            // 本地模式
            ai_mode = 2;
            printGBK1("当前为本地模式，正在检测本地 AI...\n");

            judgment = new Judgmentai(2, aiController, aiTrace);

            std::string result = judgment->runOnce("");

            if (result == "0" || result == "1")
            {
                printUTF81(result);
                printGBK1("本地 AI 校验成功。\n");
                break;
            }
            else
            {
                printGBK1("本地 AI 未部署或模型不可用，请重新选择模式。\n");
                delete judgment;
                judgment = nullptr;
                continue;
            }
        }
    }

    printGBK1("ai设置初始化完成。\n");
}
void AiManager::inimodwrite()
{
    printGBK1("模块初始化配置中...\n");
    printGBK1("是否启用语音功能：1 启用，0 不启用\n");

    int value = 0;
    std::cin >> value;
    std::cin.ignore();

    if (value == 1)
    {
        envoice = true;
        printGBK1("语音功能已启用。\n");

        // 仅在启用语音后，询问语音工作模式
        printGBK1("请选择语音工作模式：\n");
        printGBK1("  1 队列模式（推送 / 消费 分离）\n");
        printGBK1("  2 阻塞模式（单线程语音）\n");

        int mode = 0;
        std::cin >> mode;
        std::cin.ignore();

        if (mode == 1)
        {
            uvq = true;
            printGBK1("已选择语音队列模式。\n");
        }
        else
        {
            uvq = false;
            printGBK1("已选择语音阻塞模式。\n");
        }
        //启动语言
        if (!speech.start())
        {
            printUTF81(u8"[Speech] 语音系统启动失败\n");
            running = false;
            return;
        }
        speech.setDebug(0);

    }
    else
    {
        envoice = false;
        uvq = false;
        printGBK1("语音功能未启用，仅使用文本模式。\n");
    }
}


void AiManager::iniread()
{
   /* HANDLE hEvent = CreateEventW(
        nullptr,          // 默认安全属性
        TRUE,             // 手动复位
        FALSE,            // 初始未触发
        L"Global\\AIMANAGER_UI_READY"
    );

    if (hEvent)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
    uiThread = std::thread(::uiThread, this);
    */
    printGBK1("\n正在初始化，请等待\n");
    //ai初始化
    chat = new ChatAI(ai_mode, aiController, aiTrace);
    execute = new ExecuteAI(ai_mode, aiController, aiTrace, plc);
    workspace = new WorkspaceAI(ai_mode, aiController, aiTrace);
    printGBK1(".........ai初始化完成\n");
    //模型动作输入初始化
    live2dWriter.init();
    //读取当前工作区并写入
    workspace->loadFromFile("workspace.json");
	chat->runExecuteRead(workspace->getWorkspaceJson());
    printGBK1(".........ai读取工作区和记忆完成\n");
	// 输入线程：负责读取控制台输入
    inputThread = std::thread(&AiManager::inputLoop, this);
    // 对话线程：处理用户输入、Chat、Judgment
    mainThread = std::thread(&AiManager::processLoop, this);
    // 工作线程：专门执行 Workspace / Execute 等耗时任务
    workThread = std::thread(&AiManager::processWorkLoop, this);
    printGBK1(".........线程初始化完成\n");

    printGBK1("\n初始化完成\n");
    printGBK1("输入：");
}
// 主运行入口，启动对话线程与后台工作线程
void AiManager::run()
{  
    iniaiwrite();
    inimodwrite();
    iniread();
    while (running)
    {
        ;
    }
}
//线程大军
// 输入线程入口，启动文本与语音输入子线程
void AiManager::inputLoop()
{
    // 文本线程始终存在
    textThread = std::thread(&AiManager::textInputLoop, this);
    if(envoice)
    { 
        if (uvq)
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
    }
    if (envoice)
    {
        if (textThread.joinable())
            textThread.join();
        if (uvq)
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
}
//入队
void AiManager::voicepushloop()
{
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
}
//出队
void AiManager::voicepoploop()
{
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
    while (running)
    {
        std::unique_lock<std::mutex> lock(workMutex);
        workcv.wait(lock, [&]() {
            return !workQueue.empty() || !running;
        });

        if (!running)
            break;
        WorkItem item = workQueue.front();
        workQueue.pop();
        lock.unlock();
        std::string resultText;
        // 任务类型：1 = Execute
        if (item.type == 1)
        {
            resultText = execute->runOnce(item.text);
        }
        // 任务类型：2 = Workspace（以后打开）
        /*
        else if (item.type == 2)
        {
            bool ok = workspace->runOnce(item.text);
            if (ok)
                resultText = workspace->getWorkspaceJson();
            else
                resultText = workspace->getAiRawOutput();
        }
        */
        else
        {
            continue;
        }

        {
            std::lock_guard<std::mutex> rlock(resultMutex);
            resultQueue.push(resultText);
        }

        handleResultInput(resultText);
    }
}
// 处理一条用户输入：单一 Chat 决策（control），决定是否投递后台任务
void AiManager::handleUserInput(const std::string& text)
{
    if (text.empty())
        return;
    std::string chatReply = chat->runExecuteRead(text);
    std::string writetext = chat->getText();
    std::string emotion = chat->getEmotion();
    int priority = chat->getPriority();
    live2dWriter.write(writetext,emotion, priority);
    printGBK1("\n");
    printUTF81(chatReply);
    printGBK1("\n");

    int control = chat->getControl();
    if (control == 0)
        return;
    if (control == 1)
    {
        std::lock_guard<std::mutex> lock(workMutex);
        WorkItem item;
        item.type = 1;     // Execute
        item.text = text;  // 原始用户输入交给 Execute
        workQueue.push(item);
        workcv.notify_one();
        return;
    }
    // control == 2：当前不投递任务，保持对话即可
    return;
}
// 工作完成后：由 Chat 统一对用户自然回复
void AiManager::handleResultInput(const std::string& text)
{
    std::string reply = chat->runExecuteRead(text);
    std::string writetext = chat->getText();
    std::string emotion = chat->getEmotion();
    int priority = chat->getPriority();
    live2dWriter.write(writetext, emotion, priority);

    printGBK1("\n");
    printUTF81(reply);
    printGBK1("\n");
}
