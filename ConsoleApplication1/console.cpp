#include "console.h"
#include <sstream>
#include <iostream>
#include <json/json.h>
#include "decisionai.h"
#include <conio.h>

// ================================
// 输出函数
// ================================
void Console::printGBK(const std::string& text)
{
    DWORD w;
    WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE),
        text.c_str(),
        (DWORD)text.size(),
        &w, NULL);
}

void Console::printUTF8(const std::string& text)
{
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (wlen <= 0) return;

    std::wstring wbuf(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wbuf[0], wlen);

    DWORD w;
    WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
        wbuf.c_str(),
        (DWORD)wbuf.size(),
        &w, NULL);
}
std::string GBKtoUTF8(const std::string& gbk)
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
bool Console::checkBreak(const std::string& cmd)
{
    return cmd == "break0";
}

// ================================
// 构造 / 析构
// ================================
Console::Console()
    : plc()
    , ai()
    , aiController(plc, ai)
{
    workspace = new WorkspaceManager(aiController);
    chatAgent = new ChatAgent(aiController, *workspace, plc);

    // ⭐ 这里是关键
    decisionAI = new DecisionAI(*chatAgent);
}




Console::~Console()
{
    delete workspace;
    delete decisionAI;
}


// ================================
// 主循环
// ================================
void Console::run()
{
    while (true)
    {
        showMainHeader();
        mainMenu();
    }
}

// ================================
// UI
// ================================
void Console::showMainHeader()
{
    printGBK("\n===================================\n");
    printUTF8(u8"        PLC + AI 控制台\n");
    printGBK("===================================\n");
    printGBK("PLC：");
    printGBK(plc.isConnected() ? "已连接\n" : "未连接\n");
    printGBK("AI Key：");
    printGBK(hasAIKey ? "已设置\n" : "未设置\n");
    printGBK("-----------------------------------\n");
    printGBK("1. 连接 PLC\n");
    printGBK("2. 设置 AI Key\n");
    printGBK("3. 手动 PLC 控制\n");
    printGBK("4. 创建 Workspace\n");
    printGBK("5. AI 对话（键盘）\n");
    printGBK("6. AI 对话（语音）\n");
    printGBK("7. 语音识别测试\n");
    printGBK("8. 决策 AI 测试\n");
    printGBK("0. 退出\n");
    printGBK("===================================\n");
}
void Console::mainMenu()
{
    printGBK("> ");
    std::string cmd;
    std::getline(std::cin, cmd);

    if (cmd == "0") exit(0);
    else if (cmd == "1") menuPLCConnect();
    else if (cmd == "2") menuAIKey();
    else if (cmd == "3") menuPLCManual();
    else if (cmd == "4") menuWorkspace();
    else if (cmd == "5") menuChat();
    else if (cmd == "6") menuVoiceChat();
    else if (cmd == "7") menuSpeechTest();
    else if (cmd == "8") menuDecisionTest();
    else printGBK("无效输入\n");
}

// ================================
// 1. PLC 连接
// ================================
void Console::menuPLCConnect()
{
    printGBK("PLC IP> ");
    std::string ip;
    std::getline(std::cin, ip);

    if (plc.connectPLC(ip, 0, 1))
        printGBK("PLC 连接成功\n");
    else
        printGBK("PLC 连接失败\n");
}

// ================================
// 2. AI Key
// ================================
void Console::menuAIKey()
{
    printGBK("请输入 AI Key：\n");
    std::string key;
    std::getline(std::cin, key);

    ai.setAPIKey(key);
    hasAIKey = true;
    printGBK("AI Key 设置完成\n");
}

// ================================
// 3. PLC 手动
// ================================
void Console::menuPLCManual()
{
    if (!plc.isConnected())
    {
        printGBK("PLC 未连接\n");
        return;
    }
    printGBK("read I0.0 | write Q0.0 1 | break0\n");
    while (true)
    {
        printGBK("plc> ");
        std::string cmd;
        std::getline(std::cin, cmd);

        if (cmd == "break0") return;

        std::stringstream ss(cmd);
        std::string op;
        ss >> op;

        if (op == "read")
        {
            std::string addr;
            ss >> addr;
            int v = 0;
            plc.readAddress(addr, v);
            std::cout << addr << " = " << v << "\n";
        }
        else if (op == "write")
        {
            std::string addr;
            int v;
            ss >> addr >> v;
            plc.writeAddress(addr, v);
            printGBK("写入完成\n");
        }
    }
}

