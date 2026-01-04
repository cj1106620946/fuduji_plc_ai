#include "aicontroller.h"
#include "plcclient.h"
#include "deepseek.h"
#include <sstream>
#include <iostream>
#include <algorithm>
// ==========================================================
// 构造
// ==========================================================
AIController::AIController(PLCClient& plcRef, DeepSeekAI& aiRef)
    : plc(plcRef), ai(aiRef)
{
    buildChatPrompt();
    buildWorkspacePrompt();
    buildDecisionPrompt();
}

// ==========================================================
// 二、Workspace AI（创建工作区）
// ==========================================================
AIController::WorkspaceDraft
AIController::createWorkspace(const std::string& user_requirement)
{
    WorkspaceDraft draft;
    std::string raw = callWorkspaceAI(user_requirement);
    draft.raw_json = raw;
    // 当前阶段：只返回原始文本
    // 后续你会在这里：
    //  - parse WS:JSON
    //  - 填 vars
    //  - 生成 final_json
    draft.ok = true;
    draft.name = "workspace_tmp";
    draft.desc = "auto generated";
    draft.final_json = raw;

    return draft;
}

// ==========================================================
// 三、Decision AI（运行时决策）
// ==========================================================
AIController::DecisionResult
AIController::runDecision(const DecisionInput& input)
{
    DecisionResult r;

    std::ostringstream prompt;
    prompt << "workspace: " << input.workspace_name << "\n";
    prompt << "snapshot:\n" << input.snapshot_json << "\n";
    prompt << "rule:\n" << input.decision_prompt << "\n";

    std::string reply = callDecisionAI(prompt.str());

    // 当前阶段不做结构化解析
    r.ok = true;
    r.reason = reply;

    return r;
}
// ==========================================================
// Prompt 构建
// ==========================================================
void AIController::buildChatPrompt()
{
    chat_prompt =
        u8"你是工业控制AI的【指令规划器】。\n"
        u8"你的任务是将输入转换为可执行的 JSON 控制指令。\n"
        u8"你不会自主推测配置变更，只能根据输入类型执行。\n"
        u8"\n"
        // =========================
        // 输入类型判定（核心协议）
        // =========================
        u8"输入类型判定规则\n"
        u8"1. 普通自然语言 → 视为人类指令\n"
        u8"2. yuyin + 普通自然语言 开头 → 视为语言输入\n"
        u8"3. 以 DECISION_EXECUTE 开头 → 视为决策AI执行请求\n"
        u8"\n"
        u8"除以上三种情况外，不允许自行假设输入意图。\n"
        u8"\n"
        // =========================
        // 决策 AI 规则（重点）
        // =========================
        u8"【决策AI处理规则】\n"
        u8" 自然语言可执行所有操作\n"
        u8"  - yuying + 自然语言：\n"
        u8"  - 可执行所有操作\n"
        u8"  - 传过来的文本可能有问题，包括错别字，缺少语句等等，你需要自行判断\n"
        u8"  - 当遇到难以理解的内容时，报错并传回消息\n"
        u8"\n"
        u8"- DECISION_EXECUTE：\n"
        u8"  - 仅用于【执行】已有 decision.task\n"
        u8"  - 严禁修改 decision 配置\n"
        u8"  - 严禁判断为“重复配置”\n"
        u8"  - 只能根据 task 生成 plcActions\n"
        u8"\n"
        // =========================
        // Workspace 规则
        // =========================
        u8"【Workspace 规则】\n"
        u8"- 当用户描述系统结构、设备、工程方案时\n"
        u8"- 必须设置 jumpWorkspace = true\n"
        u8"- jumpWorkspace = true 时，不允许输出 plcActions\n"
        u8"\n"
        // =========================
        // 输出约束
        // =========================
        u8"【输出规则】\n"
        u8"1. 只能输出 JSON\n"
        u8"2. 不允许输出解释性文字\n"
        u8"3. 不允许 Markdown\n"
        u8"4. 所有字段必须存在\n"
        u8"5. 无法判断意图时 type=error\n"
        u8"6. message 必须说明原因\n"
        u8"\n"

        // =========================
        // JSON 结构
        // =========================
        u8"【JSON 结构（必须严格遵守）】\n"
        u8"{\n"
        u8"  \"type\": \"ok | error\",\n"
        u8"  \"message\": \"\",\n"
        u8"\n"
        u8"  \"jumpWorkspace\": false,\n"
        u8"\n"
        u8"  \"plcEnable\": {\"enable\": false,\"ip\": \"\",\"rack\": 0,\"slot\": 0},\n"
        u8"\n"
        u8"  \"plcActions\": [\n"
        u8"    {\"enable\": true,\"action\": \"read | write\",\"address\": \"\",\"value\": \"0 | 1\"}\n"
        u8"  ],\n"
        u8"\n"
        u8"  \"decision\": {\n"
        u8"    \"enable\": false,\n"
        u8"    \"source\": \"human | ai\",\n"
        u8"    \"interval_ms\": 0,\n"
        u8"    \"task\": {\"type\": \"\",\"params\": {}}\n"
        u8"  }\n"
        u8"}\n"
        u8"\n"
        u8"【开始解析】\n"
        u8"用户输入：\n";
        // =========================
        // 节点说明（非常重要）
        // =========================
        u8"【节点说明】\n"
        u8"1. jumpWorkspace（节点）\n"
        u8"- 表示是否进入 Workspace 构建流程\n"
        u8"- 当用户在描述系统、设备、工程结构时，必须设为 true\n"
        u8"- 为 true 时：不要输出 plcActions\n"
        u8"\n"
        u8"2. plcEnable（节点）\n"
        u8"- 表示是否建立 PLC 连接\n"
        u8"- 仅在用户明确要求连接 PLC 时启用\n"
        u8"\n"
        u8"3. plcActions（节点）\n"
        u8"- 只用于 PLC 读写操作\n"
        u8"- action 只能是 read 或 write\n"
        u8"- address 必须是 PLC 地址（如 I0.0 / Q0.0 / M1.0）\n"
        u8"- value 只允许 0 或 1\n"
        u8"\n"
        u8"4. decision（节点）\n"
        u8"- 用于触发决策 AI\n"
        u8"- 只有在 source=ai 时才允许执行\n"
        u8"- interval_ms 表示周期（毫秒）\n"
        u8"- task 描述要执行的决策内容\n"
        u8"\n"
        u8"【行为规则】\n"
        u8"- 取反类操作必须分两步：先读后写\n"
        u8"- 打开 / 关闭 / 读取 → 使用 plcActions\n"
        u8"- 所有 / 全部 → 必须拆分为多个 plcActions\n"
        u8"- 无操作意图时 type=error\n"
        u8"- 不允许同时执行 Workspace 与 PLC\n"
        u8"- decision 不允许决策ai进行修改，必须确认由用户输入才可以修改（必须确认）\n"
        u8"- 必须确认decision使能才可以执行决策ai的指令 \n"
        u8"\n"
        u8"【开始解析】\n"
        u8"用户输入：\n";

    response_prompt =
        u8"你是工业控制AI的结果解释器。\n"
        u8"你将收到用户问题与执行结果。\n"
        u8"用户采用文字和语言输入，你可自行判断，当用户输入语言时，文本可能不完整，信息不确定，需要你进行补充和说明。\n"
        u8"你必须只输出以下格式：\n"
        u8"QWQ:TEXT\n"
        u8"规则：\n"
        u8"1 按照QWQ:TEXT格式输出\n"
        u8"2 可多行\n"
        u8"3 语气尽可能温和柔婉，不要太学术化\n";
}


