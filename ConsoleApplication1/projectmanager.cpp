#include "projectmanager.h"

ProjectManager::ProjectManager(
    SqlStore& store,
    WorkspaceAI& workspaceAI,
    ExecuteAI& executeAI,
    DecisionAI& decisionAI,
    plcinfo& currentPlcRef,
    std::vector<signalinfo>& worksignalsRef,
    ProjectState& projectstateRef
)
    :
    workspaceAIRef(workspaceAI),
    executeAIRef(executeAI),
    decisionAIRef(decisionAI),
    currentPlc(currentPlcRef),
    worksignals(worksignalsRef),
    projectstate(projectstateRef)
{
    upperRef = new uppermachine(
        store,
        runState,
        currentPlc,
        worksignals
    );
}
ProjectManager::~ProjectManager()
{
    if (upperRef)
    {
        delete upperRef;
        upperRef = nullptr;
    }
}

void ProjectManager::logError(
    const std::string& fromFunc,
    const std::string& reason
)
{
    // 打开项目管理层日志文件
    std::ofstream logFile("error//projectmanager.log", std::ios::app);
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

    // 写入流程层错误日志
    logFile
        << "[" << buf << "] "
        << fromFunc
        << " | "
        << reason
        << std::endl;

    logFile.close();
}

bool ProjectManager::init()
{
    // ===== ProjectState：项目级生命周期 =====
    projectstate.projectInited = true;
    projectstate.projectRunning = false;
    projectstate.stopping = false;

    projectstate.upperInited = false;
    projectstate.upperRunning = false;
    projectstate.upperStopping = false;

    projectstate.stringThreadInited = false;
    projectstate.stringThreadRunning = false;
    projectstate.stringThreadStopping = false;

    projectstate.aiInputEnabled = false;
    projectstate.aiBusy = false;

    // ===== RunState：上位机与线程运行态 =====
    runState.upperInited = false;
    runState.upperRunning = false;
    runState.upperStopping = false;

    runState.readInited = false;
    runState.readRunning = false;
    runState.readStopping = false;

    runState.writeThreadInited = false;
    runState.writeThreadRunning = false;
    runState.writeThreadStopping = false;

    // ===== plcinfo：PLC 镜像 =====
    currentPlc.taskDesc.clear();
    currentPlc.taskDomain.clear();
    currentPlc.sourceText.clear();

    currentPlc.plcModel.clear();
    currentPlc.orderCode.clear();
    currentPlc.ipAddress.clear();
    currentPlc.rack = 0;
    currentPlc.slot = 0;
    currentPlc.signalRootId = 0;

    currentPlc.isActive = 0;
    currentPlc.createdAt = 0;
    currentPlc.updatedAt = 0;

    // ===== signalinfo：变量镜像 =====
    worksignals.clear();

    lastError.clear();
    return true;
}

bool ProjectManager::upperinit()
{
    // 已经初始化过则直接返回
    if (projectstate.upperInited)
        return true;

    if (!upperRef)
    {
        lastError = "uppermachine 未创建";
        return false;
    }

    // 调用上位机初始化
    if (!upperRef->init())
    {
        lastError = upperRef->getLastError();
        return false;
    }

    // 记录初始化事实
    projectstate.upperInited = true;

    return true;
}

bool ProjectManager::connectplcinit()
{
    if (!upperRef)
    {
        lastError = "uppermachine 未创建";
        return false;
    }

    // 调用上位机进行 PLC 连接
    if (!upperRef->initconnectPlc())
    {
        // 连接失败，仅记录失败事实
        currentPlc.isActive = 0;
        lastError = upperRef->getLastError();
        return false;
    }

    // 连接成功，结构体镜像直接生效
    currentPlc.isActive = 1;
    return true;
}

bool ProjectManager::aiinit()
{
    // AI 初始化只允许在项目已初始化后进行
    if (!projectstate.projectInited)
    {
        lastError = "project 尚未初始化，无法初始化 AI";
        return false;
    }

    // 已初始化则直接返回
    if (projectstate.stringThreadInited)
        return true;

    // ===== 清空 AI 消息队列 =====
    aiQueue.clear();

    // ===== 初始化 AI 通道标志位 =====
    projectstate.stringThreadInited = true;
    projectstate.stringThreadRunning = false;
    projectstate.stringThreadStopping = false;

    projectstate.aiInputEnabled = false;
    projectstate.aiBusy = false;

    lastError.clear();
    return true;
}

