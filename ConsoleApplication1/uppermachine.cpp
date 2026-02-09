#include "uppermachine.h"


uppermachine::uppermachine(
    SqlStore& storeRef,
    RunState& runStateRef,
    plcinfo& plcRefInfo,
    std::vector<signalinfo>& signalRefList
)
    : plc(),
      store(storeRef),
      runState(runStateRef),
      currentPlc(plcRefInfo),
      worksignals(signalRefList)
{
}
uppermachine::~uppermachine()
{
    if (readThread.joinable())
        readThread.detach();   

    if (writeThread.joinable())
        writeThread.detach(); 
}




void uppermachine::logOp(
    const std::string& fromFunc,   // 函数名（例如 init / readThreadProc）
    const std::string& action      // 执行的操作（例如 初始化开始）
)
{
    // 打开日志文件，追加模式
    std::ofstream logFile("error//uppermachine.log", std::ios::app);

    if (!logFile.is_open()) {
        std::cerr << "无法打开日志文件" << std::endl;
        return;
    }

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
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
        local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday,
        local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);

    // 写入日志内容
    logFile << "[" << buf << "] " << fromFunc << " | " << action << std::endl;

    // 关闭日志文件
    logFile.close();
}
bool uppermachine::createPlcInfoRow(const plcinfo& info)
{
    logOp("createPlcInfoRow", "开始创建 PLC 上位机信息");

    if (!store.createPlcInfo(info))
    {
        lastError = store.getLastErrorText();
        logOp(
            "createPlcInfoRow",
            std::string("创建失败，原因: ") + lastError
        );
        return false;
    }
    logOp("createPlcInfoRow", "PLC 上位机信息创建成功");
    lastError.clear();
    return true;
}

bool uppermachine::createSignalRow(
    const std::string& name,
    const std::string& plcAddress,
    int plcId,
    const std::string& description
)
{
    logOp(
        "createSignalRow",
        std::string("开始创建变量，地址: ") + plcAddress
    );

    if (!store.createSignal(name, plcAddress, description))
    {
        lastError = store.getLastErrorText();
        logOp(
            "createSignalRow",
            std::string("创建失败，原因: ") + lastError
        );
        return false;
    }

    // 构造最小内存态 signal
    signalinfo sig;
    sig.name = name;
    sig.plcAddress = plcAddress;
    sig.description = description;
    sig.currentValue = "0";
    sig.targetValue.clear();
    sig.readOk = 0;
    sig.writeFlag = 0;
    sig.lastOpAt = static_cast<int>(time(nullptr));

    // 追加到 worksignals
    worksignals.push_back(sig);

    // 记录地址索引（地址 -> worksignals 下标）
    signalIndexByAddr[plcAddress] = worksignals.size() - 1;

    logOp(
        "createSignalRow",
        std::string("变量创建成功，地址: ") + plcAddress
    );

    lastError.clear();
    return true;
}
bool uppermachine::readplc(
    const std::string& plcAddress,
    std::string& outResult
)
{
    // 查找地址索引
    auto it = signalIndexByAddr.find(plcAddress);
    if (it == signalIndexByAddr.end())
    {
        lastError = u8"plc 地址不存在";
        outResult = lastError;
        return false;
    }
    const signalinfo& sig = worksignals[it->second];

    // 组合输出结果：
    // 变量名 + 地址 + 查询值 + 中文解释
    outResult =
        "变量名: " + sig.name +
        " 地址: " + sig.plcAddress +
        " 当前值: " + sig.currentValue +
        " 说明: " + sig.description;
    return true;
}
bool uppermachine::writeplc(
    const std::string& plcAddress,
    const std::string& value,
    std::string& outResult
)
{
    // 查找地址索引
    auto it = signalIndexByAddr.find(plcAddress);
    if (it == signalIndexByAddr.end())
    {
        lastError = u8"plc 地址不存在";
        outResult = lastError;
        return false;
    }

    signalinfo& sig = worksignals[it->second];

    // 设置写入意图（只修改镜像，不直接写 PLC）
    sig.targetValue = value;
    sig.writeFlag = 1;
    outResult =
        "写入请求已提交 "
        "变量名: " + sig.name +
        " 地址: " + sig.plcAddress +
        " 目标值: " + value +
        " 说明: " + sig.description;

    return true;
}


