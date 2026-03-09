#include "ai_plc_delegate.h"
#include <QObject>
ai_plc_delegate::ai_plc_delegate()
{
    env.live2dWriter.init();
}
ai_plc_delegate::~ai_plc_delegate()
{
    // ===== 管理层 =====
    if (managers.project)
    {
        delete managers.project;
        managers.project = nullptr;
    }

    if (managers.persona)
    {
        delete managers.persona;
        managers.persona = nullptr;
    }

    // ===== 模块层 =====
    if (modules.chatAi)
    {
        delete modules.chatAi;
        modules.chatAi = nullptr;
    }

    if (modules.memoryAi)
    {
        delete modules.memoryAi;
        modules.memoryAi = nullptr;
    }

    if (modules.workspaceAi)
    {
        delete modules.workspaceAi;
        modules.workspaceAi = nullptr;
    }

    if (modules.executeAi)
    {
        delete modules.executeAi;
        modules.executeAi = nullptr;
    }

    if (modules.decisionAi)
    {
        delete modules.decisionAi;
        modules.decisionAi = nullptr;
    }

    if (modules.speech)
    {
        delete modules.speech;
        modules.speech = nullptr;
    }

    // ===== 数据库层 =====
    if (env.sqlStore)
    {
        env.sqlStore->close();
        delete env.sqlStore;
        env.sqlStore = nullptr;
    }

    if (env.sqlClient)
    {
        delete env.sqlClient;
        env.sqlClient = nullptr;
    }

    // ===== UI =====
    if (env.ui)
    {
        delete env.ui;
        env.ui = nullptr;
    }
    if (ioThread.joinable())
    {
        pclailife.ioThreadStopping = true;
        ioThread.join();
    }

}
void ai_plc_delegate::logError(
    const std::string& fromFunc,
    const std::string& reason
)
{
    // 确保 error 目录存在
    CreateDirectoryA("error", NULL);

    // 打开 delegate 层日志文件
    std::ofstream logFile("error//ai_plc_delegate.log", std::ios::app);
    if (!logFile.is_open())
        return;

    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;

#if defined(_MSC_VER)
    localtime_s(&local_tm, &time_t_now);
#else
    local_tm = *std::localtime(&time_t_now);
#endif

    char buf[32] = { 0 };
    std::snprintf(
        buf,
        sizeof(buf),
        "%04d-%02d-%02d %02d:%02d:%02d",
        local_tm.tm_year + 1900,
        local_tm.tm_mon + 1,
        local_tm.tm_mday,
        local_tm.tm_hour,
        local_tm.tm_min,
        local_tm.tm_sec
    );

    // 写入日志
    logFile
        << "[" << buf << "] "
        << fromFunc
        << " | "
        << reason
        << std::endl;

    logFile.close();
}

bool ai_plc_delegate::initQt()
{
    if (pclailife.qtinit)
    {
        logError("initQt", "Qt 已经初始化，跳过");
        return true;
    }
    static int argc = 0;
    static char* argv[] = { nullptr };
    static QApplication app(argc, argv);
    env.ui = new qtmain(pclailife.ui, nullptr);
    if (!env.ui)
    {
        logError("initQt", "qtmain 创建失败");
        return false;
    }
    env.ui->show();
    QTimer* timer = new QTimer(env.ui);
    QObject::connect(timer, &QTimer::timeout, [this]()
    {
        while (!outputQueue.empty())
        {
            UiMessage msg = outputQueue.front();
            outputQueue.pop();
            if (env.ui)
            {
                env.ui->appendText(msg.text,0);
            }
        }
    });
    timer->start(10);
    // project 镜像刷新定时器
    QTimer* projectTimer = new QTimer(env.ui);
    QObject::connect(projectTimer, &QTimer::timeout, [this]()
    {
        // 1. 必须 project 初始化完成
        if (!pclailife.projectinit)
            return;

        // 2. 必须存在 project 管理器
        if (!managers.project)
            return;

        // 3. 必须允许上位机周期刷新
        if (!pclailife.allowUpperRefresh)
            return;

        std::vector<std::string> rows = parseProjectMirror();

        if (env.ui)
        {
            env.ui->updatePersonaMirror(rows, 2);
        }
    });

    projectTimer->start(200);
    pclailife.qtinit = true;
    logError("initQt", "Qt 初始化完成");
    return true;
}

