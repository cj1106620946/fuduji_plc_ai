#include "judgmentai.h"
#include "chatai.h"
#include "aicontroller.h"
#include "aitrace.h"
#include <string>
// 构造
Judgmentai::Judgmentai(int AICODE, AIController& aiRef, AITrace& traceRef)
    :ai(aiRef), trace(traceRef), aicode(AICODE)
{
}

// 对话唯一入口
std::string Judgmentai::runOnce(const std::string& user_input)
{
    trace.begin("judgment", aicode, user_input, ai.judgmentprompt_get());
    std::string output;
    // 调用对话 AI
    output = callJudgmentai(user_input);
    // 调用结束记录
    trace.end(1, output);
    return output;
}

// 内部：调用对话 AI
std::string Judgmentai::callJudgmentai(const std::string& user_input)
{
    return ai.judgment(aicode, user_input);
}
