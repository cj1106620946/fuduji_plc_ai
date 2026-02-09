#include "memoryai.h"
#include "aicontroller.h"
#include "aitrace.h"

#include <json/json.h>
#include <sstream>

// 构造
MemoryAI::MemoryAI(int aicode, AIController& aiRef, AITrace& traceRef)
    : aicode(aicode),
    ai(aiRef),
    trace(traceRef)
{
    // 初始化 selfMemory
    for (int i = 0; i < 3; ++i)
    {
        state.selfMemory[i].memoryKeyId = 0;
        state.selfMemory[i].keyPath.clear();
        state.selfMemory[i].content.clear();
    }

    // 初始化 userMemory
    for (int i = 0; i < 6; ++i)
    {
        state.userMemory[i].memoryKeyId = 0;
        state.userMemory[i].keyPath.clear();
        state.userMemory[i].content.clear();
    }
}

// 生成 self 1-3 的长期记忆（带人格通道）
std::string MemoryAI::runself(
    const std::string& currentText,
    const std::string& personaText
)
{
    // 开始 Trace
    trace.begin(
        u8"memory_self",
        aicode,
        currentText,
        ai.memory13prompt_get()
    );

    // 调用 AIController（self 1-3 + 人格通道）
    std::string jsonOut = ai.allairun(
        true,                       // 读取短期记忆
        false,                      // 不写入短期记忆
        aicode,                     // ai 模式
        u8"memory_self",            // 记忆槽
        currentText,                // 当前需要整理的文本
        ai.memory13prompt_get(),    // self 1-3 prompt
        personaText                 // ★ 上一次已确认的人格记忆
    );

    // 解析 JSON
    bool ok = parseMemoryJson(jsonOut, true);

    // 结束 Trace
    trace.end(ok, jsonOut);

    return jsonOut;
}

// 生成 user 4-9 的长期记忆（带人格通道）
std::string MemoryAI::runuser(
    const std::string& currentText,
    const std::string& personaText
)
{
    // 开始 Trace
    trace.begin(
        u8"memory_user",
        aicode,
        currentText,
        ai.memory49prompt_get()
    );
    // 调用 AIController（user 4-9 + 人格通道）
    std::string jsonOut = ai.allairun(
        true,                       // 读取短期记忆
        false,                      // 不写入短期记忆
        aicode,                     // ai 模式
        u8"memory_user",            // 记忆槽
        currentText,                // 当前需要整理的文本
        ai.memory49prompt_get(),    // user 4-9 prompt
        personaText                 // ★ 上一次已确认的用户长期记忆
    );
    // 解析 JSON
    bool ok = parseMemoryJson(jsonOut, false);
    // 结束 Trace
    trace.end(ok, jsonOut);
    return jsonOut;
}
std::string MemoryAI::getMemoryContent(int memoryKeyId)
{
    // self 1-3
    if (memoryKeyId >= 1 && memoryKeyId <= 3)
    {
        return state.selfMemory[memoryKeyId - 1].content;
    }

    // user 4-9
    if (memoryKeyId >= 4 && memoryKeyId <= 9)
    {
        return state.userMemory[memoryKeyId - 4].content;
    }

    // 非法 key，返回错误文本
    return u8"[error] 无效的 memoryKeyId";
}

// 统一 JSON 解析
bool MemoryAI::parseMemoryJson(
    const std::string& jsonText,
    bool isSelf
)
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;

    std::istringstream iss(jsonText);
    if (!Json::parseFromStream(builder, iss, &root, &errors))
        return false;

    // 必须是数组
    if (!root.isArray())
        return false;

    for (Json::ArrayIndex i = 0; i < root.size(); ++i)
    {
        const Json::Value& item = root[i];
        if (!item.isObject())
            continue;

        if (!item.isMember("name") || !item.isMember("text"))
            continue;

        if (!item["name"].isString() || !item["text"].isString())
            continue;

        std::string name = item["name"].asString();
        std::string text = item["text"].asString();

        if (isSelf)
        {
            if (name == "self.identity")
            {
                state.selfMemory[0].keyPath = name;
                state.selfMemory[0].content = text;
            }
            else if (name == "self.emotion")
            {
                state.selfMemory[1].keyPath = name;
                state.selfMemory[1].content = text;
            }
            else if (name == "self.attitude")
            {
                state.selfMemory[2].keyPath = name;
                state.selfMemory[2].content = text;
            }
        }
        else
        {
            // user 4-9
            if (name == "user.summary")
            {
                state.userMemory[0].keyPath = name;
                state.userMemory[0].content = text;
            }
            else if (name == "user.preference")
            {
                state.userMemory[1].keyPath = name;
                state.userMemory[1].content = text;
            }
            else if (name == "user.addressing")
            {
                state.userMemory[2].keyPath = name;
                state.userMemory[2].content = text;
            }
            else if (name == "user.interaction")
            {
                state.userMemory[3].keyPath = name;
                state.userMemory[3].content = text;
            }
            else if (name == "user.context")
            {
                state.userMemory[4].keyPath = name;
                state.userMemory[4].content = text;
            }
            else if (name == "user.constraints")
            {
                state.userMemory[5].keyPath = name;
                state.userMemory[5].content = text;
            }
        }
    }

    return true;
}
