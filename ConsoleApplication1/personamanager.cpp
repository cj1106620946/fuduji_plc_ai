#include "personamanager.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <cstdio>
personamanager::personamanager(
    ChatAI& chatRef,
    MemoryAI& memoryAiRef,
    SqlStore& storeRef,
    CurrentMemoryState& memoryStateRef,
    PersonaState& stateRef
)
    : chat(chatRef),
    memoryAi(memoryAiRef),
    store(storeRef),
    memoryState(memoryStateRef),
    state(stateRef),
    bridge(nullptr)
{
    bridge = new memorybridge(store, memoryState);
    if (!bridge->init())
    {
        logError("personamanager.ctor", u8"记忆桥接初始化失败");
    }
}
personamanager::~personamanager()
{
    if (bridge)
    {
        delete bridge;
        bridge = nullptr;
    }
}

void personamanager::logError(
    const std::string& fromFunc,
    const std::string& reason
)
{
    std::ofstream logFile("error//personamanager.log", std::ios::app);
    if (!logFile.is_open())
        return;

    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm local_tm;
#if defined(_MSC_VER)
    localtime_s(&local_tm, &time_t_now);
#else
    local_tm = *std::localtime(&time_t_now);
#endif

    char buf[32] = { 0 };
    std::snprintf(
        buf,
        sizeof(buf),
        "%04d-%02d-%02d %02d:%02d:%02d",
        local_tm.tm_year + 1900,
        local_tm.tm_mon + 1,
        local_tm.tm_mday,
        local_tm.tm_hour,
        local_tm.tm_min,
        local_tm.tm_sec
    );

    logFile
        << "[" << buf << "] "
        << fromFunc
        << " | "
        << reason
        << std::endl;
}
void personamanager::init()
{
    // ===== 重置生命周期状态 =====
    state.inited = false;
    state.running = false;
    state.stopping = false;

    state.busy = false;
    state.inputEnabled = false;

    state.memoryInited = false;
    state.memoryDirty = false;

    // 交给子模块各自清理
    initMemory();
    initAI();

    state.inited = true;
    state.inputEnabled = true;
}
void personamanager::initMemory()
{
    state.memoryInited = false;
    state.memoryDirty = false;

    for (int i = 0; i < 3; ++i)
    {
        memoryState.selfMemory[i].memoryKeyId = 0;
        memoryState.selfMemory[i].keyPath.clear();
        memoryState.selfMemory[i].content.clear();
    }

    for (int i = 0; i < 6; ++i)
    {
        memoryState.userMemory[i].memoryKeyId = 0;
        memoryState.userMemory[i].keyPath.clear();
        memoryState.userMemory[i].content.clear();
    }

    if (!bridge)
    {
        logError("initMemory", u8"记忆桥接对象为空");
        return;
    }

    if (!bridge->init())
    {
        logError("initMemory", u8"记忆桥接初始化失败");
        return;
    }

    state.memoryInited = true;
}
void personamanager::initAI()
{
    state.busy = false;

    inputQueue.clear();
    outputQueue.clear();
}
bool personamanager::writeSelfLongMemory()
{
    if (!bridge)
    {
        logError("writeSelfLongMemory", u8"记忆桥接对象为空");
        return false;
    }

    std::string shortHistory = chat.getShortHistory();

    if (shortHistory.empty())
    {
        logError("writeSelfLongMemory", u8"短期记忆为空");
        return false;
    }

    std::string lastPersonaMemory;

    for (int key = 1; key <= 3; ++key)
    {
        std::string content = memoryState.selfMemory[key - 1].content;

        if (!content.empty())
        {
            lastPersonaMemory += content;
            lastPersonaMemory += "\n";
        }
    }

    std::string result =
        memoryAi.runself(shortHistory, lastPersonaMemory);

    if (result.empty())
    {
        logError("writeSelfLongMemory", u8"人格长期记忆生成失败");
        return false;
    }

    for (int key = 1; key <= 3; ++key)
    {
        std::string content = memoryAi.getMemoryContent(key);

        if (content.empty())
            continue;

        MemoryWrite req;
        req.memoryKeyId = key;
        req.content = content;

        if (!bridge->write(req))
        {
            logError("writeSelfLongMemory", u8"写入人格长期记忆失败");
            return false;
        }
    }

    if (!bridge->init())
    {
        logError("writeSelfLongMemory", u8"记忆镜像初始化失败");
        return false;
    }

    state.memoryDirty = false;
    state.memoryInited = true;

    return true;
}
bool personamanager::writeUserLongMemory()
{
    if (!bridge)
    {
        logError("writeUserLongMemory", u8"记忆桥接对象为空");
        return false;
    }

    std::string shortHistory = chat.getShortHistory();

    if (shortHistory.empty())
    {
        logError("writeUserLongMemory", u8"短期记忆为空");
        return false;
    }

    std::string lastUserMemory;

    for (int i = 0; i < 6; ++i)
    {
        std::string content = memoryState.userMemory[i].content;

        if (!content.empty())
        {
            lastUserMemory += content;
            lastUserMemory += "\n";
        }
    }

    std::string result =
        memoryAi.runuser(shortHistory, lastUserMemory);

    if (result.empty())
    {
        logError("writeUserLongMemory", u8"用户长期记忆生成失败");
        return false;
    }

    for (int key = 4; key <= 9; ++key)
    {
        std::string content = memoryAi.getMemoryContent(key);

        if (content.empty())
            continue;

        MemoryWrite req;
        req.memoryKeyId = key;
        req.content = content;

        if (!bridge->write(req))
        {
            logError("writeUserLongMemory", u8"写入用户长期记忆失败");
            return false;
        }
    }

    if (!bridge->init())
    {
        logError("writeUserLongMemory", u8"记忆镜像初始化失败");
        return false;
    }

    state.memoryDirty = false;
    state.memoryInited = true;

    return true;
}
bool personamanager::updateAllLongMemory()
{
    if (!writeSelfLongMemory())
        return false;

    if (!writeUserLongMemory())
        return false;

    chat.clearShortHistory();

    return true;
}
bool personamanager::getCurrentMemory(
    int memoryKeyId,
    std::string& outContent
) const
{
    // self 1-3
    for (int i = 0; i < 3; ++i)
    {
        if (memoryState.selfMemory[i].memoryKeyId == memoryKeyId)
        {
            outContent = memoryState.selfMemory[i].content;
            return true;
        }
    }

    // user 4-9
    for (int i = 0; i < 6; ++i)
    {
        if (memoryState.userMemory[i].memoryKeyId == memoryKeyId)
        {
            outContent = memoryState.userMemory[i].content;
            return true;
        }
    }

    return false;
}
bool personamanager::runChatWithPersona(
    const std::string& userText,
    PersonaMessageOut& outMsg
)
{
    logError("runChatWithPersona", "进入函数");
    logError("runChatWithPersona", "用户输入: " + userText);

    if (!state.memoryInited)
    {
        logError("runChatWithPersona", u8"记忆未初始化");
        return false;
    }

    // ===== 构造人格上下文 =====
    std::string personaText;

    for (int i = 0; i < 3; ++i)
    {
        if (!memoryState.selfMemory[i].content.empty())
        {
            personaText += memoryState.selfMemory[i].content;
            personaText += "\n";
        }
    }

    for (int i = 0; i < 6; ++i)
    {
        if (!memoryState.userMemory[i].content.empty())
        {
            personaText += memoryState.userMemory[i].content;
            personaText += "\n";
        }
    }

    logError(
        "runChatWithPersona",
        "构造的人格上下文长度: " + std::to_string(personaText.size())
    );

    // ===== 调用 ChatAI =====
    std::string reply = chat.runOnce(userText, personaText);

    logError("runChatWithPersona", "runOnce 原始返回: " + reply);
    logError("runChatWithPersona", "getText: " + chat.getText());

    // ===== 错误处理判断 =====
    // 如果 JSON 解析失败，getText() 会是空
    if (chat.getText().empty())
    {
        logError("runChatWithPersona", u8"解析失败或AI服务异常");

        // 直接把底层错误字符串传回
        outMsg.text = reply;
        outMsg.source = -1;     // 标记为错误来源
        outMsg.control = 0;
        outMsg.priority = 0;
        outMsg.emotion.clear();
        outMsg.createdAt = static_cast<int>(time(nullptr));

        return true; // 这里仍然返回 true，因为我们已经组装好了输出
    }

    // ===== 正常组装输出 =====
    outMsg.text = chat.getText();
    outMsg.source = 0;
    outMsg.control = chat.getControl();
    outMsg.priority = chat.getPriority();
    outMsg.emotion = chat.getEmotion();
    outMsg.createdAt = static_cast<int>(time(nullptr));

    logError("runChatWithPersona", "函数执行完成");
    return true;
}


