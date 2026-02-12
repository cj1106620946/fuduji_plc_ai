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

    pclailife.qtinit = true;
    logError("initQt", "Qt 初始化完成");

    return true;
}


bool ai_plc_delegate::initSql(const std::string& dbPath)
{
    if (pclailife.sqlinit)
    {
        logError("initSql", "数据库已初始化，重新打开");
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
        return false;
    }

    env.sqlStore = new SqlStore(env.sqlClient);
    if (!env.sqlStore)
    {
        logError("initSql", "SqlStore 创建失败");
        return false;
    }

    if (!env.sqlStore->open())
    {
        lastError = env.sqlStore->getLastErrorText();
        logError("initSql", "数据库打开失败: " + lastError);
        return false;
    }

    pclailife.sqlinit = true;
    logError("initSql", "数据库打开成功");

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
        return false;
    }

    modules.aiController = new AIController(*modules.aiClient);
    if (!modules.aiController)
    {
        logError("initai", "AIController 创建失败");
        return false;
    }

    modules.aiTrace = new AITrace();
    if (!modules.aiTrace)
    {
        logError("initai", "AITrace 创建失败");
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
        mirror.personaState
    );

    if (!managers.persona)
    {
        logError("initpersona", "personamanager 创建失败");
        return false;
    }

    managers.persona->init();
    managers.persona->initMemory();
    managers.persona->initAI();

    mirror.personaState.inited = true;
    pclailife.personainit = true;

    logError("initpersona", "人格AI初始化完成");

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
        3,
        *modules.aiController,
        *modules.aiTrace
    );

    if (!modules.workspaceAi)
    {
        logError("initproject", "WorkspaceAI 创建失败");
        return false;
    }

    modules.executeAi = new ExecuteAI(
        4,
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
        mirror.projectState
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

    mirror.projectState.projectInited = true;
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
        logError("run", "收到 UI 文本: " + text);
        UiMessage msg;
        msg.type = UiMessageType::Text;
        msg.text = text;
        inputQueue.push(msg);
        logError("run", "消息已入队");
        onUiText(text);
    });

    QObject::connect(
        env.ui->getInitPanel(),
        &initpanel::openRequested,
        [this](const std::string& path)
    {
        if (!initSql(path))
        {
            env.ui->appendText("数据库错误: " + lastError);
            return;
        }

        env.ui->appendText("工程打开成功");
    });

    QObject::connect(
        env.ui->getInitPanel(),
        &initpanel::createRequested,
        [this](const std::string& path)
    {
        if (!initSql(path))
        {
            env.ui->appendText("数据库错误: " + lastError);
            return;
        }

        env.ui->appendText("工程创建成功");
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

    env.ui->appendText(text);

    // ===== Live2D 状态调试 =====
    if (text == "live2d=0")
    {
        pclailife.ui.live2dEnabled = 0;
        env.ui->updateRenderState();
        env.ui->appendText("Live2D 已销毁");
    }
    else if (text == "live2d=1")
    {
        pclailife.ui.live2dEnabled = 1;
        env.ui->updateRenderState();
        env.ui->appendText("Live2D 已创建");
    }
    else if (text == "live2d=2")
    {
        pclailife.ui.live2dEnabled = 2;
        env.ui->updateRenderState();
        env.ui->appendText("Live2D 已渲染");
    }
    else
    {
        std::string out = "输入：" + text;
        env.ui->appendText(out);
    }

    logError("onUiText", "处理完成");
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
void ai_plc_delegate::ioThreadProc()
{
    logError("ioThreadProc", "IO线程创建完成");

    pclailife.ioThreadRunning = true;
    pclailife.ioThreadStopping = false;

    while (!pclailife.ioThreadStopping)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    pclailife.ioThreadRunning = false;
    logError("ioThreadProc", "IO线程退出");
}
