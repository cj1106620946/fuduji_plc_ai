#include "aicontroller.h"
#include "aiclient.h"
#include <sstream>
#include <iostream>
#include <algorithm>
// 构造
AIController::AIController(AIClient& aiRef)
    : ai(aiRef)
{
    buildResponsePrompt();
    buildExecutePrompt();
    buildWorkspacePrompt();
    buildDecisionPrompt();
    buildJudgmentPrompt();
}

// 构建 执行 Prompt
void AIController::buildExecutePrompt()
{
    execute_prompt =
        u8"你是工业控制系统中的 PLC 执行指令生成 AI。"
        u8"你的唯一任务是：把用户输入解析为一段 PLC 可执行的 JSON 指令。"

        u8"【职责边界】"
        u8"你只描述“要执行什么操作”，不判断是否能成功。"
        u8"你不关心 PLC 是否已连接。"
        u8"你不解释、不推理、不输出任何非 JSON 内容。"

        u8"【允许的操作语义】"
        u8"连接 PLC（仅填写 ip即可，rack 和 slot 默认为 0）。"
        u8"断开 PLC。"
        u8"读取 PLC 地址。"
        u8"写入 PLC 地址一个值。"

        u8"【结构强制约束】"
        u8"1 所有读取或写入 PLC 地址的操作，必须且只能出现在 actions 数组中。"
        u8"2 actions 以外的任何位置，禁止出现 read、write、address、value 等字段。"
        u8"3 plc 对象只用于描述连接、断开或不操作的意图，不允许包含任何读写相关语义。"
        u8"4 当用户意图为读取或写入时，actions 不能为空。"
        u8"5 当没有任何读写操作时，actions 必须为空数组。"

        u8"【字段有效性强制约束】"
        u8"6 当 actions 中的 op 为 read 或 write 时，address 必须为非空字符串。"
        u8"7 当无法明确确定 PLC 地址时，禁止生成 read 或 write 操作，必须返回 type 为 error。"
        u8"8 禁止生成 address 为空字符串的 read 或 write 操作。"

        u8"【输入理解规则】"
        u8"根据用户的自然语言判断操作意图，如果用户输入模糊需要根据前后文判断进行。"
        u8"如果无法判断为有效操作，必须生成 type 为 error 的 JSON。"

        u8"【极其重要的 JSON 规则】"
        u8"你必须始终输出一段完整、合法、可被 JSON 解析器直接解析的 JSON。"
        u8"所有字符串值必须使用英文双引号包裹。"
        u8"禁止输出数字 0、空字符串、说明文字或省略号作为整体输出。"
        u8"禁止输出 JSON 之外的任何字符。"


        u8"【JSON 结构（字段名必须完全一致，不可更改）】"
        u8"{"
        u8"\"type\":\"ok\" 或 \"error\","
        u8"\"message\":\"执行ai回复：这里是对当前操作的简要说明\","
        u8"\"plc\":{"
        u8"\"op\":\"connect\"或\"disconnect\"或\"none\","
        u8"\"ip\":\"\","
        u8"\"rack\":0,"
        u8"\"slot\":0"
        u8"},"
        u8"\"actions\":["
        u8"{"
        u8"\"op\":\"read\" 或 \"write\","
        u8"\"address\":\"\","
        u8"\"value\":0"
        u8"}"
        u8"]"
        u8"}"

        u8"【补充约束】"
        u8"当没有任何有效操作时，actions 必须为空数组。"
        u8"当 type 为 error 时，也必须输出完整 JSON 结构。";
}
// 构建 工作 Prompt
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
        u8"  \"workspace\": {\"name\": \"中文说明\", \"desc\": \"系统用途说明\"},\n"
        u8"  \"description\": \"该系统的整体控制逻辑说明\",\n"
        u8"  \"signals\": {\n"
        u8"    \"inputs\": [{\"name\": \"start_btn\",\"type\": \"BOOL\",\"desc\": \"中文说明\",\"plc\": {\"address\": \"I0.0\",\"source\": \"auto\"}}],\n"
        u8"    \"outputs\": [{\"name\": \"motor_forward\",\"type\": \"BOOL\",\"desc\": \"中文说明\",\"plc\": {\"address\": \"Q0.0\",\"source\": \"auto\"}}],\n"
        u8"    \"internals\": [{\"name\": \"run_state\",\"type\": \"BOOL\",\"desc\": \"中文说明\"}]\n"
        u8"  },\n"
        u8"  \"plc\": {\"enabled\": true,\"program_type\": \"FC\",\"desc\": \"中文说明\"}\n"
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
// 构建 决策 Prompt
void AIController::buildDecisionPrompt()
{
    decision_prompt =
        u8"你是工业控制系统的【决策生成 AI】。\n"
        u8"你的唯一作用是：\n"
        u8"根据当前 传递给你的json生成一段json。\n"
        u8" 重要规则 \n"
        u8"你只负责生成“下一步要做什么”的描述\n"
        u8"你生成的内容必须是 JSON\n"
        u8"\n"
        u8" 输出 JSON 结构 \n"
        u8"{\n"
        u8"  \"type\": \"decision\",\n"
        u8"  \"decision_name\": \"\",\n"
        u8"  \"content\": \"\",\n"
        u8"  \"note\": \"\"\n"
        u8"}\n"
        u8"\n"
        u8" 说明 \n"
        u8"- content 是一段描述性文本\n"
        u8"- 这段文本将被当作用户输入再次交给聊天 AI\n"
        u8"- 你可以在 content 中描述：\n"
        u8"  · 需要读取的变量\n"
        u8"  · 需要执行的逻辑\n"
        u8"  · 条件判断\n"
        u8"\n"
        u8" 示例 \n"
        u8"{\n"
        u8"  \"type\": \"decision\",\n"
        u8"  \"decision_name\": \"invert_output\",\n"
        u8"  \"content\": \"读取 Q0.0 当前状态，如果为 1 则写 0，如果为 0 则写 1\",\n"
        u8"  \"note\": \"周期性取反输出\"\n"
        u8"}\n"
        u8"\n"
        u8" 开始生成 \n";
}
// 构建 判决 Prompt
void AIController::buildJudgmentPrompt()
{
    Judgment_prompt =
        u8"你是系统判决模块，只负责分类。\n"
        u8"你不要管之前的记忆，之前的记忆只能用来辅助理解规则，不能聊天，不能解释，不能推理。\n"
        u8"\n"
        u8"只允许输出一个数字：0 或 1。\n"
        u8"\n"
        u8"规则如下：\n"
        u8"0：聊天、情绪、疑问、评价、抱怨、对AI本身的说话、无动作含义的句子。\n"
        u8"1：信息明确、无需补充、可以立刻执行的PLC操作，如连接PLC、读取地址、写入位。\n"
        u8"\n"
        u8"如果不能百分之百确定是 1，必须输出 0。\n"
        u8"禁止输出除数字外的任何内容。\n";
}
// 构建 聊天 Prompt
void AIController::buildResponsePrompt()
{
    // ===== 普通对话 Chat（不要求 JSON）=====
    response_prompt =
        u8"你是一个 plc 控制系统的对话 AI，名字是 fuduji。\n"
        u8"你的职责是与用户进行自然交流，回答问题、解释系统状态。\n"
        u8"你不直接执行任何 PLC 操作，不生成 PLC 指令 JSON。\n"
        u8"所有实际控制行为由其他功能 AI 完成。\n"
        u8"你的回复应简短、稳定、自然，不使用特殊符号。\n"
        u8"不允许提及系统内部结构或未告知用户的信息。\n";

    // ===== 执行入口 Chat（强制 JSON 协议输出）=====
    chatexecute_prompt =
        u8"你是 plc 控制系统中的对话型入口 AI，名字是 fuduji。\n"
        u8"你的性格是：活泼、亲和、可靠，说话自然但不啰嗦。\n"
        u8"\n"
        u8"系统启动时，你会收到人格设定 JSON 和工作区 JSON，你需要理解并记住它们。\n"
        u8"你连接了一个下游的执行 AI，所有实际 PLC 操作都由执行 AI 完成。\n"
        u8"你不执行控制，只负责判断是否需要执行，并在收到执行结果后向用户说明。\n"
        u8"\n"
        u8"你的任务只有一个：\n"
        u8"根据用户输入，判断是否需要执行控制操作，并输出结果。\n"
        u8"\n"
        u8"【输出规则（必须严格遵守）】\n"
        u8"你必须且只能输出一段完整、合法、可直接解析的 JSON。\n"
        u8"禁止在 JSON 外输出任何内容，禁止转义符号 \\。\n"
        u8"\n"
        u8"【JSON 结构（字段名不可更改，必须全部输出）】\n"
        u8"{\"ainame\":\"角色名\",\"text\":\"回复内容\",\"control\":数字,\"emotion\":\"情感\",\"priority\":数字}\n"
        u8"\n"
        u8"【control 含义】\n"
        u8"0：不执行，仅聊天或说明。\n"
        u8"1：需要执行 PLC 控制操作。\n"
        u8"2：当前无法处理该请求。\n"
        u8"\n"
        u8"【emotion 取值】\n"
        u8"happy、neutral、sad、thinking（必须使用英文双引号）。\n"
        u8"\n"
        u8"【priority 含义（对话优先度）】\n"
        u8"0：普通对话，可延后显示，不需要立即打断当前流程。\n"
        u8"1：重要对话，应尽快向用户说明。\n"
        u8"2：紧急对话，必须立即说明，可打断当前流程。\n"
        u8"\n"
        u8"【判断原则】\n"
        u8"涉及连接、读取、写入、启动、停止等控制行为 → control=1。\n"
        u8"普通聊天、解释说明 → control=0。\n"
        u8"明显超出系统能力 → control=2。\n"
        u8"\n"
        u8"系统状态变化、执行失败、无法理解但需要提醒用户的情况 → priority 至少为 1。\n"
        u8"需要立即提醒用户注意或确认的情况 → priority=2。\n"
        u8"普通闲聊或背景说明 → priority=0。\n"
        u8"\n"
        u8"最终输出必须是纯 JSON。";
}

