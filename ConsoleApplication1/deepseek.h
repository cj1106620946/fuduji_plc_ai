#ifndef DEEPSEEK_H
#define DEEPSEEK_H

#include <string>
#include <vector>
#include <windows.h>
#include <json/json.h>

// 消息结构体
struct Message {
    std::string role;     // "user" 或 "assistant"
    std::string content;  // 内容
};

class DeepSeekAI
{
public:
    DeepSeekAI();
    ~DeepSeekAI();
    void setAPIKey(const std::string& key);
    std::string ask(const std::string& userMessage);
    std::string ask(const std::string& userMessage,const std::string& systemPrompt);
    void showHistory();
    void clearHistory();
private:
    std::string apiKey;
    std::vector<Message> history;
    bool systemPromptUsed = false;
    std::string lastPrompt = "";  

    void addMessage(const std::string& role, const std::string& content);

    // 支持 systemPrompt
    std::string callAPI(const std::string& userMessage,
        const std::string& systemPrompt);

    // JSON 解析
    std::string parseResponse(const std::string& jsonResponse);
};

#endif // DEEPSEEK_H
