#include "chatagent.h"
#include "aicontroller.h"
#include "workspacemanager.h"
#include "plcclient.h"

#include <json/json.h>
#include <sstream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <deque>
#include <string>

//时间戳
static std::string make_timestamp()
{
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
// JSON 字符串安全转义（保留中文，不生成 \\uXXXX）
static std::string json_escape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s)
    {
        switch (c)
        {
        case '\\': out += "\\\\"; break;
        case '\"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out.push_back(c); break;
        }
    }
    return out;
}


static std::string read_recent_chat_jsonl(
    const std::string& path,
    size_t max_lines = 10)
{
    std::ifstream f(path);
    if (!f) return "";

    std::deque<std::string> lines;
    std::string line;

    while (std::getline(f, line))
    {
        if (!line.empty())
        {
            lines.push_back(line);
            if (lines.size() > max_lines)
                lines.pop_front();
        }
    }

    std::string result;
    for (const auto& l : lines)
    {
        result += l;
        result += "\n";
    }

    return result;
}


// ==========================================================
// 构造
// ==========================================================
ChatAgent::ChatAgent(AIController& aiRef,
    WorkspaceManager& ws,
    PLCClient& plcRef)
    : ai(aiRef), workspace(ws), plc(plcRef)
{
}

// ==========================================================
// 外部唯一入口
// ==========================================================
std::string ChatAgent::query_once(const std::string& user_input)
{
    // 1 规划阶段
    std::string plan_text = call_plan_ai(user_input);
    // 2 解析阶段
    PlanResult plan = parse_plan_json(plan_text);
    // 3 执行阶段
    std::string exec_snapshot = execute_plan(plan);
    // 4 解释阶段
    return explain_result(user_input, plan_text, exec_snapshot);
}

// ==========================================================
// 调用规划 AI
// ==========================================================
std::string ChatAgent::call_plan_ai(const std::string& user_input)
{
    std::ostringstream ss;

    // ===== workspace =====
    {
        std::ifstream f("workspace.json");
        if (f)
        {
            ss << "WORKSPACE_JSON:\n";
            ss << f.rdbuf() << "\n";
        }
    }
    // ===== 最近聊天上下文（限制长度）=====
    {
        std::string recent = read_recent_chat_jsonl("ai_explain.jsonl", 20);
        if (!recent.empty())
        {
            ss << "RECENT_CONTEXT:\n";
            ss << recent << "\n";
        }
    }
    // ===== 当前输入 =====
    ss << "USER_INPUT:\n";
    ss << user_input << "\n";

    return ai.askPlan(ss.str());
}


// ==========================================================
// JSON → 执行计划
// 不跳过，不提前返回，所有异常交给最后解释器
// ==========================================================
ChatAgent::PlanResult
ChatAgent::parse_plan_json(const std::string& plan_text)
{
    PlanResult result;
    result.raw_plan_text = plan_text;

    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string err;

    // 尝试按 JSON 解析
    std::istringstream ss(plan_text);
    if (!Json::parseFromStream(builder, ss, &root, &err))
    {
        // 解析失败也不返回，记录下来给解释器
        result.json_parsed = false;
        result.json_error = "JSON 解析失败";
        return result;
    }

    result.json_parsed = true;

    // type=error 也不提前返回，记录给执行快照
    // 仍然尝试解析动作，尽量执行能执行的部分
    // 解释器最终会根据快照自行输出错误

    // plcEnable
    if (root.isMember("plcEnable") && root["plcEnable"].isObject())
    {
        const auto& plcEnable = root["plcEnable"];

        bool en = plcEnable.isMember("enable") ? plcEnable["enable"].asBool() : false;
        if (en)
        {
            PlanItem it;
            it.type = PlanItem::Type::Connect;

            it.ip = plcEnable.isMember("ip") ? plcEnable["ip"].asString() : "";
            it.rack = plcEnable.isMember("rack") ? plcEnable["rack"].asInt() : 0;
            it.slot = plcEnable.isMember("slot") ? plcEnable["slot"].asInt() : 0;

            // address 这里保留为空，执行时用 ip
            result.items.push_back(it);
        }
    }

    // plcActions
    if (root.isMember("plcActions") && root["plcActions"].isArray())
    {
        for (const auto& act : root["plcActions"])
        {
            PlanItem it;

            // enable 默认 true，不存在也不跳过
            bool en = act.isMember("enable") ? act["enable"].asBool() : true;

            // 即使 enable=false 也不跳过，记录下来让解释器决定
            // 这里用 raw_action 标记 enable 情况
            it.raw_action = act.isMember("action") ? act["action"].asString() : "";
            if (!en)
            {
                // 用 Invalid 记录禁用项，执行快照会写出来
                it.type = PlanItem::Type::Invalid;
                it.raw_action = "disabled:" + it.raw_action;
                it.address = act.isMember("address") ? act["address"].asString() : "";
                it.value = act.isMember("value") ? std::atoi(act["value"].asCString()) : 0;
                result.items.push_back(it);
                continue;
            }
            std::string action = act.isMember("action") ? act["action"].asString() : "";
            if (action == "read")
                it.type = PlanItem::Type::Read;
            else if (action == "write")
                it.type = PlanItem::Type::Write;
            else
                it.type = PlanItem::Type::Invalid;

            it.address = act.isMember("address") ? act["address"].asString() : "";
            it.value = act.isMember("value") ? std::atoi(act["value"].asCString()) : 0;

            result.items.push_back(it);
        }
    }

    return result;
}

