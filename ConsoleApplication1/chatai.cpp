#include "chatai.h"
#include "aicontroller.h"
#include "aitrace.h"

#include <json/json.h>
#include <sstream>

// 构造
ChatAI::ChatAI(int AICODE, AIController& aiRef, AITrace& traceRef)
    : aicode(AICODE),
    ai(aiRef),
    trace(traceRef)
{
    // 初始化结果缓存
    r.control = 0;
    r.priority = 0;
}
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

std::string ChatAI::runOnce(const std::string& user_input)
{
    // 开始 Trace
    trace.begin(
        "chat",
        aicode,
        user_input,
        ai.chatprompt_get()
    );
    // 调用 AIController（Chat 通道）
    std::string jsonOut = ai.allairun(
        true,                   // 读取短期记忆
        true,                   // 写入短期记忆
        aicode,                 // ai 模式
        "chat",                 // 记忆槽
        user_input,             // 用户输入
        ai.chatprompt_get()     // 固定 Chat Prompt
    );
    // 解析 JSON
    bool ok = parseChatJson(jsonOut);
    // 结束 Trace
    trace.end(ok, jsonOut);
    // 返回用于显示的文本
    if (!ok)
        return jsonOut;
    if (!r.ainame.empty())
        return r.ainame + u8"：" + r.text;
    return r.text;
}
std::string ChatAI::runOnce(
    const std::string& user_input,
    const std::string& personaText
)
{
    // 开始 Trace
    trace.begin(
        "chat",
        aicode,
        user_input,
        ai.chatprompt_get()
    );

    // 调用 AIController（Chat 通道 + 人格注入）
    std::string jsonOut = ai.allairun(
        true,                   // 读取短期记忆
        true,                   // 写入短期记忆
        aicode,                 // ai 模式
        "chat",                 // 记忆槽
        user_input,             // 用户输入
        ai.chatprompt_get(),    // 固定 Chat Prompt
        personaText             // ★ 人格设定（system 级）
    );

    // 解析 JSON
    bool ok = parseChatJson(jsonOut);

    // 结束 Trace
    trace.end(ok, jsonOut);

    // 返回用于显示的文本
    if (!ok)
        return jsonOut;

    if (!r.ainame.empty())
        return r.ainame + u8"：" + r.text;

    return r.text;
}

bool ChatAI::parseChatJson(const std::string& jsonText)
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;

    std::istringstream iss(jsonText);
    if (!Json::parseFromStream(builder, iss, &root, &errors))
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

    // priority
    if (root.isMember("priority") && root["priority"].isInt())
        r.priority = root["priority"].asInt();
    else
        r.priority = 0;

    // emotion（支持字符串或对象）
    if (root.isMember("emotion"))
    {
        if (root["emotion"].isString())
        {
            r.emotion = root["emotion"].asString();
        }
        else if (root["emotion"].isObject() &&
            root["emotion"].isMember("state") &&
            root["emotion"]["state"].isString())
        {
            r.emotion = root["emotion"]["state"].asString();
        }
        else
        {
            r.emotion.clear();
        }
    }
    else
    {
        r.emotion.clear();
    }

    return true;
}
