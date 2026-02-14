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
    // ===== 已初始化检查 =====
    if (pclailife.qtinit)
    {
        logError("initQt", "Qt 已经初始化，跳过");
        return true;
    }

    // ===== 创建 Qt 应用对象 =====
    // 使用静态变量保证 QApplication 只创建一次
    static int argc = 0;
    static char* argv[] = { nullptr };
    static QApplication app(argc, argv);

    // ===== 创建主界面 =====
    env.ui = new qtmain(pclailife.ui, nullptr);
    if (!env.ui)
    {
        logError("initQt", "qtmain 创建失败");
        return false;
    }

    env.ui->show();
    // ===== 创建主线程调度定时器 =====
    QTimer* timer = new QTimer(env.ui);
    QObject::connect(timer, &QTimer::timeout, [this]()
    {
        // ===== 消费输出队列 =====
        while (!outputQueue.empty())
        {
            UiMessage msg = outputQueue.front();
            outputQueue.pop();
            // 主线程安全更新 UI
            if (env.ui)
            {
                env.ui->appendText(msg.text,0);
            }
        }
    });
    // 每 10ms 执行一次
    timer->start(10);

    // ===== 标记 Qt 初始化完成 =====
    pclailife.qtinit = true;
    logError("initQt", "Qt 初始化完成");
    return true;
}

bool ai_plc_delegate::initSql(const std::string& dbPath)
{
    // ===== 已打开则拒绝 =====
    if (pclailife.sqlinit)
    {
        logError("initSql", "数据库已打开，拒绝重复打开");
        lastError = "数据库已打开，请先关闭当前工程";
        return false;
    }

    logError("initSql", "开始初始化数据库: " + dbPath);

    // ===== 清理旧对象（理论上不会执行，但保持安全） =====
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

    // ===== 创建数据库客户端 =====
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

    // ===== 打开数据库 =====
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

    // ===== 标记数据库初始化成功 =====
    pclailife.sqlinit = true;
    logError("initSql", "数据库打开成功");

    // ===== 初始化 AI =====
    if (!initai())
    {
        pclailife.sqlinit = false;
        return false;
    }

    // ===== 初始化 人格 =====
    if (!initpersona())
    {
        pclailife.sqlinit = false;
        return false;
    }

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
    callDesc.useCloud = 0;
    callDesc.provider = AIProvider::Ollama;
    callDesc.apiKey.clear();
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

            // ===== Live2D 指令处理 =====
            if (msg.text == "live2d=0")
            {
                pclailife.ui.live2dEnabled = 0;
                outMsg.text = "Live2D 已销毁";
            }
            else if (msg.text == "live2d=1")
            {
                pclailife.ui.live2dEnabled = 1;
                outMsg.text = "Live2D 已创建";
            }
            else if (msg.text == "live2d=2")
            {
                pclailife.ui.live2dEnabled = 2;
                outMsg.text = "Live2D 已渲染";
            }
           // ===== 记忆测试指令 =====
            else if (msg.text == "memory1")
            {
                if (!pclailife.personainit || !managers.persona)
                {
                    outMsg.text = "人格未初始化";
                }
                else
                {
                    if (managers.persona->writeSelfLongMemory())
                    {
                        // ===== 重新解析镜像 =====
                        std::vector<std::string> rows = parsePersonaMirror();

                        // ===== 刷新 UI =====
                        if (env.ui)
                        {
                            env.ui->updatePersonaMirror(rows, 1);
                        }

                        outMsg.text = "人格1-3长期记忆已更新";
                    }
                    else
                    {
                        outMsg.text = "人格1-3更新失败";
                    }
                }
            }
            else if (msg.text == "memory2")
            {
                if (!pclailife.personainit || !managers.persona)
                {
                    outMsg.text = "人格未初始化";
                }
                else
                {
                    if (managers.persona->writeUserLongMemory())
                    {
                        // ===== 重新解析镜像 =====
                        std::vector<std::string> rows = parsePersonaMirror();

                        // ===== 刷新 UI =====
                        if (env.ui)
                        {
                            env.ui->updatePersonaMirror(rows, 1);
                        }

                        outMsg.text = "用户4-9长期记忆已更新";
                    }
                    else
                    {
                        outMsg.text = "用户4-9更新失败";
                    }
                }
            }
            else if (msg.text == "memoryoff")
            {
                if (!pclailife.personainit || !managers.persona)
                {
                    outMsg.text = "人格未初始化";
                }
                else
                {
                    // ===== 清空 Chat 短期记忆 =====
                    modules.chatAi->clearShortHistory();

                    outMsg.text = "短期记忆已清空";
                }
            }

            else
            {
                if (!pclailife.personainit || !managers.persona)
                {
                    outMsg.text = "人格AI未初始化";
                }
                else
                {
                    logError("ioThreadProc", "进入人格ai调用");
                    PersonaMessageIn inMsg;
                    inMsg.type = 1;
                    inMsg.text = msg.text;
                    inMsg.createdAt = static_cast<int>(time(nullptr));
                    managers.persona->pushInput(inMsg);
                    managers.persona->processOnce();
                    PersonaMessageOut personaOut;
                    if (managers.persona->popOutput(personaOut))
                    {
                        outMsg.text = personaOut.text;
                    }
                    else
                    {
                        outMsg.text = "人格AI无输出";
                    }
                }
            }
            outputQueue.push(outMsg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    pclailife.ioThreadRunning = false;
    logError("ioThreadProc", "IO线程退出");
}

void ai_plc_delegate::upperThreadProc()
{
    logError("upperThreadProc", "上位机线程创建完成");

    pclailife.upperThreadRunning = true;
    pclailife.upperThreadStopping = false;

    while (!pclailife.upperThreadStopping)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    pclailife.upperThreadRunning = false;
    logError("upperThreadProc", "上位机线程退出");
}

std::vector<std::string> ai_plc_delegate::parsePersonaMirror()
{
    std::vector<std::string> rows;

    // ===== 解析 selfMemory =====
    for (int i = 0; i < 3; ++i)
    {
        const CurrentMemory& mem = mirror.memoryState.selfMemory[i];

        std::string line = mem.keyPath;
        line += "|";
        line += mem.content;

        rows.push_back(line);
    }

    // ===== 解析 userMemory =====
    for (int i = 0; i < 6; ++i)
    {
        const CurrentMemory& mem = mirror.memoryState.userMemory[i];

        std::string line = mem.keyPath;
        line += "|";
        line += mem.content;

        rows.push_back(line);
    }

    return rows;
}
