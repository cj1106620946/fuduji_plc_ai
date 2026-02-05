#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>

#include "uppermachine.h"
#include "workspaceai.h"
#include "executeai.h"
#include "decisionai.h"

#include "sqlstore.h"    
#include"plcclient.h"

class ProjectManager
{
public:
    ProjectManager(
        PLCClient& plc,
        uppermachine& upper,
        WorkspaceAI& workspaceAI,
        ExecuteAI& executeAI,
        DecisionAI& decisionAI
    );
    // 生命周期接口
    bool init();
    bool start();
    void stop();
    bool isReady() const;

    // 由 AI 生成并创建 PLC 工程
    bool createPlcByAI(const std::string& userInput);
    // 由 AI 生成并创建 Signal 定义
    bool createSignalsByAI(const std::string& userInput);

    bool createPlcWorkspaceByAI(
        const std::string& userInput
    );

    bool createSignalWorkspaceByAI(
        const std::string& userInput
    );
    // 从系统镜像读取变量
    bool readSignal(
        const std::string& plcAddress,
        std::string& outResult
    );

    // 向系统镜像写入变量（写意图）
    bool writeSignal(
        const std::string& plcAddress,
        const std::string& value,
        std::string& outResult
    );

    bool executeByAI(
        const std::string& userInput,
        std::vector<std::string>& outMessages
    );
    const std::string& getLastError() const;
private:
    // 外部注入对象
    uppermachine& upperRef;
    WorkspaceAI& workspaceAIRef;
    ExecuteAI& executeAIRef;
    DecisionAI& decisionAIRef;
    PLCClient& plcRef;
    // 管理层错误
    std::string lastError;
    // 系统状态镜像
    RunState runState;
    plcinfo currentPlc;

    std::vector<signalinfo> worksignals;
    std::atomic<bool> running;
    // 管理线程
    std::thread runThread;      // manager 主运行线程
    std::thread upperThread;    // 上位机线程
    std::thread aiThread;       // AI 输入输出线程
private:
    void logError(
        const std::string& fromFunc,
        const std::string& reason
    );
private:
    void runThreadProc();       // manager 主循环
    void upperThreadProc();     // 上位机 run 封装
    void aiThreadProc();        // AI 输入输出循环


};