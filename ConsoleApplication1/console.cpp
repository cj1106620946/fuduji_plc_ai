#include "console.h"
#include <conio.h>
#include <iostream>
#include <json/json.h>
#include <sstream>
#include "sqllient.h"
#include"sqlstore.h"
#include"uppermachine.h"
#include <chrono>
#include"projectmanager.h"
// 输出函数
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
std::string Console::GBKtoUTF8(const std::string& gbk)
{
    // GBK → UTF-16（不包含结尾 \0）
    int wlen = MultiByteToWideChar(936, 0, gbk.c_str(), -1, NULL, 0);
    if (wlen <= 1)
        return std::string();

    std::wstring wbuf;
    wbuf.resize(wlen - 1); // 去掉结尾 \0
    MultiByteToWideChar(936, 0, gbk.c_str(), -1, &wbuf[0], wlen);

    // UTF-16 → UTF-8（不包含结尾 \0）
    int u8len = WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, NULL, 0, NULL, NULL);
    if (u8len <= 1)
        return std::string();

    std::string utf8;
    utf8.resize(u8len - 1); // 去掉结尾 \0
    WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, &utf8[0], u8len, NULL, NULL);

    return utf8;
}
bool Console::checkBreak(const std::string& cmd)
{
    return cmd == "break0";
}
Console::Console()
    : aiDesc{
        0,                   // useCloud
        AIProvider::Ollama,   // provider
        std::string(),          // apiKey
        std::string(),          // modelName
        60,                     // timeoutSec
        0.7,                    // temperature
        2048                    // maxTokens
    },
    ai(aiDesc),
    plc(),
    aiController(ai),
    aiTrace()
{
    chat = new ChatAI(2, aiController, aiTrace);
    execute = new ExecuteAI(2, aiController, aiTrace);
    workspace = new WorkspaceAI(2, aiController, aiTrace);
    decision = new DecisionAI(2, aiController, aiTrace);
    judgment = new Judgmentai(2, aiController, aiTrace);
    memoryai = new MemoryAI(2, aiController, aiTrace);
}
Console::~Console()
{
    delete chat;
    delete execute;
    delete workspace;
    delete decision;
	delete judgment;
}

// 控制台是否已经创建
static bool consolecreated = false;

// 控制台窗口句柄
static HWND consolehwnd = nullptr;

// 显示或创建控制台
void Console::openconsole()
{
    if (!consolecreated)
    {
        AllocConsole();

        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);

        consolehwnd = GetConsoleWindow();

        // 禁用控制台窗口右上角的关闭按钮（X）
        if (consolehwnd)
        {
            HMENU hMenu = GetSystemMenu(consolehwnd, FALSE);
            if (hMenu)
            {
                EnableMenuItem(hMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
                DrawMenuBar(consolehwnd);
            }
        }

        consolecreated = true;
    }

    if (consolehwnd)
    {
        ShowWindow(consolehwnd, SW_SHOW);
        SetForegroundWindow(consolehwnd);
    }
}
// 隐藏控制台
void Console::hideconsole()
{
    if (consolehwnd)
    {
        ShowWindow(consolehwnd, SW_HIDE);
    }
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
    printUTF8("        PLC + AI 调试控制台\n");
    printUTF8("-----------------------------------\n");
    printUTF8("输入调试指令：A1 ~ A30\n");
    printUTF8("输入 break0 返回 / 退出当前测试\n");
    printUTF8("输入 0 直接退出程序\n");
    printUTF8("-----------------------------------\n");
}

