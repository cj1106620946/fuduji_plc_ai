#pragma once
#include <string>

class AIController;
class AITrace;
// WorkspaceAI
// 职责：
// - 管理 Workspace 生命周期
// - 调用 Workspace AI 生成结构 JSON
// - 保存 / 加载 workspace
// 不解析 PLC、不做执行、不做决策
class WorkspaceAI
{
public:
    explicit WorkspaceAI(int aicode, AIController& aiRef, AITrace& traceRef);
    // 生命周期
    void reset();
    bool hasWorkspace() const;
    // 构建 Workspace入口
    bool runOnce(const std::string& userInput);
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

    bool callWorkspaceAI(const std::string& userInput);

    // JSON 提取
    bool extractWorkspaceJson(const std::string& aiText);
    static bool extractFirstJsonObject(
        const std::string& s,
        size_t startPos,
        std::string& outJson
    );

private:
    int aicode;
    AIController& ai;
    AITrace& trace;
    std::string user_input;
    std::string ai_output;
    std::string workspace_json;
    std::string error_message;

    bool workspace_ready = false;
};
