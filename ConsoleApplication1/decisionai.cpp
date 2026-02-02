#include "decisionai.h"
#include "aicontroller.h"
#include "aitrace.h"

#include <json/json.h>
#include <string>

// 构造
DecisionAI::DecisionAI(
    int AICODE,
    AIController& aiRef,
    AITrace& traceRef
)
    : aicode(AICODE),
    ai(aiRef),
    trace(traceRef)
{
}

// 唯一执行入口
std::string DecisionAI::runOnce(
    const std::string& snapshot,
    const std::string& user_input
)
{
	std::string trace_in = snapshot+"|" + user_input;
    // 开始 trace 记录
    trace.begin(
        "decision",
        aicode,
        trace_in,
        ai.decisionprompt_get()
    );
    trace.debug(u8"[STEP] 调用决策 AI，获取原始 JSON");
    // 调用 AI
    std::string jsonOut = callDecisionAI(snapshot, user_input);

    // 解析 JSON
    std::string content;
    if (!parseDecisionJson(jsonOut, content))
    {
        trace.debug(u8"[PARSE] decision JSON 解析失败，直接返回原始输出");
        trace.end(false, jsonOut);
        return jsonOut;
    }
    trace.debug(u8"[PARSE] decision JSON 解析成功");
    trace.debug(u8"[CONTENT]");
    trace.debug(content);
    trace.end(true, jsonOut);
    return content;
}

// 调用 AIController
std::string DecisionAI::callDecisionAI(
    const std::string& snapshot,
    const std::string& user_input
)
{
    // snapshot + user_input 作为输入上下文
    std::string input;
    input.reserve(snapshot.size() + user_input.size() + 16);
    input.append(snapshot);
    input.append("\n");
    input.append(user_input);

    return ai.allairun(
        true,
        true,
        aicode,
        "decision",
        input,
        ai.decisionprompt_get()
    );
}

// 解析 decision JSON，只提取 content
bool DecisionAI::parseDecisionJson(
    const std::string& jsonText,
    std::string& outContent
)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(jsonText, root))
        return false;
    if (root.isMember("content") && root["content"].isString())
    {
        outContent = root["content"].asString();
        return true;
    }
    return false;
}
