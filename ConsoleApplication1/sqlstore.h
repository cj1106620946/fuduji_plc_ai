#pragma once

#include <ctime>
#include <string>
#include <vector>

class Sqllient;

struct plcinfo
{
    std::string taskDesc;
    std::string taskDomain;
    std::string sourceText;

    std::string plcModel;
    std::string orderCode;
    std::string ipAddress;
    int rack;
    int slot;
    int signalRootId;

    int isActive;
    int createdAt;
    int updatedAt;
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
