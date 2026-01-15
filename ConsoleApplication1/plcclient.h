#pragma once
#include <string>
#include "snap7.h"
#include <regex>
#include <iostream>
#include <vector>

// PLCClient：封装 Snap7 客户端，用于连接 PLC、解析地址、读写数据
class PLCClient
{
public:
    PLCClient();
    ~PLCClient();
    // 连接到 PLC
    // plc_ip: PLC 的 IP 地址
    // rack: 机架号（一般 0）
    // slot: 插槽号（一般 1）
    bool connectPLC(const std::string& plc_ip, int rack, int slot);
    // 断开与 PLC 的连接
    void disconnectPLC();
    // 返回当前连接状态
    bool isConnected() const;
    // 自动根据字符串地址读取值
    bool readAddress(const std::string& addr, int32_t& value);
    // 自动根据字符串地址写入值
    bool writeAddress(const std::string& addr, int32_t value);
    // 读取 PLC CPU 运行状态
// cpuStatus: Snap7 定义的状态值
    bool getCpuStatus(int& cpuStatus);
    // 获取最近一次 Snap7 错误文本
    std::string getLastErrorText() const;
    // 批量读取 DB 区域，用于一致性状态快照
    bool readDbBlock(
        int dbNumber,
        int start,
        int size,
        std::vector<uint8_t>& buffer
    );
    bool setPlcStop();
    bool setPlcRun();
    struct PlcIdentity
    {
        std::string moduleName;    // 模块名称
        std::string orderCode;     // 订货号
        int versionMajor;          // 主版本号
        int versionMinor;          // 次版本号
        int versionPatch;          // 补丁版本号
    };
    bool getPlcIdentity(PlcIdentity& info);
    // PLC 时间信息结构
    struct PlcTime
    {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
    };

    // 读取 PLC 当前系统时间
    bool getPlcTime(PlcTime& time);

    // 将 PLC 系统时间设置为当前本机时间
    bool syncPlcTimeWithLocal();
    
private:
    TS7Client* client;  // Snap7 客户端对象
    bool connected;     // 当前是否连接
    int lastError;
    // PLC 身份信息结构

    // 解析字符串地址为 PLC 区域信息
    // 返回解析是否成功
    bool parseAddress(const std::string& addr,
        int& area,       // 内存区域：I/Q/M/DB
        int& dbNumber,   // DB块号（非DB则为0）
        int& start,      // 起始字节
        int& bitIndex,   // 位索引（如果是字节/字等则为 -1）
        int& dataSize);  // 数据大小（1字节 / 2字节 / 4字节）
};