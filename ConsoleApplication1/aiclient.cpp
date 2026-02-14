#include "aiclient.h"
#include <sstream>
#include <curl/curl.h>
#include <fstream>
// cURL 写入回调
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}
AIClient::AIClient(AICallDesc& desc)
    : callDesc(desc)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}
// 析构函数
AIClient::~AIClient()
{
    curl_global_cleanup();
}
std::string AIClient::resolveCallDesc(
    AICallDesc& desc,
    std::string& errorText
)
{
    errorText.clear();
    // 参数兜底修正
    if (desc.timeoutSec <= 0)
        desc.timeoutSec = 60;
    if (desc.maxTokens <= 0)
        desc.maxTokens = 2048;
    // 云端模式
    if (desc.useCloud)
    {
        if (desc.apiKey.empty())
        {
            errorText = u8"云端调用未设置 apiKey";
            return std::string();
        }

        switch (desc.provider)
        {
        case AIProvider::DeepSeek:
            if (desc.modelName.empty())
                desc.modelName = "deepseek-chat";
            return "https://api.deepseek.com/v1/chat/completions";

        case AIProvider::OpenAI:
            if (desc.modelName.empty())
                desc.modelName = "gpt-4o-mini";
            return "https://api.openai.com/v1/chat/completions";

        case AIProvider::Anthropic:
            if (desc.modelName.empty())
                desc.modelName = "claude-3-sonnet";
            return "https://api.anthropic.com/v1/messages";

        case AIProvider::Google:
            if (desc.modelName.empty())
                desc.modelName = "gemini-pro";
            return "https://generativelanguage.googleapis.com/v1/models";

        default:
            errorText = u8"未知云端 AI 服务提供方";
            return std::string();
        }
    }

    // 本地模式
    if (desc.provider != AIProvider::Ollama)
    {
        errorText = u8"本地模式下仅支持 Ollama";
        return std::string();
    }

    if (desc.modelName.empty())
        desc.modelName = "qwen2.5:7b-instruct-q4_K_M";
    return "http://127.0.0.1:11434/api/chat";
}

// 添加一条消息到指定记忆槽
void AIClient::addMessage(
    const std::string& memkey,
    const std::string& role,
    const std::string& content)
{
    auto& mem = memories[memkey];
    mem.push_back({ role, content });

    // 控制单个记忆槽的长度
    if (mem.size() > 40)
        mem.erase(mem.begin());
}
// 打印指定记忆槽的聊天历史
void AIClient::showHistory(const std::string& memkey)
{
    auto it = memories.find(memkey);
    if (it == memories.end())
        return;

    // 文件名：memkey.txt
    std::string filename = memkey + ".txt";

    std::ofstream out(filename.c_str(), std::ios::out | std::ios::trunc);
    if (!out.is_open())
        return;

    for (const auto& msg : it->second)
    {
        // 每条消息固定格式写入，便于后续解析或人工查看
        out << "[" << msg.role << "]\n";
        out << msg.content << "\n";
        out << "\n";
    }

    out.close();
}
//读取记忆
std::string AIClient::getHistory(const std::string& memkey)
{
    auto it = memories.find(memkey);
    if (it == memories.end())
        return "";  // 如果没有找到对应的记忆槽位，返回空字符串

    // 构建返回的字符串
    std::string history;

    for (const auto& msg : it->second)
    {
        // 每条消息按照指定格式拼接
        history += "[" + msg.role + "]\n";
        history += msg.content + "\n";
        history += "\n";  // 保证每条消息后面有一个换行符
    }

    return history;  // 返回拼接的历史内容
}
// 清空指定记忆槽
void AIClient::clearHistory(const std::string& memkey)
{
    memories[memkey].clear();
}

// 聊天接口
std::string AIClient::askChat(
    bool readHistory,
    bool pd,
    const std::string& memkey,
    const std::string& userMessage,
    const std::string& systemPrompt,
    const std::string& extraSystemText
)
{
    return callChatAPI(
        readHistory,
        pd,
        memkey,
        userMessage,
        systemPrompt,
        extraSystemText
    );
}
// 聊天接口（无人格）
std::string AIClient::askChat(
    bool readHistory,
    bool pd,
    const std::string& memkey,
    const std::string& userMessage,
    const std::string& systemPrompt
)
{
    return callChatAPI(
        readHistory,
        pd,
        memkey,
        userMessage,
        systemPrompt,
        std::string()   // extraSystemText 为空
    );
}