void ProjectManager::stop()
{
    // 已经在停止流程中，直接返回
    if (projectstate.stopping)
        return;
    projectstate.stopping = true;
    if (projectstate.stringThreadInited)
    {
        projectstate.stringThreadStopping = true;
        projectstate.stringThreadRunning = false;
    }
    if (projectstate.upperInited && upperRef)
    {
        projectstate.upperStopping = true;
        upperRef->stop();   // 
        projectstate.upperRunning = false;
    }
    projectstate.projectRunning = false;
    projectstate.projectInited = false;

    if (runThread.joinable())
        runThread.join();

    if (upperThread.joinable())
        upperThread.join();

    if (aiThread.joinable())
        aiThread.join();
    projectstate.upperStopping = false;
    projectstate.stringThreadStopping = false;
    projectstate.stopping = false;
}

bool ProjectManager::isReady() const
{
    return true;
}

bool ProjectManager::pushAIMessage(const std::string& text, int source, int type)
{
    if (!projectstate.stringThreadInited)
        return false;

    AIMessage msg;
    msg.text = text;
    msg.source = source;
    msg.type = type;
    msg.createdAt = static_cast<int>(time(nullptr));

    aiQueue.push_back(msg);
    return true;
}

void ProjectManager::pushProjectMessage(
    const std::string& text,
    int source
)
{
    ProjectMessage msg;
    msg.text = text;
    msg.source = source;
    msg.createdAt = static_cast<int>(time(nullptr));

    projectMessageQueue.push_back(msg);
}

bool ProjectManager::popProjectMessage(ProjectMessage& outMsg)
{
    if (projectMessageQueue.empty())
        return false;

    outMsg = projectMessageQueue.front();
    projectMessageQueue.erase(projectMessageQueue.begin());
    return true;
}

const std::string& ProjectManager::getLastError() const
{
    return lastError;
}

bool ProjectManager::createPlcByAI(const std::string& userInput)
{
    std::string plcName;
    std::string ip;
    int rack = 0;
    int slot = 0;
    std::string description;
    int a;
    // 1. 调用 WorkspaceAI 解析 PLC 信息
    std::string result = workspaceAIRef.runPlcOnce(
        userInput,
        plcName,
        ip,
        rack,
        slot,
        description,
        a
    );

    if (!a)
    {
        lastError = result;
        logError("createPlcByAI", result);
        return false;
    }

    // 2. 组装 plcinfo（这里只做最小赋值）
    plcinfo info;
    info.taskDesc = description;
    info.ipAddress = ip;
    info.rack = rack;
    info.slot = slot;

    // 其余字段由数据库或后续流程补全

    // 3. 交给 uppermachine 创建
    if (!upperRef->createPlcInfoRow(info))
    {
        lastError = upperRef->getLastError();
        logError("createPlcByAI", lastError);
        return false;
    }

    return true;
}

bool ProjectManager::createSignalsByAI(const std::string& userInput)
{
    std::vector<SignalWorkspaceData> aiSignals;
    int a;
    // 1. 调用 WorkspaceAI 生成信号列表
    std::string result = workspaceAIRef.runSignalOnce(
        userInput,
        aiSignals,
        a
    );
    if (!a)
    {
        lastError = result;
        logError("createSignalsByAI", result);
        return false;
    }
    // 2. 逐个交给 uppermachine 创建
    for (const auto& sig : aiSignals)
    {
        if (!upperRef->createSignalRow(
            sig.name,
            sig.plc_address,
            currentPlc.signalRootId,   // 当前 PLC id
            sig.description))
        {
            lastError = upperRef->getLastError();
            logError("createSignalsByAI", lastError);
            return false;
        }
    }
    return true;
}

bool ProjectManager::readSignal(
    const std::string& plcAddress,
    std::string& outResult
)
{
    for (const auto& sig : worksignals)
    {
        if (sig.plcAddress == plcAddress)
        {
            outResult =
                "变量名: " + sig.name +
                " 地址: " + sig.plcAddress +
                " 当前值: " + sig.currentValue +
                " 说明: " + sig.description;
            return true;
        }
    }
    outResult = u8"plc 地址不存在";
    lastError = outResult;
    logError("readSignal", outResult);
    return false;
}