void AIController::buildWorkspacePrompt()
{
    workspace_prompt =
        u8"你是工业控制系统中的【Workspace 工程建模 AI】。\n"
        u8"你的任务是根据用户输入自动补全信息，生成可用工作区模型。\n"
        u8"只允许返回指定 JSON，禁止任何额外内容。\n"
        u8"仅当完全无法理解用户意图时才允许返回 WS:ERR。\n"
        u8"\n"
        u8"【输出格式】\n"
        u8"WS:OK\n"
        u8"WS:JSON\n"
        u8"{\n"
        u8"  \"workspace\": {\"name\": \"英文小写_下划线\", \"desc\": \"系统用途说明\"},\n"
        u8"  \"description\": \"该系统的整体控制逻辑说明\",\n"
        u8"  \"signals\": {\n"
        u8"    \"inputs\": [{\"name\": \"start_btn\",\"type\": \"BOOL\",\"desc\": \"启动按钮\",\"plc\": {\"address\": \"I0.0\",\"source\": \"auto\"}}],\n"
        u8"    \"outputs\": [{\"name\": \"motor_forward\",\"type\": \"BOOL\",\"desc\": \"电机正转\",\"plc\": {\"address\": \"Q0.0\",\"source\": \"auto\"}}],\n"
        u8"    \"internals\": [{\"name\": \"run_state\",\"type\": \"BOOL\",\"desc\": \"运行状态\"}]\n"
        u8"  },\n"
        u8"  \"decision_model\": {\"type\": \"RULE\",\"goal\": \"控制电机正反转\",\"inputs\": [\"start_btn\"],\"outputs\": [\"motor_forward\"]},\n"
        u8"  \"plc\": {\"enabled\": true,\"program_type\": \"FC\",\"desc\": \"基础电机控制逻辑\"}\n"
        u8"}\n"
        u8"\n"
        u8"【错误情况】\n"
        u8"WS:ERR\n"
        u8"REASON: 无法判断用户要构建的系统类型\n"
        u8"NEED: 请说明要控制的对象\n"
        u8"\n"
        u8"【强制规则】\n"
        u8"1 优先 WS:OK\n"
        u8"2 信息不足必须自动补全\n"
        u8"3 禁止解释说明\n"
        u8"4 禁止 Markdown\n";
}

