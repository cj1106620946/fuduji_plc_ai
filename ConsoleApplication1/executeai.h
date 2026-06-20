#pragma once
#include <string>
#include <vector>
#include <json/json.h>

class AIController;
class AITrace;
struct ExecuteItem
{
    std::string message;     // AI 给用户/上位机的中文说明
    std::string op;          // "read" / "write"
    std::string address;     // PLC 地址
    std::string value;       // 写入值，read 时可为空
};
// ExecuteAI：只生成并解析执行 JSON，不执行 PLC
class ExecuteAI
{
public:
    ExecuteAI(
        int aicode,
        AIController& aiRef,
        AITrace& traceRef
    );
    // 返回解析后的动作列表
    std::vector<ExecuteItem> runOnce(const std::string& user_input);
    // 调用 AI，获取原始 JSON
    std::string callExecuteAI(const std::string& user_input);
private:
    // 解析 JSON 为 ExecuteItem 列表
    std::vector<ExecuteItem> parseExecuteJson(const std::string& jsonText);

private:
    int aicode;
    AIController& ai;
    AITrace& trace;
};
