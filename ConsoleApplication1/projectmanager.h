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
struct ProjectState
{
    // ===== 项目自身 =====
    bool projectInited;         // 项目是否完成基础初始化
    bool projectRunning;        // 项目是否整体处于运行态
    bool stopping;              // 项目是否进入停止流程
    bool destroyed;             // Project 是否已进入销毁阶段

    // ===== 上位机生命周期（调度级）=====
    bool upperStartRequested;   // 是否已请求启动上位机
    bool upperInited;           // 上位机是否完成 init
    bool upperRunning;          // 上位机是否正在运行（run 中）
    bool upperStopping;         // 是否正在请求上位机停止
    bool upperDestroyed;        // 上位机资源是否已销毁

    // ===== AI 与字符串输入通道 =====
    bool stringThreadInited;    // 字符串读取线程是否已初始化
    bool stringThreadRunning;   // 字符串读取线程是否正在运行
    bool stringThreadStopping;  // 字符串读取线程是否进入停止流程

    bool aiInputEnabled;        // 是否允许字符串触发 AI
    bool aiBusy;                // AI 是否正在消耗（防重入）

    // ===== 错误与安全 =====
    bool fatalError;            // 是否发生不可恢复错误
};

class ProjectManager
{
public:
    ProjectManager(
        SqlStore& store,
        RunState& runState,
        plcinfo& currentPlc,
        std::vector<signalinfo>& worksignals,
        ProjectState& projectstate,
        WorkspaceAI& workspaceAI,
        ExecuteAI& executeAI,
        DecisionAI& decisionAI
    );
	~ProjectManager();

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
    uppermachine* upperRef;

    WorkspaceAI& workspaceAIRef;
    ExecuteAI& executeAIRef;
    DecisionAI& decisionAIRef;

    // 系统状态镜像
    RunState runState;
    plcinfo currentPlc;
    std::vector<signalinfo> worksignals;
    ProjectState projectstate;
    // 管理层错误
    std::string lastError;
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