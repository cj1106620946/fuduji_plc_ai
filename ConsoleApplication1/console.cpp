#include "console.h"
#include <conio.h>
#include <iostream>
#include <json/json.h>
#include <sstream>
#include "sqllient.h"
#include"sqlstore.h"
#include"uppermachine.h"
#include <chrono>
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
    workspace = new WorkspaceAI(1,aiController, aiTrace);
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
    printUTF8(u8"        PLC + AI 调试控制台\n");
    printGBK("-----------------------------------\n");
    printGBK("输入调试指令：A1 ~ A30\n");
    printGBK("输入 break0 返回 / 退出当前测试\n");
    printGBK("输入 0 直接退出程序\n");
    printGBK("-----------------------------------\n");
}

void Console::mainMenu()
{
    printGBK("> ");
    std::string cmd;
    std::getline(std::cin, cmd);

    if (cmd == "0")
    {
        exit(0);
    }
    else if (cmd == "A1") menuTestA1();
    else if (cmd == "A2") menuTestA2();
    else if (cmd == "A3") menuTestA3();
    else if (cmd == "A4") menuTestA4();
    else if (cmd == "A5") menuTestA5();
    else if (cmd == "A6") menuTestA6();
    else if (cmd == "A7") menuTestA7();
    else if (cmd == "A8") menuTestA8();
    else if (cmd == "A9") menuTestA9();
    else if (cmd == "A10") menuTestA10();
    else if (cmd == "A11") menuTestA11();
    else if (cmd == "A12") menuTestA12();
    else if (cmd == "A13") menuTestA13();
    else if (cmd == "A14") menuTestA14();
    else if (cmd == "A15") menuTestA15();
    else if (cmd == "A16") menuTestA16();
    else if (cmd == "A17") menuTestA17();
    else if (cmd == "A18") menuTestA18();
    else if (cmd == "A19") menuTestA19();
    else if (cmd == "A20") menuTestA20();
    else if (cmd == "A21") menuTestA21();
    else if (cmd == "A22") menuTestA22();
    else if (cmd == "A23") menuTestA23();
    else if (cmd == "A24") menuTestA24();
    else if (cmd == "A25") menuTestA25();
    else if (cmd == "A26") menuTestA26();
    else if (cmd == "A27") menuTestA27();
    else if (cmd == "A28") menuTestA28();
    else if (cmd == "A29") menuTestA29();
    else if (cmd == "A30") menuTestA30();
    else
    {
        printGBK("无效输入，仅支持 A1-A30 或 0\n");
    }
}

