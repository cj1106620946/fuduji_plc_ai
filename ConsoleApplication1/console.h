#pragma once

#include <string>
#include <windows.h>

//  已包含的具体头文件 
#include "plcclient.h"
#include "aiclient.h"
#include "aitrace.h"
#include "aimanager.h"
#include "aicontroller.h"
#include "piperengine.h"
#include "speechagent.h"

#include "chatai.h"
#include "executeai.h"
#include "workspaceai.h"
#include "decisionai.h"
#include "judgmentai.h"
#include"memorybridge.h"
#include"memoryai.h"
// PLC / 数据库
class Sqllient;
class SqlStore;
class uppermachine;

// AI 基础
class AIClient;
class AIController;
class AITrace;

// AI 模块
class WorkspaceAI;
class ChatAI;
class DecisionAI;
class ExecuteAI;
class Judgmentai;  

// 语音相关
class speechagent;
class PiperEngine;

class Console
{
public:
    Console();
    ~Console();
    void run();
    // 显示或创建控制台
    void openconsole();
    // 隐藏控制台
    void hideconsole();


private:
    // 数据库与上位机
    Sqllient* sqlClient = nullptr;
    SqlStore* store = nullptr;
    uppermachine* upper = nullptr;

    // PLC 与 AI
    PLCClient plc;
    AIClient ai;
    AIController aiController;
    AITrace aiTrace;
   //记忆管理
    memorybridge* memory = nullptr;    
    // AI 模块
    WorkspaceAI* workspace = nullptr;
    ChatAI* chat = nullptr;

    MemoryAI* memoryai = nullptr;
    DecisionAI* decision = nullptr;
    ExecuteAI* execute = nullptr;
    Judgmentai* judgment = nullptr;

    // 状态
    bool hasAIKey = false;
    AICallDesc aiDesc;

std::string GBKtoUTF8(const std::string& gbk);
private:
    // 基础工具
    void printUTF8(const std::string& text);
    bool checkBreak(const std::string& cmd);
    void showMainHeader();
    void mainMenu();
    void menuTestA1();
    void menuTestA2();
    void menuTestA3();
    void menuTestA4();
    void menuTestA5();
    void menuTestA6();
    void menuTestA7();
    void menuTestA8();
    void menuTestA9();
    void menuTestA10();
    void menuTestA11();
    void menuTestA12();
    void menuTestA13();
    void menuTestA14();
    void menuTestA15();
    void menuTestA16();
    void menuTestA17();
    void menuTestA18();
    void menuTestA19();
    void menuTestA20();
    void menuTestA21();
    void menuTestA22();
    void menuTestA23();
    void menuTestA24();
    void menuTestA25();
    void menuTestA26();
    void menuTestA27();
    void menuTestA28();
    void menuTestA29();
    void menuTestA30();
};