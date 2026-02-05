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
      worksignals(signalRefList),
      boolread(false),
      boolwrite(false)
{
}



uppermachine::~uppermachine()
{
    if (readThread.joinable())
        readThread.join();

    if (writeThread.joinable())
        writeThread.join();
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

    runState.life = 1; // starting

    plcinfo info;
    if (!store.readPlcInfo(info))
    {
        lastError = store.getLastErrorText();
        logOp("init", "读取 PLC 信息失败: " + lastError);
        runState.life = 0;
        return false;
    }
    currentPlc = info;
    logOp(
        "init",
        "当前 PLC: IP=" + currentPlc.ipAddress +
        " rack=" + std::to_string(currentPlc.rack) +
        " slot=" + std::to_string(currentPlc.slot)
    );

    std::vector<signalinfo> dbSignals;
    if (!store.readAllSignalInfo(dbSignals))
    {
        lastError = store.getLastErrorText();
        logOp("init", "读取变量列表失败: " + lastError);
        runState.life = 0;
        return false;
    }
    worksignals = dbSignals;
    logOp("init", "加载变量数量: " + std::to_string(worksignals.size()));
    logOp("init", "尝试连接 PLC");
    if (!plc.connectPLC(
        currentPlc.ipAddress,
        currentPlc.rack,
        currentPlc.slot))
    {
        lastError = plc.getLastErrorText();
        logOp("init", "PLC 连接失败: " + lastError);
        runState.life = 0;
        return false;
    }

    logOp("init", "PLC 连接成功，开始读取 PLC 实际身份信息");
    PlcIdentity identity;
    if (plc.getPlcIdentity(identity))
    {
        currentPlc.orderCode = identity.orderCode;
        currentPlc.plcModel = identity.moduleName;

        logOp(
            "init",
            std::string("PLC 身份信息读取成功，型号: ") +
            currentPlc.plcModel +
            " 订货号: " +
            currentPlc.orderCode
        );
    }
    else
    {
        logOp(
            "init",
            std::string("PLC 身份信息读取失败: ") +
            plc.getLastErrorText()
        );
        // 注意：这里不 return false
        // 连接已成功，允许系统继续运行
    }
    runState.read = 0;
    runState.write = 0;
    runState.fatalReason = 0;
    runState.life = 2;
    boolwrite = true;
    boolread = true;
    logOp("init", "初始化完成，进入运行状态");
    lastError.clear();
    return true;
}

bool uppermachine::run()
{
    lastError.clear();
    logOp("run", "run 启动");

    // 1. 初始化
    if (!init())
    {
        logOp("run", "init 失败，run 中止");
        return false;
    }

    // 2. 启动线程
    readThread = std::thread(&uppermachine::readThreadProc, this);
    writeThread = std::thread(&uppermachine::writeThreadProc, this);

    logOp("run", "读写线程已启动");

    // 3. 进入运行循环（占位生命周期）
    while (runState.life == 2) // running
    {
        // 当前阶段不做任何事情
        // 后续可在此加入：
        // - 健康检测
        // - 心跳
        // - 状态同步
        std::this_thread::sleep_for(
            std::chrono::milliseconds(500)
        );
    }

    logOp("run", "run 退出");
    return true;
}
void uppermachine::readThreadProc()
{
    logOp("readThreadProc", "读取线程启动");

    while (1)
    {
        // 读取未启用时，线程保持存活但不工作
        if (!boolread)
        {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(1000)
            );
            continue;
        }

        for (signalinfo& sig : worksignals)
        {
            int32_t value = 0;
            bool ok = plc.readAddress(sig.plcAddress, value);

            if (ok)
            {
                sig.currentValue = std::to_string(value);
                sig.readOk = 1;
            }
            else
            {
                sig.readOk = 0;
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
        // 写入未启用时，线程保持存活但不工作
        if (!boolwrite)
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
                // 非法写入值，结束本次写请求，避免死循环
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
                // 写入成功，清除写请求并同步镜像值
                sig.writeFlag = 0;
                sig.currentValue = sig.targetValue;
                sig.readOk = 1;

                logOp(
                    "writeThreadProc",
                    "写入成功 地址=" + sig.plcAddress +
                    " value=" + std::to_string(value)
                );
            }
            else
            {
                // 写入失败，不清 writeFlag，允许后续重试
                sig.readOk = 0;

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
