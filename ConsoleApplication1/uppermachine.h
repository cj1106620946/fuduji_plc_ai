#pragma once
#include <unordered_map>

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <ctime>
#include <fstream>
#include <chrono>

#include "sqlstore.h"
#include "PLCClient.h"

// 前向声明
class PLCClient;
class SqlStore;
struct RunState
{
    bool upperInited;          // 上位机初始化
    bool upperRunning;         // 上位机运行状态
    bool upperStopping;        // 上位机是否处于停止流程中

    bool readInited;           // 读取线程是否完成初始化
    bool readRunning;          // 读取线程是否处于运行状态
    bool readStopping;         // 读取线程是否处于停止流程中


    bool writeThreadInited;    // 写入线程是否完成初始化
    bool writeThreadRunning;   // 写入线程是否处于运行状态
    bool writeThreadStopping;  // 写入线程是否处于停止流程中
};
class uppermachine
{
public:
    uppermachine(
        SqlStore& storeRef,
        RunState& runStateRef,
        plcinfo& plcRefInfo,
        std::vector<signalinfo>& signalRefList
    );

    ~uppermachine();
    bool init();
    bool initloadConfig();
    bool initconnectPlc();
    bool createPlcInfoRow(const plcinfo& info);

    bool createSignalRow(
        const std::string& name,
        const std::string& plcAddress,
        int plcId,
        const std::string& description
    );
    bool readplc(
        const std::string& plcAddress,
        std::string& outResult
    );

    bool writeplc(
        const std::string& plcAddress,
        const std::string& value,
        std::string& outResult
    );
    bool run();
    bool stop();
    const std::string& getLastError() const;
private:

    void readThreadProc();
    void writeThreadProc();

    void logOp(
        const std::string& fromFunc,
        const std::string& action
    );

private:
    PLCClient plc;
    SqlStore& store;
    RunState& runState;
    plcinfo& currentPlc;
    std::vector<signalinfo>& worksignals;
    std::thread readThread;
    std::thread writeThread;
    std::string lastError;
    std::unordered_map<std::string, size_t> signalIndexByAddr;
};
