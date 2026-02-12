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
    bool projectInited;         // 初始化完成标志位
    bool projectRunning;        // 运行标志位
    bool stopping;              // 停止完成标志位

    bool upperInited;           // 上位机初始化完成标志位
    bool upperRunning;          // 上位机运行标志位
    bool upperStopping;         // 停止完成标志位

    bool stringThreadInited;    // 初始化完成标志位
    bool stringThreadRunning;   // 启动标志位
    bool stringThreadStopping;  // 停止完成标志位

    bool aiInputEnabled;        // 激活AI
    bool aiBusy;                // 当前是否使用AI
};
struct AIMessage
{
    std::string text;      // 原始输入文本（用户 / 系统 / AI / 其它）
    int source;            // 消息意图 / 通道类型
    int type;              // 具体子类型
    int createdAt;         // 时间戳
};

struct ProjectMessage
{
    std::string text;    // 实际输出内容
    int source;          // 输出来源（AI / system / plc / project）
    int createdAt;       // 时间戳
};


class ProjectManager
{
public:
    ProjectManager(
        SqlStore& store,
        WorkspaceAI& workspaceAI,
        ExecuteAI& executeAI,
        DecisionAI& decisionAI,
        plcinfo& currentPlcRef,
        std::vector<signalinfo>& worksignalsRef,
        ProjectState& projectstateRef
    );


	~ProjectManager();

    // 生命周期接口
    bool init();
    bool upperinit();
    bool connectplcinit();
    bool aiinit();
    void stop();
    bool isReady() const;

    void createRunThread();
    void destroyRunThread();
    void createUpperThread();
    void destroyUpperThread();
    void createAIThread();
    void destroyAIThread();

    bool pushAIMessage(const std::string& text, int source, int type);

    void pushProjectMessage(
        const std::string& text,
        int source
    );
    bool popProjectMessage(ProjectMessage& outMsg);

    // 由 AI 生成并创建 PLC 工程
    bool createPlcByAI(const std::string& userInput);
    // 由 AI 生成并创建 Signal 定义
    bool createSignalsByAI(const std::string& userInput);

    bool createPlcWorkspaceByAI(
        const std::string& userInput,
        std::vector<std::string>& outMessages
    );

    bool createSignalWorkspaceByAI(
        const std::string& userInput,
        std::vector<std::string>& outMessages
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


    // ===== 上位机运行状态镜像 =====
    RunState runState;

    // ===== 当前 PLC 工程镜像 =====
    plcinfo& currentPlc;

    // ===== 信号变量镜像 =====
    std::vector<signalinfo>& worksignals;

    // ===== 项目状态结构 =====
    ProjectState& projectstate;

    // ===== AI 消息队列（由 ProjectManager 自身管理）=====
    std::vector<AIMessage> aiQueue;

    // ===== 项目消息队列（由 ProjectManager 自身管理）=====
    std::vector<ProjectMessage> projectMessageQueue;

    // 管理层错误
    std::string lastError;
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