#include "projectmanager.h"

// 构造函数：初始化引用成员并创建上位机对象 uppermachine
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

// 析构函数：释放上位机对象资源
ProjectManager::~ProjectManager()
{
    if (upperRef)
    {
        delete upperRef;
        upperRef = nullptr;
    }
}

// 日志记录：把错误信息追加到项目管理层日志文件（包含时间戳和来源函数）
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

// 初始化项目：设置 ProjectState / RunState 初始值并从数据库加载镜像
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

    // ===== 从数据库加载配置到镜像 =====
    if (upperRef)
    {
        if (!upperRef->initloadConfig())
        {
            lastError = upperRef->getLastError();
            logError("init", "从数据库加载配置失败: " + lastError);
            return false;
        }
    }
    upperinit();
    connectplcinit();
    aiinit();
    lastError.clear();
    return true;
}
// 上位机初始化：若未创建上位机或初始化失败则返回错误信息
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
// PLC 连接初始化：调用上位机的连接接口并更新 currentPlc.isActive 状态
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
// AI 通道初始化：准备 AI 队列与状态位，要求项目已初始化
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
// 停止项目：协调停止各线程与上位机，并清理运行标志
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
// 加载镜像：只从数据库读取配置到镜像，不改变运行状态
bool ProjectManager::loadProjectMirror()
{
    // 项目必须已经初始化
    if (!projectstate.projectInited)
    {
        lastError = "project not inited";
        return false;
    }

    if (!upperRef)
    {
        lastError = "upperRef null";
        return false;
    }

    // 只读取数据库中的镜像数据
    if (!upperRef->initloadConfig())
    {
        lastError = upperRef->getLastError();
        logError("loadProjectMirror", lastError);
        return false;
    }

    // 不修改 upperInited
    lastError.clear();
    return true;
}
// 检查准备状态：当前实现始终返回 true（保留扩展点）
bool ProjectManager::isReady() const
{
    return true;
}
// 推送 AI 消息：将用户输入封装为 AIMessage 并追加到队列（要求 AI 通道已初始化）
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
// 推送项目消息：将消息追加到 projectMessageQueue（供 UI 或日志消费）
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
// 弹出项目消息：从队列取出最先入队的一条消息并返回
bool ProjectManager::popProjectMessage(ProjectMessage& outMsg)
{
    if (projectMessageQueue.empty())
        return false;

    outMsg = projectMessageQueue.front();
    projectMessageQueue.erase(projectMessageQueue.begin());
    return true;
}

// 获取最近错误信息的只读引用
const std::string& ProjectManager::getLastError() const
{
    return lastError;
}

// 使用 WorkspaceAI 解析输入并通过 uppermachine 创建 PLC（最小信息写入）
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

    // 2. 组装 plcinfo
    plcinfo info;
    info.taskDesc = description;
    info.sourceText = userInput;  // 把用户原始输入作为 sourceText
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

// 使用 WorkspaceAI 生成信号列表并逐条通过 uppermachine 创建信号
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

// 从本地变量镜像中读取指定 PLC 地址的信息并格式化返回
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

// 在本地变量镜像中登记写入请求（设置 targetValue 与 writeFlag）
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

