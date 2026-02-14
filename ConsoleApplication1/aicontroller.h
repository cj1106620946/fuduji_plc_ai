#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "aiclient.h"

class AIClient;

class AIController
{
public:
    explicit AIController(AIClient& aiRef);
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
    AIClient& getClient();

    // 读取 Prompt 接口
    std::string executeprompt_get();
    std::string chatprompt_get();
    std::string workspaceplcprompt_get();
    std::string workspacesigprompt_get();
    std::string decisionprompt_get();
    std::string judgmentprompt_get();
    std::string chatexecuteprompt_get();
    std::string memory13prompt_get();
    std::string memory49prompt_get();

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
    std::string memorywrite13_prompt;
    std::string memorywrite49_prompt;

};
