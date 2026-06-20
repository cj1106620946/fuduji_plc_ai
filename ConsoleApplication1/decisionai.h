#pragma once
#include <string>

class AIController;
class AITrace;

// 决策 AI：分析当前状态并给出建议
class DecisionAI
{
public:
    // 构造
    DecisionAI(int AICODE, AIController& aiRef, AITrace& traceRef);

    // 返回：解析后的 content 文本，失败时返回错误说明
    std::string runOnce(
        const std::string& snapshot,
        const std::string& user_input
    );

private:
    // 调用 AI
    std::string callDecisionAI(
        const std::string& snapshot,
        const std::string& user_input
    );

    // 解析 decision JSON，只提取 content
    bool parseDecisionJson(
        const std::string& jsonText,
        std::string& outContent
    );

private:
    int aicode;
    AIController& ai;
    AITrace& trace;
};
