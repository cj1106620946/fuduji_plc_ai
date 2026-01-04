#include "deepseek.h"
#include <iostream>
#include <sstream>
#include <curl/curl.h>

// -------- cURL 写入回调（保持原始 UTF-8） --------
static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t totalSize = size * nmemb;
    response->append((char*)contents, totalSize);  // 保留 UTF-8 原样
    return totalSize;
}

DeepSeekAI::DeepSeekAI() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

DeepSeekAI::~DeepSeekAI() {
    curl_global_cleanup();
}

void DeepSeekAI::setAPIKey(const std::string& key) {
    apiKey = key;
}

void DeepSeekAI::addMessage(const std::string& role, const std::string& content) {
    history.push_back({ role, content });
    if (history.size() > 40) history.erase(history.begin());
}

void DeepSeekAI::showHistory() {
    std::cout << "\n=== 对话历史 ===\n";
    int i = 0;
    for (auto& msg : history) {
        std::cout << msg.role << "(" << i++ << "): " << msg.content << "\n";
    }
    std::cout << "================\n";
}

void DeepSeekAI::clearHistory() {
    history.clear();
}

std::string DeepSeekAI::ask(const std::string& userMessage) {
    // 使用UTF-8编码的中文提示词
    std::string prompt =
        /*u8"你是一个专业的PLC控制助手。"
        u8"重要：你必须专注于PLC控制、Snap7、工控逻辑、数据分析、错误排查等技术问题。"
        u8"如果用户询问与PLC无关的内容，你必须礼貌但坚定地引导回工控主题。"
        u8"禁止介绍自己的身份或能力，直接提供技术帮助。"
        u8"用户：你是谁？ 回复：你是 fuduji - PLC 上位机助手。"
        u8"用户：讲个笑话 回复：我专注于PLC技术问题，请咨询相关工控内容。";
        */
        "";
    return callAPI(userMessage, prompt);
}
std::string DeepSeekAI::ask(const std::string& userMessage, const std::string& systemPrompt) {
    return callAPI(userMessage, systemPrompt);
}

std::string DeepSeekAI::callAPI(const std::string& userMessage, const std::string& systemPrompt) {
    if (apiKey.empty())
        return "api未绑定";

    std::string response;

    CURL* curl = curl_easy_init();
    if (!curl)
        return "CURL 初始化失败.";

    curl_easy_setopt(curl, CURLOPT_URL, "https://api.deepseek.com/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    // -------- 构造 JSON 请求 --------
    Json::Value root;
    Json::Value messages(Json::arrayValue);

    if (!systemPrompt.empty()) {
        Json::Value sm;
        sm["role"] = "system";
        sm["content"] = systemPrompt;
        messages.append(sm);
    }

    for (auto& msg : history) {
        Json::Value m;
        m["role"] = msg.role;
        m["content"] = msg.content;
        messages.append(m);
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

    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);

    if (res != CURLE_OK)
        return std::string("Request error: ") + curl_easy_strerror(res);

    addMessage("user", userMessage);
    return parseResponse(response);
}

// -------- 保证 UTF-8 输出，不做任何转换 --------
std::string DeepSeekAI::parseResponse(const std::string& jsonResponse) {
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::stringstream ss(jsonResponse);

    if (!Json::parseFromStream(reader, ss, &root, &errors)) {
        return "Error: JSON parse failed\n原始数据：" + jsonResponse;
    }

    if (root.isMember("error"))
        return "API Error: " + root["error"]["message"].asString();

    std::string reply = root["choices"][0]["message"]["content"].asString();

    addMessage("assistant", reply);
    return reply;
}