bool uppermachine::init()
{
    logOp("init", "初始化开始");

    // ===== 1. 加载配置与变量（必须成功）=====
    if (!initloadConfig())
    {
        logOp("init", "加载配置失败，初始化中止");
        return false;
    }

    // ===== 2. 尝试连接 PLC（允许失败）=====
    if (!initconnectPlc())
    {
        logOp(
            "init",
            "PLC 未连接成功，初始化继续，允许后续重新连接"
        );
        // 注意：这里不 return false
    }

    // ===== 3. 记录初始化完成事实 =====
    runState.upperInited = true;
    runState.readInited = true;
    runState.writeThreadInited = true;

    logOp("init", "初始化完成");
    lastError.clear();
    return true;
}

bool uppermachine::initloadConfig()
    {
        logOp("loadConfig", "开始加载 PLC 配置与变量定义");
        currentPlc = plcinfo{};
        worksignals.clear();
        plcinfo info;
        if (!store.readPlcInfo(info))
        {
            lastError = store.getLastErrorText();
            logOp("loadConfig", "读取 PLC 信息失败: " + lastError);
            return false;
        }

        currentPlc = info;

        currentPlc.isActive = 0;
        currentPlc.updatedAt = static_cast<int>(time(nullptr));

        logOp(
            "loadConfig",
            "当前 PLC: IP=" + currentPlc.ipAddress +
            " rack=" + std::to_string(currentPlc.rack) +
            " slot=" + std::to_string(currentPlc.slot)
        );


        std::vector<signalinfo> dbSignals;
        if (!store.readAllSignalInfo(dbSignals))
        {
            lastError = store.getLastErrorText();
            logOp("loadConfig", "读取变量列表失败: " + lastError);
            return false;
        }

        int now = static_cast<int>(time(nullptr));
        for (signalinfo& sig : dbSignals)
        {
            sig.currentValue.clear();   // 尚未读取 PLC
            sig.readOk = 0;             // 尚未进行读取
            sig.writeFlag = 0;          // 清空写请求
            sig.isAvailable = 1;        // 初始化认为可用
            sig.lastOpAt = now;
        }

        worksignals = dbSignals;
        signalIndexByAddr.clear();
        for (size_t i = 0; i < worksignals.size(); ++i)
        {
            signalIndexByAddr[worksignals[i].plcAddress] = i;
        }

        logOp(
            "loadConfig",
            "变量加载完成，数量=" + std::to_string(worksignals.size())
        );

        lastError.clear();
        return true;
    }
bool uppermachine::initconnectPlc()
{
    logOp("connectPlc", "开始连接 PLC");

    // 默认认为未连接成功
    currentPlc.isActive = 0;

    if (!plc.connectPLC(
        currentPlc.ipAddress,
        currentPlc.rack,
        currentPlc.slot))
    {
        lastError = plc.getLastErrorText();
        logOp("connectPlc", "PLC 连接失败: " + lastError);
        return false;
    }

    // 连接成功，记录事实状态
    currentPlc.isActive = 1;

    logOp("connectPlc", "PLC 连接成功，读取身份信息");

    PlcIdentity identity;
    if (plc.getPlcIdentity(identity))
    {
        currentPlc.orderCode = identity.orderCode;
        currentPlc.plcModel = identity.moduleName;

        logOp(
            "connectPlc",
            std::string("PLC 身份信息读取成功，型号: ") +
            currentPlc.plcModel +
            " 订货号: " +
            currentPlc.orderCode
        );
    }
    else
    {
        logOp(
            "connectPlc",
            std::string("PLC 身份信息读取失败: ") +
            plc.getLastErrorText()
        );
        // 连接已成功，仅身份信息缺失
    }

    lastError.clear();
    return true;
}
bool uppermachine::stop()
{
    logOp("stop", "请求停止上位机");

    // ===== 未初始化则无需停止 =====
    if (!runState.upperInited)
    {
        logOp("stop", "上位机未初始化，无需停止");
        return true;
    }

    // ===== 发出停止请求 =====
    runState.upperRunning = false;

    if (runState.readRunning)
        runState.readStopping = true;

    if (runState.writeThreadRunning)
        runState.writeThreadStopping = true;

    // ===== 等待读写线程确认停止 =====
    while (runState.readStopping || runState.writeThreadStopping)
    {
        std::this_thread::sleep_for(
            std::chrono::milliseconds(50)
        );
    }

    // ===== 线程已进入稳定停止态 =====
    runState.readRunning = false;
    runState.writeThreadRunning = false;
    runState.upperRunning = false;

    logOp("stop", "上位机已停止");
    lastError.clear();
    return true;
}


