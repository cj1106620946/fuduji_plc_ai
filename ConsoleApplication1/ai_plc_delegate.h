#pragma once
#include <vector>
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
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
enum class UiMessageType
{
    Text = 0,      // 文本输入
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
    bool qtinit;
	bool sqlinit;
    UiState ui;     // UI 状态控制
};
struct PlcAiMirror
{
    // ===== 人格状态镜像 =====
    PersonaState personaState;
    // ===== 项目状态镜像 =====
    ProjectState projectState;
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

    bool initPersonaAI();   
    bool initUpperMachine(); 

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
    void logError(
        const std::string& fromFunc,
        const std::string& reason
    );
};
