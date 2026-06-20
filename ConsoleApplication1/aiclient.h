#ifndef AICLIENT_H
#define AICLIENT_H

#include <string>
#include <vector>
#include <map>
#include <windows.h>
#include <json/json.h>
struct Message
{
    std::string role;     // "system" / "user" / "assistant"
    std::string content;  // 消息内容
};
enum class AIProvider
{
    // 云端
    OpenAI,        // ChatGPT / OpenAI 官方
    DeepSeek,      // DeepSeek 官方
    Anthropic,     // Claude（预留）
    Google,        // Gemini（预留）
    // 本地
    Ollama         // 本地 Ollama（唯一正式支持）
};
struct AICallDesc
{
    // 是否使用云端
    bool useCloud = 0;
    // AI 服务提供方
    AIProvider provider = AIProvider::DeepSeek;
    //Key
    std::string apiKey;
    // 模型
    std::string modelName;
    // 请求超时时间（秒）
    int timeoutSec = 60;
    // 生成温度（仅 Chat 使用）
    double temperature = 0.7;
    // 最大生成 token 数（仅 Chat 使用）
    int maxTokens = 2048;
};
class AIClient
{
public:
    AIClient(AICallDesc& desc);
    ~AIClient();

    // 聊天接口
    std::string askChat(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt,
        const std::string& extraSystemText
    );
    std::string askChat(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt
    );
    // 打印
    void showHistory(const std::string& memkey);
    // 清空
    void clearHistory(const std::string& memkey);
    // 获取历史文本
    std::string getHistory(const std::string& memkey);
private:
    AICallDesc& callDesc;
    // 短期记忆
    std::map<std::string, std::vector<Message>> memories;
    std::string resolveCallDesc(
        AICallDesc& desc,
        std::string& errorText
    );
    void addMessage(
        const std::string& memkey,
        const std::string& role,
        const std::string& content
    );
    std::string callChatAPI(
        bool readHistory,
        bool messagepd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt,
        const std::string& extraSystemText
    );
    std::string callChatLocalAPI(
        bool readHistory,
        bool messagepd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt,
        const std::string& extraSystemText
    );
    std::string parseResponse(
        bool judgmentai,
        const std::string& memkey,
        const std::string& jsonResponse
    );
};

#endif // AICLIENT_H
