#pragma once

#include <ctime>
#include <string>
#include<vector>
// 前置声明，不包含 sqlite3
class Sqllient;

class SqlStore
{
public:
    explicit SqlStore(Sqllient* client);
    ~SqlStore();

    // 生命周期
    bool open();
    void close();
    bool isAvailable() const;
    // 错误信息
    const char* getLastErrorText() const;


    bool createWorkspace(
        const std::string& taskDesc,
        const std::string& taskDomain,
        const std::string& sourceText
	);

    bool readWorkspace(
        int workspaceId,
        std::string& taskDesc,
        std::string& taskDomain,
        std::string& sourceText,
        int& createdAt,
        int& updatedAt
    );
    bool writeWorkspace(
        int workspaceId,
        const std::string& taskDesc,
        const std::string& taskDomain
	);

    bool createSignal(
        const std::string& name,
        const std::string& plcAddress,
        const std::string& workspaceName,
        const std::string& description
    );
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
    bool getAllSignalAddresses(std::vector<std::string>& addrs);
    bool updateSignalReadResult(
        const std::string& addr,
        int32_t value,
        bool readOk
    );
    bool getAllWriteSignals(
        std::vector<std::string>& addrs,
        std::vector<int32_t>& values
    );
    bool updateSignalWriteResult(
        const std::string& addr,
        bool writeOk
    );
    bool initSelfMemoryKeys();
private:
    // 禁止拷贝
    SqlStore(const SqlStore&) = delete;
    SqlStore& operator=(const SqlStore&) = delete;

    // 确认 / 创建 meta 表并校验数据库身份
    bool initmeta();
    // 创建系统必须存在的表
    bool inittables();

private:
    Sqllient* sql;        // 底层数据库客户端
    bool available;       // 当前是否可用
    std::string lastError; // 最近一次错误
};