// 输入：仅入队
void personamanager::pushInput(const PersonaMessageIn& msg)
{
    inputQueue.push_back(msg);
}
// 输出：弹出一条
bool personamanager::popOutput(PersonaMessageOut& outMsg)
{
    if (outputQueue.empty())
        return false;

    outMsg = outputQueue.front();
    outputQueue.erase(outputQueue.begin());
    return true;
}
// 处理函数
void personamanager::processOnce()
{
    logError("processOnce", "开始处理一条输入");

    if (inputQueue.empty())
    {
        logError("processOnce", "输入队列为空，直接返回");
        return;
    }
    // ===== 取出输入 =====
    PersonaMessageIn inMsg = inputQueue.front();
    inputQueue.erase(inputQueue.begin());
    logError("processOnce", "收到输入内容: " + inMsg.text);
    logError("processOnce", "输入类型: " + std::to_string(inMsg.type));
    PersonaMessageOut outMsg;
    outMsg.source = 0;
    outMsg.control = 0;
    outMsg.priority = 0;
    outMsg.emotion.clear();
    outMsg.createdAt = inMsg.createdAt;

    // ===== type 分发 =====
    if (inMsg.type == 0)
    {
        logError("processOnce", "类型0: 无法识别");
        outMsg.text = u8"无法识别输入内容";
    }
    else if (inMsg.type == 1)
    {
        logError("processOnce", "类型1: 调用人格Chat");

        if (!runChatWithPersona(inMsg.text, outMsg))
        {
            outMsg.text = u8"人格对话失败";
            logError("processOnce.chat", u8"runChatWithPersona 执行失败");
        }
        else
        {
            logError("processOnce.chat", "runChatWithPersona 执行成功");
        }
    }
    else if (inMsg.type == 2)
    {
        logError("processOnce", "类型2: 写入 self 长期记忆");

        if (!writeSelfLongMemory())
        {
            outMsg.text = u8"人格记忆更新失败";
            logError("processOnce.writeSelf", u8"写入 self 1-3 失败");
        }
        else
        {
            outMsg.text = u8"人格长期记忆已更新";
            logError("processOnce.writeSelf", u8"写入 self 1-3 成功");
        }
    }
    else if (inMsg.type == 3)
    {
        logError("processOnce", "类型3: 写入 user 长期记忆");

        if (!writeUserLongMemory())
        {
            outMsg.text = u8"用户记忆更新失败";
            logError("processOnce.writeUser", u8"写入 user 4-9 失败");
        }
        else
        {
            outMsg.text = u8"用户长期记忆已更新";
            logError("processOnce.writeUser", u8"写入 user 4-9 成功");
        }
    }
    else
    {
        logError("processOnce", "未知输入类型: " + std::to_string(inMsg.type));
        outMsg.text = u8"未知输入类型";
    }
    logError("processOnce", "输出内容: " + outMsg.text);
    // ===== 输出入队 =====
    outputQueue.push_back(outMsg);
    logError("processOnce", "处理完成并入输出队列");
}

