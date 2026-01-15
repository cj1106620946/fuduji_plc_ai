#include "executeai.h"
#include "aicontroller.h"
#include "plcclient.h"
#include <json/json.h>
#include "aitrace.h"
#include <sstream>

// 构造
ExecuteAI::ExecuteAI(
    int AICODE,
    AIController& aiRef,
    AITrace& traceRef,
    PLCClient& plcRef
)
    : ai(aiRef), trace(traceRef), plc(plcRef), aicode(AICODE)
{
}

// 外部唯一入口
std::string ExecuteAI::runOnce(const std::string& user_input)
{
    trace.begin("execute", aicode, user_input, ai.executeprompt_get());
    std::string json_text = newcallExecuteAI(user_input);
    ExecutePlan plan = parseExecuteJson(json_text);
    std::string output = executePLC(plan);
    trace.end(true, output);
    return output;
}
// 调用执行 AI
std::string ExecuteAI::callExecuteAI(const std::string& user_input)
{
    return ai.chatExecute(aicode, user_input);
}

std::string ExecuteAI::newcallExecuteAI(const std::string& user_input)
{
    return ai.execute(true,false,aicode, "chat_e",user_input);
}

// 解析执行 JSON
ExecuteAI::ExecutePlan
ExecuteAI::parseExecuteJson(const std::string& json_text)
{
    ExecutePlan plan;
    plan.raw_json = json_text;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string err;

    std::istringstream ss(json_text);
    if (!Json::parseFromStream(builder, ss, &root, &err))
    {
        plan.json_parsed = false;
        plan.json_error = u8"JSON 解析失败";
        return plan;
    }

    plan.json_parsed = true;

    // ===== 解析 plc.op =====
    if (root.isMember("plc") && root["plc"].isObject())
    {
        const auto& plcObj = root["plc"];
        std::string plcOp = plcObj.get("op", "none").asString();

        if (plcOp == "connect")
        {
            ActionItem it;
            it.type = ActionItem::Type::Connect;
            it.ip = plcObj.get("ip", "").asString();
            it.rack = plcObj.get("rack", 0).asInt();
            it.slot = plcObj.get("slot", 0).asInt();
            plan.actions.push_back(it);
        }
        else if (plcOp == "disconnect")
        {
            ActionItem it;
            it.type = ActionItem::Type::Disconnect;
            plan.actions.push_back(it);
        }
        // plcOp == "none" → 不产生任何动作
    }

    // ===== 解析 actions =====
    if (root.isMember("actions") && root["actions"].isArray())
    {
        for (const auto& act : root["actions"])
        {
            ActionItem it;
            it.raw_action = act.get("op", "").asString();
            it.address = act.get("address", "").asString();
            it.value = act.get("value", 0).asInt();

            if (it.raw_action == "read")
                it.type = ActionItem::Type::Read;
            else if (it.raw_action == "write")
                it.type = ActionItem::Type::Write;
            else
                it.type = ActionItem::Type::Invalid;

            plan.actions.push_back(it);
        }
    }

    return plan;
}
// 执行 PLC 操作
std::string ExecuteAI::executePLC(const ExecutePlan& plan)
{
    std::ostringstream dbg;
    dbg << "EXEC:BEGIN\n";
    dbg << "PLAN_RAW_BEGIN\n";
    dbg << plan.raw_json << "\n";
    dbg << "PLAN_RAW_END\n";
    dbg << "PLAN_JSON_PARSED=" << (plan.json_parsed ? "1" : "0") << "\n";

    if (!plan.json_parsed)
    {
        dbg << "PLAN_JSON_ERROR=" << plan.json_error << "\n";
        dbg << "EXEC:END\n";
        trace.debug(dbg.str());

        std::string out;
        out += u8"[EXEC_ERROR]\n";
        out += u8"执行指令解析失败。\n";
        out += u8"[/EXEC_ERROR]\n";
        out += u8"[RAW_JSON]\n";
        out += plan.raw_json;
        out += u8"\n[/RAW_JSON]";
        return out;
    }

    bool anyAction = false;
    bool allSuccess = true;

    std::ostringstream execResult;
    for (const auto& it : plan.actions)
    {
        anyAction = true;

        if (it.type == ActionItem::Type::Connect)
        {
            bool ok = plc.connectPLC(it.ip, it.rack, it.slot);
            dbg << "PLC_CONNECT ip=" << it.ip
                << " rack=" << it.rack
                << " slot=" << it.slot
                << " res=" << (ok ? "OK" : "FAIL") << "\n";

            if (ok)
                execResult << u8"PLC 已连接：" << it.ip << u8"\n";
            else
            {
                execResult << u8"PLC 连接失败：" << it.ip << u8"\n";
                allSuccess = false;
            }
        }
        else if (it.type == ActionItem::Type::Disconnect)
        {
            plc.disconnectPLC();
            dbg << "PLC_DISCONNECT res=OK\n";
            execResult << u8"PLC 已断开连接\n";
        }
        else if (it.type == ActionItem::Type::Read)
        {
            int v = 0;
            bool ok = plc.readAddress(it.address, v);
            dbg << "PLC_READ addr=" << it.address
                << " res=" << (ok ? "OK" : "FAIL")
                << " value=" << (ok ? std::to_string(v) : "NA") << "\n";

            if (ok)
                execResult << u8"读取 " << it.address << u8" = " << v << u8"\n";
            else
            {
                execResult << u8"读取失败：" << it.address << u8"\n";
                allSuccess = false;
            }
        }
        else if (it.type == ActionItem::Type::Write)
        {
            bool ok = plc.writeAddress(it.address, it.value);
            dbg << "PLC_WRITE addr=" << it.address
                << " value=" << it.value
                << " res=" << (ok ? "OK" : "FAIL") << "\n";

            if (ok)
                execResult << u8"写入成功：" << it.address << u8" = " << it.value << u8"\n";
            else
            {
                execResult << u8"写入失败：" << it.address << u8"\n";
                allSuccess = false;
            }
        }
        else
        {
            dbg << "PLC_INVALID action=" << it.raw_action << "\n";
            execResult << u8"无效操作：" << it.raw_action << u8"\n";
            allSuccess = false;
        }
    }

    dbg << "EXEC:END\n";
    trace.debug(dbg.str());

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string err;
    std::istringstream ss(plan.raw_json);

    std::string aiMsg;
    if (Json::parseFromStream(builder, ss, &root, &err))
        aiMsg = root.get("message", "").asString();


    std::string out;

    if (!aiMsg.empty())
    {
        out += aiMsg;
        out += u8"\n";
    }

    out += u8"\n";

    if (!anyAction)
    {
        out += u8"未执行任何 PLC 操作。\n";
    }
    else
    {
        out += execResult.str();
    }

    out += u8"执行ai操作：";

    return out;
}
