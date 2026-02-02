#include "executeai.h"
#include "aicontroller.h"
#include "aitrace.h"
#include <json/json.h>
#include <sstream>

// 构造
ExecuteAI::ExecuteAI(
    int AICODE,
    AIController& aiRef,
    AITrace& traceRef
)
    : aicode(AICODE),
    ai(aiRef),
    trace(traceRef)
{
}

// 调用执行 AI，获取原始 JSON
std::string ExecuteAI::callExecuteAI(const std::string& user_input)
{
    return ai.allairun(
        true,
        true,
        aicode,
        "execute",
        user_input,
        ai.executeprompt_get()
    );
}

// 外部唯一入口
// 返回：解析后的 ExecuteItem 列表
std::vector<ExecuteItem>
ExecuteAI::runOnce(const std::string& user_input)
{
    trace.begin(
        "execute",
        aicode,
        user_input,
        ai.executeprompt_get()
    );

    std::string jsonText = callExecuteAI(user_input);

    std::vector<ExecuteItem> items = parseExecuteJson(jsonText);

    trace.end(true, jsonText);
    return items;
}

// 解析执行 JSON
std::vector<ExecuteItem>
ExecuteAI::parseExecuteJson(const std::string& jsonText)
{
    std::vector<ExecuteItem> result;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string err;

    std::istringstream ss(jsonText);
    if (!Json::parseFromStream(builder, ss, &root, &err))
    {
        ExecuteItem it;
        it.message = u8"执行 JSON 解析失败";
        result.push_back(it);
        return result;
    }

    // type != ok，直接返回 message
    std::string type = root.get("type", "").asString();
    std::string message = root.get("message", "").asString();

    if (type != "ok")
    {
        ExecuteItem it;
        it.message = message.empty() ? u8"无法执行当前指令" : message;
        result.push_back(it);
        return result;
    }

    // actions 必须是数组
    if (!root.isMember("actions") || !root["actions"].isArray())
    {
        ExecuteItem it;
        it.message = u8"执行指令中缺少 actions 数组";
        result.push_back(it);
        return result;
    }

    const Json::Value& actions = root["actions"];

    // 无动作：返回一条 message 即可
    if (actions.empty())
    {
        ExecuteItem it;
        it.message = message;
        result.push_back(it);
        return result;
    }

    // 逐条解析 action
    for (const auto& act : actions)
    {
        ExecuteItem it;

        it.message = message;
        it.op = act.get("op", "").asString();
        it.address = act.get("address", "").asString();

        if (act.isMember("value"))
        {
            if (act["value"].isString())
                it.value = act["value"].asString();
            else if (act["value"].isInt())
                it.value = std::to_string(act["value"].asInt());
            else if (act["value"].isDouble())
                it.value = std::to_string(act["value"].asDouble());
        }

        result.push_back(it);
    }

    return result;
}