// A1:连接 PLC。提示用户输入 PLC IP，调用 plc.connectPLC 并显示连接结果。
void Console::menuTestA1()
{
    printGBK("PLC IP> ");
    std::string ip;
    std::getline(std::cin, ip);
    if (plc.connectPLC(ip,0,1))
        printGBK("PLC连接成功\n");
    else
        printGBK("PLC连接失败\n");
}
// A2: 设置 AI Key。提示用户输入并保存到 ai 对象。
void Console::menuTestA2()
{
    printGBK("请输入 AI Key：\n");
    std::string key;
    std::getline(std::cin, key);
  
    ai.setAPIKey(key);
    hasAIKey = true;
    printGBK("AI Key 设置完成\n");
}
// A3:进入 PLC 手动控制模式，支持 read/write 命令和 break0退出。
void Console::menuTestA3()
{
    printGBK("read I0.0 | write Q0.01 | break0\n");
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
            int v =0;
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
// A4:交互式创建 Workspace。读取用户输入（GBK），转换为 UTF-8，调用 WorkspaceAI生成并可保存为文件。
void Console::menuTestA4()
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
// A5: 键盘交互式 AI 聊天，支持 break0结束并在退出时显示历史。
void Console::menuTestA5()
{
    printGBK("\n进入 AI 对话模式（调试）\n");
    printGBK("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printGBK("chat> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
        {
            ai.showHistory("chat_e");
            return;
        }

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);

        // 提示 AI 正在处理
        printGBK("[AI] 正在解析...\n");

        // 唯一一次执行入口（会写入 chat_e）
        std::string reply = chat->runExecuteRead(utf8);

        // 正常输出（给用户看的）
        printGBK("[AI] 输出：\n");
        printUTF8(reply);
        printGBK("\n\n");
    }
}
// A6: 使用语音识别进行交互，识别结果发送给 AI 并由 TTS 播放回复，输入 break0 可退出。
void Console::menuTestA6()
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
        //语音识别结果
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
// A7:仅测试麦克风语音识别，打印识别到的文本，输入 break0 返回。
void Console::menuTestA7()
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
        printGBK("[Speech]识别结果：\n");
        printUTF8(text);
        printGBK("\n\n");
    }
}
// A8: 占位函数，当前未实现具体逻辑。
void Console::menuTestA8()
{
    ;
}
// A9: 使用 Piper TTS 将输入文本合成为语音并播放。
void Console::menuTestA9()
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
//A10：测速
void Console::menuTestA10()
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
        if (decisionValue ==2)
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
        if (decisionValue ==1)
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
//A11:简单模块测试
void Console::menuTestA11()
{
    printGBK("\n--- 模块化 AI 系统测试模式 ---\n");
    printGBK("此模式将进入 AiManager\n");
    printGBK("由系统接管输入输出\n");
    AiManager manager;
    manager.run();

    printGBK("\n--- 已退出模块化 AI 系统 ---\n");
    printGBK("返回主菜单\n\n");
}
void Console::menuTestA12()
{
    printGBK("[A12] 创建数据库测试开始\n");

    sqlClient = new Sqllient("plc.db");
    store = new SqlStore(sqlClient);
	upper = new uppermachine(plc, *store);
    if (!store->open())
    {
        printGBK("[A12] 数据库创建失败: ");
        printUTF8(store->getLastErrorText());
        printGBK("\n");
        delete store;
        delete sqlClient;
        return;
    }
    printGBK("[A12] 数据库创建并初始化成功\n");
}
void Console::menuTestA13()
{
    printGBK("[A13] 创建上位机（数据库直连）\n");

    if (!store)
    {
        printGBK("[A13] 数据库未初始化\n");
        return;
    }

    plcinfo info;

    printGBK("请输入 PLC IP 地址: ");
    std::cin >> info.ipAddress;

    printGBK("请输入 rack: ");
    std::cin >> info.rack;

    printGBK("请输入 slot: ");
    std::cin >> info.slot;

    std::cin.ignore();

    std::string input;

    printGBK("请输入任务描述(taskDesc): ");
    std::getline(std::cin, input);
    info.taskDesc = input;   // 直接使用 GBK

    printGBK("请输入任务领域(taskDomain): ");
    std::getline(std::cin, input);
    info.taskDomain = input; // 直接使用 GBK

    printGBK("请输入原始输入(sourceText): ");
    std::getline(std::cin, input);
    info.sourceText = input; // 直接使用 GBK

    printGBK("请输入 PLC 型号(plcModel): ");
    std::getline(std::cin, input);
    info.plcModel = input;   // 直接使用 GBK（临时）

    printGBK("请输入订货号(orderCode): ");
    std::getline(std::cin, input);
    info.orderCode = input;  // 直接使用 GBK（临时）

    info.signalRootId = 0;
    info.isActive = 1;

    if (!store->createPlcInfo(info))
    {
        printGBK("[A13] 创建失败: ");
        printUTF8(store->getLastErrorText());
        printGBK("\n");
        return;
    }

    printGBK("[A13] 上位机创建成功\n");
}
void Console::menuTestA14()
{
    printGBK("[A14] 创建变量（数据库直连）\n");

    if (!store)
    {
        printGBK("[A14] 数据库未初始化\n");
        return;
    }

    signalinfo sig;
    std::string input;

    std::cin.ignore();

    printGBK("请输入变量名(name): ");
    std::getline(std::cin, input);
    sig.name = input;              // GBK 原样

    printGBK("请输入 PLC 地址(plcAddress): ");
    std::getline(std::cin, input);
    sig.plcAddress = input;        // GBK 原样

    printGBK("请输入变量说明(description): ");
    std::getline(std::cin, input);
    sig.description = input;       // GBK 原样

    sig.isAvailable = 1;

    if (!store->createSignalInfo(sig))
    {
        printGBK("[A14] 创建变量失败: ");
        printUTF8(store->getLastErrorText());
        printGBK("\n");
        return;
    }

    printGBK("[A14] 变量创建成功\n");
}

