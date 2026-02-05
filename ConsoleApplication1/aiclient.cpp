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

// 本地聊天接口
std::string AIClient::askChatLocal(
    bool readHistory,
    bool pd,
    const std::string& memkey,
    const std::string& userMessage,
    const std::string& systemPrompt,
    const std::string& extraSystemText
)
{
    return callChatLocalAPI(readHistory,
        pd,memkey, 
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
// 本地聊天接口（无人格）
std::string AIClient::askChatLocal(
    bool readHistory,
    bool pd,
    const std::string& memkey,
    const std::string& userMessage,
    const std::string& systemPrompt
)
{
    return callChatLocalAPI(
        readHistory,
        pd,
        memkey,
        userMessage,
        systemPrompt,
        std::string()   // extraSystemText 为空
    );
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

// 云端 Chat（人格优先）
std::string AIClient::callChatAPI(
    bool readHistory,
    bool messagepd,
    const std::string& memkey,
    const std::string& userMessage,
    const std::string& systemPrompt,      // 规则 / JSON / control
    const std::string& extraSystemText)   // 人格 / 记忆
{
    if (apiKey.empty())
        return u8"api未绑定";

    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl)
        return u8"CURL 初始化失败";

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.deepseek.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    Json::Value root;
    Json::Value messages(Json::arrayValue);
    // [1] 人格 / 记忆 —— 第一优先级 system
// 定义“我是谁”，影响整体身份与语气
    if (!extraSystemText.empty())
    {
        Json::Value persona;
        persona["role"] = "system";
        persona["content"] = extraSystemText;
        messages.append(persona);
    }
    // [2] 行为规则 / JSON 约束 —— 第二优先级 system
    // 只约束输出形式与 control 逻辑
    // 不再定义“我是 AI”
    if (!systemPrompt.empty())
    {
        Json::Value rule;
        rule["role"] = "system";
        rule["content"] = systemPrompt;
        messages.append(rule);
    }

    // [3] 短期记忆（历史对话）
    if (readHistory)
    {
        std::vector<Message>& mem = memories[memkey];
        for (auto& msg : mem)
        {
            Json::Value m;
            m["role"] = msg.role;
            m["content"] = msg.content;
            messages.append(m);
        }
    }

    // [4] 当前用户输入
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

    // 写入短期记忆（user）
    if (messagepd)
    {
        memories[memkey].push_back({ "user", userMessage });
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

