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

    // === 数据库操作（仍然允许） ===
    bool createPlcInfoRow(const plcinfo& info);

    bool createSignalRow(
        const std::string& name,
        const std::string& plcAddress,
        int plcId,
        const std::string& description
    );

    // === 执行接口（只操作镜像引用） ===
    bool readplc(
        const std::string& plcAddress,
        std::string& outResult
    );

    bool writeplc(
        const std::string& plcAddress,
        const std::string& value,
        std::string& outResult
    );

    // 执行主循环（由 ProjectManager 调度）
    bool run();

    const std::string& getLastError() const;

private:
    bool init();

    void readThreadProc();
    void writeThreadProc();

    void logOp(
        const std::string& fromFunc,
        const std::string& action
    );

private:
    // === 外部资源 ===
    PLCClient plc;
    SqlStore& store;

    // === 系统镜像（引用，不拥有） ===
    RunState& runState;
    plcinfo& currentPlc;
    std::vector<signalinfo>& worksignals;

    // === 执行器内部状态 ===
    std::thread readThread;
    std::thread writeThread;

    bool boolread;
    bool boolwrite;

    std::string lastError;

    // 地址索引缓存（执行层私有）
    std::unordered_map<std::string, size_t> signalIndexByAddr;
};
