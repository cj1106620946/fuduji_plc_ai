#pragma once

#include <string>
#include <vector>
#include "snap7.h"

// PLC 程序块类型
enum PlcBlockType
{
    PlcBlockOB = 0x38,
    PlcBlockDB = 0x41,
    PlcBlockFC = 0x43,
    PlcBlockFB = 0x44
};

// PlcProgramManager
// 只负责 PLC 程序块与本地文件之间的导入与导出
// 不解析程序内容，不理解语义，不参与控制逻辑
class PlcProgramManager
{
public:
    PlcProgramManager(TS7Client* clientRef);
    ~PlcProgramManager();

    // 从 PLC 中导出指定程序块到文件
    // filePath: 本地文件路径，例如 "templates/fb10.bin"
    bool exportBlockToFile(
        PlcBlockType blockType,
        int blockNumber,
        const std::string& filePath
    );

    // 从文件导入程序块并写入 PLC
    // filePath: 本地程序块文件路径
    bool importBlockFromFile(
        PlcBlockType blockType,
        int blockNumber,
        const std::string& filePath
    );

    // 判断 PLC 中是否存在指定程序块
    bool hasBlock(
        PlcBlockType blockType,
        int blockNumber
    );

    // 获取最近一次错误码
    int getLastError() const;

private:
    TS7Client* client;
    int lastError;
};
