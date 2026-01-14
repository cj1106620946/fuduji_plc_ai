#pragma once

#include <string>

// Live2D 侧状态读取器
// 职责：
// 1 读取 live2dstate.json
// 2 dirty == true 时取出 value
// 3 读取成功后立刻清除 dirty
class JsonStateReader
{
public:
    // path 例如 "live2dstate.json"
    explicit JsonStateReader(const std::string& path);

    // 读取状态
    // 返回 true  : 读到了新状态
    // 返回 false : 没有新状态
    bool read(std::string& outText,
        std::string& outEmotion,
        int& outPriority);

private:
    void clearDirty();

private:
    std::string filepath;
};
