#include "jsonstatereader.h"
#include <fstream>

// 构造
JsonStateReader::JsonStateReader(const std::string& path)
    : filepath(path)
{
}
// 读取状态
bool JsonStateReader::read(
    std::string& outText,
    std::string& outEmotion,
    int& outPriority)
{
    std::ifstream in(filepath.c_str(), std::ios::binary);
    if (!in.is_open())
    {
        return false;
    }

    bool dirty = false;
    std::string text;
    std::string emotion;
    int priority = 0;
    std::string line;
    while (std::getline(in, line))
    {
        // dirty
        if (line.find("\"dirty\"") != std::string::npos)
        {
            if (line.find("true") != std::string::npos)
                dirty = true;
        }

        // text
        if (line.find("\"text\"") != std::string::npos)
        {
            std::size_t p1 = line.find("\"", line.find(":") + 1);
            std::size_t p2 = line.find("\"", p1 + 1);
            if (p1 != std::string::npos && p2 != std::string::npos)
            {
                text = line.substr(p1 + 1, p2 - p1 - 1);
            }
        }

        // emotion
        if (line.find("\"emotion\"") != std::string::npos)
        {
            std::size_t p1 = line.find("\"", line.find(":") + 1);
            std::size_t p2 = line.find("\"", p1 + 1);
            if (p1 != std::string::npos && p2 != std::string::npos)
            {
                emotion = line.substr(p1 + 1, p2 - p1 - 1);
            }
        }

        // priority
        if (line.find("\"priority\"") != std::string::npos)
        {
            std::size_t p = line.find(":");
            if (p != std::string::npos)
            {
                priority = std::atoi(line.substr(p + 1).c_str());
            }
        }
    }

    in.close();

    // dirty=false，说明没有新状态
    if (!dirty)
    {
        return false;
    }

    outText = text;
    outEmotion = emotion;
    outPriority = priority;

    // 读完立刻清 dirty
    clearDirty();
    return true;
}

// 清除 dirty 标志
void JsonStateReader::clearDirty()
{
    std::ofstream out(filepath.c_str(), std::ios::binary | std::ios::trunc);
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

