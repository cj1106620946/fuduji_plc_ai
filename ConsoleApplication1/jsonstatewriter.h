#pragma once

#include <string>


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
