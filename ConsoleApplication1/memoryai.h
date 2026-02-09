#pragma once

#include <string>

class AIController;
class AITrace;

struct MemoryAIItem
{
    int memoryKeyId;
    std::string keyPath;
    std::string content;
};

// MemoryAI 专用记忆状态
struct MemoryAIState
{
    MemoryAIItem selfMemory[3];
    MemoryAIItem userMemory[6];
};

class MemoryAI
{
public:
    // 构造
    MemoryAI(int aicode, AIController& aiRef, AITrace& traceRef);

    // 生成 self 1-3 的长期记忆
    std::string runself(const std::string& user_input,
        const std::string& personaText
    );

    // 生成 user 4-9 的长期记忆
    std::string runuser(const std::string& user_input,
        const std::string& personaText
    );
    std::string getMemoryContent(int memoryKeyId);

private:
    // 统一 JSON 解析（[{name,text}, ...]）
    // 解析结果直接写入 state
    bool parseMemoryJson(
        const std::string& jsonText,
        bool isSelf   // true = selfMemory，false = userMemory
    );

private:
    int aicode;
    AIController& ai;
    AITrace& trace;

    // 内部状态，仅供 MemoryAI 自己使用
    MemoryAIState state;
};
