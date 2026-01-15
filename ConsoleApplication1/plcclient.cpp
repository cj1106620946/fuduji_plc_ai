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
//运行状态
bool PLCClient::getCpuStatus(int& cpuStatus)
{
    if (!connected)
        return false;

    int result = client->PlcStatus();
    cpuStatus = result; // Snap7 的 PlcStatus() 返回 CPU 状态（0x08=RUN, 0x04=STOP, 0x00=未知）
if (result < 0) {
    lastError = result;
    return false;
}
return true;
}
//返回错误文本
std::string PLCClient::getLastErrorText() const
{
    if (!client)
        return std::string();
    // 使用 Snap7 提供的全局函数 CliErrorText 获取错误文本
    return CliErrorText(lastError);
}

// 读取 PLC 的 DB 块数据（快照读取）
// dbNumber: DB 块号，例如 DB1
// start: DB 内起始字节偏移
// size: 需要读取的字节数
// buffer: 输出缓冲区，返回时包含读取到的原始字节数据
bool PLCClient::readDbBlock(int dbNumber,int start,int size,std::vector<uint8_t>& buffer
)
{
    // 未连接 PLC，直接失败
    if (!connected)
        return false;

    // 根据读取大小调整缓冲区长度
    buffer.resize(size);

    // 调用 Snap7 接口读取 DB 区域
    // 该操作一次性读取指定范围，保证数据来自同一时刻
    int result = client->DBRead(dbNumber, start, size, buffer.data());

    // 返回值不为 0 表示读取失败
    if (result != 0)
    {
        // 记录最近一次错误码，供外部查询错误原因
        lastError = result;
        return false;
    }

    // 读取成功
    return true;
}
// 将 PLC 切换到 RUN 状态
// 返回 true 表示指令发送成功
bool PLCClient::setPlcRun()
{
    // 未连接 PLC，直接失败
    if (!connected)
        return false;

    // HotStart：不清除数据区，直接启动 CPU
    int result = client->PlcHotStart();

    // 小于 0 表示发生错误
    if (result < 0)
    {
        lastError = result;
        return false;
    }

    return true;
}
// 将 PLC 切换到 STOP 状态
bool PLCClient::setPlcStop()
{
    // 未连接 PLC，直接失败
    if (!connected)
        return false;

    // 停止 PLC CPU 运行
    int result = client->PlcStop();

    // 小于 0 表示发生错误
    if (result < 0)
    {
        lastError = result;
        return false;
    }

    return true;
}
// 读取 PLC 的身份信息（模块信息与固件版本）
bool PLCClient::getPlcIdentity(PlcIdentity& info)
{
    // 未连接 PLC，直接失败
    if (!connected)
        return false;
    TS7OrderCode order{};
    TS7CpuInfo cpuInfo{};
    // 读取 PLC 订货号与模块信息
    int r1 = client->GetOrderCode(&order);
    if (r1 != 0)
    {
        lastError = r1;
        return false;
    }
    // 读取 PLC CPU 固件版本信息
    int r2 = client->GetCpuInfo(&cpuInfo);
    if (r2 != 0)
    {
        lastError = r2;
        return false;
    }
    // 填充身份信息结构体
    info.orderCode = order.Code;
    info.moduleName = cpuInfo.ModuleTypeName;
    // 使用订货号中的 V1/V2/V3 作为版本号
    info.versionMajor = order.V1;
    info.versionMinor = order.V2;
    info.versionPatch = order.V3;

    return true;
}

// 读取 PLC 当前系统时间
bool PLCClient::getPlcTime(PlcTime& time)
{
    if (!connected)
        return false;

    tm plcTime{};
    int result = client->GetPlcDateTime(&plcTime);
    if (result != 0)
    {
        lastError = result;
        return false;
    }

    // Snap7 返回的是标准 tm 结构
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
    if (!connected)
        return false;

    // 获取当前本机时间
    time_t now = time(nullptr);
    tm localTime{};

#if defined(_WIN32)
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif

    // 将本机时间写入 PLC
    int result = client->SetPlcDateTime(&localTime);
    if (result != 0)
    {
        lastError = result;
        return false;
    }

    return true;
}