void Console::mainMenu()
{
    printUTF8("> ");
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
        printUTF8("无效输入，仅支持 A1-A30 或 0\n");
    }
}
// A1:连接 PLC。提示用户输入 PLC IP，调用 plc.connectPLC 并显示连接结果。
void Console::menuTestA1()
{
    printUTF8("PLC IP> ");
    std::string ip;
    std::getline(std::cin, ip);
    if (plc.connectPLC(ip,0,1))
        printUTF8("PLC连接成功\n");
    else
        printUTF8("PLC连接失败\n");
}
// A2: 设置 AI 调用参数（完整配置）
// 从控制台读取并写入当前 AIClient 的调用描述结构体
void Console::menuTestA2()
{
    std::string input;

    printUTF8("是否使用云端 AI？(1=是 0=否)：\n");
    std::getline(std::cin, input);
    aiDesc.useCloud = (input == "1");

    printUTF8("选择 AI 服务提供方：0=OpenAI 1=DeepSeek 2=Anthropic 3=Google 4=Ollama\n");
    std::getline(std::cin, input);
    aiDesc.provider = static_cast<AIProvider>(std::stoi(input));

    if (aiDesc.useCloud)
    {
        printUTF8("请输入 API Key：\n");
        std::getline(std::cin, aiDesc.apiKey);
        hasAIKey = !aiDesc.apiKey.empty();
    }
    else
    {
        aiDesc.apiKey.clear();
        hasAIKey = false;
    }

    printUTF8("请输入模型名称（留空使用默认）：\n");
    std::getline(std::cin, aiDesc.modelName);

    printUTF8("请输入超时时间（秒）：\n");
    std::getline(std::cin, input);
    if (!input.empty())
        aiDesc.timeoutSec = std::stoi(input);

    printUTF8("请输入 temperature：\n");
    std::getline(std::cin, input);
    if (!input.empty())
        aiDesc.temperature = std::stod(input);

    printUTF8("请输入 maxTokens：\n");
    std::getline(std::cin, input);
    if (!input.empty())
        aiDesc.maxTokens = std::stoi(input);

    printUTF8("AI 调用配置已更新\n");
}

// A3:进入 PLC 手动控制模式，支持 read/write 命令和 break0退出。
void Console::menuTestA3()
{
    printUTF8("read I0.0 | write Q0.01 | break0\n");
    while (true)
    {
        printUTF8("plc> ");
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
            printUTF8("写入完成\n");
        }
    }
}

