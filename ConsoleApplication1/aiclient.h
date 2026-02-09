#ifndef AICLIENT_H
#define AICLIENT_H

#include <string>
#include <vector>
#include <map>
#include <windows.h>
#include <json/json.h>

// 单条消息结构
struct Message
{
    std::string role;     // "system" / "user" / "assistant"
    std::string content;  // 消息内容
};
// AI 服务提供方枚举
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

// AI 调用描述结构体（仅描述调用方式，不参与执行）
struct AICallDesc
{
    // 是否使用云端
    bool useCloud = true;
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

// AI 调用统一类
class AIClient
{
public:
    AIClient();
    ~AIClient();
    // 设置云端 API Key（仅用于云 API）
    void setAPIKey(const std::string& key);



    // 聊天接口（云端 Chat）
    std::string askChat(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt,
        const std::string& extraSystemText
    );
    // 本地聊天接口（Ollama Chat）
    std::string askChatLocal(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt,
        const std::string& extraSystemText
    );
    // 聊天接口（云端 Chat）
    std::string askChat(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt
    );
    // 本地聊天接口（Ollama Chat）
    std::string askChatLocal(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt
    );
    // 推理 / 判断接口（R 接口，不使用短期记忆）
    std::string askReason(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );

    // 本地推理接口
    std::string askReasonLocal(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );

    // 打印指定记忆槽的聊天历史
    void showHistory(const std::string& memkey);

    // 清空指定记忆槽
    void clearHistory(const std::string& memkey);

    // 获取指定记忆槽的历史文本
    std::string getHistory(const std::string& memkey);

private:
    // 云 API Key
    std::string apiKey;

    // 短期记忆槽：memkey -> 消息列表
    std::map<std::string, std::vector<Message>> memories;

    // 添加一条消息到短期记忆
    void addMessage(
        const std::string& memkey,
        const std::string& role,
        const std::string& content
    );

    // 云端 Chat 底层实现
    std::string callChatAPI(
        bool readHistory,
        bool messagepd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt,
        const std::string& extraSystemText
    );

    // 本地 Chat 底层实现
    std::string callChatLocalAPI(
        bool readHistory,
        bool messagepd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt,
        const std::string& extraSystemText
    );

    // 云端 Reason 底层实现
    std::string callReasonAPI(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );

    // 本地 Reason 底层实现
    std::string callReasonLocalAPI(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );

    // 解析 AI 返回的 JSON 数据
    std::string parseResponse(
        bool judgmentai,
        const std::string& memkey,
        const std::string& jsonResponse
    );
};

#endif // AICLIENT_H
