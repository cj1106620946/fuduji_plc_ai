// executeai.h
#pragma once
#include <string>
#include <vector>
#include <json/json.h>

class AIController;
class PLCClient;
class AITrace;
// 执行 AI：只负责 PLC 指令生成与执行
class ExecuteAI
{
public:
    ExecuteAI(int aicode, AIController& aiRef, AITrace& traceRef,PLCClient& plcRef);
    std::string runOnce(const std::string& user_input);
    std::string newcallExecuteAI(const std::string& user_input);

private:
    struct ActionItem
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
        std::string address;
        double value = 0.0;
        std::string ip;
        int rack = 0;
        int slot = 0;
        std::string raw_action;
    };

    struct ExecutePlan
    {
        bool json_parsed = false;
        std::string json_error;
        std::string raw_json;
        std::vector<ActionItem> actions;
    };
private:
    std::string callExecuteAI(const std::string& user_input);
    ExecutePlan parseExecuteJson(const std::string& json_text);
    std::string executePLC(const ExecutePlan& plan);
private:
    int aicode;
    AIController& ai;
    AITrace& trace;
    PLCClient& plc;
};
