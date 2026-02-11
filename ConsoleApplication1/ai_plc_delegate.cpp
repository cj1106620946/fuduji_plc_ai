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

// 这里传入引用
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
    env.sqlStore = new SqlStore(env.sqlClient);

    if (!env.sqlStore->open())
    {
        lastError = env.sqlStore->getLastErrorText();
        logError("initSql", "数据库打开失败: " + lastError);
        return false;
    }

    logError("initSql", "数据库打开成功");
    pclailife.sqlinit = true;
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


