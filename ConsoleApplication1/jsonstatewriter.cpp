#include "jsonstatewriter.h"
#include <fstream>

// 构造函数
JsonStateWriter::JsonStateWriter(const std::string& path)
    : filepath(path)
{
}

// 初始化：确保 json 文件存在
void JsonStateWriter::init()
{
    std::ifstream in(filepath.c_str(), std::ios::binary);
    if (in.is_open())
    {
        in.close();
        return;
    }

    // 文件不存在，创建默认 json
    std::ofstream out(filepath.c_str(), std::ios::binary);
    if (!out.is_open())
    {
        return;
    }

    out << "{\n";
    out << "  \"dirty\": false,\n";
    out << "  \"value\": \"\"\n";
    out << "}\n";

    out.close();
}

// 写入状态：直接覆盖
bool JsonStateWriter::write(const std::string& value)
{
    std::ofstream out(filepath.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    out << "{\n";
    out << "  \"dirty\": true,\n";
    out << "  \"value\": \"" << value << "\"\n";
    out << "}\n";

    out.close();
    return true;
}