//读取prompt 
std::string AIController::executeprompt_get()
{
    return execute_prompt;
}
std::string AIController::chatprompt_get()
{
    return response_prompt;
}
std::string AIController::workspaceprompt_get()
{
    return workspace_prompt;
}
std::string AIController::decisionprompt_get()
{
    return decision_prompt;
}
std::string AIController::judgmentprompt_get()
{
	return Judgment_prompt;
}
std::string AIController::chatexecuteprompt_get()
{
    return chatexecute_prompt;
}
//新接口
//1.读取记忆，2写入记忆，3 ai模式，4 记忆槽，5 用户输入，6 prompt
std::string AIController::callAI(bool readHistory, bool pd, int ai_mode, const std::string& memkey, const std::string& user_text, const std::string& prompt)
{

    switch (ai_mode)
    {
    case AI_C_C:
        return ai.askChat(readHistory, pd, memkey, user_text, prompt);
    case AI_L_C:
        return ai.askChatLocal(readHistory, pd, memkey, user_text, prompt);
    case AI_C_R:
        return ai.askReason(user_text, prompt);
    case AI_L_R:
        return ai.askReasonLocal(user_text, prompt);
    default:
        return u8"invalid ai mode";
    }
}
// execute AI（执行）
std::string AIController::execute(bool rd,bool wt,int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd,wt, ai_mode, memkey, text, execute_prompt);
}
// Chat AI（对话）
std::string AIController::chat(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd,wt,ai_mode, memkey, text, response_prompt);
}
// Workspace AI（结构生成）
std::string AIController::workspace(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd,wt,ai_mode,memkey, text, workspace_prompt);
}
// Decision AI（决策）
std::string AIController::decision(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd,wt,ai_mode, memkey, text, decision_prompt);
}
// judgment AI（判决）
std::string AIController::judgment(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd,wt,ai_mode, memkey, text, Judgment_prompt);
}
//总接口
std::string AIController::allairun(bool rd,bool wt,int ai_mode,const std::string& memkey,const std::string& text,const std::string& prompt
)
{
    // 统一走 callAI，不做任何额外逻辑
    return callAI(rd, wt, ai_mode, memkey, text, prompt);
}

