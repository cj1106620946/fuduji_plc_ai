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
