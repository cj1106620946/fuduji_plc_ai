#include "uppermachine.h"
uppermachine::uppermachine(PLCClient& plcRef, SqlStore& storeRef)
    : plc(plcRef),
    store(storeRef),
    lastError()
{
}
uppermachine::~uppermachine()
{
    if (readThread.joinable())
        readThread.join();

    if (writeThread.joinable())
        writeThread.join();
}

bool uppermachine::createPlcInfoRow(const plcinfo& info)
{
    if (!store.createPlcInfo(info))
    {
        lastError = store.getLastErrorText();
        return false;
    }

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
    if (!store.createSignal(plcAddress, name, description))
    {
        lastError = store.getLastErrorText();
        return false;
    }

    lastError.clear();
    return true;
}
bool uppermachine::getCurrentPlcInfo(plcinfo& out)
{
    // 直接交给数据库层
    if (!store.readPlcInfo(out))
    {
        lastError = store.getLastErrorText();
        return false;
    }

    lastError.clear();
    return true;
}




bool uppermachine::init()
{
    // 1. 进入 starting 状态
    runState.life = 1; // starting

    // 2. 读取当前 PLC 信息
    plcinfo info;
    if (!store.readPlcInfo(info))
    {
        lastError = store.getLastErrorText();
        runState.life = 0; // idle
        return false;
    }

    // 3. 构建当前 PLC 上下文（直接使用 plcinfo）
    currentPlc = info;

    // 4. 从数据库读取当前 PLC 下的全部变量定义（直接使用 signalinfo）
    std::vector<signalinfo> dbSignals;
    if (!store.readAllSignalInfo(dbSignals))
    {
        lastError = store.getLastErrorText();
        runState.life = 0; // idle
        return false;
    }

    // 5. 保存变量集合（不做转换）
    signals.clear();
    signals.reserve(dbSignals.size());
    for (const signalinfo& si : dbSignals)
        signals.push_back(si);

    // 6. 验证 PLC 是否可连接
    if (!plc.connectPLC(
        currentPlc.ipAddress,
        currentPlc.rack,
        currentPlc.slot))
    {
        lastError = plc.getLastErrorText();
        runState.life = 0; // idle
        return false;
    }

    // 7. 初始化运行状态
    runState.read = 0;   // ok
    runState.write = 0;  // ok
    runState.fatalReason = 0;
    runState.life = 2;   // running

    lastError.clear();
    return true;
}



bool uppermachine::run()
{
    lastError.clear();
    if (!init())
        return false;
    readThread = std::thread(&uppermachine::readThreadProc, this);
    writeThread = std::thread(&uppermachine::writeThreadProc, this);
    while (1)
    {
        ;
    }
}
void uppermachine::readThreadProc()
{
    while (1)
    {
        for (signalinfo& sig : signals)
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
            }

            sig.lastOpAt = static_cast<int>(time(nullptr));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void uppermachine::writeThreadProc()
{
    while (1)
    {
        for (signalinfo& sig : signals)
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
                continue;
            }

            bool ok = plc.writeAddress(sig.plcAddress, value);

            if (ok)
            {
                sig.writeFlag = 0;
                sig.currentValue = sig.targetValue;
                sig.readOk = 1;
            }

            sig.lastOpAt = static_cast<int>(time(nullptr));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

const std::string& uppermachine::getLastError() const
{
    return lastError;
}
