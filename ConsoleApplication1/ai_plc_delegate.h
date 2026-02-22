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

    // ===== 线程运行控制 =====
    bool upperThreadRunning = false;   // 上位机线程是否运行
    bool upperThreadStopping = false;  // 上位机线程是否进入停止流程

    bool ioThreadRunning = false;      // 输入输出线程是否运行
    bool ioThreadStopping = false;     // 输入输出线程是否进入停止流程
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

    RuntimeEnvironment()
        : live2dWriter("live2dstate.json")
    {
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
signals:
    void uiTextSubmitted(const std::string& text);

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
    void upperThreadProc();
    void ioThreadProc();

};
