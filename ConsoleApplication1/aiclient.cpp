#include "aiclient.h"
#include <sstream>
#include <curl/curl.h>

// cURL 写入回调
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response)
{
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);
    return totalSize;
}

// 构造函数
AIClient::AIClient()
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

// 析构函数
AIClient::~AIClient()
{
    curl_global_cleanup();
}

// 设置云端 API Key
void AIClient::setAPIKey(const std::string& key)
{
    apiKey = key;
}

// 添加一条消息到聊天历史
void AIClient::addMessage(const std::string& role, const std::string& content)
{
    history.push_back({ role, content });
    if (history.size() > 40)
        history.erase(history.begin());
}

// 打印当前聊天历史
void AIClient::showHistory()
{
    int i = 0;
    for (auto& msg : history)
    {
        (void)i;
    }
}

// 清空聊天历史
void AIClient::clearHistory()
{
    history.clear();
}

// 聊天接口
std::string AIClient::askChat(bool readHistory,bool pd,const std::string& userMessage,
    const std::string& systemPrompt)
{
    return callChatAPI(readHistory,pd,userMessage, systemPrompt);
}

// 本地聊天接口
std::string AIClient::askChatLocal(bool readHistory,bool pd,const std::string& userMessage,
    const std::string& systemPrompt)
{
    return callChatLocalAPI(readHistory,pd,userMessage, systemPrompt);
}

// 推理接口
std::string AIClient::askReason(const std::string& userMessage,
    const std::string& systemPrompt)
{
    return callReasonAPI(userMessage, systemPrompt);
}

// 本地推理接口
std::string AIClient::askReasonLocal(const std::string& userMessage,
    const std::string& systemPrompt)
{
    return callReasonLocalAPI(userMessage, systemPrompt);
}

// 云端 Chat
std::string AIClient::callChatAPI(bool readHistory,bool messagepd,const std::string& userMessage, const std::string& systemPrompt)
{
    if (apiKey.empty())
        return u8"api未绑定";

    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl)
        return u8"CURL 初始化失败";

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.deepseek.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    Json::Value root;
    Json::Value messages(Json::arrayValue);
    if (!systemPrompt.empty())
    {
        Json::Value sm;
        sm["role"] = "system";
        sm["content"] = systemPrompt;
        messages.append(sm);
    }
    if (readHistory)
    {
        for (auto& msg : history)
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

    root["model"] = "deepseek-chat";
    root["messages"] = messages;
    root["stream"] = false;
    root["max_tokens"] = 2048;
    root["temperature"] = 0.7;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string requestData = Json::writeString(builder, root);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    std::string auth = "Authorization: Bearer " + apiKey;
    headers = curl_slist_append(headers, auth.c_str());

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
        addMessage("user", userMessage);
    }
    return parseResponse(messagepd,response);
}

// 本地 Chat（Ollama）
std::string AIClient::callChatLocalAPI(bool readHistory,bool messagepd, const std::string& userMessage, const std::string& systemPrompt)
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

    if (!systemPrompt.empty())
    {
        Json::Value sm;
        sm["role"] = "system";
        sm["content"] = systemPrompt;
        messages.append(sm);
    }

    if (readHistory)
    {
        for (auto& msg : history)
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
    if (messagepd)
    {
        addMessage("user", userMessage);
    }
    return parseResponse(messagepd,response);
}

// 云端 Reason
std::string AIClient::callReasonAPI(const std::string&, const std::string& systemPrompt)
{
    if (apiKey.empty())
        return u8"api未绑定";

    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl)
        return u8"CURL 初始化失败";

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.deepseek.com/v1/completions");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    Json::Value root;
    root["model"] = "deepseek-R1";
    root["prompt"] = systemPrompt;
    root["max_tokens"] = 1024;
    root["temperature"] = 0.0;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    std::string requestData = Json::writeString(builder, root);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestData.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    std::string auth = "Authorization: Bearer " + apiKey;
    headers = curl_slist_append(headers, auth.c_str());

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        return u8"Request error: " + std::string(curl_easy_strerror(res));

    Json::Value rootResp;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::stringstream ss(response);

    if (!Json::parseFromStream(reader, ss, &rootResp, &errors))
        return u8"JSON 解析失败";

    if (!rootResp.isMember("choices") || rootResp["choices"].empty())
        return u8"v1 返回为空";

    return rootResp["choices"][0]["text"].asString();
}

// 本地 Reason
std::string AIClient::callReasonLocalAPI(
    const std::string& userInput,
    const std::string& systemPrompt)
{
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl)
        return u8"CURL 初始化失败";

    curl_easy_setopt(curl, CURLOPT_URL, "http://127.0.0.1:11434/api/generate");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    Json::Value root;

    // 使用你本地真实存在的 R 模型
    root["model"] = "deepseek-r1:7b";

    // 本地 Ollama 不区分 system / user
    // 所以 systemPrompt 本身就必须是完整推理提示
    root["prompt"] = systemPrompt;

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

    Json::Value rootResp;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::stringstream ss(response);

    if (!Json::parseFromStream(reader, ss, &rootResp, &errors))
        return u8"JSON 解析失败";

    if (!rootResp.isMember("response"))
        return u8"Local reason 返回无 response 字段";

    return rootResp["response"].asString();
}

// Chat 响应解析
std::string AIClient::parseResponse(bool messagepd,const std::string& jsonResponse)
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
        reply = root["message"]["content"].asString();
    else if (root.isMember("choices") && !root["choices"].empty() &&
        root["choices"][0].isMember("message") &&
        root["choices"][0]["message"].isMember("content"))
        reply = root["choices"][0]["message"]["content"].asString();
    else
        return u8"无法识别的AI响应格式";
    if (messagepd)
    {
        addMessage("assistant", reply);
    }
    return reply;
}