bool ai_plc_delegate::initSql(const std::string& dbPath)
{
    if (pclailife.sqlinit)
    {
        logError("initSql", "数据库已打开，拒绝重复打开");
        lastError = "数据库已打开，请先关闭当前工程";
        return false;
    }

    logError("initSql", "开始初始化数据库: " + dbPath);
    if (env.sqlStore)
    {
        env.sqlStore->close();
        delete env.sqlStore;
        env.sqlStore = nullptr;
    }

    if (env.sqlClient)
    {
        delete env.sqlClient;
        env.sqlClient = nullptr;
    }
    env.sqlClient = new Sqllient(dbPath);
    if (!env.sqlClient)
    {
        logError("initSql", "Sqllient 创建失败");
        lastError = "数据库客户端创建失败";
        return false;
    }

    env.sqlStore = new SqlStore(env.sqlClient);
    if (!env.sqlStore)
    {
        logError("initSql", "SqlStore 创建失败");
        lastError = "数据库存储对象创建失败";
        delete env.sqlClient;
        env.sqlClient = nullptr;
        return false;
    }
    if (!env.sqlStore->open())
    {
        lastError = env.sqlStore->getLastErrorText();
        logError("initSql", "数据库打开失败: " + lastError);

        delete env.sqlStore;
        env.sqlStore = nullptr;

        delete env.sqlClient;
        env.sqlClient = nullptr;

        return false;
    }
    pclailife.sqlinit = true;
    logError("initSql", "数据库打开成功");
    if (!initai())
    {
        pclailife.sqlinit = false;
        return false;
    }
    if (!initpersona())
    {
        pclailife.sqlinit = false;
        return false;
    }
    initproject();
    return true;

}

bool ai_plc_delegate::initai()
{
    if (pclailife.aiinit)
    {
        logError("initai", "AI 基础层已初始化，跳过");
        return true;
    }

    modules.aiClient = new AIClient(env.defaultCallDesc);
    if (!modules.aiClient)
    {
        logError("initai", "AIClient 创建失败");
        lastError = "AIClient 创建失败";
        if (env.ui)
        {
            env.ui->showError("ai初始化失败: " + lastError);
        }
        return false;
    }

    modules.aiController = new AIController(*modules.aiClient);
    if (!modules.aiController)
    {
        logError("initai", "AIController 创建失败");
        lastError = "AIController 创建失败";
        if (env.ui)
        {
            env.ui->showError("ai初始化失败: " + lastError);
        }
        delete modules.aiClient;
        modules.aiClient = nullptr;
        return false;
    }

    modules.aiTrace = new AITrace();
    if (!modules.aiTrace)
    {
        logError("initai", "AITrace 创建失败");
        lastError = "AITrace 创建失败";
        if (env.ui)
        {
            env.ui->showError("ai初始化失败: " + lastError);
        }
        delete modules.aiController;
        modules.aiController = nullptr;
        delete modules.aiClient;
        modules.aiClient = nullptr;
        return false;
    }

    modules.aiTrace->setRootDir("ai_trace");

    pclailife.aiinit = true;
    logError("initai", "AI 基础层初始化完成");
    return true;
}

bool ai_plc_delegate::initpersona()
{
    if (pclailife.personainit)
    {
        logError("initpersona", "人格AI已初始化，跳过");
        return true;
    }

    if (!pclailife.sqlinit)
    {
        logError("initpersona", "数据库未初始化");
        return false;
    }

    if (!pclailife.aiinit)
    {
        logError("initpersona", "AI基础层未初始化");
        return false;
    }

    modules.chatAi = new ChatAI(
        1,
        *modules.aiController,
        *modules.aiTrace
    );

    if (!modules.chatAi)
    {
        logError("initpersona", "ChatAI 创建失败");
        return false;
    }

    modules.memoryAi = new MemoryAI(
        2,
        *modules.aiController,
        *modules.aiTrace
    );

    if (!modules.memoryAi)
    {
        logError("initpersona", "MemoryAI 创建失败");
        return false;
    }

    managers.persona = new personamanager(
        *modules.chatAi,
        *modules.memoryAi,
        *env.sqlStore,
        mirror.memoryState,
        pclailife.personaState
    );

    if (!managers.persona)
    {
        logError("initpersona", "personamanager 创建失败");
        return false;
    }

    managers.persona->init();
    managers.persona->initMemory();
    managers.persona->initAI();

    pclailife.personaState.inited = true;
    pclailife.personainit = true;
    if (!pclailife.ioThreadRunning)
    {
        pclailife.ioThreadStopping = false;
        ioThread = std::thread(&ai_plc_delegate::ioThreadProc, this);
    }

    logError("initpersona", "人格AI初始化完成");
    // ===== 初始化完成后刷新人格镜像到UI =====
    if (env.ui)
    {
        std::vector<std::string> rows = parsePersonaMirror();
        env.ui->updatePersonaMirror(rows, 1);
    }

    return true;
}

