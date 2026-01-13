#include "console.h"
#include <conio.h>
#include <iostream>
#include <json/json.h>
#include <sstream>

// 输出函数

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
Console::Console() :ai(),plc(),aiController(ai),aiTrace()
{
    chat = new ChatAI(2,aiController,aiTrace);
    execute = new ExecuteAI(2,aiController, aiTrace,plc);
    workspace = new WorkspaceAI(2,aiController, aiTrace);
    decision = new DecisionAI(4,aiController, aiTrace);
	judgment = new Judgmentai(2, aiController, aiTrace);

}
Console::~Console()
{
    delete chat;
    delete execute;
    delete workspace;
    delete decision;
	delete judgment;
}

// 主循环
void Console::run()
{
    while (true)
    {
        showMainHeader();
        mainMenu();
    }
}
// UI
void Console::showMainHeader()
{
    printUTF8(u8"        PLC + AI 控制台\n");
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
    printGBK("9. 文本转语音测试\n");
    printGBK("10. AI 模块速度测试\n");
    printGBK("11. 模块化 AI 系统测试\n");

    printGBK("0. 退出\n");
    printGBK("-----------------------------------\n");
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
    else if (cmd == "9") menuTtsTest();
    else if (cmd == "10") menuAiBenchmark();
    else if (cmd == "11") menuAiManagerTest();

    else printGBK("无效输入\n");
}
// 1. PLC 连接
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
// 2. AI Key
void Console::menuAIKey()
{
    printGBK("请输入 AI Key：\n");
    std::string key;
    std::getline(std::cin, key);
  
    ai.setAPIKey(key);
    hasAIKey = true;
    printGBK("AI Key 设置完成\n");
}
// 3. PLC 手动
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
// 4. Workspace 创建（最终版）
void Console::menuWorkspace()
{
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
        // = 调用 Workspace =
        if (!workspace->runOnce(utf8Input))
        {
            printGBK("Workspace 尚未完成\n");
            printGBK("原因：\n");
            printUTF8(workspace->getErrorMessage());
            printGBK("\n\n");
 
            printGBK("AI 原始输出\n");
            printUTF8(workspace->getAiRawOutput());
            printGBK("\n\n");
            printGBK("请补充说明后继续输入\n\n");
            continue;
        }
        // = 成功 =
        printGBK("Workspace 创建成功\n\n");
        printUTF8("=== Workspace JSON ===\n");
        printUTF8(workspace->getWorkspaceJson());
        printGBK("\n==\n");
        if (workspace->saveToFile("workspace.json"))
            printGBK("已保存到 workspace.json\n");
        else
            printGBK("保存失败（无法写入文件）\n");
        printGBK("\n你可以继续补充需求，或输入 break0 返回菜单。\n");
    }
}
// 5. AI 对话（键盘）
void Console::menuChat()
{
    printGBK("\n进入 AI 对话模式（调试）\n");
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

        // 1 提示 AI 正在处理
        printGBK("[AI] 正在解析...\n");

        // 2 调用 Chat 的“执行入口”（唯一入口）
        std::string reply = chat->runExecuteRead(utf8);
        // 3 正常输出（给用户看的）
        printGBK("[AI] 输出：\n");
        printUTF8(reply);
        printGBK("\n");

        // 4 调试输出：原始 AI 返回（JSON 或普通文本）
        printGBK("[DEBUG] 原始 AI 返回内容：\n");

        // 这里再次调用一次“仅执行、不拼接显示”的内部逻辑
        // ⚠ 注意：Console 仍然不解析 JSON，只做原样打印
        std::string raw = chat->runExecuteOnce(utf8);

        printUTF8(raw);
        printGBK("\n\n");
    }
}

