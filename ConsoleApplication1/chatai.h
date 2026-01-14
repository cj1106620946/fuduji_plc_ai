#pragma once

#include <string>

class AIController;
class AITrace;
class ChatAI
{
private:
    struct ChatResult
    {
        std::string ainame;
        std::string text;
        int control = 0;
        std::string emotion;
        int priority = 0;
    };

public:
    ChatAI(int aicode,AIController& aiRef, AITrace& traceRef);
    // 对话唯一入口
    std::string runOnce(const std::string& user_input);


    std::string getAiName();
    std::string getText();
    int getControl();
    int getPriority();
    std::string getEmotion();
    std::string runExecuteRead(const std::string& user_input);
    std::string runExecuteOnce(const std::string& user_input);
    bool parseChatJson(const std::string& jsonText);
private:

    // 调用对话 AI（只走 chatTalk prompt）
    std::string callChatAI(const std::string& user_input);
    std::string callChatExecuteAI(const std::string& user_input);
private:
    struct ChatResult r;
    int aicode;
    AIController& ai;
    AITrace& trace;
};