bool ai_plc_delegate::initproject()
{
    if (pclailife.projectinit)
    {
        logError("initproject", "Project 已初始化，跳过");
        return true;
    }

    if (!pclailife.sqlinit)
    {
        logError("initproject", "数据库未初始化");
        return false;
    }

    if (!pclailife.aiinit)
    {
        logError("initproject", "AI基础层未初始化");
        return false;
    }

    modules.workspaceAi = new WorkspaceAI(
        1,
        *modules.aiController,
        *modules.aiTrace
    );

    if (!modules.workspaceAi)
    {
        logError("initproject", "WorkspaceAI 创建失败");
        return false;
    }

    modules.executeAi = new ExecuteAI(
        1,
        *modules.aiController,
        *modules.aiTrace
    );

    if (!modules.executeAi)
    {
        logError("initproject", "ExecuteAI 创建失败");
        return false;
    }

    modules.decisionAi = new DecisionAI(
        5,
        *modules.aiController,
        *modules.aiTrace
    );

    if (!modules.decisionAi)
    {
        logError("initproject", "DecisionAI 创建失败");
        return false;
    }

    managers.project = new ProjectManager(
        *env.sqlStore,
        *modules.workspaceAi,
        *modules.executeAi,
        *modules.decisionAi,
        mirror.currentPlc,
        mirror.worksignals,
        pclailife.projectState
    );

    if (!managers.project)
    {
        logError("initproject", "ProjectManager 创建失败");
        return false;
    }

    if (!managers.project->init())
    {
        logError("initproject", "ProjectManager init 失败");
        return false;
    }

    // ===== 启动三个核心线程 =====
    managers.project->createRunThread();
    managers.project->createUpperThread();
    managers.project->createAIThread();

    // ===== 调试输出 =====
    qDebug() << "===== 初始化后镜像内容 =====";
    qDebug() << "currentPlc.taskDesc:" << QString::fromStdString(mirror.currentPlc.taskDesc);
    qDebug() << "currentPlc.ipAddress:" << QString::fromStdString(mirror.currentPlc.ipAddress);
    qDebug() << "currentPlc.rack:" << mirror.currentPlc.rack;
    qDebug() << "currentPlc.slot:" << mirror.currentPlc.slot;
    qDebug() << "worksignals 数量:" << mirror.worksignals.size();

    for (size_t i = 0; i < mirror.worksignals.size(); ++i)
    {
        qDebug() << "信号" << i << "名称:" << QString::fromStdString(mirror.worksignals[i].name);
        qDebug() << "信号" << i << "地址:" << QString::fromStdString(mirror.worksignals[i].plcAddress);
        qDebug() << "信号" << i << "当前值:" << QString::fromStdString(mirror.worksignals[i].currentValue);
    }

    qDebug() << "==========================";
    if (env.ui)
    {
        std::vector<std::string> rows = parseProjectMirror();
        env.ui->updatePersonaMirror(rows, 2);
    }
    pclailife.projectState.projectInited = true;
    pclailife.projectinit = true;
    logError("initproject", "Project 初始化完成");
    return true;
}

void ai_plc_delegate::run()
{
    if (!initQt())
    {
        logError("run", "initQt failed");
        return;
    }

    QObject::connect(
        env.ui,
        &qtmain::uiTextSubmitted,
        [this](const std::string& text, int mode)
    {
        if (!env.ui)
            return;

        logError("run", "收到 UI 文本: " + text);

        // UI 回显输入内容
        env.ui->appendText("输入：" + text, 0);

        UiMessage msg;
        msg.text = text;

        switch (mode)
        {
        case 1: // 指令模式
            msg.type = UiMessageType::Command;
            env.ui->appendText("指令已提交", 3);
            break;

        case 0: // 聊天模式
        default:
            if (!pclailife.personainit)
            {
                env.ui->showMiniTip("人格AI未初始化");
                return;
            }
            msg.type = UiMessageType::Text;
            env.ui->appendText("AI 正在思考...", 1);
            break;
        }

        inputQueue.push(msg);
        logError("run", "消息已入队");
    });

    QObject::connect(
        env.ui->getInitPanel(),
        &initpanel::openRequested,
        [this](const std::string& path)
    {
        // 已经打开则拒绝
        if (pclailife.sqlinit)
        {
            env.ui->showMiniTip("工程已打开，请先关闭当前工程");
            return;
        }

        if (!initSql(path))
        {
            env.ui->showError("数据库错误: " + lastError);
            return;
        }
        env.ui->showMiniTip("工程打开成功");
    });


    QObject::connect(
        env.ui->getInitPanel(),
        &initpanel::createRequested,
        [this](const std::string& path)
    {
        if (!initSql(path))
        {
            env.ui->showError("数据库错误: " + lastError);
            return;
        }
        env.ui->showMiniTip("工程创建成功");
    });

    QObject::connect(
        env.ui->getInitPanel(),
        &initpanel::closeRequested,
        [this]()
    {
        if (!pclailife.sqlinit)
        {
            env.ui->showMiniTip("当前没有打开工程");
            return;
        }

        pclailife.personainit = false;
        pclailife.projectinit = false;

        // 如果 IO 线程依赖人格运行，可以在这里控制
        // 这里只做基础回退，不销毁模块

        if (env.sqlStore)
        {
            env.sqlStore->close();
        }

        // 重置数据库状态
        pclailife.sqlinit = false;
        // 清空镜像（可选但推荐）
        mirror.memoryState = CurrentMemoryState();
        mirror.worksignals.clear();

        // 刷新 UI 镜像为空
        std::vector<std::string> emptyRows;
        env.ui->updatePersonaMirror(emptyRows, 1);
        env.ui->showMiniTip("工程已关闭");
    });

    QObject::connect(
        env.ui,
        &qtmain::personaMirrorEdited,
        [this](const std::string& key, const std::string& content)
    {
        for (int i = 0; i < 3; ++i)
        {
            if (mirror.memoryState.selfMemory[i].keyPath == key)
            {
                mirror.memoryState.selfMemory[i].content = content;
                return;
            }
        }

        for (int i = 0; i < 6; ++i)
        {
            if (mirror.memoryState.userMemory[i].keyPath == key)
            {
                mirror.memoryState.userMemory[i].content = content;
                return;
            }
        }
    });

    QObject::connect(
        env.ui->getInitProject(),
        &initproject::con1Clicked,
        [this]()
    {
        pclailife.allowUpperRefresh = !pclailife.allowUpperRefresh;
        env.ui->showMiniTip("取反");
    });

    QObject::connect(
        env.ui->getInitProject(),
        &initproject::con2Clicked,  // 改成信号
        [this]()
    {
        std::vector<std::string> rows = parseProjectMirror();
        if (env.ui)
        {
            env.ui->updatePersonaMirror(rows, 2);
        }
        env.ui->showMiniTip("con2按钮被点击");
        // TODO: 添加con2的具体处理逻辑
    });

    QObject::connect(
        env.ui->getInitProject(),
        &initproject::con3Clicked,  // 改成信号
        [this]()
    {
        if (managers.project->connectplcinit())
        {
            env.ui->showMiniTip("连接成功");
        }
        else
        {
			env.ui->showMiniTip("连接失败");
        }
    });

    QObject::connect(
        env.ui->getInitProject(),
        &initproject::con4Clicked,  // 改成信号
        [this]()
    {
        env.ui->showMiniTip("con4按钮被点击");
        // TODO: 添加con4的具体处理逻辑
    });

}