void Console::menuTestA15()
{
    printGBK("[A15] 通过上位机创建 PLC\n");

    plcinfo info;
    std::string input;

    printGBK("请输入 PLC IP 地址: ");
    std::cin >> info.ipAddress;

    printGBK("请输入 rack: ");
    std::cin >> info.rack;

    printGBK("请输入 slot: ");
    std::cin >> info.slot;

    std::cin.ignore();

    printGBK("请输入任务描述(taskDesc): ");
    std::getline(std::cin, input);
    info.taskDesc = input;      // GBK

    printGBK("请输入任务领域(taskDomain): ");
    std::getline(std::cin, input);
    info.taskDomain = input;    // GBK

    printGBK("请输入原始输入(sourceText): ");
    std::getline(std::cin, input);
    info.sourceText = input;    // GBK

    info.signalRootId = 0;
    info.isActive = 1;

    if (!upper->createPlcInfoRow(info))
    {
        printGBK("[A15] 创建 PLC 失败: ");
        printUTF8(upper->getLastError());
        printGBK("\n");
        return;
    }

    printGBK("[A15] PLC 创建成功\n");
}
void Console::menuTestA16()
{
    printGBK("[A16] 通过上位机创建变量\n");

    plcinfo current;
    if (!upper->getCurrentPlcInfo(current))
    {
        printGBK("[A16] 当前 PLC 不存在: ");
        printUTF8(upper->getLastError());
        printGBK("\n");
        return;
    }

    signalinfo sig;
    std::string input;

    std::cin.ignore();

    printGBK("请输入变量名(name): ");
    std::getline(std::cin, input);
    sig.name = input;            // GBK

    printGBK("请输入 PLC 地址(plcAddress): ");
    std::getline(std::cin, input);
    sig.plcAddress = input;      // GBK

    printGBK("请输入变量说明(description): ");
    std::getline(std::cin, input);
    sig.description = input;     // GBK

    int plcId = current.signalRootId;

    if (!upper->createSignalRow(
        sig.name,
        sig.plcAddress,
        plcId,
        sig.description))
    {
        printGBK("[A16] 创建变量失败: ");
        printUTF8(upper->getLastError());
        printGBK("\n");
        return;
    }

    printGBK("[A16] 变量创建成功\n");
}

void Console::menuTestA17()
{
    printGBK("[A17] 启动上位机 run\n");

    if (!upper)
    {
        printGBK("[A17] 上位机未初始化\n");
        return;
    }

    if (!upper->run())
    {
        printGBK("[A17] 上位机启动失败: ");
        printUTF8(upper->getLastError());
        printGBK("\n");
        return;
    }

    printGBK("[A17] 上位机已启动，进入运行状态\n");
}

void Console::menuTestA18()
{

}

void Console::menuTestA19()
{
}

void Console::menuTestA20() { printGBK("测试 A20\n"); }
void Console::menuTestA21() { printGBK("测试 A21\n"); }
void Console::menuTestA22() { printGBK("测试 A22\n"); }
void Console::menuTestA23() { printGBK("测试 A23\n"); }
void Console::menuTestA24() { printGBK("测试 A24\n"); }
void Console::menuTestA25() { printGBK("测试 A25\n"); }
void Console::menuTestA26() { printGBK("测试 A26\n"); }
void Console::menuTestA27() { printGBK("测试 A27\n"); }
void Console::menuTestA28() { printGBK("测试 A28\n"); }
void Console::menuTestA29() { printGBK("测试 A29\n"); }
void Console::menuTestA30() { printGBK("测试 A30\n"); }
