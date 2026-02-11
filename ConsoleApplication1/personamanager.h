#pragma once
#include <vector>
#include <string>
#include "memorybridge.h"
#include "sqlstore.h"
#include "chatai.h"
#include "memoryai.h"
#include <thread>

class ChatAI;
class MemoryAI;
class SqlStore;
class memorybridge;
struct PersonaState
{
    // ===== 初始化阶段 =====
    bool inited;            // 人格是否已初始化
    bool running;           // 人格线程是否运行中
    bool stopping;          // 是否进入停止流程

    bool busy;              // 当前是否正在处理一条输入
    bool inputEnabled;      // 是否允许运行输入处理

    bool memoryInited;      // 记忆桥接是否初始化完成
    bool memoryDirty;       // 人格记忆是否被修改（待同步）

};
// 人格输入消息
struct PersonaMessageIn
{
    std::string text;    // 原始输入文本
    int source;          // 来源 / 意图通道
    int type;            // 子类型
    int createdAt;       // 时间戳
};

// 人格输出消息
struct PersonaMessageOut
{
    std::string text;    // 人格最终输出文本
    int source;          // 输出来源
    int control;         // 控制标志
    int priority;        // 优先级
    std::string emotion; // 情绪状态
    int createdAt;       // 时间戳
};


class personamanager
{
public:
    // 构造
    personamanager(
        ChatAI& chatRef,
        MemoryAI& memoryAiRef,
        SqlStore& storeRef,
        CurrentMemoryState& memoryStateRef,
        PersonaState& stateRef
    );

    void init();
    void initMemory();
    void initAI();
    ~personamanager();
    // 输入：写入输入队列（由输入线程调用）
    void pushInput(const PersonaMessageIn& msg);
    // 输出：获取输出队列（由输出线程调用）
    bool popOutput(PersonaMessageOut& outMsg);

    // 生成并写入 self 1-3 的长期记忆
    bool writeSelfLongMemory(const std::string& text);
    // 生成并写入 user 4-9 的长期记忆
    bool writeUserLongMemory(const std::string& text);

    bool runChatWithPersona(
        const std::string& userText,
        PersonaMessageOut& outMsg
    );
    // 根据记忆 key 获取当前镜像中的记忆内容
    bool getCurrentMemory(int memoryKeyId, std::string& outContent) const;
    void processOnce();
private:
    void logError(
        const std::string& fromFunc,
        const std::string& reason
    );

    std::thread personaMainThread;   // 人格主线程
    std::thread personaIoThread;      // 人格输入输出线程
    std::thread personaMemoryThread;  // 人格记忆管理线程


private:
    // 对话 AI
    ChatAI& chat;
    // 记忆 AI
    MemoryAI& memoryAi;
    // 数据库管理
    SqlStore& store;
    // 记忆桥接（在本类中实例化）
    memorybridge* bridge;

    CurrentMemoryState& memoryState;   // 记忆状态（外部持有）
    PersonaState& state;               // 人格状态（外部持有）

    std::vector<PersonaMessageIn>  inputQueue;   // 输入队列
    std::vector<PersonaMessageOut> outputQueue;  // 输出队列

};

