#pragma once

#include <string>
#include <windows.h>
#include "speechagent.h"
#include "chatagent.h"
#include "plcclient.h"
#include "deepseek.h"
#include "aicontroller.h"
#include "workspacemanager.h"
#include "piperengine.h"   
class PLCClient;
class DeepSeekAI;
class AIController;
class WorkspaceManager;
class ChatAgent;
class DecisionAI;
class speechagent;


class Console
{
public:
    Console();
    ~Console();

    void run();

private:
    void printGBK(const std::string& text);
    void printUTF8(const std::string& text);
    bool checkBreak(const std::string& cmd);

    void showMainHeader();
    void mainMenu();

    void menuPLCConnect();
    void menuAIKey();
    void menuPLCManual();
    void menuWorkspace();
    void menuChat();
    void menuDecisionTest();
    void menuSpeechTest();
    void menuVoiceChat();
    void menuTtsTest();

private:
    PLCClient plc;
    DeepSeekAI ai;
    AIController aiController;
    WorkspaceManager* workspace = nullptr;
    ChatAgent* chatAgent = nullptr;
    DecisionAI* decisionAI = nullptr;
    bool hasAIKey = false;
};
std::string GBKtoUTF8(const std::string& gbk);

