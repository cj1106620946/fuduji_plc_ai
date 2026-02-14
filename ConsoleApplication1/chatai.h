#pragma once

#include <string>
class AIController;
class AITrace;

class ChatAI
{
private:
    struct ChatResult
    {
        std::string ainame;   // AI 名称
        std::string text;     // 回复文本
        int control = 0;      // 控制标志
        std::string emotion;  // 情绪状态
        int priority = 0;     // 优先级
    };

public:
    // 构造
    ChatAI(int aicode, AIController& aiRef, AITrace& traceRef);

    std::string runOnce(const std::string& user_input);
    std::string runOnce(
        const std::string& user_input,
        const std::string& personaText
    );
    std::string getShortHistory();
    void clearShortHistory();

    std::string getAiName();
    std::string getText();
    int getControl();
    int getPriority();
    std::string getEmotion();
private:
    // JSON 解析
    bool parseChatJson(const std::string& jsonText);

private:
    ChatResult r;
    int aicode;
    AIController& ai;
    AITrace& trace;
};
