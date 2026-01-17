#include "sqllient.h"
#include "sqlite3.h"

#include <windows.h>

// 构造函数
Sqllient::Sqllient(const char* databaseName)
    : dbName(databaseName),
    dbHandle(nullptr),
    available(false),
    lastError(nullptr)
{
}

// 析构函数
Sqllient::~Sqllient()
{
    close();
}

// 打开或创建数据库
bool Sqllient::open()
{
    if (available)
        return true;

    CreateDirectoryA("sqlite", NULL);

    char fullPath[MAX_PATH] = { 0 };
    lstrcpyA(fullPath, "sqlite\\");
    lstrcatA(fullPath, dbName);

    int result = sqlite3_open(fullPath, &dbHandle);
    if (result != SQLITE_OK)
    {
        lastError = sqlite3_errmsg(dbHandle);
        sqlite3_close(dbHandle);
        dbHandle = nullptr;
        available = false;
        return false;
    }

    available = true;
    lastError = nullptr;
    return true;
}

// 关闭数据库
void Sqllient::close()
{
    if (dbHandle != nullptr)
    {
        sqlite3_close(dbHandle);
        dbHandle = nullptr;
    }

    available = false;
}

// 是否可用
bool Sqllient::isAvailable() const
{
    return available;
}

// 获取最近一次错误
const char* Sqllient::getLastError() const
{
    return lastError;
}

// 执行不返回结果的 SQL
bool Sqllient::execute(const char* sql)
{
    if (!available)
        return false;

    char* errorMsg = nullptr;
    int result = sqlite3_exec(dbHandle, sql, nullptr, nullptr, &errorMsg);
    if (result != SQLITE_OK)
    {
        lastError = errorMsg;
        sqlite3_free(errorMsg);
        return false;
    }

    lastError = nullptr;
    return true;
}

// 准备查询语句
bool Sqllient::prepare(const char* sql, sqlite3_stmt** stmt)
{
    if (!available)
        return false;

    int result = sqlite3_prepare_v2(dbHandle, sql, -1, stmt, nullptr);
    if (result != SQLITE_OK)
    {
        lastError = sqlite3_errmsg(dbHandle);
        return false;
    }

    return true;
}

// 推进到下一行
bool Sqllient::step(sqlite3_stmt* stmt)
{
    int result = sqlite3_step(stmt);
    return result == SQLITE_ROW;
}

// 释放查询语句
void Sqllient::finalize(sqlite3_stmt* stmt)
{
    if (stmt != nullptr)
    {
        sqlite3_finalize(stmt);
    }
}

// 读取文本列
const char* Sqllient::columnText(sqlite3_stmt* stmt, int index)
{
    return reinterpret_cast<const char*>(sqlite3_column_text(stmt, index));
}

// 读取整型列
int Sqllient::columnInt(sqlite3_stmt* stmt, int index)
{
    return sqlite3_column_int(stmt, index);
}

// 开始事务
bool Sqllient::beginTransaction()
{
    return execute("BEGIN TRANSACTION;");
}

// 提交事务
bool Sqllient::commitTransaction()
{
    return execute("COMMIT;");
}

// 回滚事务
bool Sqllient::rollbackTransaction()
{
    return execute("ROLLBACK;");
}
