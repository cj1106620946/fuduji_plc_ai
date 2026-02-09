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
// 主循环 / 顶层运行聚合体
class ai_plc_delegate
{
public:
    ai_plc_delegate();
    ~ai_plc_delegate();


    void run();
private:
    bool initQt();
    bool initSql();

private:
    qtmain* ui = nullptr;
    Sqllient* sqlClient = nullptr;
    SqlStore* sqlStore = nullptr;
    ChatAI* chatAi = nullptr;
    MemoryAI* memoryAi = nullptr;
    WorkspaceAI* workspaceAi = nullptr;
    ExecuteAI* executeAi = nullptr;
    DecisionAI* decisionAi = nullptr;
    ProjectManager* project = nullptr;
    personamanager* persona = nullptr;
    speechagent* speech = nullptr;
    CurrentMemoryState memoryState;
    PersonaState personaState;
    JsonStateWriter live2dWriter;
private:
    void logError(
        const std::string& fromFunc,
        const std::string& reason
    );
};
