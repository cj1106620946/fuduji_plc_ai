
#include "projectmanager.h"
ProjectManager::ProjectManager(
    PLCClient& plc,
    uppermachine& upper,
    WorkspaceAI& workspaceAI,
    ExecuteAI& executeAI,
    DecisionAI& decisionAI
)
    : plcRef(plc),
    upperRef(upper),
    workspaceAIRef(workspaceAI),
    executeAIRef(executeAI),
    decisionAIRef(decisionAI),
    running(false)
{
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
    
}
bool ProjectManager::start()
{

}
void ProjectManager::stop()
{

}

bool ProjectManager::isReady() const
{
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

    // 1. 调用 WorkspaceAI 解析 PLC 信息
    std::string result = workspaceAIRef.runPlcOnce(
        userInput,
        plcName,
        ip,
        rack,
        slot,
        description
    );

    if (result != "OK")
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
    if (!upperRef.createPlcInfoRow(info))
    {
        lastError = upperRef.getLastError();
        logError("createPlcByAI", lastError);
        return false;
    }

    return true;
}
bool ProjectManager::createSignalsByAI(const std::string& userInput)
{
    std::vector<SignalWorkspaceData> aiSignals;
    // 1. 调用 WorkspaceAI 生成信号列表
    std::string result = workspaceAIRef.runSignalOnce(
        userInput,
        aiSignals
    );
    if (result != "OK")
    {
        lastError = result;
        logError("createSignalsByAI", result);
        return false;
    }
    // 2. 逐个交给 uppermachine 创建
    for (const auto& sig : aiSignals)
    {
        if (!upperRef.createSignalRow(
            sig.name,
            sig.plc_address,
            currentPlc.signalRootId,   // 当前 PLC id
            sig.description))
        {
            lastError = upperRef.getLastError();
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
    const std::string& userInput
)
{
    std::string plcName;
    std::string ip;
    int rack = 0;
    int slot = 0;
    std::string description;

    // 1. 调用 WorkspaceAI（PLC）
    std::string result = workspaceAIRef.runPlcOnce(
        userInput,
        plcName,
        ip,
        rack,
        slot,
        description
    );
    if (result != "OK")
    {
        lastError = result;
        logError("createPlcWorkspaceByAI", result);
        return false;
    }

    // 2. 组装最小 plcinfo（其余字段后续补）
    plcinfo info{};
    info.taskDesc = description;
    info.ipAddress = ip;
    info.rack = rack;
    info.slot = slot;
    info.isActive = 0;

    // 3. 写入数据库（通过 uppermachine）
    if (!upperRef.createPlcInfoRow(info))
    {
        lastError = upperRef.getLastError();
        logError("createPlcWorkspaceByAI", lastError);
        return false;
    }

    return true;
}
bool ProjectManager::createSignalWorkspaceByAI(
    const std::string& userInput
)
{
    std::vector<SignalWorkspaceData> aiSignals;
    // 1. 调用 WorkspaceAI（Signal）
    std::string result = workspaceAIRef.runSignalOnce(
        userInput,
        aiSignals
    );

    if (result != "OK")
    {
        lastError = result;
        logError("createSignalWorkspaceByAI", result);
        return false;
    }
    for (const auto& sig : aiSignals)
    {
        if (!upperRef.createSignalRow(
            sig.name,
            sig.plc_address,
            currentPlc.signalRootId,
            sig.description
        ))
        {
            lastError = upperRef.getLastError();
            logError("createSignalWorkspaceByAI", lastError);
            return false;
        }
    }

    return true;
}





void ProjectManager::runThreadProc()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
void ProjectManager::upperThreadProc()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
void ProjectManager::aiThreadProc()
{
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
