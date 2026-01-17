#pragma once

struct sqlite3;
struct sqlite3_stmt;

class Sqllient
{
public:
    explicit Sqllient(const char* databaseName);
    ~Sqllient();

    bool open();
    void close();

    bool isAvailable() const;
    const char* getLastError() const;

    bool execute(const char* sql);

    bool prepare(const char* sql, sqlite3_stmt** stmt);
    bool step(sqlite3_stmt* stmt);
    void finalize(sqlite3_stmt* stmt);

    const char* columnText(sqlite3_stmt* stmt, int index);
    int columnInt(sqlite3_stmt* stmt, int index);

    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

private:
    Sqllient(const Sqllient&) = delete;
    Sqllient& operator=(const Sqllient&) = delete;

private:
    const char* dbName;
    sqlite3* dbHandle;
    bool available;
    const char* lastError;
};