void ai_plc_delegate::processInputOnce()
{
    if (inputQueue.empty())
    {
        logError("processInputOnce", "队列为空");
        return;
    }
    UiMessage msg = inputQueue.front();
    inputQueue.pop();
    logError("processInputOnce", "取出消息: " + msg.text);
    if (msg.type == UiMessageType::Text)
    {
        onUiText(msg.text);
    }
}
void ai_plc_delegate::onUiText(const std::string& text)
{
    logError("onUiText", "开始处理文本: " + text);
    if (!env.ui)
    {
        logError("onUiText", "ui 为空");
        return;
    }
    // 主线程回显输入
    std::string out = "输入：" + text;
    env.ui->appendText(out,0);
    // 构造消息
    UiMessage msg;
    msg.type = UiMessageType::Text;
    msg.text = text;
    env.ui->appendText("AI 正在思考...", 1);
    // 入队
    inputQueue.push(msg);
    logError("onUiText", "文本已入队");
    logError("onUiText", "处理完成");
}
// IO 线程函数，持续处理输入输出队列
void ai_plc_delegate::ioThreadProc()
{
    logError("ioThreadProc", "IO线程创建完成");
    pclailife.ioThreadRunning = true;
    pclailife.ioThreadStopping = false;

    while (!pclailife.ioThreadStopping)
    {
        if (inputQueue.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        UiMessage msg = inputQueue.front();
        inputQueue.pop();
        if (msg.type == UiMessageType::Command)
        {
            handleCommandMessage(msg);
            continue;
        }

        if (msg.type == UiMessageType::Text)
        {
            handleTextMessage(msg);
            continue;
        }

        UiMessage outMsg;
        outMsg.type = UiMessageType::Text;
        outMsg.text = "未知消息类型";
        outputQueue.push(outMsg);
    }

    pclailife.ioThreadRunning = false;
    logError("ioThreadProc", "IO线程退出");
}
// 解析指令类型（只看命令名：token 第一个空格前的内容）
CommandType ai_plc_delegate::parseCommandType(const std::string& token)
{
    size_t pos = token.find(' ');
    std::string cmd = (pos == std::string::npos) ? token : token.substr(0, pos);
    if (cmd == "help") return CommandType::Help;
    if (cmd == "live2d") return CommandType::Live2D;
    if (cmd == "memory1") return CommandType::MemorySelf;
    if (cmd == "memory2") return CommandType::MemoryUser;
    if (cmd == "memoryoff") return CommandType::MemoryClear;

    if (cmd == "plccreate") return CommandType::PlcCreate;
    if (cmd == "signalcreate") return CommandType::SignalCreate;
    if (cmd == "signaldelete") return CommandType::SignalDelete;

    if (cmd == "get") return CommandType::Get;
    if (cmd == "set") return CommandType::Set;
    if (cmd == "getall") return CommandType::GetAll;
    return CommandType::None;
}
// 处理指令消息
void ai_plc_delegate::handleCommandMessage(const UiMessage& msg)
{
    UiMessage outMsg;
    outMsg.type = UiMessageType::Text;

    const std::string& text = msg.text;

    auto trim = [](std::string& s)
    {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
            s.erase(s.begin());
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
            s.pop_back();
    };

    auto normalizeToken = [&](std::string& token)
    {
        trim(token);

        if (!token.empty() && token.front() == '/')
            token.erase(token.begin());

        trim(token);

        if (!token.empty() && token.back() == '#')
            token.pop_back();

        trim(token);
    };

    auto splitBySpace = [&](const std::string& s, std::vector<std::string>& out)
    {
        out.clear();
        std::string cur;
        for (size_t i = 0; i < s.size(); ++i)
        {
            char c = s[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            {
                if (!cur.empty())
                {
                    out.push_back(cur);
                    cur.clear();
                }
                continue; 
            }
            cur.push_back(c);
        }
        if (!cur.empty())
            out.push_back(cur);
    };

    std::vector<std::string> tokens;

    size_t start = 0;
    while (true)
    {
        size_t pos1 = text.find('/', start);
        if (pos1 == std::string::npos)
            break;

        size_t pos2 = text.find('#', pos1);
        if (pos2 == std::string::npos)
            break;

        std::string token = text.substr(pos1 + 1, pos2 - pos1 - 1);
        normalizeToken(token);

        if (!token.empty())
            tokens.push_back(token);

        start = pos2 + 1;
    }

    if (tokens.empty())
    {
        std::string token = text;
        normalizeToken(token);
        if (!token.empty())
            tokens.push_back(token);
    }

    std::string result;

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        const std::string& token = tokens[i];

        logError("command", token);

        CommandType type = parseCommandType(token);

        switch (type)
        {
        case CommandType::Live2D:
        {
            std::vector<std::string> parts;
            splitBySpace(token, parts);

            if (parts.size() < 2)
            {
                result += "live2d 参数缺失\n";
                break;
            }

            const std::string& param = parts[1];

            if (param == "0")
            {
                pclailife.ui.live2dEnabled = 0;
                result += "Live2D 已关闭\n";
            }
            else if (param == "1")
            {
                pclailife.ui.live2dEnabled = 1;
                result += "Live2D 已创建\n";
            }
            else if (param == "2")
            {
                pclailife.ui.live2dEnabled = 2;
                result += "Live2D 已渲染\n";
            }
            else
            {
                result += "live2d 参数错误\n";
            }
            break;
        }
        case CommandType::MemorySelf:
        {
            if (!pclailife.personainit || !managers.persona)
            {
                result += "人格未初始化\n";
                break;
            }
            if (managers.persona->writeSelfLongMemory())
            {
                std::vector<std::string> rows = parsePersonaMirror();
                if (env.ui)
                {
                    env.ui->updatePersonaMirror(rows, 1);
                }
                result += "人格1-3长期记忆已更新\n";
            }
            else
            {
                result += "人格1-3更新失败\n";
            }
            break;
        }
        case CommandType::MemoryUser:
        {
            if (!pclailife.personainit || !managers.persona)
            {
                result += "人格未初始化\n";
                break;
            }
            if (managers.persona->writeUserLongMemory())
            {
                std::vector<std::string> rows = parsePersonaMirror();
                if (env.ui)
                {
                    env.ui->updatePersonaMirror(rows, 1);
                }
                result += "用户4-9长期记忆已更新\n";
            }
            else
            {
                result += "用户4-9更新失败\n";
            }
            break;
        }
        case CommandType::MemoryClear:
        {
            if (!pclailife.personainit || !managers.persona)
            {
                result += "人格未初始化\n";
                break;
            }
            if (modules.chatAi)
            {
                modules.chatAi->clearShortHistory();
            }
            result += "短期记忆已清空\n";
            break;
        }
        case CommandType::PlcCreate:
        {
            std::vector<std::string> parts;
            splitBySpace(token, parts);

            // 至少需要 name 和 ip
            if (parts.size() < 3)
            {
                result += "plccreate 参数不足\n";
                result += "格式: plccreate name ip [rack slot desc]\n";
                break;
            }

            std::string name = parts[1];
            std::string ip = parts[2];

            int rack = 0;
            int slot = 1;
            std::string desc;

            // 如果只给了 rack 没给 slot，报错
            if (parts.size() == 4)
            {
                result += "plccreate rack 和 slot 必须同时提供\n";
                break;
            }

            // 如果提供了 rack slot
            if (parts.size() >= 5)
            {
                try
                {
                    rack = std::stoi(parts[3]);
                    slot = std::stoi(parts[4]);
                }
                catch (...)
                {
                    result += "plccreate rack slot 参数错误\n";
                    break;
                }

                // 如果还有解释
                if (parts.size() >= 6)
                {
                    desc = parts[5];
                }
            }

            pclailife.allowUpperRefresh = 0;

            std::vector<std::string> outMessages;
            bool ok = managers.project->createPlcWorkspaceByCmd(
                ip,
                name,
                rack,
                slot,
                desc,
                outMessages
            );

            pclailife.allowUpperRefresh = 1;

            if (ok)
            {
                // 重新从数据库加载镜像
                if (!managers.project->loadProjectMirror())
                {
                    result += "镜像加载失败\n";
                    break;
                }

                std::vector<std::string> rows = parseProjectMirror();
                if (env.ui)
                {
                    env.ui->updateProjectMirror(rows);
                }
            }

            for (size_t k = 0; k < outMessages.size(); ++k)
            {
                result += outMessages[k];
                result += "\n";
            }

            if (outMessages.empty())
            {
                result += ok ? "PLC 创建完成\n" : "PLC 创建失败\n";
            }

            break;
        }
        case CommandType::SignalCreate:
        {
            std::vector<std::string> parts;
            splitBySpace(token, parts);

            if (parts.size() < 4)
            {
                result += "signalcreate 参数不足\n";
                result += "格式: signalcreate name addr desc [name addr desc ...]\n";
                break;
            }

            if (((int)parts.size() - 1) % 3 != 0)
            {
                result += "signalcreate 参数必须三元组 name addr desc\n";
                break;
            }

            pclailife.allowUpperRefresh = 0;

            std::vector<std::string> outMessages;
            bool ok = managers.project->createSignalWorkspaceByCmd(parts, outMessages);

            pclailife.allowUpperRefresh = 1;

            if (ok)
            {
                if (!managers.project->loadProjectMirror())
                {
                    result += "镜像加载失败\n";
                    break;
                }

                std::vector<std::string> rows = parseProjectMirror();
                if (env.ui)
                {
                    env.ui->updateProjectMirror(rows);
                }
            }

            for (size_t k = 0; k < outMessages.size(); ++k)
            {
                result += outMessages[k];
                result += "\n";
            }

            if (outMessages.empty())
            {
                result += ok ? "变量创建完成\n" : "变量创建失败\n";
            }

            break;
        }
        case CommandType::SignalDelete:
        {
            if (!pclailife.projectinit || !managers.project)
            {
                result += "工程未初始化\n";
                break;
            }

            std::vector<std::string> parts;
            splitBySpace(token, parts);

            if (parts.size() < 2)
            {
                result += "signaldelete 参数不足\n";
                break;
            }

            int id = 0;
            try
            {
                id = std::stoi(parts[1]);
            }
            catch (...)
            {
                result += "signaldelete 参数错误\n";
                break;
            }

            pclailife.allowUpperRefresh = 0;

            bool ok = managers.project->removeSignalById(id);

            pclailife.allowUpperRefresh = 1;

            if (ok)
            {
                if (!managers.project->loadProjectMirror())
                {
                    result += "镜像加载失败\n";
                    break;
                }

                std::vector<std::string> rows = parseProjectMirror();
                if (env.ui)
                {
                    env.ui->updateProjectMirror(rows);
                }

                result += "删除成功\n";
            }
            else
            {
                result += "删除失败\n";
            }

            break;
        }
        case CommandType::Get:
        {
            std::vector<std::string> parts;
            splitBySpace(token, parts);

            if (parts.size() < 2)
            {
                result += "get 参数不足\n";
                break;
            }

            const std::string& key = parts[1];

            if (key == "key")
            {
                result += "apiKey: ";
                result += env.defaultCallDesc.apiKey;
                result += "\n";
            }
            else if (key == "aimod")
            {
                result += "aimod: ";

                if (env.defaultCallDesc.provider == AIProvider::Ollama)
                    result += "1 (ollama)\n";
                else
                    result += "0 (cloud)\n";
            }
            else if (key == "usecloud")
            {
                result += "usecloud: ";
                result += std::to_string(env.defaultCallDesc.useCloud);
                result += "\n";
            }
            else
            {
                result += "未知get字段\n";
            }

            break;
        }
        case CommandType::Set:
        {
            std::vector<std::string> parts;
            splitBySpace(token, parts);

            if (parts.size() < 3)
            {
                result += "set 参数不足\n";
                break;
            }

            const std::string& key = parts[1];
            const std::string& value = parts[2];

            if (key == "key")
            {
                env.defaultCallDesc.apiKey = value;
                result += "apiKey 已更新\n";
            }
            else if (key == "aimod")
            {
                if (value == "0")
                {
                    env.defaultCallDesc.provider = AIProvider::DeepSeek;
                    result += "AI模式已切换: cloud\n";
                }
                else if (value == "1")
                {
                    env.defaultCallDesc.provider = AIProvider::Ollama;
                    result += "AI模式已切换: ollama\n";
                }
                else
                {
                    result += "aimod 参数错误\n";
                }
            }
            else if (key == "usecloud")
            {
                if (value == "0")
                {
                    env.defaultCallDesc.useCloud = 0;
                    result += "usecloud 已关闭\n";
                }
                else if (value == "1")
                {
                    env.defaultCallDesc.useCloud = 1;
                    result += "usecloud 已开启\n";
                }
                else
                {
                    result += "usecloud 参数错误\n";
                }
            }
            else
            {
                result += "未知set字段\n";
            }

            break;
        }
        case CommandType::GetAll:
        {
            std::vector<std::string> parts;
            splitBySpace(token, parts);

            if (parts.size() > 1)
            {
                result += "getall 不需要参数\n";
                break;
            }

            result += "AI当前配置:\n";

            result += "apiKey: ";
            result += env.defaultCallDesc.apiKey;
            result += "\n";

            result += "aimod: ";
            if (env.defaultCallDesc.provider == AIProvider::Ollama)
                result += "1 (ollama)\n";
            else
                result += "0 (cloud)\n";

            result += "timeout: ";
            result += std::to_string(env.defaultCallDesc.timeoutSec);
            result += "\n";

            result += "temperature: ";
            result += std::to_string(env.defaultCallDesc.temperature);
            result += "\n";

            result += "maxTokens: ";
            result += std::to_string(env.defaultCallDesc.maxTokens);
            result += "\n";

            result += "useCloud: ";
            result += std::to_string(env.defaultCallDesc.useCloud);
            result += "\n";

            break;
        }
        case CommandType::Help:
        {
            for (const auto& item : g_commandHelp)
            {
                result += "指令: " + item.command + "\n";
                result += "用法: " + item.usage + "\n";
                result += "说明: " + item.description + "\n\n";
            }
            break;
        }
        case CommandType::None:
        default:
        {
            result += "未知指令:";
            result += token;
            result += "\n";
            break;
        }
        }
    }

    outMsg.text = result.empty() ? "指令无输出" : result;
    outputQueue.push(outMsg);
}
// 处理AI消息
void ai_plc_delegate::handleTextMessage(const UiMessage& msg)
{
    UiMessage outMsg;
    outMsg.type = UiMessageType::Text;

    if (!pclailife.personainit || !managers.persona)
    {
        outMsg.text = "人格AI未初始化";
        outputQueue.push(outMsg);
        return;
    }

    // 1) 原文 -> 第一次 chat
    PersonaMessageIn inMsg;
    inMsg.type = 1;
    inMsg.text = msg.text;
    inMsg.createdAt = static_cast<int>(time(nullptr));

    PersonaMessageOut personaOut;
    if (!managers.persona->runOnce(inMsg, personaOut))
    {
        outMsg.text = "人格AI执行失败";
        outputQueue.push(outMsg);
        return;
    }

    // 第一次 chat 输出直接展示
    outMsg.text = personaOut.text;
    outputQueue.push(outMsg);

    // Live2D 显示第一次 chat
    if (!env.live2dWriter.write(personaOut.text, personaOut.emotion, personaOut.priority))
    {
        logError("handleTextMessage", "live2dWriter 写入失败");
    }

    // control=0 直接结束
    if (personaOut.control == 0)
    {
        return;
    }

    // 2) 组织执行输入：原文 + 第一次 chat 输出
    std::string execInput;
    execInput += "原文：";
    execInput += msg.text;
    execInput += "\n";
    execInput += "Chat输出：";
    execInput += personaOut.text;
    execInput += "\n";

    // 3) 执行模块：不直接显示 rawResults
    bool actionOk = true;
    std::string actionName;
    std::vector<std::string> rawResults;

    if (!pclailife.projectinit || !managers.project)
    {
        actionOk = false;
        rawResults.push_back("项目未初始化");
    }
    else
    {
        switch (personaOut.control)
        {
        case 1:
            actionName = "执行";
            logError("handleTextMessage", "control=1 执行AI");
            if (!managers.project->executeByAI(execInput, rawResults))
            {
                actionOk = false;
                rawResults.clear();
                rawResults.push_back(managers.project->getLastError());
            }
            break;

        case 2:
            actionName = "创建PLC";
            logError("handleTextMessage", "control=2 创建PLC");
            if (!managers.project->createPlcWorkspaceByAI(execInput, rawResults))
            {
                actionOk = false;
            }
            if (actionOk)
            {
                managers.project->loadProjectMirror();
            }
            break;

        case 3:
            actionName = "创建变量";
            logError("handleTextMessage", "control=3 创建变量");
            if (!managers.project->createSignalWorkspaceByAI(execInput, rawResults))
            {
                actionOk = false;
            }
            if (actionOk)
            {
                managers.project->loadProjectMirror();
            }
            break;

        default:
            actionOk = false;
            actionName = "未知操作";
            logError("handleTextMessage", "control=未知值，忽略: " + std::to_string(personaOut.control));
            rawResults.push_back("未知控制类型");
            break;
        }
    }

    // 4) 第二次 chat：解释执行结果
    std::string explainInput;
    explainInput += "你现在负责把功能执行结果解释给用户。\n";
    explainInput += "要求：不要复读原文，不要输出原始列表，不要输出指令格式。\n";
    explainInput += "只给结论和下一步。\n\n";

    explainInput += "功能：";
    explainInput += actionName;
    explainInput += "\n";

    explainInput += "第一次Chat输出：\n";
    explainInput += personaOut.text;
    explainInput += "\n\n";

    explainInput += "执行是否成功：";
    explainInput += (actionOk ? "成功" : "失败");
    explainInput += "\n";

    explainInput += "执行返回：\n";
    if (rawResults.empty())
    {
        explainInput += actionOk ? "无返回内容\n" : "无返回内容\n";
    }
    else
    {
        for (size_t i = 0; i < rawResults.size(); ++i)
        {
            explainInput += rawResults[i];
            explainInput += "\n";
        }
    }

    PersonaMessageIn explainMsg;
    explainMsg.type = 1;
    explainMsg.text = explainInput;
    explainMsg.createdAt = static_cast<int>(time(nullptr));

    PersonaMessageOut explainOut;
    if (managers.persona->runOnce(explainMsg, explainOut))
    {
        UiMessage explainUi;
        explainUi.type = UiMessageType::Text;
        explainUi.text = explainOut.text;
        outputQueue.push(explainUi);

        if (!env.live2dWriter.write(explainOut.text, explainOut.emotion, explainOut.priority))
        {
            logError("handleTextMessage", "live2dWriter 写入失败(解释)");
        }
    }
    else
    {
        UiMessage explainUi;
        explainUi.type = UiMessageType::Text;
        explainUi.text = "解释失败，但功能已执行完成";
        outputQueue.push(explainUi);
    }
}
// 获取人格镜像
std::vector<std::string> ai_plc_delegate::parsePersonaMirror()
{
    std::vector<std::string> rows;

    // ===== 解析 selfMemory（人格自身记忆）=====
    const char* selfTitles[3] = {
        u8"AI自我.自我认知",
        u8"AI自我.情感基调",
        u8"AI自我.处事方式"
    };

    for (int i = 0; i < 3; ++i)
    {
        const CurrentMemory& mem = mirror.memoryState.selfMemory[i];

        // 第一列显示中文标题，第二列显示内容
        std::string line = selfTitles[i];
        line += "|";
        line += mem.content;

        rows.push_back(line);
    }

    // ===== 解析 userMemory（用户相关记忆）=====
    const char* userTitles[6] = {
        u8"面向用户.用户总结",
        u8"面向用户.用户偏好",
        u8"面向用户.称呼方式",
        u8"面向用户.交互策略",
        u8"面向用户.情境背景",
        u8"面向用户.长期边界"
    };

    for (int i = 0; i < 6; ++i)
    {
        const CurrentMemory& mem = mirror.memoryState.userMemory[i];

        std::string line = userTitles[i];
        line += "|";
        line += mem.content;

        rows.push_back(line);
    }

    return rows;
}
//获取镜像
std::vector<std::string> ai_plc_delegate::parseProjectMirror()
{
    std::vector<std::string> rows;

    // ===== PLC 信息组（根节点）=====
    rows.push_back("#A#PLC信息");
    plcinfo& plc = mirror.currentPlc;
    rows.push_back("任务描述|" + plc.taskDesc);
    rows.push_back("任务域|" + plc.taskDomain);
    rows.push_back("来源文本|" + plc.sourceText);
    rows.push_back("PLC型号|" + plc.plcModel);
    rows.push_back("订货号|" + plc.orderCode);
    rows.push_back("IP地址|" + plc.ipAddress);
    rows.push_back("机架|" + std::to_string(plc.rack));
    rows.push_back("槽号|" + std::to_string(plc.slot));
    rows.push_back("连接状态|" + std::string(plc.isActive ? "已连接" : "未连接"));

    // ===== 变量列表（PLC信息下的子节点）=====
    rows.push_back("#A#变量列表");
    // ===== 遍历所有变量 =====
    for (auto& s : mirror.worksignals)
    {
        // 每个变量作为变量列表下的子节点
        rows.push_back("#B#" + s.name);
        // 变量的属性
        rows.push_back("信号ID|" + std::to_string(s.signalId));
        rows.push_back("变量名|" + s.name);
        rows.push_back("PLC地址|" + s.plcAddress);
        rows.push_back("所属PLC ID|" + std::to_string(s.plcId));
        rows.push_back("说明|" + s.description);
        rows.push_back("创建时间|" + std::to_string(s.createdAt));
        rows.push_back("当前值|" + s.currentValue);
        rows.push_back("读取状态|" + std::string(s.readOk ? "成功" : "失败"));
        rows.push_back("目标值|" + s.targetValue);
        rows.push_back("写入标记|" + std::string(s.writeFlag ? "等待写入" : "无"));
        rows.push_back("最后操作时间|" + std::to_string(s.lastOpAt));
        rows.push_back("可用状态|" + std::string(s.isAvailable ? "可用" : "不可用"));

        // 关键：添加一个标记让界面层把栈退回上一级
        rows.push_back("#BACK#");
    }

    return rows;
}