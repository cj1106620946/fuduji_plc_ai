#pragma once
#include <string>
#include <windows.h>
#include "plcclient.h"
#include "aiclient.h"
#include"aitrace.h"
#include "aimanager.h"
#include "aicontroller.h"
#include "piperengine.h"
#include "speechagent.h"

#include"chatai.h"
#include"executeai.h"
#include"workspaceai.h"
#include"decisionai.h"
#include"judgmentai.h"

class PLCClient;
class AIClient;
class AIController;
class AITrace;

class WorkspaceAI;
class ChatAI;
class DecisionAI;
class ExecuteAI;

class speechagent;
class PiperEngine;

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
    void menuAiBenchmark();
    void menuAiManagerTest();

private:
    PLCClient plc;
    AIClient ai;
    AIController aiController;
    AITrace aiTrace;

    WorkspaceAI* workspace = nullptr;
    ChatAI* chat = nullptr;
    DecisionAI* decision = nullptr;
    ExecuteAI* execute = nullptr;
	Judgmentai* judgment = nullptr;
    bool hasAIKey = false;
};
std::string GBKtoUTF8(const std::string& gbk);

