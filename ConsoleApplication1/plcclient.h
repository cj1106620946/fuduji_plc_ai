#pragma once
#include <string>
#include "snap7.h"
#include <regex>
#include <iostream>
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
    // addr: 如 "I0.0"、"Q0.0"、"M10.2"、"MW20"、"DB1.DBW2"
    // value: 输出结果
    bool readAddress(const std::string& addr, int32_t& value);
    // 自动根据字符串地址写入值
    bool writeAddress(const std::string& addr, int32_t value);
private:
    TS7Client* client;  // Snap7 客户端对象
    bool connected;     // 当前是否连接

    // 解析字符串地址为 PLC 区域信息
    // 返回解析是否成功
    bool parseAddress(const std::string& addr,
        int& area,       // 内存区域：I/Q/M/DB
        int& dbNumber,   // DB块号（非DB则为0）
        int& start,      // 起始字节
        int& bitIndex,   // 位索引（如果是字节/字等则为 -1）
        int& dataSize);  // 数据大小（1字节 / 2字节 / 4字节）
};