#pragma once

#include <string>

class AIController;
class AITrace;

// 对话 AI
// 职责：
// 1 只负责与用户进行自然语言对话
// 2 不生成 PLC JSON
// 3 不执行 PLC
// 4 不创建 Workspace
// 5 不参与 Decision
// 6 记录完整对话过程到 AITrace
class ChatAI
{
public:
    ChatAI(int aicode,AIController& aiRef, AITrace& traceRef);
    // 对话唯一入口
    // 输入：用户原始文本
    // 输出：AI 的自然语言回复
    std::string runOnce(const std::string& user_input);
private:
    // 调用对话 AI（只走 chatTalk prompt）
    std::string callChatAI(const std::string& user_input);
private:
    int aicode;
    AIController& ai;
    AITrace& trace;
};
