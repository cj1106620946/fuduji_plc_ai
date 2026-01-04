#pragma once
#include <string>

class AIController;

class WorkspaceManager
{
public:
    explicit WorkspaceManager(AIController& aiCtrl);

    // 生命周期
    void reset();
    bool hasWorkspace() const;

    // 构建 Workspace
    bool buildFromUserInput(const std::string& userInput);

    // 状态判断
    bool isReadyForDecision() const;
    bool isReadyForPLC() const;

    // 读取接口
    const std::string& getUserInput() const;
    const std::string& getAiRawOutput() const;
    const std::string& getWorkspaceJson() const;
    const std::string& getErrorMessage() const;

    // 文件
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

private:
    // AI 调用
    bool callWorkspaceAI(const std::string& userInput);

    // JSON 提取
    bool extractWorkspaceJson(const std::string& aiText);
    static bool extractFirstJsonObject(
        const std::string& s,
        size_t startPos,
        std::string& outJson
    );

private:
    AIController& ai;

    std::string user_input;
    std::string ai_output;
    std::string workspace_json;
    std::string error_message;

    bool workspace_ready = false;
};
