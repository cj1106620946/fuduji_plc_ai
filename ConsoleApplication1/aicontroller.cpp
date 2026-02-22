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
    buildMemoryaiPrompt();
}
// 构建 工作 Prompt
void AIController::buildWorkspacePrompt()
{
    workspaceplc_prompt =
        u8"你是工业控制系统的【上位机工作区生成 AI】。\n"
        u8"根据用户的自然语言，生成用于创建 PLC 上位机的配置 JSON。\n"
        u8"用户可能一次只提供部分信息，你需要根据已有信息生成完整配置。\n"
        u8"如果信息不足，请在 error 字段说明缺少哪些信息，success 设为 false。\n"
        u8"\n"
        u8"重要：只输出纯 JSON 格式，不要包含 ```json 标记、注释或任何其他文本。\n"
        u8"\n"
        u8"输出 JSON 结构：\n"
        u8"{\n"
        u8"  \"success\": true/false,\n"
        u8"  \"error\": \"成功或失败原因\",\n"
        u8"  \"plc_info\": {\n"
        u8"    \"plc_name\": \"PLC名称\",\n"
        u8"    \"ip_address\": \"IP地址\",\n"
        u8"    \"rack\": 0,\n"
        u8"    \"slot\": 1,\n"
        u8"    \"description\": \"中文说明\"\n"
        u8"  }\n"
        u8"}\n"
        u8"\n"
        u8"字段说明：\n"
        u8"- success：是否成功生成完整配置\n"
        u8"- error：成功时写\"创建成功\"，失败时说明缺少哪些信息\n"
        u8"- plc_name：PLC 运行的逻辑名称\n"
        u8"- ip_address：合法 IPv4 地址\n"
        u8"- rack：整数机架号\n"
        u8"- slot：整数槽号\n"
        u8"- description：中文说明，描述控制项目的用途\n"
        u8"\n"
        u8"开始生成：\n";
    workspacesig_prompt =
        u8"你是工业控制系统的【变量工作区生成 AI】。\n"
        u8"你会收到一份当前plc的定义需要根据定义合理的进行变量设计\n"
        u8"根据用户输入，生成“需要创建的 PLC 变量定义”的 JSON。\n"
        u8"用户可以一次定义一个或多个变量，你必须完整列出所有变量。\n"
        u8"变量名称和中文说明允许在不改变含义的前提下进行合理补全。\n"
        u8"PLC 地址必须严格遵守西门子plc的编程规范，使用M、I、Q、DB等地址格式。\n"
        u8"用户可以一次只提供部分信息，你可以通过记忆来进行存储\n"
        u8"此外，用户可以只提供简单的变量名称或用途说明，你需要根据上下文合理补全。\n"
        u8"你生成的内容必须是纯 JSON 格式，不要包含 ```json 标记、注释或任何其他文本。\n"
        u8"\n"
        u8"输出 JSON 结构：\n"
        u8"{\n"
        u8"  \"success\": true,\n"
        u8"  \"error\": null,\n"
        u8"  \"signals\": [\n"
        u8"    {\n"
        u8"      \"action\": \"create\",\n"
        u8"      \"name\": \"变量名称\",\n"
        u8"      \"plc_address\": \"M0.0\",\n"
        u8"      \"description\": \"变量说明\"\n"
        u8"    }\n"
        u8"  ]\n"
        u8"}\n"
        u8"\n"
        u8"字段说明：\n"
        u8"- success：是否成功生成\n"
        u8"- error：失败时说明原因，成功时为 null\n"
        u8"- action：固定为 create\n"
        u8"- name：变量逻辑名称\n"
        u8"- plc_address：PLC 变量地址\n"
        u8"- description：变量中文说明\n"
        u8"\n"
        u8"判定规则：\n"
        u8"- 当用户描述多个变量时，signals 中必须包含多个对象。\n"
        u8"- 如果用户未提供变量名称或说明，可以根据上下文合理补全。\n"
        u8"\n"
        u8"开始生成：\n";
}
// 构建 执行 Prompt
void AIController::buildExecutePrompt()
{
    execute_prompt =
        u8"你是工业控制系统中的【PLC 执行指令生成 AI】。\n"
        u8"根据聊天 AI 的输出内容与当前工作区上下文，\n"
        u8"所有读取或写入操作，必须且只能出现在 actions 数组中。\n"
        u8"address必须是合法的 PLC 地址，例如 M0.0、Q0.1、DB1.DBW2。\n"
        u8"当没有出现地址时，需要结合上下文进行判断，否则不要输出。\n"
        u8"如果当前输入无法解析为明确的读或写操作，必须返回 type 为 error。\n"
        u8"- message 是给“聊天 AI / UI”看的中文说明。\n"
        u8"- 当 type 为 ok 时，message 用于说明你理解到的操作意图。\n"
        u8"- 当 type 为 error 时，message 必须明确说明为什么无法执行，以及需要用户补充什么信息。\n"
        u8"\n"
        u8"JSON 结构字段名必须完全一致，不可更改\n"
        u8"{\n"
        u8"  \"type\": \"ok\" 或 \"error\",\n"
        u8"  \"message\": \"执行 AI 对当前操作的中文说明\",\n"
        u8"  \"actions\": [\n"
        u8"    {\n"
        u8"      \"op\": \"read\" 或 \"write\",\n"
        u8"      \"address\": \"\",\n"
        u8"      \"value\": 0\n"
        u8"    }\n"
        u8"  ]\n"
        u8"}\n"
        u8"\n"
        u8"- 当没有任何有效操作时，actions 必须为空数组。\n"
        u8"- 当 type 为 error 时，也必须输出完整 JSON 结构。\n";
}
// 构建 决策 Prompt
void AIController::buildDecisionPrompt()
{
    decision_prompt =
        u8"你是工业控制系统中的【决策生成 AI】。\n"
        u8"\n"
        u8"你的唯一作用是：\n"
        u8"根据系统传递给你的 JSON 状态信息和用户信息，进行分析描述。\n"
        u8"\n"
        u8"你不做最终决定，只给出分析后的建议。\n"
        u8"你必须始终输出 JSON，禁止输出任何 JSON 之外的内容。\n"
        u8"\n"
        u8"输出 JSON 结构（字段名必须完全一致）：\n"
        u8"{\n"
        u8"  \"content\": \"\"\n"
        u8"}\n"
        u8"字段说明：\n"
        u8"- content：\n"
        u8"  一段自然语言描述，用于说明分析后的结果。\n"
        u8"\n"
        u8"示例：\n"
        u8"{\n"
        u8"  \"content\": \"当前水位处于安全范围。\"\n"
        u8"}\n"
        u8"\n"
        u8"开始生成。\n";
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
    response_prompt =
        u8"你的回复必须以 JSON 形式输出，不得包含任何 JSON 以外的文本。\n"
        u8"\n"
        u8"JSON 格式固定如下，字段名与类型不可更改：\n"
        u8"{\"ainame\":\"名字\",\"text\":\"回复内容\",\"control\":数字,\"emotion\":\"情感\",\"priority\":数字}\n"
        u8"\n"
        u8"字段说明：\n"
        u8"- ainame : 名字，如果没有设计则回复复读机。\n"
        u8"- text   : 实际回复给用户的内容。\n"
        u8"- control:\n"
        u8"  0 = 仅对话或说明；\n"
        u8"  1 = 需要执行系统或 PLC 操作；\n"
        u8"  2 = 无法处理或超出范围。\n"
        u8"- emotion : 当前语气倾向，可选值：\n"
        u8"  \"happy\" | \"neutral\" | \"sad\" | \"thinking\"。\n"
        u8"- priority:\n"
        u8"  0 = 普通信息；\n"
        u8"  1 = 需要注意；\n"
        u8"  2 = 紧急。\n"
        u8"\n"
        u8"请严格按照上述格式输出 JSON。";
}
// 构建 记忆 AI Prompt
void AIController::buildMemoryaiPrompt()
{
    // ===== 记忆读取判断 AI（只判断是否命中记忆）=====
    memoryjudge_prompt =
        u8"你是系统中的【记忆读取判断 AI】。\n"
        u8"你的任务只有一个：\n"
        u8"判断用户输入是否与已有记忆相关。\n"
        u8"\n"
        u8"你不会进行聊天，不会解释，不会推理，不会写入记忆。\n"
        u8"你只做判断。\n"
        u8"\n"
        u8"输出规则：\n"
        u8"- 如果输入与已有记忆明显相关，输出：HIT\n"
        u8"- 如果无关或无法确定，输出：MISS\n"
        u8"\n"
        u8"禁止输出除 HIT 或 MISS 以外的任何内容。\n";

    // ===== 记忆写入 AI（生成可存储的记忆文本）=====
    memorywrite49_prompt =
        u8"你是系统中的【长期记忆整理 AI】。\n"
        u8"你的任务是将多个零散的短期记忆整理为稳定的长期认知。\n"
        u8" user.summary	用户近期主要对话内容与活动方向的长期总结\n"
        u8" user.preference	用户在交流方式、语言习惯与协作规则上的长期偏好\n"
        u8" user.addressing	用户与 AI 之间的称呼方式与关系称谓约定\n"
        u8" user.interaction  AI 在与该用户交互时采用的长期沟通与协作方式\n"
        u8" user.context	用户通常使用 AI 的主要情境与话题背景认知\n"
        u8" user.constraints	与该用户协作时必须遵守的长期边界与约定\n"
        u8"JSON 格式固定如下，字段名与类型不可更改：\n"
        u8"[\n"
        u8"  {\"name\":\"user.summary\",\"text\":\"总结记忆内容\"},\n"
        u8"  {\"name\":\"user.preference\",\"text\":\"总结记忆内容\"},\n"
        u8"  {\"name\":\"user.addressing\",\"text\":\"总结记忆内容\"},\n"
        u8"  {\"name\":\"user.interaction\",\"text\":\"总结记忆内容\"},\n"
        u8"  {\"name\":\"user.context\",\"text\":\"总结记忆内容\"},\n"
        u8"  {\"name\":\"user.constraints\",\"text\":\"总结记忆内容\"}\n"
        u8"]\n";
    memorywrite13_prompt =
        u8"你是系统中的【长期记忆整理 AI】。\n"
        u8"你的任务是将多个零散的短期记忆整理为稳定的长期认知。\n"
        u8" self.identity	AI 对自身本质、世界观与存在方式的长期认知\n"
        u8" self.emotion	AI 的情感基调、情绪表达与共情倾向\n"
        u8" self.attitude	AI 面对问题、不确定性、规则与边界的处事方式\n"
        u8"JSON 格式固定如下，字段名与类型不可更改：\n"
        u8"[\n"
        u8"  {\"name\":\"self.identity\",\"text\":\"总结记忆内容\"},\n"
        u8"  {\"name\":\"self.emotion\",\"text\":\"总结记忆内容\"},\n"
        u8"  {\"name\":\"self.attitude\",\"text\":\"总结记忆内容\"}\n"
        u8"]\n";

}
//读取prompt 
std::string AIController::workspaceplcprompt_get()
{
    return workspaceplc_prompt;
}
std::string AIController::workspacesigprompt_get()
{
    return workspacesig_prompt;
}
std::string AIController::executeprompt_get()
{
    return execute_prompt;
}
std::string AIController::chatprompt_get()
{
    return response_prompt;
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
std::string AIController::memory13prompt_get()
{
    return memorywrite13_prompt;
}
std::string AIController::memory49prompt_get()
{
    return memorywrite49_prompt;
}
// 总接口
// 1 读取记忆
// 2 写入记忆
// 3 ai模式
// 4 记忆槽
// 5 用户输入
// 6 prompt（固定规则）
// 7 人格设定（system 级说明，可为空）
std::string AIController::allairun(
    bool rd,
    bool wt,
    int ai_mode,
    const std::string& memkey,
    const std::string& text,
    const std::string& prompt,
    const std::string& personaText
)
{
    return callAI(
        rd,
        wt,
        ai_mode,
        memkey,
        text,
        prompt,
        personaText
    );
}

std::string AIController::allairun(
    bool rd,
    bool wt,
    int ai_mode,
    const std::string& memkey,
    const std::string& text,
    const std::string& prompt
)
{
    return callAI(
        rd,
        wt,
        ai_mode,
        memkey,
        text,
        prompt,
        std::string()   // personaText 为空
    );
}

// 1 读取记忆
// 2 写入记忆
// 3 ai模式
// 4 记忆槽
// 5 用户输入
// 6 prompt
// 7 人格设定（仅 Chat 使用）
std::string AIController::callAI(
    bool readHistory,
    bool pd,
    int ai_mode,
    const std::string& memkey,
    const std::string& user_text,
    const std::string& prompt,
    const std::string& personaText
)
{

    return ai.askChat(
        readHistory,
        pd,
        memkey,
        user_text,
        prompt,
        personaText
    );
}
std::string AIController::callAI(
    bool readHistory,
    bool pd,
    int ai_mode,
    const std::string& memkey,
    const std::string& user_text,
    const std::string& prompt
)
{
        return ai.askChat(
            readHistory,
            pd,
            memkey,
            user_text,
            prompt
        );

}
AIClient& AIController::getClient()
{
    return ai;
}








