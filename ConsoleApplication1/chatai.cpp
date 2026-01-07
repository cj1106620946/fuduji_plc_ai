#include "chatai.h"
#include "aicontroller.h"
#include "aitrace.h"
#include <string>
// 构造
ChatAI::ChatAI(int AICODE,AIController& aiRef,AITrace& traceRef)
:ai(aiRef), trace(traceRef), aicode(AICODE)
{
}

// 对话唯一入口
std::string ChatAI::runOnce(const std::string& user_input)
{
    // 调用开始记录
    trace.begin("chat", aicode, user_input,ai.chatTalkprompt_get());
    std::string output;
    // 调用对话 AI
    output = callChatAI(user_input);
    // 调用结束记录
    trace.end(1,output);
    return output;
}

// 内部：调用对话 AI
std::string ChatAI::callChatAI(const std::string& user_input)
{
    return ai.chatTalk(aicode,user_input);
}