// 6. AI 对话（语音）
void Console::menuVoiceChat()
{
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
    PiperEngine tts;
    tts.init("\\piper");

    while (true)
    {
        if (_kbhit())
        {
            std::string cmd;
            std::getline(std::cin, cmd);
            if (checkBreak(cmd))
            {
                speech.stop();
                return;
            }
        }
        printGBK("voice> ");
        // 语音识别结果
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
        printGBK("AI jx...\n");
        // AI 查询
        std::string reply = chat->runOnce(text);
        // 控制台输出
        printGBK("AI;\n");
        printUTF8(reply);
        printGBK("\n\n");
        std::string utf8reply = reply;
        if (!utf8reply.empty())
        {
            tts.speak(utf8reply);
        }
    }
}
// 7. 语音识别测试（调试）
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
// 8. 决策 AI
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
        printGBK("[Decision AI 输出]\n");
        printGBK("\n");
    }

}
// 9. 文本转语音测试
void Console::menuTtsTest()
{
    printUTF8(u8"\n--- 文本转语音测试（Piper）---\n");
    // 创建 TTS 实例（只在这个模式用）
    PiperEngine tts;
    tts.init("\\piper");   
    while (true)
    {
        printUTF8(u8"tts> ");
        std::string gbk;
        std::getline(std::cin, gbk);
        if (checkBreak(gbk))
        {
            return;
        }
        // GBK → UTF-8
        std::string utf8 = GBKtoUTF8(gbk);
        if (utf8.empty())
        {
            printUTF8(u8"编码转换失败\n");
            continue;
        }

        // 调用 Piper
        bool ok = tts.speak(utf8);
        if (!ok)
        {
            printUTF8(u8"语音合成失败\n");
        }
    }
}

#include <chrono>

void Console::menuAiBenchmark()
{
    printGBK("\n--- AI 模块测速模式 ---\n");
    printGBK("输入一句话，将依次测速各 AI\n");
    printGBK("输入 break0 返回\n\n");

    while (true)
    {
        printGBK("bench> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        std::string utf8 = GBKtoUTF8(input);
        if (utf8.empty())
        {
            printGBK("编码转换失败\n");
            continue;
        }

        using clock = std::chrono::steady_clock;
        using ms = std::chrono::milliseconds;

        // ChatAI 测速
        auto t1 = clock::now();
        std::string chatReply = chat->runOnce(utf8);
        auto t2 = clock::now();
        auto chatCost = std::chrono::duration_cast<ms>(t2 - t1).count();
        printGBK("\n[ChatAI]\n");
        printGBK("耗时(ms): ");
        printGBK(std::to_string(chatCost));
        printGBK("\n");
        printUTF8(chatReply);
        printGBK("\n\n");
        // JudgmentAI 测速
        t1 = clock::now();
        std::string judgeReply = judgment->runOnce(utf8);
        t2 = clock::now();
        auto judgeCost = std::chrono::duration_cast<ms>(t2 - t1).count();

        printGBK("[JudgmentAI]\n");
        printGBK("耗时(ms): ");
        printGBK(std::to_string(judgeCost));
        printGBK("\n");
        printUTF8(judgeReply);
        printGBK("\n\n");

        int decisionValue = std::atoi(judgeReply.c_str());

        // WorkspaceAI 测速（不保存文件）
        if (decisionValue == 2)
        {
            t1 = clock::now();
            bool ok = workspace->runOnce(utf8);
            t2 = clock::now();
            auto wsCost = std::chrono::duration_cast<ms>(t2 - t1).count();

            printGBK("[WorkspaceAI]\n");
            printGBK("耗时(ms): ");
            printGBK(std::to_string(wsCost));
            printGBK("\n");
            printGBK(ok ? "状态：成功\n\n" : "状态：未完成\n\n");
        }

        // ExecuteAI 测速（不真实操作 PLC）
        if (decisionValue == 1)
        {
            t1 = clock::now();
            std::string execReply = execute->runOnce(utf8);
            t2 = clock::now();
            auto execCost = std::chrono::duration_cast<ms>(t2 - t1).count();

            printGBK("[ExecuteAI]\n");
            printGBK("耗时(ms): ");
            printGBK(std::to_string(execCost));
            printGBK("\n");
            printUTF8(execReply);
            printGBK("\n\n");
        }
        printGBK("测速完成，可继续输入\n\n");
    }
}

void Console::menuAiManagerTest()
{
    printGBK("\n--- 模块化 AI 系统测试模式 ---\n");
    printGBK("此模式将进入 AiManager\n");
    printGBK("由系统接管输入输出\n");
    printGBK("输入 exit 返回主菜单\n\n");

    // 创建并运行 AiManager
    AiManager manager;
    manager.run();

    printGBK("\n--- 已退出模块化 AI 系统 ---\n");
    printGBK("返回主菜单\n\n");
}
