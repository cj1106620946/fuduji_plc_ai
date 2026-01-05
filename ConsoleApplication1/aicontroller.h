#pragma once
#include <string>
#include <vector>
#include <cstdint>

class PLCClient;
class DeepSeekAI;
class AIController
{
public:
    explicit AIController(PLCClient& plc, DeepSeekAI& ai);
    
    // 一、Chat AI（人机交互）
    
    struct ChatResult
    {
        bool ok = false;
        std::string text;            // 给用户显示的自然语言
        std::string action;          // READ / WRITE / NONE
        std::string target;          // 变量名 or 地址
        int value = 0;               // 写入值
    };
    struct WorkspaceVar
    {
        std::string name;
        std::string dir;
        std::string type;
        std::string default_value;
        std::string desc;
        std::string addr;    
    };

    struct WorkspaceDraft
    {
        bool ok = false;
        std::string err;

        std::string name;
        std::string desc;

        int db_number = -1;
        std::string db_name;

        int fb_number = -1;
        std::string fb_name;

        std::vector<WorkspaceVar> vars;

        std::string raw_json;     // AI 输出
        std::string final_json;   // 处理后
    };

    // 创建工作区
    WorkspaceDraft createWorkspace(const std::string& user_requirement);
    
    // 三、Decision AI（运行时决策）
    
    struct DecisionInput
    {
        std::string workspace_name;
        std::string snapshot_json;     // 当前 PLC/变量快照
        std::string decision_prompt;   // 来自 workspace
    };

    struct DecisionResult
    {
        bool ok = false;
        std::vector<std::string> actions;  // WRITE xxx
        std::string reason;
    };
    DecisionResult runDecision(const DecisionInput& input);
    // 阶段1：指令规划（P:xxx）
    std::string askPlan(const std::string& user_input);
    // 阶段2：结果解释（Q:TEXT）
    std::string askExplain(const std::string& explain_input);
    std::string callWorkspaceAI(const std::string& text);
    std::string callDecisionAI(const std::string& text);

private:
    
    // 内部：Prompt 构建
    
    void buildChatPrompt();
    void buildWorkspacePrompt();
    void buildDecisionPrompt();
private:
    PLCClient& plc;
    DeepSeekAI& ai;

    // prompt 模板（一次构建）
    std::string chat_prompt;
    std::string response_prompt;
    std::string workspace_prompt;
    std::string decision_prompt;
};
