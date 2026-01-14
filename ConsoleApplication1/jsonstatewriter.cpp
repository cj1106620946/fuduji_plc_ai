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

    std::ofstream out(filepath.c_str(), std::ios::binary);
    if (!out.is_open())
    {
        return;
    }

    out << "{\n";
    out << "  \"dirty\": false,\n";
    out << "  \"text\": \"\",\n";
    out << "  \"emotion\": \"neutral\",\n";
    out << "  \"priority\": 0\n";
    out << "}\n";

    out.close();
}

bool JsonStateWriter::write(
    const std::string& text,
    const std::string& emotion,
    int priority)
{
    std::ofstream out(filepath.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    out << "{\n";
    out << "  \"dirty\": true,\n";
    out << "  \"text\": \"" << text << "\",\n";
    out << "  \"emotion\": \"" << emotion << "\",\n";
    out << "  \"priority\": " << priority << "\n";
    out << "}\n";

    out.close();
    return true;
}