// ==========================================================
// 真正执行 PLC
// 不提前 return，不跳过，全部写入快照
// ==========================================================
std::string ChatAgent::execute_plan(const PlanResult& plan)
{
    std::ostringstream out;

    out << "EXEC:BEGIN\n";

    // planner 原始文本，便于解释器判断 planner 越界输出
    out << "PLAN_RAW_BEGIN\n";
    out << plan.raw_plan_text << "\n";
    out << "PLAN_RAW_END\n";

    // JSON 解析状态
    out << "PLAN_JSON_PARSED=" << (plan.json_parsed ? "1" : "0") << "\n";
    if (!plan.json_parsed)
    {
        out << "PLAN_JSON_ERROR=" << plan.json_error << "\n";
    }

    // 逐条执行
    for (const auto& it : plan.items)
    {
        if (it.type == PlanItem::Type::Connect)
        {
            bool ok = plc.connectPLC(it.ip, it.rack, it.slot);
            out << "PLC_CONNECT ip=" << it.ip
                << " rack=" << it.rack
                << " slot=" << it.slot
                << " res=" << (ok ? "OK" : "FAIL") << "\n";
        }
        else if (it.type == PlanItem::Type::Disconnect)
        {
            plc.disconnectPLC();
            out << "PLC_DISCONNECT res=OK\n";
        }
        else if (it.type == PlanItem::Type::Read)
        {
            int v = 0;
            bool ok = plc.readAddress(it.address, v);
            out << "PLC_READ addr=" << it.address
                << " res=" << (ok ? "OK" : "FAIL")
                << " value=" << (ok ? std::to_string(v) : "NA") << "\n";
        }
        else if (it.type == PlanItem::Type::Write)
        {
            bool ok = plc.writeAddress(it.address, it.value);
            out << "PLC_WRITE addr=" << it.address
                << " value=" << it.value
                << " res=" << (ok ? "OK" : "FAIL") << "\n";
        }
        else
        {
            out << "PLC_INVALID action=" << it.raw_action
                << " addr=" << it.address
                << " value=" << it.value
                << " res=SKIP\n";
        }
    }

    out << "EXEC:END\n";
    return out.str();
}

std::string ChatAgent::explain_result(const std::string& user_input,
    const std::string& plan_text,
    const std::string& exec_snapshot)
{
    std::ostringstream msg;
    msg << "user_input:\n" << user_input << "\n";
    msg << "plan_text:\n" << plan_text << "\n";
    msg << "exec_snapshot:\n" << exec_snapshot << "\n";
    std::string ai_reply = ai.askExplain(msg.str());
    std::string ts = make_timestamp();
    // ==========================================================
  // 保存 AI 规划（覆盖格式）
  // ==========================================================
    {
        std::ofstream f("ai_plan.json", std::ios::binary | std::ios::trunc);
        if (f)
        {
            auto clean = [](std::string& s) {
                s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
            };

            std::string p = plan_text;
            clean(p);

            std::string pe = json_escape(p);

            f << "{\n";
            f << "  \"time\": \"" << ts << "\",\n";
            f << "  \"plan_json\": \"" << pe << "\"\n";
            f << "}\n";
        }
    }
    // ==========================================================
    // 保存 AI 对话（追加 JSONL 格式）
    // ==========================================================
    {
        std::ofstream f("ai_explain.jsonl", std::ios::binary | std::ios::app);
        if (f)
        {
            auto clean = [](std::string& s) {
                s.erase(std::remove(s.begin(), s.end(), '\0'), s.end());
            };

            std::string u = user_input;
            std::string a = ai_reply;
            clean(u);
            clean(a);

            std::string ue = json_escape(u);
            std::string ae = json_escape(a);

            f << "{"
                << "\"time\":\"" <<"\n" << ts << "\","
                << "\"user_input\":\"" << "\n" << ue << "\","
                << "\"ai_reply\":\"" << "\n" << ae << "\""
                << "}\n";
        }
    }
    return ai_reply;
}