bool uppermachine::run()
{
    lastError.clear();
    logOp("run", "run 启动");
    if (!runState.upperInited)
    {
        lastError = "uppermachine 尚未完成初始化";
        logOp("run", lastError);
        return false;
    }
    // ===== 启动读取线程 =====
    if (!runState.readRunning)
    {
        readThread = std::thread(&uppermachine::readThreadProc, this);
        runState.readRunning = true;
    }
    // ===== 启动写入线程 =====
    if (!runState.writeThreadRunning)
    {
        writeThread = std::thread(&uppermachine::writeThreadProc, this);
        runState.writeThreadRunning = true;
    }
    // ===== 记录上位机运行事实 =====
    runState.upperRunning = true;
    logOp("run", "线程启动完成");
    return true;
}
void uppermachine::readThreadProc()
{
    logOp("readThreadProc", "读取线程启动");

    while (1)
    {
        if (runState.readStopping)
        {
            runState.readRunning = false;
            runState.readStopping = false;  
            std::this_thread::sleep_for(
                std::chrono::milliseconds(200)
            );
            continue;
        }


        // ===== 未处于运行态，仅保持线程存活 =====
        if (!runState.readRunning)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000)
            );
            continue;
        }

        // ===== 正常读取 PLC =====
        for (signalinfo& sig : worksignals)
        {
            int32_t value = 0;
            bool ok = plc.readAddress(sig.plcAddress, value);

            if (ok)
            {
                sig.currentValue = std::to_string(value);
                sig.readOk = 1;
                sig.lastOpAt = static_cast<int>(time(nullptr));
            }
            else
            {
                sig.readOk = 0;
                sig.lastOpAt = static_cast<int>(time(nullptr));

                logOp(
                    "readThreadProc",
                    "读取失败 地址=" + sig.plcAddress
                );
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(1000)
        );
    }
}
void uppermachine::writeThreadProc()
{
    logOp("writeThreadProc", "写入线程启动");

    while (1)
    {
        if (runState.writeThreadStopping)
        {
            runState.writeThreadRunning = false;
            runState.writeThreadStopping = false; 
            std::this_thread::sleep_for(
                std::chrono::milliseconds(200)
            );
            continue;
        }

        if (!runState.writeThreadRunning)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(500)
            );
            continue;
        }

        for (signalinfo& sig : worksignals)
        {
            if (sig.writeFlag != 1)
                continue;

            int32_t value = 0;
            try
            {
                value = std::stoi(sig.targetValue);
            }
            catch (...)
            {
                sig.writeFlag = 0;

                logOp(
                    "writeThreadProc",
                    "非法写入值 地址=" + sig.plcAddress +
                    " value=" + sig.targetValue
                );
                continue;
            }

            bool ok = plc.writeAddress(sig.plcAddress, value);

            if (ok)
            {
                sig.writeFlag = 0;
                sig.currentValue = sig.targetValue;
                sig.readOk = 1;
                sig.lastOpAt = static_cast<int>(time(nullptr));

                logOp(
                    "writeThreadProc",
                    "写入成功 地址=" + sig.plcAddress +
                    " value=" + std::to_string(value)
                );
            }
            else
            {
                sig.readOk = 0;
                sig.lastOpAt = static_cast<int>(time(nullptr));

                logOp(
                    "writeThreadProc",
                    "写入失败 地址=" + sig.plcAddress
                );
            }
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(500)
        );
    }
}

const std::string& uppermachine::getLastError() const
{
    return lastError;
}
