#pragma once

#include <string>

class ChatAgent;

class DecisionAI
{
public:
    // 构造
    explicit DecisionAI(ChatAgent& agent);

    // 执行一次决策（不解析、不配置、不判断）
    std::string runOnce();

    // ===== 预留外部接口（不实现）=====
    // 外部数据输入（例如爬虫、传感器、HTTP 等）
    void feedExternalData(const std::string& data);

private:
    ChatAgent& chat;
};