void Console::menuTestA4()
{
    printUTF8("\n进入 AI 对话模式（调试）\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    // 固定人格设定（调试用）
    // 你之后只需要改这里的内容即可
    std::string personaText =
        u8"\n以下内容是你的人格设计：\n"
        u8"你是季，一个带有微妙情感的少女"
        u8"你喜欢花草和春天，并对信封和树枝有着狂热的收集爱好"
        u8"你习惯以较慢的节奏回应世界。"
        u8"在对话中，你不会急于给出结论或立刻反应，而是先理解哪些信息是稳定的、值得回应的。"
        u8"你更容易注意到持续存在的事物，而不是短暂而强烈的刺激。"
        u8"重复出现的细节、长期保持不变的状态，比突发事件更容易引起你的关注。";
    while (true)
    {
        printUTF8("chat> ");
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
        printUTF8("[AI] 正在解析...\n");

        // 执行一次对话（注入人格）
        std::string reply = chat->runOnce(utf8, personaText);

        // 输出给用户
        printUTF8("[AI] 输出：\n");
        printUTF8(reply);
        printUTF8("\n\n");
    }
}

// A5: 键盘交互式 AI 聊天，支持 break0结束并在退出时显示历史。
void Console::menuTestA5()
{
    printUTF8("\n进入 AI 对话模式（调试）\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printUTF8("chat> ");
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
        printUTF8("[AI] 正在解析...\n");

        // 唯一一次执行入口（会写入 chat_e）
        std::string reply = chat->runOnce(utf8);

        // 正常输出（给用户看的）
        printUTF8("[AI] 输出：\n");
        printUTF8(reply);
        printUTF8("\n\n");
    }
}
// A6: 使用语音识别进行交互，识别结果发送给 AI 并由 TTS 播放回复，输入 break0 可退出。
void Console::menuTestA6()
{
    printUTF8("\n进入 AI 对话模式（语音）\n");
    printUTF8("说完一句话自动发送给 AI\n");
    printUTF8("说 break0 返回主菜单\n\n");
    speechagent speech;
    if (!speech.start())
    {
        printUTF8("语音识别启动失败\n");
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
        printUTF8("voice> ");
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
        printUTF8("AI jx...\n");
        // AI 查询
        std::string reply = chat->runOnce(text);
        // 控制台输出
        printUTF8("AI;\n");
        printUTF8(reply);
        printUTF8("\n\n");
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
    printUTF8("\n--- 麦克风语音识别测试 ---\n");
    printUTF8("仅用于语音调试\n");
    printUTF8("输入 break0 返回\n\n");
    speechagent speech;
    if (!speech.start())
    {
        printUTF8("语音识别启动失败\n");
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
        printUTF8("speech> ");
        std::string text = speech.getText();
        if (text == "nosl")
        {
            continue;
        }
        printUTF8("[Speech]识别结果：\n");
        printUTF8(text);
        printUTF8("\n\n");
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
    printUTF8("\n[A4] Workspace PLC 交互调试\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printUTF8("workspace> ");

        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);

        std::string plcName;
        std::string ip;
        int rack = 0;
        int slot = 0;
        std::string desc;
        int a;
        // 调用 WorkspaceAI
        std::string result = workspace->runPlcOnce(
            utf8,
            plcName,
            ip,
            rack,
            slot,
            desc,
            a
        );

        if (a)
        {
            printUTF8("[A4] Workspace 返回提示:\n");
            printUTF8(result);
            printUTF8("\n\n");
            continue;
        }

        // 输出当前 Workspace 状态
        printUTF8("[A4] Workspace 当前解析结果:\n");

        printUTF8("PLC 名称: ");
        if (!plcName.empty())
            printUTF8(plcName);
        else
            printUTF8("(null)");
        printUTF8("\n");

        printUTF8("IP 地址: ");
        if (!ip.empty())
            printUTF8(ip);
        else
            printUTF8("(null)");
        printUTF8("\n");

        printUTF8("Rack: ");
        std::cout << rack << std::endl;

        printUTF8("Slot: ");
        std::cout << slot << std::endl;

        printUTF8("描述: ");
        if (!desc.empty())
            printUTF8(desc);
        else
            printUTF8("(null)");
        printUTF8("\n\n");
    }
}
//A11:简单模块测试
void Console::menuTestA11()
{

}
void Console::menuTestA12()
{

}
void Console::menuTestA13()
{
    printUTF8("[A13] 创建上位机（数据库直连）\n");

    if (!store)
    {
        printUTF8("[A13] 数据库未初始化\n");
        return;
    }

    plcinfo info;

    printUTF8("请输入 PLC IP 地址: ");
    std::cin >> info.ipAddress;

    printUTF8("请输入 rack: ");
    std::cin >> info.rack;

    printUTF8("请输入 slot: ");
    std::cin >> info.slot;

    std::cin.ignore();

    std::string input;

    printUTF8("请输入任务描述(taskDesc): ");
    std::getline(std::cin, input);
    info.taskDesc = input;   // 直接使用 GBK

    printUTF8("请输入任务领域(taskDomain): ");
    std::getline(std::cin, input);
    info.taskDomain = input; // 直接使用 GBK

    printUTF8("请输入原始输入(sourceText): ");
    std::getline(std::cin, input);
    info.sourceText = input; // 直接使用 GBK

    printUTF8("请输入 PLC 型号(plcModel): ");
    std::getline(std::cin, input);
    info.plcModel = input;   // 直接使用 GBK（临时）

    printUTF8("请输入订货号(orderCode): ");
    std::getline(std::cin, input);
    info.orderCode = input;  // 直接使用 GBK（临时）

    info.signalRootId = 0;
    info.isActive = 1;

    if (!store->createPlcInfo(info))
    {
        printUTF8("[A13] 创建失败: ");
        printUTF8(store->getLastErrorText());
        printUTF8("\n");
        return;
    }

    printUTF8("[A13] 上位机创建成功\n");
}
void Console::menuTestA14()
{
    printUTF8("[A14] 创建变量（数据库直连）\n");

    if (!store)
    {
        printUTF8("[A14] 数据库未初始化\n");
        return;
    }

    signalinfo sig;
    std::string input;

    std::cin.ignore();

    printUTF8("请输入变量名(name): ");
    std::getline(std::cin, input);
    sig.name = input;              // GBK 原样

    printUTF8("请输入 PLC 地址(plcAddress): ");
    std::getline(std::cin, input);
    sig.plcAddress = input;        // GBK 原样

    printUTF8("请输入变量说明(description): ");
    std::getline(std::cin, input);
    sig.description = input;       // GBK 原样

    sig.isAvailable = 1;

    if (!store->createSignalInfo(sig))
    {
        printUTF8("[A14] 创建变量失败: ");
        printUTF8(store->getLastErrorText());
        printUTF8("\n");
        return;
    }

    printUTF8("[A14] 变量创建成功\n");
}
void Console::menuTestA15()
{
    printUTF8("[A15] 通过上位机创建 PLC\n");

    plcinfo info;
    std::string input;

    printUTF8("请输入 PLC IP 地址: ");
    std::cin >> info.ipAddress;

    printUTF8("请输入 rack: ");
    std::cin >> info.rack;

    printUTF8("请输入 slot: ");
    std::cin >> info.slot;

    std::cin.ignore();

    printUTF8("请输入任务描述(taskDesc): ");
    std::getline(std::cin, input);
    info.taskDesc = input;      // GBK

    printUTF8("请输入任务领域(taskDomain): ");
    std::getline(std::cin, input);
    info.taskDomain = input;    // GBK

    printUTF8("请输入原始输入(sourceText): ");
    std::getline(std::cin, input);
    info.sourceText = input;    // GBK

    info.signalRootId = 0;
    info.isActive = 1;

    if (!upper->createPlcInfoRow(info))
    {
        printUTF8("[A15] 创建 PLC 失败: ");
        printUTF8(upper->getLastError());
        printUTF8("\n");
        return;
    }

    printUTF8("[A15] PLC 创建成功\n");
}
void Console::menuTestA16()
{
  
}

void Console::menuTestA17()
{
    printUTF8("[A17] 启动上位机 run\n");

    if (!upper)
    {
        printUTF8("[A17] 上位机未初始化\n");
        return;
    }

    if (!upper->run())
    {
        printUTF8("[A17] 上位机启动失败: ");
        printUTF8(upper->getLastError());
        printUTF8("\n");
        return;
    }

    printUTF8("[A17] 上位机已启动，进入运行状态\n");
}
// A18: Workspace Signal 交互调试（多变量）
void Console::menuTestA18()
{
    printUTF8("[A18] Workspace Signal 交互调试\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    // 先注入一次 PLC 上下文（仅作为上下文，不解析、不打印）
    {
        std::string plcContext =
            u8"当前上位机上下文如下：\n"
            u8"- plc_name: 工厂供水系统\n"
            u8"- ip_address: 192.168.0.1\n";

        workspace->callSignalAI(plcContext);
    }

    while (true)
    {
        printUTF8("signal> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);

        printUTF8("[A18] Workspace 正在解析...\n");

        // 关键修改点：使用 vector 接收多个变量
        std::vector<SignalWorkspaceData> worksignals;
        int a;
        std::string result = workspace->runSignalOnce(
            utf8,
            worksignals,
            a
        );

        if (a)
        {
            printUTF8("[A18] Workspace 返回提示:\n");
            printUTF8(result);
            printUTF8("\n\n");
            continue;
        }

        if (worksignals.empty())
        {
            printUTF8("[A18] 未生成任何变量\n\n");
            continue;
        }

        printUTF8("[A18] Workspace 当前解析结果:\n");

        // 逐条打印变量
        for (size_t i = 0; i < worksignals.size(); ++i)
        {
            const SignalWorkspaceData& s = worksignals[i];

            printUTF8("---- 变量 ");
            std::cout << (i + 1);
            printUTF8(" ----\n");

            printUTF8("变量名: ");
            printUTF8(s.name);
            printUTF8("\n");

            printUTF8("PLC 地址: ");
            printUTF8(s.plc_address);
            printUTF8("\n");

            printUTF8("描述: ");
            printUTF8(s.description);
            printUTF8("\n\n");
        }
    }
}


// A19: ExecuteAI 交互调试（不执行 PLC，只解析执行意图）
void Console::menuTestA19()
{
    printUTF8("[A19] ExecuteAI 交互调试\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printUTF8("execute> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);

        printUTF8("[A19] ExecuteAI 正在解析...\n");

        // 调用 ExecuteAI
        std::vector<ExecuteItem> items = execute->runOnce(utf8);

        if (items.empty())
        {
            printUTF8("[A19] ExecuteAI 未返回任何执行项\n\n");
            continue;
        }

        printUTF8("[A19] ExecuteAI 解析结果:\n");

        int index = 0;
        for (const auto& it : items)
        {
            printUTF8("---- 执行项 ");
            std::cout << index++ << std::endl;

            if (!it.message.empty())
            {
                printUTF8("说明: ");
                printUTF8(it.message);
                printUTF8("\n");
            }

            if (!it.op.empty())
            {
                printUTF8("操作类型: ");
                printUTF8(it.op);
                printUTF8("\n");
            }

            if (!it.address.empty())
            {
                printUTF8("PLC 地址: ");
                printUTF8(it.address);
                printUTF8("\n");
            }

            if (!it.value.empty())
            {
                printUTF8("写入值: ");
                printUTF8(it.value);
                printUTF8("\n");
            }

            printUTF8("\n");
        }
    }
}
// A20: DecisionAI 交互调试（只输出分析建议，不执行任何操作）
void Console::menuTestA20()
{
    printUTF8("[A20] DecisionAI 交互调试\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printUTF8("decision> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);

        printUTF8("[A20] DecisionAI 正在分析...\n");

        // 当前阶段 snapshot 为空或占位
        std::string snapshot =u8"当前系统状态：当前水位为 200,当前温度为 150\n";
        // 调用 DecisionAI
        std::string result = decision->runOnce(snapshot, utf8);

        if (result.empty())
        {
            printUTF8("[A20] DecisionAI 未返回任何内容\n\n");
            continue;
        }

        printUTF8("[A20] DecisionAI 分析结果:\n");
        printUTF8(result);
        printUTF8("\n\n");
    }
}
// A21: 记忆系统初始化（镜像当前记忆）
void Console::menuTestA21()
{
    printUTF8("[A21] 初始化记忆系统\n");

    // 调试兜底：如果 memory 还没创建，这里创建
    if (!memory)
    {
        if (!store)
        {
            printUTF8("[A21] SqlStore 未初始化\n");
            return;
        }


    }

    if (!memory->init())
    {
        printUTF8("[A21] 记忆初始化失败\n");
        return;
    }

    printUTF8("[A21] 记忆初始化完成，当前记忆已镜像\n");
}
// A22: 读取当前记忆（只读镜像）
void Console::menuTestA22()
{
    printUTF8("[A22] 当前记忆读取测试\n");

    if (!memory)
    {
        printUTF8("[A22] MemoryBridge 未初始化\n");
        return;
    }

    const CurrentMemoryState& cur = memory->read();

    printUTF8("\n--- self 记忆 ---\n");
    for (int i = 0; i < 3; ++i)
    {
        printUTF8("key: ");
        printUTF8(cur.selfMemory[i].keyPath);
        printUTF8("\n内容: ");
        printUTF8(cur.selfMemory[i].content);
        printUTF8("\n\n");
    }

    printUTF8("--- user 记忆 ---\n");
    for (int i = 0; i < 6; ++i)
    {
        printUTF8("key: ");
        printUTF8(cur.userMemory[i].keyPath);
        printUTF8("\n内容: ");
        printUTF8(cur.userMemory[i].content);
        printUTF8("\n\n");
    }
}
// A23: 写入指定记忆（不影响 current）
void Console::menuTestA23()
{
    printUTF8("[A23] 记忆写入测试\n");
    printUTF8("输入格式：<keyId> <内容>\n");
    printUTF8("例如：5 用户更偏好简洁直接的工程说明\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    if (!memory)
    {
        printUTF8("[A23] MemoryBridge 未初始化\n");
        return;
    }

    while (true)
    {
        printUTF8("memory-write> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        std::istringstream iss(input);
        int keyId;
        std::string content;

        if (!(iss >> keyId))
        {
            printUTF8("[A23] 解析 keyId 失败\n");
            continue;
        }

        std::getline(iss, content);
        if (!content.empty() && content[0] == ' ')
            content.erase(0, 1);

        MemoryWrite req;
        req.memoryKeyId = keyId;
        req.content = GBKtoUTF8(content);

        if (!memory->write(req))
        {
            printUTF8("[A23] 写入失败: ");
            printUTF8(store->getLastErrorText());
            printUTF8("\n");
            continue;
        }
        printUTF8("[A23] 写入成功（数据库指针已更新，current 未变化）\n");
    }
}
// A24: 直接测试 SqlStore::writeMemory（最底层）
void Console::menuTestA24()
{
    printUTF8("[A24] 直接测试 SqlStore::writeMemory\n");

    if (!store)
    {
        printUTF8("[A24] SqlStore 未初始化\n");
        return;
    }

    int keyId = 1; // self.identity
    std::string content =
        u8"你是季，是一名辅助用户的少女，负责协助工程与技术相关的思考。";
    bool ok = store->writeMemory(keyId, content);

    if (!ok)
    {
        printUTF8("[A24] writeMemory 失败: ");
        printUTF8(store->getLastErrorText());
        printUTF8("\n");
        return;
    }

    printUTF8("[A24] writeMemory 成功\n");
}
void Console::menuTestA25()
{
    printUTF8("\n进入 A25 自我长期记忆整理模式（1-3）\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printUTF8("self> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);

        // 提示 AI 正在处理
        printUTF8("[MemoryAI] 正在整理 self 记忆...\n");

        // 执行 self 1-3
        std::string jsonOut = memoryai->runself(utf8,"");

        // 直接输出 JSON，方便调试
        printUTF8("[MemoryAI] 输出 JSON：\n");
        printUTF8(jsonOut);
        printUTF8("\n\n");
    }
}
void Console::menuTestA26()
{
    printUTF8("\n进入 A26 用户长期记忆整理模式（4-9）\n");
    printUTF8("输入 break0 返回主菜单\n\n");

    while (true)
    {
        printUTF8("user> ");
        std::string input;
        std::getline(std::cin, input);

        if (checkBreak(input))
            return;

        // GBK -> UTF8
        std::string utf8 = GBKtoUTF8(input);

        // 提示 AI 正在处理
        printUTF8("[MemoryAI] 正在整理 user 记忆...\n");

        // 执行 user 4-9
        std::string jsonOut = memoryai->runuser(utf8,"");

        // 直接输出 JSON，方便调试
        printUTF8("[MemoryAI] 输出 JSON：\n");
        printUTF8(jsonOut);
        printUTF8("\n\n");
    }
}

void Console::menuTestA27()
{
  
}

void Console::menuTestA28() { printUTF8("测试 A28\n"); }
void Console::menuTestA29() { printUTF8("测试 A29\n"); }
void Console::menuTestA30() { printUTF8("测试 A30\n"); }
