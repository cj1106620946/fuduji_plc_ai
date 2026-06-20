#pragma once

#include <string>
#include "sqlstore.h"
// 前向声明
class SqlStore;


struct MemoryWrite
{
    int memoryKeyId;
    std::string content;
};
class memorybridge
{
public:
    memorybridge(
        SqlStore& storeRef,
        CurrentMemoryState& memoryRef
    );
    ~memorybridge();
    bool init();
    // 读取：只能读 current，不访问数据库
    const CurrentMemoryState& read() const;

    // 写入：只能写数据库，不修改 current
    bool write(const MemoryWrite& req);
private:
    SqlStore& store;
    CurrentMemoryState& current;   
    std::string lastError;
};