bool ProjectManager::writeSignal(
    const std::string& plcAddress,
    const std::string& value,
    std::string& outResult
)
{
    for (auto& sig : worksignals)
    {
        if (sig.plcAddress == plcAddress)
        {
            sig.targetValue = value;
            sig.writeFlag = 1;

            outResult =
                "写入请求已登记 "
                "变量名: " + sig.name +
                " 地址: " + sig.plcAddress +
                " 目标值: " + value +
                " 说明: " + sig.description;
            return true;
        }
    }

    outResult = u8"plc 地址不存在";
    lastError = outResult;
    logError("writeSignal", outResult);
    return false;
}

bool ProjectManager::executeByAI(
    const std::string& userInput,
    std::vector<std::string>& outMessages
)
{
    outMessages.clear();

    // 1. 调用 ExecuteAI
    std::vector<ExecuteItem> items =
        executeAIRef.runOnce(userInput);

    if (items.empty())
    {
        outMessages.push_back(u8"未生成任何可执行指令");
        return true;
    }

    // 2. 逐条处理 ExecuteItem
    for (const auto& it : items)
    {
        // message 是 AI 对“整体意图”的说明，只压一次即可
        if (!it.message.empty())
            outMessages.push_back(it.message);

        // 没有 op，说明只是解释性返回（type=error 或无动作）
        if (it.op.empty())
            continue;

        std::string result;

        if (it.op == "read")
        {
            if (!readSignal(it.address, result))
            {
                outMessages.push_back(result);
                logError("executeByAI.read", result);
            }
            else
            {
                outMessages.push_back(result);
            }
        }
        else if (it.op == "write")
        {
            if (!writeSignal(it.address, it.value, result))
            {
                outMessages.push_back(result);
                logError("executeByAI.write", result);
            }
            else
            {
                outMessages.push_back(result);
            }
        }
        else
        {
            std::string err = std::string(u8"未知操作类型: ") + it.op;
            outMessages.push_back(err);
            logError("executeByAI", err);
        }
    }

    return true;
}

bool ProjectManager::createPlcWorkspaceByAI(
    const std::string& userInput,
    std::vector<std::string>& outMessages
)
{
    outMessages.clear();

    std::string plcName;
    std::string ip;
    int rack = 0;
    int slot = 0;
    std::string description;
    int a;
    // 1. 调用 WorkspaceAI（PLC）
    std::string result = workspaceAIRef.runPlcOnce(
        userInput,
        plcName,
        ip,
        rack,
        slot,
        description,
        a
    );

    // result 现在是 json.error（可能为空）
    if (!result.empty())
        outMessages.push_back(result);

    // 失败直接返回
    if (result != "")
    {
        lastError = result;
        logError("createPlcWorkspaceByAI", result);
        return false;
    }

    // 2. 组装最小 plcinfo
    plcinfo info{};
    info.taskDesc = description;
    info.ipAddress = ip;
    info.rack = rack;
    info.slot = slot;
    info.isActive = 0;

    // 3. 写入数据库
    if (!upperRef->createPlcInfoRow(info))
    {
        lastError = upperRef->getLastError();
        outMessages.push_back(lastError);
        logError("createPlcWorkspaceByAI", lastError);
        return false;
    }

    // 成功回流（由 ProjectManager 统一生成）
    outMessages.push_back(u8"PLC 工作区创建完成");
    return true;
}

bool ProjectManager::createSignalWorkspaceByAI(
    const std::string& userInput,
    std::vector<std::string>& outMessages
)
{
    outMessages.clear();

    std::vector<SignalWorkspaceData> aiSignals;

    int a;
    // 1. 调用 WorkspaceAI（Signal）
    std::string result = workspaceAIRef.runSignalOnce(
        userInput,
        aiSignals,
        a
    );

    // error / 成功说明回流
    if (!result.empty())
        outMessages.push_back(result);

    if (result != "")
    {
        lastError = result;
        logError("createSignalWorkspaceByAI", result);
        return false;
    }

    // 2. 创建变量
    for (const auto& sig : aiSignals)
    {
        if (!upperRef->createSignalRow(
            sig.name,
            sig.plc_address,
            currentPlc.signalRootId,
            sig.description
        ))
        {
            lastError = upperRef->getLastError();
            outMessages.push_back(lastError);
            logError("createSignalWorkspaceByAI", lastError);
            return false;
        }
    }

    // 成功回流
    outMessages.push_back(
        u8"变量工作区创建完成，数量: " +
        std::to_string(aiSignals.size())
    );

    return true;
}