bool personamanager::runOnce(
    const PersonaMessageIn& inMsg,
    PersonaMessageOut& outMsg
)
{
    logError("runOnce", "开始处理一条输入");

    // ===== 初始化输出 =====
    outMsg.source = 0;
    outMsg.control = 0;
    outMsg.priority = 0;
    outMsg.emotion.clear();
    outMsg.createdAt = inMsg.createdAt;

    logError("runOnce", "收到输入内容: " + inMsg.text);
    logError("runOnce", "输入类型: " + std::to_string(inMsg.type));

    // ===== type 分发 =====
    if (inMsg.type == 0)
    {
        logError("runOnce", "类型0: 无法识别");
        outMsg.text = u8"无法识别输入内容";
    }
    else if (inMsg.type == 1)
    {
        logError("runOnce", "类型1: 调用人格Chat");

        if (!runChatWithPersona(inMsg.text, outMsg))
        {
            outMsg.text = u8"人格对话失败";
            logError("runOnce.chat", u8"runChatWithPersona 执行失败");
            return false;
        }

        logError("runOnce.chat", "runChatWithPersona 执行成功");
    }
    else if (inMsg.type == 2)
    {
        logError("runOnce", "类型2: 写入 self 长期记忆");

        if (!writeSelfLongMemory())
        {
            outMsg.text = u8"人格记忆更新失败";
            logError("runOnce.writeSelf", u8"写入 self 1-3 失败");
            return false;
        }

        outMsg.text = u8"人格长期记忆已更新";
        logError("runOnce.writeSelf", u8"写入 self 1-3 成功");
    }
    else if (inMsg.type == 3)
    {
        logError("runOnce", "类型3: 写入 user 长期记忆");

        if (!writeUserLongMemory())
        {
            outMsg.text = u8"用户记忆更新失败";
            logError("runOnce.writeUser", u8"写入 user 4-9 失败");
            return false;
        }

        outMsg.text = u8"用户长期记忆已更新";
        logError("runOnce.writeUser", u8"写入 user 4-9 成功");
    }
    else
    {
        logError("runOnce", "未知输入类型: " + std::to_string(inMsg.type));
        outMsg.text = u8"未知输入类型";
    }

    logError("runOnce", "输出内容: " + outMsg.text);
    logError("runOnce", "处理完成");

    return true;
}
