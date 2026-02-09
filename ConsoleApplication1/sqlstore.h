#pragma once

#include <ctime>
#include <string>
#include <vector>

class Sqllient;

struct plcinfo
{
    std::string taskDesc;       // 当前plc干的事情
    std::string taskDomain;     // 给决策或者其他乱起八糟的用
    std::string sourceText;     // 原始输入文本或来源描述

    std::string plcModel;       // PLC 型号
    std::string orderCode;      // PLC 订货号
    std::string ipAddress;      // PLC IP 地址
    int rack;                   // PLC rack 号
    int slot;                   // PLC slot 号
    int signalRootId;           // 信号根节点 ID

    int isActive;               // PLC 当前是否处于可用状态（1 已连接 / 0 未连接）
    int createdAt;              // 记录创建时间戳
    int updatedAt;              // 最近一次状态或配置更新时间戳
};
struct signalinfo
{
    int signalId;               // signal_def.signal_id，变量唯一 ID

    std::string name;           // 变量名（如 水泵1）
    std::string plcAddress;     // PLC 地址（如 M0.0）

    int plcId;                  // 所属 PLC（对应 plc_info.plc_id）

    std::string description;    // 变量中文说明
    int createdAt;              // 创建时间

    std::string currentValue;   // 当前值（数据库记录，文本形式）
    int readOk;                 // 最近一次读取是否成功（1 成功 / 0 失败）

    std::string targetValue;    // 目标写入值（数据库记录，文本形式）
    int writeFlag;              // 写入标记（0 无 / 1 等待写入）

    int lastOpAt;               // 最近一次读或写时间戳
    int isAvailable;            // 是否可用（1 可用 / 0 不可用）
};
// “数据库中当前被 memory_pointer 指向的那一条内容”
struct CurrentMemory
{
    int memoryKeyId;        // 记忆结构位 ID（对应 memory_key.memory_key_id）
    std::string keyPath;    // 记忆路径（如 self.identity / user.summary）
    std::string content;    // 当前生效的记忆文本内容
};

// 不允许在 manager 中随意修改，修改必须回写数据库
struct CurrentMemoryState
{
    CurrentMemory selfMemory[3];   // 人格自身记忆（固定 1–3：identity / emotion / attitude）
    CurrentMemory userMemory[6];   // 用户相关长期记忆（固定 4–9）
};






class SqlStore
{
public:
    explicit SqlStore(Sqllient* client);
    ~SqlStore();

    bool open();
    void close();
    //占位
    bool isAvailable() const;

    const char* getLastErrorText() const;

    bool createPlcInfo(const plcinfo& in);
    bool writePlcInfo(const plcinfo& in);
    bool readPlcInfo(plcinfo& out);

    bool initMemorySnapshots();

    bool writeMemory(
        int memoryKeyId,
        const std::string& content
    );

    bool readMemory(
        int memoryKeyId,
        std::string& outContent
    );

    bool createSignal(
        const std::string& plcAddress,
        const std::string& workspaceName,
        const std::string& description
    );
    bool setCurrentPlcPointer(int plcId);
    bool getAllSignalMapText(
        std::string& outText
    );

    bool aiReadSignal(
        const std::string& queryName,
        const std::string& queryAddress,
        std::string& outName,
        std::string& outAddress,
        std::string& outValue,
        bool& readSuccess
    );

    bool aiWriteSignal(
        const std::string& queryName,
        const std::string& queryAddress,
        const std::string& targetValue
    );

    bool createSignalInfo(
        const signalinfo& in
    );
    bool readAllSignalInfo(
        std::vector<signalinfo>& out
    );
    bool readSignalInfoByName(
        const std::string& name,
        signalinfo& out
    );
    bool readSignalInfoByAddress(
        const std::string& plcAddress,
        signalinfo& out
    );
    bool writeSignalInfo(
        const signalinfo& in
    );
    bool removeSignalInfo(
        const std::string& plcAddress
    );


    bool initSelfMemoryKeys();
    bool initMemoryPointer();

private:
    SqlStore(const SqlStore&) = delete;
    SqlStore& operator=(const SqlStore&) = delete;

    bool initmeta();
    bool inittables();

private:
    Sqllient* sql;
    bool available;
    std::string lastError;
};
