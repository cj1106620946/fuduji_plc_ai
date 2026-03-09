#pragma once
#include <vector>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include "qtmain.h"
#include "sqllient.h"
#include "sqlstore.h"
#include "chatai.h"
#include "memoryai.h"
#include "workspaceai.h"
#include "executeai.h"
#include "decisionai.h"
#include "projectmanager.h"
#include "personamanager.h"
#include "jsonstatewriter.h"
#include "speechagent.h"
#include "aiclient.h"
#include "aicontroller.h"
#include "aitrace.h" 
enum class UiMessageType
{
    Text = 0,      // 文本输
    Command = 1,   // 指令类输入
    System = 2     // 系统事件
};
enum class CommandType
{
    None = 0,
    Help,
    Live2D,
    MemorySelf,
    MemoryUser,
    MemoryClear,

    PlcCreate,
    SignalCreate,
    SignalDelete,

    Get,
    Set,
    GetAll
};
struct CommandHelp
{
    std::string command;      // 指令名称
    std::string usage;        // 使用格式
    std::string description;  // 中文说明
};

static std::vector<CommandHelp> g_commandHelp =
{
    {
        "plccreate",
        "plccreate name ip [rack slot desc]",
        "创建PLC工作区，name和ip为必填，rack和slot默认0和1，desc为解释说明"
    },
    {
        "signalcreate",
        "signalcreate name addr desc [name addr desc ...]",
        "创建变量，三元组格式，每个变量必须包含名称 地址 中文说明"
    },
    {
        "signaldelete",
        "signaldelete id",
        "删除指定ID的变量"
    },
    {
        "live2d",
        "live2d 0/1/2",
        "控制Live2D状态"
    },
    {
        "memory1",
        "memory1",
        "写入人格1-3长期记忆"
    },
    {
        "memory2",
        "memory2",
        "写入用户4-9长期记忆"
    },
    {
        "get",
        "get keyword",
        "读取名称 地址 说明中包含keyword的变量信息"
    },
    {
        "set",
        "set keyword value",
        "将匹配到的变量写入指定值"
    },
    {
        "getall",
        "getall",
        "读取当前全部变量信息"
    }
};

// 输入消息结构
struct UiMessage
{
    UiMessageType type;
    std::string text;
};
struct plcai
{
    // ===== 初始化状态 =====
    bool qtinit = false;
    bool sqlinit = false;
    bool aiinit = false;
    bool projectinit = false;
    bool personainit = false;

    bool ioThreadRunning = false;
    bool ioThreadStopping = false;

    // ===== 上位机周期刷新控制 =====
    bool allowUpperRefresh = 0;   // 是否允许上位机周期刷新

    UiState ui;

    // ===== 人格生命周期=====
    PersonaState personaState;

    // ===== project生命周期 =====
    ProjectState projectState;
};
struct PlcAiMirror
{

    // ===== PLC 当前镜像 =====
    plcinfo currentPlc;
    // ===== 信号镜像 =====
    std::vector<signalinfo> worksignals;
    // ===== 人格长期记忆镜像 =====
    CurrentMemoryState memoryState;
};
struct RuntimeEnvironment
{
    qtmain* ui = nullptr;
    Sqllient* sqlClient = nullptr;
    SqlStore* sqlStore = nullptr;
    JsonStateWriter live2dWriter;
    AICallDesc defaultCallDesc;

    RuntimeEnvironment()
        : live2dWriter("live2dstate.json")
    {
        // 初始化默认AI调用参数
        defaultCallDesc.useCloud = 1;
        defaultCallDesc.provider = AIProvider::Ollama;
        defaultCallDesc.apiKey = "0";
        defaultCallDesc.modelName.clear();
        defaultCallDesc.timeoutSec = 60;
        defaultCallDesc.temperature = 0.7;
        defaultCallDesc.maxTokens = 2048;
    }
};


struct RuntimeModules
{
    // ===== AI 底层 =====
    AIClient* aiClient = nullptr;          // AI 调用客户端
    AIController* aiController = nullptr;  // AI 调度控制器
    AITrace* aiTrace = nullptr;            // AI 调用追踪工具

    ChatAI* chatAi = nullptr;
    MemoryAI* memoryAi = nullptr;
    WorkspaceAI* workspaceAi = nullptr;
    ExecuteAI* executeAi = nullptr;
    DecisionAI* decisionAi = nullptr;

    speechagent* speech = nullptr;
};


struct RuntimeManagers
{
    ProjectManager* project = nullptr;
    personamanager* persona = nullptr;
};


class ai_plc_delegate
{
public:
    ai_plc_delegate();
    ~ai_plc_delegate();
    void run();
private:
    bool initQt();
    bool initSql(const std::string& dbPath);
    bool initpersona();
    bool initproject();
    bool initai();
    void onUiText(const std::string& text);
    void processInputOnce();
    CommandType parseCommandType(const std::string& token);
signals:
    void uiTextSubmitted(const std::string& text,int mode);

private:
    RuntimeEnvironment env;
    RuntimeModules modules;
    RuntimeManagers managers;
    plcai pclailife;
    PlcAiMirror mirror;

private:
    std::string lastError;
    // 输入池
    std::queue<UiMessage> inputQueue;
    std::queue<UiMessage> outputQueue;

    void handleCommandMessage(const UiMessage& msg);
    void handleTextMessage(const UiMessage& msg);

    std::vector<std::string> parsePersonaMirror();
    std::vector<std::string> parseProjectMirror();
    void logError(
        const std::string& fromFunc,
        const std::string& reason
    );
    // ===== 线程 =====
    std::thread upperThread;          // 上位机线程
    std::thread ioThread;             // 输入输出处理线程
    // 线程函数
    void ioThreadProc();

};
