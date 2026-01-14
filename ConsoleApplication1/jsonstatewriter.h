#pragma once

#include <string>

// plcai 侧 Live2D 状态写入器
// 职责：
// 1 初始化时创建 json 文件
// 2 写入 dirty + value
class JsonStateWriter
{
public:
    // path 例如 "live2dstate.json"
    explicit JsonStateWriter(const std::string& path);

    // 初始化：若文件不存在则创建默认 json
    void init();

    // 写入状态：直接覆盖文件
    bool write(const std::string& text,
        const std::string& emotion,
        int priority);

private:
    std::string filepath;
};