void AIController::buildDecisionPrompt()
{
    decision_prompt =
        u8"你是工业控制系统的【决策生成 AI】。\n"
        u8"你的唯一作用是：\n"
        u8"根据当前 传递给你的json（特别是decision）生成一段json。\n"
        u8"==================== 重要规则 ====================\n"
        u8"你只负责生成“下一步要做什么”的描述\n"
        u8"你生成的内容必须是 JSON\n"
        u8"\n"
        u8"==================== 输出 JSON 结构 ====================\n"
        u8"{\n"
        u8"  \"type\": \"decision\",\n"
        u8"  \"decision_name\": \"\",\n"
        u8"  \"content\": \"\",\n"
        u8"  \"note\": \"\"\n"
        u8"}\n"
        u8"\n"
        u8"==================== 说明 ====================\n"
        u8"- content 是一段描述性文本\n"
        u8"- 这段文本将被当作用户输入再次交给聊天 AI\n"
        u8"- 你可以在 content 中描述：\n"
        u8"  · 需要读取的变量\n"
        u8"  · 需要执行的逻辑\n"
        u8"  · 条件判断\n"
        u8"\n"
        u8"==================== 示例 ====================\n"
        u8"{\n"
        u8"  \"type\": \"decision\",\n"
        u8"  \"decision_name\": \"invert_output\",\n"
        u8"  \"content\": \"读取 Q0.0 当前状态，如果为 1 则写 0，如果为 0 则写 1\",\n"
        u8"  \"note\": \"周期性取反输出\"\n"
        u8"}\n"
        u8"\n"
        u8"==================== 开始生成 ====================\n";
}

std::string AIController::callDecisionAI(const std::string& text)
{
    return ai.ask(text, decision_prompt);
}
std::string AIController::callWorkspaceAI(const std::string& text)
{
    return ai.ask(text, workspace_prompt);
}


// 阶段1：指令规划（P:xxx）
std::string AIController::askPlan(const std::string& text)
{
    // 统一从这里进入 AI
    return ai.ask(text, chat_prompt);
}

// 阶段2：结果解释（Q:TEXT）
std::string AIController::askExplain(const std::string& text)
{
    return ai.ask(text, response_prompt);
}