std::string AIClient::callChatAPI(
    bool readHistory,
    bool messagepd,
    const std::string& memkey,
    const std::string& userMessage,
    const std::string& systemPrompt,
    const std::string& extraSystemText
)
{
    std::string errorText;
    std::string requestUrl = resolveCallDesc(callDesc, errorText);
    if (requestUrl.empty())
        return errorText;

    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl)
        return u8"CURL 初始化失败";

    curl_easy_setopt(curl, CURLOPT_URL, requestUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, callDesc.timeoutSec);

    Json::Value root;
    Json::Value messages(Json::arrayValue);

    // [1] 人格 / 记忆
    if (!extraSystemText.empty())
    {
        Json::Value persona;
        persona["role"] = "system";
        persona["content"] = extraSystemText;
        messages.append(persona);
    }

    // [2] 行为规则
    if (!systemPrompt.empty())
    {
        Json::Value rule;
        rule["role"] = "system";
        rule["content"] = systemPrompt;
        messages.append(rule);
    }

    // [3] 短期记忆
    if (readHistory)
    {
        auto& mem = memories[memkey];
        for (auto& msg : mem)
        {
            Json::Value m;
            m["role"] = msg.role;
            m["content"] = msg.content;
            messages.append(m);
        }
    }

    // [4] 当前输入
    {
        Json::Value um;
        um["role"] = "user";
        um["content"] = userMessage;
        messages.append(um);
    }

    root["model"] = callDesc.modelName;
    root["messages"] = messages;
    root["stream"] = false;

    // Chat 参数仅在支持的情况下写入
    root["max_tokens"] = callDesc.maxTokens;
    root["temperature"] = callDesc.temperature;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string requestData = Json::writeString(builder, root);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    // 云端才需要 Authorization
    if (callDesc.useCloud)
    {
        std::string auth = "Authorization: Bearer " + callDesc.apiKey;
        headers = curl_slist_append(headers, auth.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        return u8"Request error: " + std::string(curl_easy_strerror(res));

    if (messagepd)
    {
        addMessage(memkey, "user", userMessage);
    }

    return parseResponse(messagepd, memkey, response);
}

// 本地 Chat（Ollama，人格优先）
std::string AIClient::callChatLocalAPI(
    bool readHistory,
    bool messagepd,
    const std::string& memkey,
    const std::string& userMessage,
    const std::string& systemPrompt,      // 规则
    const std::string& extraSystemText)   // 人格 / 记忆
{
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl)
        return u8"CURL 初始化失败";

    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:11434/api/chat");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    Json::Value root;
    Json::Value messages(Json::arrayValue);

    if (!extraSystemText.empty())
    {
        Json::Value persona;
        persona["role"] = "system";
        persona["content"] = extraSystemText;
        messages.append(persona);
    }

    if (!systemPrompt.empty())
    {
        Json::Value rule;
        rule["role"] = "system";
        rule["content"] = systemPrompt;
        messages.append(rule);
    }

    if (readHistory)
    {
        auto& mem = memories[memkey];
        for (auto& msg : mem)
        {
            Json::Value m;
            m["role"] = msg.role;
            m["content"] = msg.content;
            messages.append(m);
        }
    }
    {
        Json::Value um;
        um["role"] = "user";
        um["content"] = userMessage;
        messages.append(um);
    }

    root["model"] = "qwen2.5:7b-instruct-q4_K_M";
    root["messages"] = messages;
    root["stream"] = false;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string requestData = Json::writeString(builder, root);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        return u8"Local request error: " + std::string(curl_easy_strerror(res));

    // 写入短期记忆
    if (messagepd)
    {
        addMessage(memkey, "user", userMessage);
    }

    return parseResponse(messagepd, memkey, response);
}


// Chat 响应解析
std::string AIClient::parseResponse(
    bool messagepd,
    const std::string& memkey,
    const std::string& jsonResponse)
{
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::stringstream ss(jsonResponse);

    if (!Json::parseFromStream(reader, ss, &root, &errors))
        return u8"JSON 解析失败";

    if (root.isMember("error"))
        return u8"API 错误";

    std::string reply;
    if (root.isMember("message") && root["message"].isMember("content"))
    {
        reply = root["message"]["content"].asString();
    }
    else if (root.isMember("choices") &&
        !root["choices"].empty() &&
        root["choices"][0].isMember("message") &&
        root["choices"][0]["message"].isMember("content"))
    {
        reply = root["choices"][0]["message"]["content"].asString();
    }
    else
    {
        return u8"无法识别的AI响应格式";
    }

    // 写回同一个记忆槽
    if (messagepd)
    {
        addMessage(memkey, "assistant", reply);
    }

    return reply;
}

