#include "plcclient.h"
using namespace std;
PLCClient::PLCClient()
{
    client = new TS7Client();  // 创建 Snap7 客户端

    lastError = 0;
}
PLCClient::~PLCClient()
{
    disconnectPLC();           // 如果还在连接，先断开
    delete client;             // 释放 Snap7 客户端对象
}
//连接plc
bool PLCClient::connectPLC(const std::string& plc_ip, int rack, int slot)
{
    lastError = client->ConnectTo(plc_ip.c_str(), rack, slot);
    if (lastError == 0)
    {
        return true;
    }
    return false;
}
//断开连接
void PLCClient::disconnectPLC()
{
    if (!client)
        return;
    lastError = client->Disconnect();
}
//判断区域代码
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
//读
bool PLCClient::readAddress(const std::string& addr, int32_t& value)
{
    int area, dbNumber, start, bitIndex, dataSize;

    if (!parseAddress(addr, area, dbNumber, start, bitIndex, dataSize))
    {
        lastError = -1;
        return false;
    }

    uint8_t buffer[4] = { 0 };

    lastError = (area == S7AreaDB)
        ? client->DBRead(dbNumber, start, dataSize, buffer)
        : client->ReadArea(area, 0, start, dataSize, S7WLByte, buffer);

    if (lastError != 0)
        return false;

    if (bitIndex >= 0)
    {
        value = (buffer[0] >> bitIndex) & 1;
        return true;
    }

    if (dataSize == 1)
        value = buffer[0];
    else if (dataSize == 2)
        value = (buffer[0] << 8) | buffer[1];
    else if (dataSize == 4)
        value = (buffer[0] << 24) | (buffer[1] << 16)
        | (buffer[2] << 8) | buffer[3];

    return true;
}
//写
bool PLCClient::writeAddress(const std::string& addr, int32_t value)
{
    int area, dbNumber, start, bitIndex, dataSize;

    if (!parseAddress(addr, area, dbNumber, start, bitIndex, dataSize))
    {
        lastError = -1;
        return false;
    }

    uint8_t buffer[4] = { 0 };

    if (bitIndex >= 0)
    {
        uint8_t b = 0;

        lastError = (area == S7AreaDB)
            ? client->DBRead(dbNumber, start, 1, &b)
            : client->ReadArea(area, 0, start, 1, S7WLByte, &b);

        if (lastError != 0)
            return false;

        if (value)
            b |= (uint8_t)(1u << bitIndex);
        else
            b &= (uint8_t)~(1u << bitIndex);

        lastError = (area == S7AreaDB)
            ? client->DBWrite(dbNumber, start, 1, &b)
            : client->WriteArea(area, 0, start, 1, S7WLByte, &b);

        return lastError == 0;
    }

    if (dataSize == 1)
        buffer[0] = (uint8_t)value;
    else if (dataSize == 2)
    {
        buffer[0] = (value >> 8) & 0xFF;
        buffer[1] = value & 0xFF;
    }
    else if (dataSize == 4)
    {
        buffer[0] = (value >> 24) & 0xFF;
        buffer[1] = (value >> 16) & 0xFF;
        buffer[2] = (value >> 8) & 0xFF;
        buffer[3] = value & 0xFF;
    }

    lastError = (area == S7AreaDB)
        ? client->DBWrite(dbNumber, start, dataSize, buffer)
        : client->WriteArea(area, 0, start, dataSize, S7WLByte, buffer);

    return lastError == 0;
}
//运行状态
bool PLCClient::getCpuStatus(int& cpuStatus)
{
    lastError = client->PlcStatus();

    if (lastError < 0)
    {
        cpuStatus = 0;
        return false;
    }
    cpuStatus = lastError;
    return true;
}
bool PLCClient::setPlcRun()
{
    lastError = client->PlcHotStart();

    if (lastError < 0)
        return false;

    return true;
}
bool PLCClient::setPlcStop()
{
    lastError = client->PlcStop();

    if (lastError < 0)
        return false;

    return true;
}
// 返回最近一次 Snap7 错误文本
std::string PLCClient::getLastErrorText() const
{
    if (!client)
        return std::string();

    return CliErrorText(lastError);
}

// 读取 PLC 的身份信息（模块信息与固件版本）
bool PLCClient::getPlcIdentity(PlcIdentity& info)
{
    TS7OrderCode order{};
    TS7CpuInfo cpuInfo{};

    lastError = client->GetOrderCode(&order);
    if (lastError != 0)
        return false;

    lastError = client->GetCpuInfo(&cpuInfo);
    if (lastError != 0)
        return false;

    info.orderCode = order.Code;
    info.moduleName = cpuInfo.ModuleTypeName;
    info.versionMajor = order.V1;
    info.versionMinor = order.V2;
    info.versionPatch = order.V3;

    return true;
}

// 读取 PLC 当前系统时间
bool PLCClient::getPlcTime(PlcTime& time)
{
    tm plcTime{};

    lastError = client->GetPlcDateTime(&plcTime);
    if (lastError != 0)
        return false;

    time.year = plcTime.tm_year + 1900;
    time.month = plcTime.tm_mon + 1;
    time.day = plcTime.tm_mday;
    time.hour = plcTime.tm_hour;
    time.minute = plcTime.tm_min;
    time.second = plcTime.tm_sec;

    return true;
}

// 将 PLC 系统时间同步为当前本机时间
bool PLCClient::syncPlcTimeWithLocal()
{
    time_t now = time(nullptr);
    tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    lastError = client->SetPlcDateTime(&localTime);
    if (lastError != 0)
        return false;

    return true;
}

// 读取 PLC 的 DB 块数据（快照读取）
bool PLCClient::readDbBlock(
    int dbNumber,
    int start,
    int size,
    std::vector<uint8_t>& buffer
)
{
    buffer.resize(size);

    lastError = client->DBRead(dbNumber, start, size, buffer.data());
    if (lastError != 0)
        return false;

    return true;
}