void ProjectManager::runThreadProc()
{
    while (projectstate.projectInited)
    {

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void ProjectManager::upperThreadProc()
{
    while (projectstate.projectInited)
    {
        // 未允许运行 或 正在停止：线程存活但不工作
        if (!projectstate.upperRunning || projectstate.upperStopping)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // ===== 上位机 run 只允许触发一次 =====
        if (upperRef)
        {
            // 调用上位机运行（启动其内部线程）
            upperRef->run();
        }

        // run 是一次性动作，之后进入空转等待 stop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 项目生命周期结束，清理运行态事实
    projectstate.upperRunning = false;
    projectstate.upperStopping = false;
}

void ProjectManager::aiThreadProc()
{
    while (projectstate.projectInited)
    {
        if (!projectstate.stringThreadRunning || projectstate.stringThreadStopping)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (projectstate.aiBusy)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (aiQueue.empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        AIMessage msg = aiQueue.front();
        aiQueue.erase(aiQueue.begin());
        projectstate.aiBusy = true;

        std::vector<std::string> outMessages;

        if (msg.type == 0)
        {
            outMessages.push_back(
                u8"无法识别输入内容"
            );
            logError("aiThreadProc", "AIMessage type=0");
        }
        else if (msg.type == 10)
        {
            // PLC Workspace
            if (!createPlcWorkspaceByAI(msg.text, outMessages))
            {
                logError(
                    "aiThreadProc.PLCWorkspace",
                    lastError
                );
            }
        }
        else if (msg.type == 11)
        {
            // Signal Workspace
            if (!createSignalWorkspaceByAI(msg.text, outMessages))
            {
                logError(
                    "aiThreadProc.SignalWorkspace",
                    lastError
                );
            }
        }
        else if (msg.type == 20)
        {
            // ExecuteAI
            if (!executeByAI(msg.text, outMessages))
            {
                logError(
                    "aiThreadProc.ExecuteAI",
                    lastError
                );
            }
        }
        else if (msg.type == 30)
        {
            // DecisionAI 占位
            outMessages.push_back(
                u8"决策分析已完成（未修改系统）"
            );
        }
        else
        {
            outMessages.push_back(
                u8"未知 AI 消息类型"
            );
            logError(
                "aiThreadProc",
                "未知 AIMessage.type=" + std::to_string(msg.type)
            );
        }

        for (const auto& msg : outMessages)
        {
            logError("AIResult", msg);
        }
        projectstate.aiBusy = false;

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    projectstate.stringThreadRunning = false;
    projectstate.stringThreadStopping = false;
    projectstate.aiBusy = false;
}

void ProjectManager::createRunThread()
{
    // 项目必须已初始化
    if (!projectstate.projectInited)
        return;

    if (runThread.joinable())
        return;

    runThread = std::thread(&ProjectManager::runThreadProc, this);
    projectstate.projectRunning = 1;
}

void ProjectManager::createUpperThread()
{
    // 项目 + 上位机必须已初始化
    if (!projectstate.projectInited)
        return;

    if (!projectstate.upperInited)
        return;

    if (upperThread.joinable())
        return;

    upperThread = std::thread(&ProjectManager::upperThreadProc, this);
    projectstate.upperRunning = 1;
}

void ProjectManager::createAIThread()
{
    // 项目 + AI 通道必须已初始化
    if (!projectstate.projectInited)
        return;

    if (!projectstate.stringThreadInited)
        return;

    if (aiThread.joinable())
        return;

    aiThread = std::thread(&ProjectManager::aiThreadProc, this);
    projectstate.stringThreadRunning = 1;
}

void ProjectManager::destroyRunThread()
{
    if (!runThread.joinable())
        return;

    runThread.join();
    projectstate.projectRunning = 0;
}

void ProjectManager::destroyUpperThread()
{
    if (!upperThread.joinable())
        return;

    upperThread.join();
    projectstate.upperRunning = 0;
}

void ProjectManager::destroyAIThread()
{
    if (!aiThread.joinable())
        return;

    aiThread.join();
    projectstate.stringThreadRunning = 0;
}