// 使用 ExecuteAI 生成执行项并按类型处理（支持 read/write），收集并返回执行结果文本列表
bool ProjectManager::executeByAI(
    const std::string& userInput,
    std::vector<std::string>& outMessages
)
{
    outMessages.clear();

    // 1) 打包当前镜像里的变量 name + address，交给 ExecuteAI
    std::string packed;
    packed.reserve(userInput.size() + worksignals.size() * 32 + 64);

    packed += "vars:\n";
    for (size_t i = 0; i < worksignals.size(); ++i)
    {
        const signalinfo& s = worksignals[i];
        packed += s.name;
        packed += " ";
        packed += s.plcAddress;
        packed += "\n";
    }
    packed += "user:\n";
    packed += userInput;

    // 2) 调用 ExecuteAI
    std::vector<ExecuteItem> items = executeAIRef.runOnce(packed);

    if (items.empty())
    {
        outMessages.push_back(u8"未生成任何可执行指令");
        return true;
    }

    // 3) 逐条处理 ExecuteItem
    for (const auto& it : items)
    {
        if (!it.message.empty())
            outMessages.push_back(it.message);

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

// 使用 WorkspaceAI 创建 PLC 工作区并写入数据库，返回创建过程中的信息回流
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
    info.sourceText = userInput;  // 加上这一行
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

// 使用 WorkspaceAI 创建变量工作区并写入数据库，返回创建数量信息
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

// 项目主运行线程体：当前为空转循环，保留作为扩展点
void ProjectManager::runThreadProc()
{
    while (projectstate.projectInited)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
// 上位机线程体：在允许运行时触发 uppermachine.run 并空转等待停止
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
        if (!runState.upperRunning)
        {
            upperRef->run();
        }

        // run 是一次性动作，之后进入空转等待 stop
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 项目生命周期结束，清理运行态事实
    projectstate.upperRunning = false;
    projectstate.upperStopping = false;
}
// AI 线程体：从 aiQueue 取消息并按类型分发到对应处理函数，记录回流日志
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
// 启动项目运行线程（如果尚未启动并且项目已初始化）
void ProjectManager::createRunThread()
{
    logError("createRunThread", "尝试启动 runThread");

    // 项目必须已初始化
    if (!projectstate.projectInited)
    {
        logError("createRunThread", "projectstate.projectInited = false");
        return;
    }

    if (runThread.joinable())
    {
        logError("createRunThread", "runThread 已存在");
        return;
    }

    runThread = std::thread(&ProjectManager::runThreadProc, this);
    projectstate.projectRunning = 1;

    logError("createRunThread", "runThread 启动成功");
}
// 启动上位机线程（在项目和上位机已初始化时）
void ProjectManager::createUpperThread()
{
    logError("createUpperThread", "尝试启动 upperThread");

    // 项目 + 上位机必须已初始化
    if (!projectstate.projectInited)
    {
        logError("createUpperThread", "projectstate.projectInited = false");
        return;
    }

    if (!projectstate.upperInited)
    {
        logError("createUpperThread", "upper 未初始化");
        return;
    }

    if (upperThread.joinable())
    {
        logError("createUpperThread", "upperThread 已存在");
        return;
    }

    upperThread = std::thread(&ProjectManager::upperThreadProc, this);
    projectstate.upperRunning = 1;

    logError("createUpperThread", "upperThread 启动成功");
}
// 启动 AI 处理线程（在项目和 AI 通道已初始化时）
void ProjectManager::createAIThread()
{
    logError("createAIThread", "尝试启动 aiThread");

    // 项目 + AI 通道必须已初始化
    if (!projectstate.projectInited)
    {
        logError("createAIThread", "projectstate.projectInited = false");
        return;
    }

    if (!projectstate.stringThreadInited)
    {
        logError("createAIThread", "AI 通道未初始化");
        return;
    }

    if (aiThread.joinable())
    {
        logError("createAIThread", "aiThread 已存在");
        return;
    }

    aiThread = std::thread(&ProjectManager::aiThreadProc, this);
    projectstate.stringThreadRunning = 1;

    logError("createAIThread", "aiThread 启动成功");
}
// 等待并销毁项目运行线程（阻塞直到线程结束）
void ProjectManager::destroyRunThread()
{
    if (!runThread.joinable())
        return;

    runThread.join();
    projectstate.projectRunning = 0;
}
// 等待并销毁上位机线程（阻塞直到线程结束）
void ProjectManager::destroyUpperThread()
{
    if (!upperThread.joinable())
        return;

    upperThread.join();
    projectstate.upperRunning = 0;
}
// 等待并销毁 AI 线程（阻塞直到线程结束）
void ProjectManager::destroyAIThread()
{
    if (!aiThread.joinable())
        return;

    aiThread.join();
    projectstate.stringThreadRunning = 0;
}
// 通过上位机接口删除指定 ID 的信号，要求上位机已初始化
bool ProjectManager::removeSignalById(int signalId)
{
    if (!upperRef)
    {
        lastError = "upper 未初始化";
        return false;
    }

    return upperRef->removeSignalById(signalId);
}
// 使用 WorkspaceAI 解析输入并通过 uppermachine 创建 PLC 工作区，返回过程信息
bool ProjectManager::createPlcWorkspaceByCmd(
    const std::string& ip,
    const std::string& name,
    int rack,
    int slot,
    const std::string& desc,
    std::vector<std::string>& outMessages
)
{
    outMessages.clear();

    if (!projectstate.projectInited)
    {
        lastError = "project 未初始化";
        outMessages.push_back(lastError);
        logError("createPlcWorkspaceByCmd", lastError);
        return false;
    }

    if (!upperRef)
    {
        lastError = "upperRef null";
        outMessages.push_back(lastError);
        logError("createPlcWorkspaceByCmd", lastError);
        return false;
    }

    if (ip.empty())
    {
        lastError = "IP 不能为空";
        outMessages.push_back(lastError);
        logError("createPlcWorkspaceByCmd", lastError);
        return false;
    }

    if (name.empty())
    {
        lastError = "PLC 名称不能为空";
        outMessages.push_back(lastError);
        logError("createPlcWorkspaceByCmd", lastError);
        return false;
    }

    plcinfo info{};

    // PLC 名称
    info.taskDesc = name;

    // 解释说明
    info.taskDomain = desc;
    // 原始输入来源
    info.sourceText = "plccreate " + name + " " + ip;
    // 连接参数
    info.ipAddress = ip;
    info.rack = rack;
    info.slot = slot;

    // 初始状态
    info.isActive = 0;

    // 时间
    int now = static_cast<int>(time(nullptr));
    info.createdAt = now;
    info.updatedAt = now;

    // 初始信号根
    info.signalRootId = 0;

    if (!upperRef->createPlcInfoRow(info))
    {
        lastError = upperRef->getLastError();
        outMessages.push_back(lastError);
        logError("createPlcWorkspaceByCmd", lastError);
        return false;
    }

    outMessages.push_back("PLC 工作区创建完成");
    lastError.clear();
    return true;
}

bool ProjectManager::createSignalWorkspaceByCmd(
    const std::vector<std::string>& parts,
    std::vector<std::string>& outMessages
)
{
    outMessages.clear();

    if (!projectstate.projectInited)
    {
        lastError = "project 未初始化";
        outMessages.push_back(lastError);
        logError("createSignalWorkspaceByCmd", lastError);
        return false;
    }

    if (!upperRef)
    {
        lastError = "upperRef null";
        outMessages.push_back(lastError);
        logError("createSignalWorkspaceByCmd", lastError);
        return false;
    }

    // signalcreate 至少需要 1 组: name addr desc
    if (parts.size() < 4)
    {
        lastError = "signalcreate 参数不足";
        outMessages.push_back(lastError);
        logError("createSignalWorkspaceByCmd", lastError);
        return false;
    }

    // 三元组校验: 去掉命令名后，剩余必须是 3 的倍数
    if (((int)parts.size() - 1) % 3 != 0)
    {
        lastError = "signalcreate 参数必须三元组 name addr desc";
        outMessages.push_back(lastError);
        logError("createSignalWorkspaceByCmd", lastError);
        return false;
    }

    int createdCount = 0;

    // 从 parts[1] 开始，每 3 个一组
    for (size_t i = 1; i + 2 < parts.size(); i += 3)
    {
        const std::string& sigName = parts[i];
        const std::string& sigAddr = parts[i + 1];
        const std::string& sigDesc = parts[i + 2];

        if (sigName.empty() || sigAddr.empty())
        {
            lastError = "变量名或地址为空";
            outMessages.push_back(lastError);
            logError("createSignalWorkspaceByCmd", lastError);
            return false;
        }

        // 中文说明允许为空，如果你希望禁止为空，这里再加判断
        if (!upperRef->createSignalRow(
            sigName,
            sigAddr,
            currentPlc.signalRootId,
            sigDesc
        ))
        {
            lastError = upperRef->getLastError();
            outMessages.push_back(lastError);
            logError("createSignalWorkspaceByCmd", lastError);
            return false;
        }

        createdCount += 1;
    }

    outMessages.push_back("变量工作区创建完成，数量: " + std::to_string(createdCount));
    lastError.clear();
    return true;
}

