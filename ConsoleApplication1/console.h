#pragma once

#include <string>
#include <windows.h>

#include "speechagent.h"
#include "chatagent.h"
#include "plcclient.h"
#include "deepseek.h"
#include "aicontroller.h"
#include "workspacemanager.h"

// ===== 前置声明，避免头文件互相污染 =====
class PLCClient;
class DeepSeekAI;
class AIController;
class WorkspaceManager;
class ChatAgent;
class DecisionAI;
class speechagent;

// ================================
// Console
// ================================
class Console
{
public:
    Console();
    ~Console();

    void run();

private:
    // ================================
    // 输出 / 工具
    // ================================
    void printGBK(const std::string& text);
    void printUTF8(const std::string& text);
    bool checkBreak(const std::string& cmd);

    // ================================
    // UI 主流程
    // ================================
    void showMainHeader();
    void mainMenu();

    // ================================
    // 菜单功能
    // ================================
    void menuPLCConnect();
    void menuAIKey();
    void menuPLCManual();
    void menuWorkspace();
    void menuChat();
    void menuDecisionTest();
    void menuSpeechTest();
    void menuVoiceChat();

private:
    // ================================
    // 核心对象
    // ================================
    PLCClient plc;
    DeepSeekAI ai;
    AIController aiController;

    WorkspaceManager* workspace = nullptr;
    ChatAgent* chatAgent = nullptr;
    DecisionAI* decisionAI = nullptr;

    // ================================
    // 状态
    // ================================
    bool hasAIKey = false;
};

// ================================
// 全局工具函数
// ================================
std::string GBKtoUTF8(const std::string& gbk);