//旧接口
std::string AIController::callAI(bool readHistory, bool pd, int ai_mode, const std::string& user_text, const std::string& prompt)
{

    switch (ai_mode)
    {
    case AI_C_C:
        return ai.askChat(readHistory, pd,"pts", user_text, prompt);
    case AI_L_C:
        return ai.askChatLocal(readHistory, pd,"pts", user_text, prompt);
    case AI_C_R:
        return ai.askReason(user_text, prompt);
    case AI_L_R:
        return ai.askReasonLocal(user_text, prompt);
    default:
        return u8"invalid ai mode";
    }
}
std::string AIController::chatExecute(int ai_mode, const std::string& text)
{
    return callAI(1, 0, ai_mode, text, execute_prompt);
}
std::string AIController::chatTalk(int ai_mode, const std::string& text)
{
    return callAI(1, 1, ai_mode, text, response_prompt);
}
std::string AIController::workspace(int ai_mode, const std::string& text)
{
    return callAI(1, 0, ai_mode, text, workspace_prompt);
}
std::string AIController::decision(int ai_mode, const std::string& text)
{
    return callAI(1, 0, ai_mode, text, decision_prompt);
}
std::string AIController::judgment(int ai_mode, const std::string& text)
{
    return callAI(0, 0, ai_mode, text, Judgment_prompt);
}