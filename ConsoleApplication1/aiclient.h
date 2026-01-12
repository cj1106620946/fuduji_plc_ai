#ifndef AICLIENT_H
#define AICLIENT_H
#include <string>
#include <vector>
#include <windows.h>
#include <json/json.h>
struct Message
{
    std::string role;     // 消息角色："system"、"user"、"assistant"
    std::string content;  // 消息内容
};
// AI 调用统一类
class AIClient
{
public:
    AIClient();
    ~AIClient();
    // 设置云端 API Key
    // 仅用于云 API 调用
    void setAPIKey(const std::string& key);
    // 聊天接口（Chat 专用）
    std::string askChat(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt
    );
    //本地聊天接口
    std::string askChatLocal(
        bool readHistory,
        bool pd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt
    );
    // 推理 / 判断接口（R 接口）
    std::string askReason(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );
    // 本地推理接口
    std::string askReasonLocal(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );
    // 打印当前聊天历史
    void showHistory(const std::string& memkey);
    // 清空聊天历史
    void clearHistory(const std::string& memkey);

private:
    // 云 API Key
    std::string apiKey;
   // 新增 多记忆容器
    std::map<std::string, std::vector<Message>> memories;
    // 添加一条消息到历史记录
    void addMessage(
        const std::string& memkey,
        const std::string& role,
        const std::string& content
    );
    // 聊天接口的底层实现
    std::string callChatAPI(
        bool readHistory,
        bool messagepd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt
    );
    std::string callChatLocalAPI(
        bool readHistory,
        bool messagepd,
        const std::string& memkey,
        const std::string& userMessage,
        const std::string& systemPrompt
    );
    // 推理接口的底层实现
    std::string callReasonAPI(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );
    // 本地推理接口底层实现
    std::string callReasonLocalAPI(
        const std::string& taskPrompt,
        const std::string& systemPrompt
    );
    // 解析 AI 返回的 JSON 数据
    // 提取最终文本内容
    std::string parseResponse(
        bool judgmentai,
        const std::string& memkey,
        const std::string& jsonResponse
    );
};
#endif // AICLIENT_H
