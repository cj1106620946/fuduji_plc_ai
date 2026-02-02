#pragma once

#include <string>
#include <vector>
#include <cstdint>

class AIClient;

// AIController
// 职责：
// 1. 定义 AI 角色（Chat / Workspace / Decision）
// 2. 管理 Prompt
// 3. 选择并调用 AIClient 的具体通道
class AIController
{
public:
    explicit AIController(AIClient& aiRef);

    // AI 调用通道约定
    // 1 : 云端 Chat
    // 2 : 本地 Chat
    // 3 : 云端 Reason
    // 4 : 本地 Reason
    enum
    {
        AI_C_C = 1,
        AI_L_C = 2,
        AI_C_R = 3,
        AI_L_R = 4
    };

    // 总入口
    // 1 rd            : 是否读取短期记忆
    // 2 wt            : 是否写入短期记忆
    // 3 ai_mode       : AI 模式
    // 4 memkey        : 记忆槽
    // 5 text          : 用户输入
    // 6 prompt        : 固定 Prompt（规则）
    // 7 personaText   : 人格 / 阶段设定（system 级，可为空）
    std::string allairun(
        bool rd,
        bool wt,
        int ai_mode,
        const std::string& memkey,
        const std::string& text,
        const std::string& prompt,
        const std::string& personaText
    );
    std::string allairun(
        bool rd,
        bool wt,
        int ai_mode,
        const std::string& memkey,
        const std::string& text,
        const std::string& prompt
    );

    // 读取 Prompt 接口
    std::string executeprompt_get();
    std::string chatprompt_get();
    std::string workspaceplcprompt_get();
    std::string workspacesigprompt_get();

    std::string decisionprompt_get();
    std::string judgmentprompt_get();
    std::string chatexecuteprompt_get();
    std::string memoryprompt_get();

    // Chatexcute AI（执行）
    std::string execute(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text);
    // ChatTalk AI（对话）
    std::string chat(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text);
    // Workspace AI（结构生成）
    std::string workspace(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text);
    // Decision AI（决策）
    std::string decision(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text);
    //judgment AI（判决）
    std::string judgment(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text);


private:
    std::string callAI(
        bool readHistory,
        bool pd,
        int ai_mode,
        const std::string& memkey,
        const std::string& user_text,
        const std::string& prompt,
        const std::string& personaText
    );
    std::string callAI(
        bool readHistory,
        bool pd,
        int ai_mode,
        const std::string& memkey,
        const std::string& user_text,
        const std::string& prompt
    );
private:
    // Prompt 构建
    void buildResponsePrompt();
    void buildExecutePrompt();
    void buildWorkspacePrompt();
    void buildDecisionPrompt();
    void buildJudgmentPrompt();
    void buildMemoryaiPrompt();

private:
    AIClient& ai;

    // Prompt 模板（只存字符串）
    std::string response_prompt;
    std::string execute_prompt;
    std::string workspacesig_prompt;
    std::string workspaceplc_prompt;
    std::string decision_prompt;
    std::string Judgment_prompt;
    std::string chatexecute_prompt;
    std::string memoryjudge_prompt;
    std::string memorywrite_prompt;
    std::string memorymanage_prompt;
};
