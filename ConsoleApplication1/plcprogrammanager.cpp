#include "plcprogrammanager.h"
#include <fstream>
#include <vector>

// 构造
PlcProgramManager::PlcProgramManager(TS7Client* clientRef)
    : client(clientRef), lastError(0)
{
}

// 析构
PlcProgramManager::~PlcProgramManager()
{
}

// 从 PLC 导出程序块到文件
bool PlcProgramManager::exportBlockToFile(
    PlcBlockType blockType,
    int blockNumber,
    const std::string& filePath
)
{
    if (!client)
        return false;

    int size = 0;

    // 第一次调用，仅获取程序块大小
    int result = client->Upload(
        static_cast<int>(blockType),
        blockNumber,
        nullptr,
        &size
    );

    if (result != 0 || size <= 0)
    {
        lastError = result;
        return false;
    }

    std::vector<uint8_t> buffer;
    buffer.resize(size);

    // 第二次调用，读取程序块内容
    result = client->Upload(
        static_cast<int>(blockType),
        blockNumber,
        buffer.data(),
        &size
    );

    if (result != 0)
    {
        lastError = result;
        return false;
    }

    std::ofstream out(filePath.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return false;

    out.write(reinterpret_cast<const char*>(buffer.data()), size);
    out.close();

    return true;
}

// 从文件导入程序块并写入 PLC
bool PlcProgramManager::importBlockFromFile(
    PlcBlockType blockType,
    int blockNumber,
    const std::string& filePath
)
{
    if (!client)
        return false;

    std::ifstream in(filePath.c_str(), std::ios::binary | std::ios::ate);
    if (!in.is_open())
        return false;

    std::streamsize size = in.tellg();
    if (size <= 0)
        return false;

    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer;
    buffer.resize(static_cast<size_t>(size));

    if (!in.read(reinterpret_cast<char*>(buffer.data()), size))
        return false;

    in.close();

    // 将程序块写入 PLC — 调用 TS7Client::Download(blockNum, pUsrData, Size)
    int result = client->Download(
        blockNumber,
        static_cast<void*>(buffer.data()),
        static_cast<int>(buffer.size())
    );

    if (result != 0)
    {
        lastError = result;
        return false;
    }

    return true;
}

// 判断 PLC 中是否存在指定程序块
bool PlcProgramManager::hasBlock(
    PlcBlockType blockType,
    int blockNumber
)
{
    if (!client)
        return false;

    TS7BlockInfo info{};
    int result = client->GetAgBlockInfo(
        static_cast<int>(blockType),
        blockNumber,
        &info
    );

    if (result != 0)
    {
        lastError = result;
        return false;
    }

    return true;
}

// 获取最近一次错误码
int PlcProgramManager::getLastError() const
{
    return lastError;
}
