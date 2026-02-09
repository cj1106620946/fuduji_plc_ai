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

bool personamanager::writeSelfLongMemory(const std::string& text)
{
    if (!bridge)
    {
        logError("writeSelfLongMemory", u8"记忆桥接对象为空");
        return false;
    }

    // ===== 取上一次 self 1-3 的长期记忆 =====
    std::string lastPersonaMemory;
    for (int key = 1; key <= 3; ++key)
    {
        std::string content = memoryAi.getMemoryContent(key);
        if (content.find(u8"[error]") == std::string::npos)
        {
            lastPersonaMemory += content;
            lastPersonaMemory += "\n";
        }
    }

    // ===== 调用 MemoryAI（self 1-3）=====
    std::string result = memoryAi.runself(
        text,
        lastPersonaMemory
    );

    if (result.empty())
    {
        logError("writeSelfLongMemory", u8"人格长期记忆生成失败");
        return false;
    }

    // ===== 写入数据库（1-3）=====
    for (int key = 1; key <= 3; ++key)
    {
        std::string content = memoryAi.getMemoryContent(key);
        if (content.find(u8"[error]") != std::string::npos)
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

    // ===== 写入完成后，统一刷新镜像 =====
    if (!bridge->init())
    {
        logError("writeSelfLongMemory", u8"记忆镜像初始化失败");
        return false;
    }

    state.memoryDirty = false;
    state.memoryInited = true;
    return true;
}

bool personamanager::writeUserLongMemory(const std::string& text)
{
    if (!bridge)
    {
        logError("writeUserLongMemory", u8"记忆桥接对象为空");
        return false;
    }

    // ===== 取上一次 user 4-9 的长期记忆 =====
    std::string lastUserMemory;
    for (int key = 4; key <= 9; ++key)
    {
        std::string content = memoryAi.getMemoryContent(key);
        if (content.find(u8"[error]") == std::string::npos)
        {
            lastUserMemory += content;
            lastUserMemory += "\n";
        }
    }

    // ===== 调用 MemoryAI（user 4-9）=====
    std::string result = memoryAi.runuser(
        text,
        lastUserMemory
    );

    if (result.empty())
    {
        logError("writeUserLongMemory", u8"用户长期记忆生成失败");
        return false;
    }

    // ===== 写入数据库（4-9）=====
    for (int key = 4; key <= 9; ++key)
    {
        std::string content = memoryAi.getMemoryContent(key);
        if (content.find(u8"[error]") != std::string::npos)
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

    // ===== 写入完成后，统一刷新镜像 =====
    if (!bridge->init())
    {
        logError("writeUserLongMemory", u8"记忆镜像初始化失败");
        return false;
    }

    state.memoryDirty = false;
    state.memoryInited = true;
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
    if (!state.memoryInited)
    {
        logError("runChatWithPersona", u8"记忆未初始化");
        return false;
    }

    // ===== 构造人格上下文（1-9）=====
    std::string personaText;

    // self 1-3
    for (int i = 0; i < 3; ++i)
    {
        if (!memoryState.selfMemory[i].content.empty())
        {
            personaText += memoryState.selfMemory[i].content;
            personaText += "\n";
        }
    }

    // user 4-9
    for (int i = 0; i < 6; ++i)
    {
        if (!memoryState.userMemory[i].content.empty())
        {
            personaText += memoryState.userMemory[i].content;
            personaText += "\n";
        }
    }

    // ===== 调用 ChatAI =====
    std::string reply = chat.runOnce(
        userText,
        personaText
    );

    if (reply.empty())
    {
        logError("runChatWithPersona", u8"ChatAI 返回空结果");
        return false;
    }

    // ===== 组装输出 =====
    outMsg.text = chat.getText();
    outMsg.source = 0;// 人格 AI
    outMsg.control = chat.getControl();
    outMsg.priority = chat.getPriority();
    outMsg.emotion = chat.getEmotion();
    outMsg.createdAt = static_cast<int>(time(nullptr));
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
    if (inputQueue.empty())
        return;

    PersonaMessageIn inMsg = inputQueue.front();
    inputQueue.erase(inputQueue.begin());

    PersonaMessageOut outMsg;
    outMsg.source = 0;
    outMsg.control = 0;
    outMsg.priority = 0;
    outMsg.emotion.clear();
    outMsg.createdAt = inMsg.createdAt;

    // ===== type 分发 =====
    if (inMsg.type == 0)
    {
        // 识别失败
        outMsg.text = u8"无法识别输入内容";
    }
    else if (inMsg.type == 1)
    {
        // 调用人格 Chat
        if (!runChatWithPersona(inMsg.text, outMsg))
        {
            outMsg.text = u8"人格对话失败";
            logError("processOnce.chat", u8"runChatWithPersona 执行失败");
        }
    }
    else if (inMsg.type == 2)
    {
        // 写入 self 1-3 长期记忆
        if (!writeSelfLongMemory(inMsg.text))
        {
            outMsg.text = u8"人格记忆更新失败";
            logError("processOnce.writeSelf", u8"写入 self 1-3 失败");
        }
        else
        {
            outMsg.text = u8"人格长期记忆已更新";
        }
    }
    else if (inMsg.type == 3)
    {
        // 写入 user 4-9 长期记忆
        if (!writeUserLongMemory(inMsg.text))
        {
            outMsg.text = u8"用户记忆更新失败";
            logError("processOnce.writeUser", u8"写入 user 4-9 失败");
        }
        else
        {
            outMsg.text = u8"用户长期记忆已更新";
        }
    }
    else
    {
        outMsg.text = u8"未知输入类型";
        logError(
            "processOnce",
            u8"未知 PersonaMessageIn.type"
        );
    }

    outputQueue.push_back(outMsg);
}

