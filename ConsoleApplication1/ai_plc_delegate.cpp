#include "ai_plc_delegate.h"
#include <QObject>
ai_plc_delegate::ai_plc_delegate()
{
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
    // 新增：project 镜像刷新定时器
    QTimer* projectTimer = new QTimer(env.ui);
    QObject::connect(projectTimer, &QTimer::timeout, [this]()
    {
        if (!pclailife.projectinit)
            return;

        if (!managers.project)
            return;

        std::vector<std::string> rows = parseProjectMirror();

        if (env.ui)
        {
            env.ui->updatePersonaMirror(rows, 2);
        }
    });
    projectTimer->start(200); // 200ms 刷新一次

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
    static AICallDesc callDesc;
    callDesc.useCloud = 1;
    callDesc.provider = AIProvider::DeepSeek;
    callDesc.apiKey="sk-bb3b9af89db147eda4eedf1c0412c5f2";
    callDesc.modelName.clear();
    callDesc.timeoutSec = 60;
    callDesc.temperature = 0.7;
    callDesc.maxTokens = 2048;
    modules.aiClient = new AIClient(callDesc);
    if (!modules.aiClient)
    {
        logError("initai", "AIClient 创建失败");
        env.ui->showError("ai初始化失败: " + lastError);
        return false;
    }
    modules.aiController = new AIController(*modules.aiClient);
    if (!modules.aiController)
    {
        logError("initai", "AIController 创建失败");
        env.ui->showError("ai初始化失败: " + lastError);
        return false;
    }
    modules.aiTrace = new AITrace();
    if (!modules.aiTrace)
    {
        logError("initai", "AITrace 创建失败");
        env.ui->showError("ai初始化失败: " + lastError);
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
        [this](const std::string& text)
    {
        if (!pclailife.personainit)
        {
            env.ui->showMiniTip("人格AI未初始化");

            return;
        }
        logError("run", "收到 UI 文本: " + text);
        UiMessage msg;
        msg.type = UiMessageType::Text;
        msg.text = text;
        logError("run", "消息已入队");
        onUiText(msg.text);
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
        &initproject::con1Clicked,  // 改成信号，不是槽函数
        [this]()
    {
        env.ui->showMiniTip("con1按钮被点击");
        // TODO: 添加con1的具体处理逻辑
    });

    QObject::connect(
        env.ui->getInitProject(),
        &initproject::con2Clicked,  // 改成信号
        [this]()
    {
        env.ui->showMiniTip("con2按钮被点击");
        // TODO: 添加con2的具体处理逻辑
    });

    QObject::connect(
        env.ui->getInitProject(),
        &initproject::con3Clicked,  // 改成信号
        [this]()
    {
        env.ui->showMiniTip("con3按钮被点击");
        // TODO: 添加con3的具体处理逻辑
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
void ai_plc_delegate::ioThreadProc()
{
    logError("ioThreadProc", "IO线程创建完成");
    pclailife.ioThreadRunning = true;
    pclailife.ioThreadStopping = false;

    while (!pclailife.ioThreadStopping)
    {
        if (!inputQueue.empty())
        {
            UiMessage msg = inputQueue.front();
            inputQueue.pop();

            UiMessage outMsg;
            outMsg.type = UiMessageType::Text;

            std::string text = msg.text;

            // ================= 指令解析区 =================
            std::vector<std::string> commands;
            size_t start = 0;
            while (true)
            {
                size_t pos1 = text.find('/', start);
                if (pos1 == std::string::npos)
                    break;

                size_t pos2 = text.find('#', pos1);
                if (pos2 == std::string::npos)
                    break;

                std::string cmd = text.substr(pos1 + 1, pos2 - pos1 - 1);
                commands.push_back(cmd);

                start = pos2 + 1;
            }

            // ================= 指令处理入口（暂时空骨架） =================
            for (size_t i = 0; i < commands.size(); ++i)
            {
                std::string& cmd = commands[i];

                // 这里后面接入你的指令表
                logError("command", cmd);
            }

            // ================= 纯文本进入人格AI =================
            if (!pclailife.personainit || !managers.persona)
            {
                outMsg.text = "人格AI未初始化";
            }
            else
            {
                PersonaMessageIn inMsg;
                inMsg.type = 1;
                inMsg.text = text;
                inMsg.createdAt = static_cast<int>(time(nullptr));
                PersonaMessageOut personaOut;
                if (managers.persona->runOnce(inMsg, personaOut))
                {
                    outMsg.text =
                        "中文：控制标志|" + std::to_string(personaOut.control) + "\n" +
                        "中文：创建时间|" + std::to_string(personaOut.createdAt) + "\n" +
                        "中文：情绪|" + personaOut.emotion + "\n" +
                        "中文：优先级|" + std::to_string(personaOut.priority) + "\n" +
                        "中文：来源|" + std::to_string(personaOut.source) + "\n" +
                        "中文：文本|" + personaOut.text;
                }
                else
                {
                    outMsg.text = "人格AI执行失败";
                }
            }
            outputQueue.push(outMsg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    pclailife.ioThreadRunning = false;
    logError("ioThreadProc", "IO线程退出");
}
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