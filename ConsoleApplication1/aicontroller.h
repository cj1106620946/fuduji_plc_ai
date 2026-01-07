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
    // 4 : 本地 Reaso
    enum
    {
        AI_C_C = 1,
        AI_L_C = 2,
        AI_C_R = 3,
        AI_L_R = 4
    };
	// Chatexcute AI（执行）
    std::string chatExecute(int ai_mode, const std::string& text);
	// ChatTalk AI（对话）
    std::string chatTalk(int ai_mode, const std::string& text);
    // Workspace AI（结构生成）
    std::string workspace(int ai_mode,const std::string& requirement_text);
    // Decision AI（决策）
    std::string decision(int ai_mode,const std::string& decision_input);
	//judgment AI（判决）
    std::string judgment(int ai_mode,const std::string& decision_input);
    //读取prompt 
    std::string chatExecuteprompt_get();
    std::string chatTalkprompt_get();
    std::string workspaceprompt_get();
    std::string decisionprompt_get();
    std::string judgmentprompt_get();
private:
    // 统一 AI 调用入口
    // 只负责路由，不做任何业务处理
    std::string callAI(
        bool readHistory,
        bool pd,
        int ai_mode,
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
private:
    AIClient& ai;
    // Prompt 模板（只存字符串）
    std::string response_prompt;
    std::string execute_prompt;
    std::string workspace_prompt;
    std::string decision_prompt;
    std::string Judgment_prompt;
};
