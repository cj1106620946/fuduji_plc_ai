#include "memoryai.h"
#include "aicontroller.h"
#include "aitrace.h"

// 构造
MemoryAI::MemoryAI(int aicode, AIController& aiRef, AITrace& traceRef)
    : aicode(aicode),
    ai(aiRef),
    trace(traceRef)
{
}

// 记忆读取判断
std::string MemoryAI::runJudge(const std::string& user_input)
{
    return std::string();
}

// 记忆写入
std::string MemoryAI::runWrite(const std::string& user_input)
{
    return std::string();
}

// 长期记忆整理
std::string MemoryAI::runManage(const std::string& user_input)
{
    return std::string();
}
