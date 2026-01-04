#include "plcclient.h"
using namespace std;
PLCClient::PLCClient()
{
    client = new TS7Client();  // 创建 Snap7 客户端
    connected = false;         // 默认未连接
}
PLCClient::~PLCClient()
{
    disconnectPLC();           // 如果还在连接，先断开
    delete client;             // 释放 Snap7 客户端对象
}
//连接plc
bool PLCClient::connectPLC(const  string& plc_ip, int rack, int slot)
{
    int result = client->ConnectTo(plc_ip.c_str(), rack, slot);
    if (result == 0) {       
        connected = true;
        return true;
    }
    connected = false;
    return false;
}
//断开连接
void PLCClient::disconnectPLC()
{
    if (connected) {
        client->Disconnect(); 
        connected = false;
    }
}
//查询是否连接
bool PLCClient::isConnected() const
{
    return connected;
}
int areaCode(char c)
{
    if (c == 'I')
        return 0x81;  // 输入区

    if (c == 'Q')
        return 0x82;  // 输出区

    if (c == 'M')
        return 0x83;  // M区

    return -1;
}
//输入转换
bool PLCClient::parseAddress(const  string& addr,int& area, int& dbNumber, int& start,int& bitIndex, int& dataSize)
{
    dbNumber = 0;
    bitIndex = -1;
    // 正则：位地址 
     regex bitPattern(R"(([IQM])(\d+)\.(\d+))");
    // 正则：字节/字/双字
     regex bytePattern(R"(([IQM])([BWD])(\d+))");
    // 正则：DB区
     regex dbPattern(R"(DB(\d+)\.DB([XWD])(\d+)(?:\.(\d+))?)");
     smatch m;
    // 将区域字符 I/Q/M 映射为 Snap7 区域代码
    //第一种：位地址 I0.0
    if ( regex_match(addr, m, bitPattern)) {
        area = areaCode(m[1].str()[0]);   // I/Q/M
        start =  stoi(m[2].str());     // 字节
        bitIndex =  stoi(m[3].str());     // 位号
        dataSize = 1;                         // 读1字节
        return true;
    }
    // 第二种
    if ( regex_match(addr, m, bytePattern)) {
        area = areaCode(m[1].str()[0]);
        char type = m[2].str()[0];
        start =  stoi(m[3].str());
        if (type == 'B') dataSize = 1;  // 字节
        else if (type == 'W') dataSize = 2;  // 字
        else if (type == 'D') dataSize = 4;  // 双字
        else return false;
        return true;
    }
    //DB 地址
    if ( regex_match(addr, m, dbPattern)) {

        area = S7AreaDB;
        dbNumber =  stoi(m[1].str());  // DB号

        char type = m[2].str()[0];
        start =  stoi(m[3].str());     // 字节偏移

        if (m[4].matched)
            bitIndex =  stoi(m[4].str());  // 位索引（仅 DBX）

        if (type == 'X') dataSize = 1;
        else if (type == 'W') dataSize = 2;
        else if (type == 'D') dataSize = 4;
        else return false;
        return true;
    }
    return false;  // 解析失败
}
//读操作
bool PLCClient::readAddress(const  string& addr, int32_t& value)
{
    if (!connected) return false;
    int area, dbNumber, start, bitIndex, dataSize;
    if (!parseAddress(addr, area, dbNumber, start, bitIndex, dataSize))
        return false;
    uint8_t buffer[4] = { 0 };   // 最大读4字节
    // DB区读取 or 普通区读取
    int result = (area == S7AreaDB)
        ? client->DBRead(dbNumber, start, dataSize, buffer)
        : client->ReadArea(area, 0, start, dataSize, S7WLByte, buffer);
    if (result != 0)
        return false;
    //  位访问
    if (bitIndex >= 0) {
        value = (buffer[0] >> bitIndex) & 1;
        return true;
    }
    //  字节访问B  
    if (dataSize == 1) {
        value = buffer[0];
    }
    //  字访问W
    else if (dataSize == 2) {
        value = (buffer[0] << 8) | buffer[1];  // 大端
    }
    //  双字访问D 
    else if (dataSize == 4) {
        value = (buffer[0] << 24) | (buffer[1] << 16)
            | (buffer[2] << 8) | buffer[3];
    }
    return true;
}
//写操作
bool PLCClient::writeAddress(const  string& addr, int32_t value)
{
    if (!connected) return false;
    int area, dbNumber, start, bitIndex, dataSize;
    if (!parseAddress(addr, area, dbNumber, start, bitIndex, dataSize))
        return false;
    uint8_t buffer[4] = { 0 };
    if (bitIndex >= 0)
    {
        // 1) 先读出当前 1 字节（避免写一个位把其他位清零）
        uint8_t b = 0;
        int r = (area == S7AreaDB)
            ? client->DBRead(dbNumber, start, 1, &b)
            : client->ReadArea(area, 0, start, 1, S7WLByte, &b);

        if (r != 0) return false;

        // 2) 修改目标 bit，其它 bit 保持不变
        if (value)
            b |= (uint8_t)(1u << bitIndex);
        else
            b &= (uint8_t)~(1u << bitIndex);

        // 3) 写回这个字节
        int w = (area == S7AreaDB)
            ? client->DBWrite(dbNumber, start, 1, &b)
            : client->WriteArea(area, 0, start, 1, S7WLByte, &b);

        return w == 0;
    }


    // ---------- 字节 ----------
    if (dataSize == 1) {
        buffer[0] = (uint8_t)value;
    }
    // ---------- 字 ----------
    else if (dataSize == 2) {
        buffer[0] = (value >> 8) & 0xFF;
        buffer[1] = value & 0xFF;
    }
    // ---------- 双字 ----------
    else if (dataSize == 4) {
        buffer[0] = (value >> 24) & 0xFF;
        buffer[1] = (value >> 16) & 0xFF;
        buffer[2] = (value >> 8) & 0xFF;
        buffer[3] = value & 0xFF;
    }

    int result = (area == S7AreaDB)
        ? client->DBWrite(dbNumber, start, dataSize, buffer)
        : client->WriteArea(area, 0, start, dataSize, S7WLByte, buffer);

    return result == 0;
}