// ================================
// 4. Workspace 创建（最终版）
// ================================
void Console::menuWorkspace()
{
    if (!hasAIKey)
    {
        printGBK("请先设置 AI Key\n");
        return;
    }

    printGBK("\n--- Workspace 创建模式 ---\n");
    printGBK("输入自然语言创建工作区\n");
    printGBK("输入 break0 返回主菜单\n");

    while (true)
    {
        printGBK("ws> ");

        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // 转 UTF-8
        std::string utf8Input = GBKtoUTF8(input);

        printGBK("正在构建 Workspace...\n");

        // ===== 调用 Workspace =====
        if (!workspace->buildFromUserInput(utf8Input))
        {
            printGBK("Workspace 尚未完成\n");
            printGBK("原因：\n");
            printUTF8(workspace->getErrorMessage());
            printGBK("\n\n");
            // ⭐ 关键：打印 AI 原始输出
            printGBK("【AI 原始输出】\n");
            printUTF8(workspace->getAiRawOutput());
            printGBK("\n==========================\n");

            printGBK("请补充说明后继续输入\n\n");
            continue;
        }

        // ===== 成功 =====
        printGBK("Workspace 创建成功\n\n");

        printUTF8("=== Workspace JSON ===\n");
        printUTF8(workspace->getWorkspaceJson());
        printGBK("\n======================\n");

        if (workspace->saveToFile("workspace.json"))
            printGBK("已保存到 workspace.json\n");
        else
            printGBK("保存失败（无法写入文件）\n");

        printGBK("\n你可以继续补充需求，或输入 break0 返回菜单。\n");
    }
}

//
// 5. AI 对话（键盘）
void Console::menuChat()
{
    if (!hasAIKey)
    {
        printGBK("请先设置 AI Key\n");
        return;
    }

    printGBK("\n进入 AI 对话模式\n");
    printGBK("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printGBK("chat> ");

        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);
        // ============================
        // ① 提示 AI 正在处理
        // ============================
        printGBK("[AI] 正在解析...\n");
        // ============================
        // ② 调用 ChatAgent
        // ============================
        std::string reply = chatAgent->query_once(utf8);
        // ============================
        // ③ 输出结果
        // ============================
        printGBK("[AI] 结果：\n");
        printUTF8(reply);
        printGBK("\n\n");
    }
}
// ================================
// 6. AI 对话（语音）
// ================================
void Console::menuVoiceChat()
{
    if (!hasAIKey)
    {
        printGBK("请先设置 AI Key\n");
        return;
    }
    printGBK("\n进入 AI 对话模式（语音）\n");
    printGBK("说完一句话自动发送给 AI\n");
    printGBK("说 break0 返回主菜单\n\n");
    speechagent speech;
    if (!speech.start())
    {
        printGBK("语音识别启动失败\n");
        return;
    }
    speech.setDebug(0);
    while (true)
    {
        if (_kbhit())
        {
            std::string cmd;
            std::getline(std::cin, cmd);

            if (checkBreak(cmd))
                return;
        }
        printGBK("voice> ");
        std::string text = speech.getText();
        if (text == "nosl")
        {
            continue;
        }
        if (checkBreak(text))
        {
            speech.stop();
            return;
        }

        printGBK("[AI] 正在解析...\n");
        std::string reply = chatAgent->query_once("yuyin:" + text);
        printGBK("[AI] 结果：\n");
        printUTF8(reply);
        printGBK("\n\n");
    }
}

// ================================
// 7. 语音识别测试（调试）
// ================================
void Console::menuSpeechTest()
{
    printGBK("\n--- 麦克风语音识别测试 ---\n");
    printGBK("仅用于语音调试\n");
    printGBK("输入 break0 返回\n\n");
    speechagent speech;
    if (!speech.start())
    {
        printGBK("语音识别启动失败\n");
        return;
    }
    while (true)
    {
        if (_kbhit())
        {
            std::string cmd;
            std::getline(std::cin, cmd);
            if (checkBreak(cmd))
                return;
        }
        printGBK("speech> ");
        std::string text = speech.getText();
        if (text == "nosl")
        {
            continue;
        }
        printGBK("[Speech] 识别结果：\n");
        printUTF8(text);
        printGBK("\n\n");
    }
}

// ================================
// 8. 决策 AI
// ================================
void Console::menuDecisionTest()
{
    if (!hasAIKey)
    {
        printGBK("请先设置 AI Key\n");
        return;
    }

    printGBK("\n--- 决策 AI 测试模式 ---\n");
    printGBK("输入 1 执行一次决策\n");
    printGBK("输入 break0 返回\n\n");

    while (true)
    {
        // 每 100 ms 检查一次是否有输入
        for (int i = 0; i < 100; ++i)
        {
            // 非阻塞检查输入
            if (_kbhit())
            {
                std::string cmd;
                std::getline(std::cin, cmd);

                if (checkBreak(cmd))
                    return;
            }

            Sleep(100);
        }

        // 每 10 秒执行一次决策
        printGBK("[Decision] 正在执行...\n");
        std::string reply = decisionAI->runOnce();
        printGBK("[Decision AI 输出]\n");
        printUTF8(reply);
        printGBK("\n");
    }

}


