#pragma once

#include <string>
#include <vector>
#include <json/json.h>

class AIController;
class WorkspaceManager;
class PLCClient;

// ==========================================================
// ChatAgent
// ==========================================================
class ChatAgent
{
public:
    ChatAgent(
        AIController& aiRef,
        WorkspaceManager& ws,
        PLCClient& plcRef
    );

    // 外部唯一入口
    std::string query_once(const std::string& user_input);

private:
    // ======================================================
    // 计划项
    // ======================================================
    struct PlanItem
    {
        enum class Type
        {
            Connect,
            Disconnect,
            Read,
            Write,
            Invalid
        };
        Type type = Type::Invalid;
        // PLC 参数
        std::string address;
        int value = 0;

        // 连接参数
        std::string ip;
        int rack = 0;
        int slot = 0;

        // 原始动作
        std::string raw_action;
    };

    // ======================================================
    // 解析结果
    // ======================================================
    struct PlanResult
    {
        bool json_parsed = false;
        std::string json_error;
        std::string raw_plan_text;
        std::vector<PlanItem> items;
    };

private:
    // ======================================================
    // 内部逻辑
    // ======================================================
    std::string call_plan_ai(const std::string& user_input);
    PlanResult parse_plan_json(const std::string& plan_text);
    std::string execute_plan(const PlanResult& plan);
    std::string explain_result(
        const std::string& user_input,
        const std::string& plan_text,
        const std::string& exec_snapshot
    );

private:
    // ======================================================
    // 依赖对象
    // ======================================================
    AIController& ai;
    WorkspaceManager& workspace;
    PLCClient& plc;
};
