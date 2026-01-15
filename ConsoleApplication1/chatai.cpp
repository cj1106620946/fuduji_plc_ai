#include "chatai.h"
#include "aicontroller.h"
#include "aitrace.h"
#include <string>
#include <json/json.h>
#include <sstream>

// 构造
ChatAI::ChatAI(int AICODE,AIController& aiRef,AITrace& traceRef)
:ai(aiRef), trace(traceRef), aicode(AICODE)
{

}

//读取
std::string ChatAI::getAiName()
{
    return r.ainame;
}
std::string ChatAI::getText()
{
    return r.text;
}
int ChatAI::getControl()
{
    return r.control;
}
int ChatAI::getPriority()
{
    return r.priority;
}
std::string ChatAI::getEmotion()
{
    return r.emotion;
}
//执行ai对话入口
std::string ChatAI::runExecuteRead(const std::string& user_input)
{
    trace.begin(
        "chat_execute",
        aicode,
        user_input,
        ai.chatexecuteprompt_get()
    );
    trace.debug(u8"[STEP] 调用执行入口，获取 AI 原始 JSON");
    std::string jsonOut = runExecuteOnce(user_input);
    if (!parseChatJson(jsonOut))
    {
        trace.debug(u8"[PARSE] JSON 解析失败，直接返回原始输出");
        trace.end(false, jsonOut);
        return jsonOut;
    }
    std::string displayText;
    if (!r.ainame.empty())
        displayText = r.ainame + u8"：" + r.text;
    else
        displayText = r.text;
    trace.debug(u8"[PARSE] JSON 解析成功");
    trace.debug(u8"[DISPLAY] ");
    trace.debug(displayText);trace.end(true, jsonOut);
    return displayText;
}
std::string ChatAI::runExecuteOnce(const std::string& user_input)
{
    return callChatExecuteAI(user_input);
}
std::string ChatAI::callChatExecuteAI(const std::string& user_input)
{
    // 这里使用新的 chatExecute 接口
    return ai.allairun(
         true,
         true,
        aicode,
        "chat_e",
        user_input,
        ai.chatexecuteprompt_get()
    );
}
bool ChatAI::parseChatJson(const std::string& jsonText)
{
    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(jsonText, root))
        return false;

    // ainame
    if (root.isMember("ainame") && root["ainame"].isString())
        r.ainame = root["ainame"].asString();
    else
        r.ainame.clear();

    // text
    if (root.isMember("text") && root["text"].isString())
        r.text = root["text"].asString();
    else
        r.text.clear();

    // control
    if (root.isMember("control") && root["control"].isInt())
        r.control = root["control"].asInt();
    else
        r.control = 0;

    // emotion
    if (root.isMember("emotion") && root["emotion"].isString())
        r.emotion = root["emotion"].asString();
    else if (root.isMember("emotion") &&
        root["emotion"].isObject() &&
        root["emotion"].isMember("state") &&
        root["emotion"]["state"].isString())
        r.emotion = root["emotion"]["state"].asString();
    else
        r.emotion.clear();

    // priority（新增，对话优先度）
    if (root.isMember("priority") && root["priority"].isInt())
        r.priority = root["priority"].asInt();
    else
        r.priority = 0;

    return true;
}
// 普通ai对话入口
std::string ChatAI::runOnce(const std::string& user_input)
{
    // 调用开始记录
    trace.begin("chat", aicode, user_input, ai.chatprompt_get());
    std::string output;
    // 调用对话 AI
    output = callChatAI(user_input);
    // 调用结束记录
    trace.end(1, output);
    return output;
}
std::string ChatAI::callChatAI(const std::string& user_input)
{
    return ai.chatTalk(aicode, user_input);
}
