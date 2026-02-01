#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <ctime>

#include "sqlstore.h"
#include "PLCClient.h"
// 前向声明
class PLCClient;
class SqlStore;

struct RunState
{
    int life;        // 上位机运行生命周期状态
    // 0：空闲（idle，允许创建 / 切换 PLC）
    // 1：启动中（starting，正在 init）
    // 2：运行中（running，读写线程工作中）
    // 3：停止中（stopping，等待线程退出）
    // 4：致命错误（fatal，不可恢复）

    int read;        // 读取通道状态
    // 0：正常
    // 1：可用但异常（降级）
    // 2：失败（读取不可用）

    int write;       // 写入通道状态
    // 0：正常
    // 1：可用但异常（降级）
    // 2：失败（写入不可用）

    int fatalReason; // 致命错误原因（仅 life == 4 时有效）
    // 0：无
    // 1：数据库错误（结构不一致、不可用）
    // 2：PLC 错误（长期不可连接等）
    // 3：内部逻辑错误
};

/*
struct CurrentPlcContext
{
    int plcId;                // 当前 PLC 的唯一标识（来自数据库）

    std::string ipAddress;    // PLC IP 地址
    int rack;                 // 机架号
    int slot;                 // 槽号

    std::string plcModel;     // PLC 型号
    std::string orderCode;    // PLC 订货号

    std::string taskDesc;     // 当前控制任务描述
    std::string taskDomain;   // 任务所属领域
    std::string sourceText;   // 用户原始输入文本
};


struct SignalSnapshot
{
    std::string plcAddress;    // PLC 变量地址（如 M0.0 / DB1.DBW2）
    std::string description;   // 变量中文说明（来自数据库，用于 AI 理解）

    int32_t currentValue;      // 当前内存中的变量值
    bool readOk;               // 最近一次读取是否成功

    bool hasWriteRequest;      // 是否存在待处理的写入请求
    int32_t targetValue;       // 目标写入值（仅在有写请求时有效）

    int lastOpTime;            // 最近一次读或写操作的时间戳
};

*/
class uppermachine
{
public:
    uppermachine(PLCClient& plcRef, SqlStore& storeRef);
    ~uppermachine();
    // 在数据库中创建一条 plc_info 记录（上位机）
    bool createPlcInfoRow(const plcinfo& info);
    // 在数据库中创建一条 signal_def 记录（变量）
    bool createSignalRow(
        const std::string& name,
        const std::string& plcAddress,
        int plcId,
        const std::string& description
    );
    // 读取当前 PLC 上位机上下文
    bool getCurrentPlcInfo(plcinfo& out);

    // 执行一轮上位机逻辑（启动线程）
    bool run();
    // 最近一次错误
    const std::string& getLastError() const;
private:
    // 初始化上位机
    bool init();

    // 读取线程函数
    void readThreadProc();

    // 写入线程函数
    void writeThreadProc();

private:
    PLCClient& plc;
    SqlStore& store;
    std::thread readThread;
    std::thread writeThread;
    std::string lastError;
    std::vector<std::string> cachedSignalAddrs;

    RunState runState;
    plcinfo currentPlc;
    std::vector<signalinfo> signals;
};
