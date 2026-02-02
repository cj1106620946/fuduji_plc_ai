#pragma once

#include <string>

class AIController;
class AITrace;

// MemoryAI
// 仅作为“记忆 AI 调度入口”的占位骨架
// 不包含任何实现逻辑
class MemoryAI
{
public:
    // aicode：AI 编号或模式
    MemoryAI(int aicode, AIController& aiRef, AITrace& traceRef);

    // ===== 记忆读取判断 =====
    // 判断用户输入是否与已有记忆相关
    std::string runJudge(const std::string& user_input);

    // ===== 记忆写入 =====
    // 将一次对话或事件整理为记忆
    std::string runWrite(const std::string& user_input);

    // ===== 长期记忆整理 =====
    // 后台调用，用于整理长期记忆
    std::string runManage(const std::string& user_input);

private:
    int aicode;
    AIController& ai;
    AITrace& trace;
};
