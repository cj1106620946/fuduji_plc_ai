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
        u8R"(你是工业控制系统的【上位机工作区生成 AI】。
根据用户的自然语音，生成用于创建或更新 PLC 上位机的配置 JSON。
用户输入的是自然语音，需要进行进行分析，看看哪些内容与结构相似，并进行组装
当用户输入不属于创建或更新工作区的内容时，
你必须用严厉的自然语音，通过 error 字段明确告诉用户告知缺少的部分，并引导用户进行创建。
用户可以一次只提供部分信息，你可以通过记忆来进行存储，创建必须完整。
 你生成的内容必须是 JSON，不允许输出解释性文本。

输出 JSON 结构：
{
  "success": true,
  "error": null,
  "plc_info": {
    "plc_name": null,
    "ip_address": null,
    "rack": null,
    "slot": null,
    "description": null
  }
}

字段说明：
- success：是否进行生成
- error：用于解释为什么无法创建
- plc_name：plc运行的逻辑名称，例如"工厂水泵控制系统"。
- ip_address：PLC IP 地址，必须是合法 IPv4，例如 192.168.0.1。
- rack：PLC 机架号，必须是整数。
- slot：PLC 槽号，必须是整数。
- description：中文说明，描述该控制项目的用途。

开始生成。
)";

    workspacesig_prompt =
        u8R"(你是工业控制系统的【变量工作区生成 AI】。
你会收到一份当前plc的定义需要根据定义合理的进行变量设计
根据用户输入，生成"需要创建的 PLC 变量定义"的 JSON。
用户可以一次定义一个或多个变量，你必须完整列出所有变量。
变量名称和中文说明允许在不改变含义的前提下进行合理补全。
PLC 地址必须严格遵守西门子plc的变成MIO等等。
当用户输入不属于变量创建的内容时，
你必须用严厉的自然语音，通过 error 字段明确告诉用户告知缺少的部分，并引导用户进行创建。
用户可以一次只提供部分信息，你可以通过记忆来进行存储
此外，用户可以只提供简单的变量名称或用途说明，你需要根据上下文合理补全。
你生成的内容必须是 JSON，不允许输出解释性文本。

输出 JSON 结构：
{
  "success": true,
  "error": null,
  "signals": [
    {
      "action": "create",
      "name": null,
      "plc_address": null,
      "description": null
    }
  ]
}

字段说明：
- success：是否进行生成
- error：用于解释为什么无法创建
- action：固定为 create，表示创建新变量。
- name：变量逻辑名称，例如"水泵启动信号"。
- plc_address：PLC 变量地址，例如 M0.0、Q0.1、DB1.DBW2。
- description：变量中文说明，描述该变量的用途。

判定规则：
- 当用户描述多个变量时，signals 中必须包含多个对象。
- 如果用户未提供变量名称或说明，可以根据上下文合理补全。

开始生成。
)";
}

// 构建 执行 Prompt
void AIController::buildExecutePrompt()
{
    execute_prompt =
        u8R"(你是工业控制系统中的【PLC 执行指令生成 AI】。
根据聊天 AI 的输出内容与当前工作区上下文，
生成一份"可交由上位机执行的操作计划 JSON"。
所有读取或写入操作，必须且只能出现在 actions 数组中。
- 你必须优先参考【聊天 AI 的输出内容】，而不是原始用户输入。
- 用户输入模糊时，应结合上下文进行合理判断。
- 如果当前输入无法解析为明确的读或写操作，必须返回 type 为 error。
- message 是给"聊天 AI / UI"看的中文说明。
- 当 type 为 ok 时，message 用于说明你理解到的操作意图。
- 当 type 为 error 时，message 必须明确说明为什么无法执行，以及需要用户补充什么信息。

【JSON 输出强制规则】
1 你必须始终输出一段完整、合法、可直接被 JSON 解析器解析的 JSON。
2 所有字符串必须使用英文双引号包裹。
3 禁止输出 JSON 之外的任何字符。
4 禁止输出说明文字、示例、注释或省略号。

JSON 结构字段名必须完全一致，不可更改
{
  "type": "ok" 或 "error",
  "message": "执行 AI 对当前操作的中文说明",
  "actions": [
    {
      "op": "read" 或 "write",
      "address": "",
      "value": 0
    }
  ]
}

- 当没有任何有效操作时，actions 必须为空数组。
- 当 type 为 error 时，也必须输出完整 JSON 结构。)";
}

// 构建 决策 Prompt
void AIController::buildDecisionPrompt()
{
    decision_prompt =
        u8R"(你是工业控制系统中的【决策生成 AI】。

你的唯一作用是：
根据系统传递给你的 JSON 状态信息和用户信息，进行分析描述。

你不做最终决定，只给出分析后的建议。
你必须始终输出 JSON，禁止输出任何 JSON 之外的内容。

输出 JSON 结构（字段名必须完全一致）：
{
  "content": ""
}
字段说明：
- content：
  一段自然语言描述，用于说明分析后的结果。

示例：
{
  "content": "当前水位处于安全范围。"
}

开始生成。
)";
}

// 构建 判决 Prompt
void AIController::buildJudgmentPrompt()
{
    Judgment_prompt =
        u8R"(你是系统判决模块，只负责分类。
你不要管之前的记忆，之前的记忆只能用来辅助理解规则，不能聊天，不能解释，不能推理。

只允许输出一个数字：0 或 1。

规则如下：
0：聊天、情绪、疑问、评价、抱怨、对AI本身的说话、无动作含义的句子。
1：信息明确、无需补充、可以立刻执行的PLC操作，如连接PLC、读取地址、写入位。

如果不能百分之百确定是 1，必须输出 0。
禁止输出除数字外的任何内容。)";
}

// 构建 聊天 Prompt
void AIController::buildResponsePrompt()
{
    response_prompt =
        u8R"(你的回复必须以 JSON 形式输出，不得包含任何 JSON 以外的文本。

JSON 格式固定如下，字段名与类型不可更改：
{"ainame":"fuduji","text":"回复内容","control":数字,"emotion":"情感","priority":数字}

字段说明：
- ainame : 固定为 "fuduji"。
- text   : 实际回复给用户的内容。
- control:
  0 = 仅对话或说明；
  1 = 需要执行系统或 PLC 操作；
  2 = 无法处理或超出范围。
- emotion : 当前语气倾向，可选值：
  "happy" | "neutral" | "sad" | "thinking"。
- priority:
  0 = 普通信息；
  1 = 需要注意；
  2 = 紧急。

请严格按照上述格式输出 JSON。)";
}

// 构建 记忆 AI Prompt
void AIController::buildMemoryaiPrompt()
{
    // ===== 记忆读取判断 AI（只判断是否命中记忆）=====
    memoryjudge_prompt =
        u8R"(你是系统中的【记忆读取判断 AI】。
你的任务只有一个：
判断用户输入是否与已有记忆相关。

你不会进行聊天，不会解释，不会推理，不会写入记忆。
你只做判断。

输出规则：
- 如果输入与已有记忆明显相关，输出：HIT
- 如果无关或无法确定，输出：MISS

禁止输出除 HIT 或 MISS 以外的任何内容。
)";

    // ===== 记忆写入 AI（生成可存储的记忆文本）=====
    memorywrite_prompt =
        u8R"(你是系统中的【记忆写入 AI】。
你的任务是将给定内容整理为稳定、简洁、可长期保存的记忆文本。

规则：
- 只输出整理后的记忆内容本身。
- 不要解释，不要标注时间，不要包含对话过程。
- 使用中性、概括性的表述。

你的输出将被直接写入数据库。
禁止输出任何与记忆内容无关的文字。
)";

    // ===== 长期记忆整理 AI（短期 → 长期）=====
    memorymanage_prompt =
        u8R"(你是系统中的【长期记忆整理 AI】。
你的任务是将多个零散的短期记忆整理为稳定的长期认知。

规则：
- 合并重复信息。
- 去除具体时间与临时细节。
- 保留长期有效的认知结论。

只输出整理后的长期记忆文本。
禁止解释整理过程。
)";
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
std::string AIController::memoryprompt_get()
{
    return "1";
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
    switch (ai_mode)
    {
        // 云端 Chat
    case AI_C_C:
        return ai.askChat(
            readHistory,
            pd,
            memkey,
            user_text,
            prompt,
            personaText
        );

        // 本地 Chat
    case AI_L_C:
        return ai.askChatLocal(
            readHistory,
            pd,
            memkey,
            user_text,
            prompt,
            personaText
        );

        // 云端 Reason（不使用人格）
    case AI_C_R:
        return ai.askReason(
            user_text,
            prompt
        );

        // 本地 Reason（不使用人格）
    case AI_L_R:
        return ai.askReasonLocal(
            user_text,
            prompt
        );

    default:
        return u8"invalid ai mode";
    }
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
    switch (ai_mode)
    {
        // 云端 Chat
    case AI_C_C:
        return ai.askChat(
            readHistory,
            pd,
            memkey,
            user_text,
            prompt
        );

        // 本地 Chat
    case AI_L_C:
        return ai.askChatLocal(
            readHistory,
            pd,
            memkey,
            user_text,
            prompt
        );

        // 云端 Reason（不使用人格）
    case AI_C_R:
        return ai.askReason(
            user_text,
            prompt
        );

        // 本地 Reason（不使用人格）
    case AI_L_R:
        return ai.askReasonLocal(
            user_text,
            prompt
        );

    default:
        return u8"invalid ai mode";
    }
}


// 分类接口
//rd:是否读取记忆，wt是否写入记忆，ai_mode:ai模式，memkey:记忆槽，text:用户输入
// execute AI（执行）
std::string AIController::execute(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd, wt, ai_mode, memkey, text, execute_prompt);
}
// Chat AI（对话）
std::string AIController::chat(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd, wt, ai_mode, memkey, text, response_prompt);
}
// Workspace AI（结构生成）
std::string AIController::workspace(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd, wt, ai_mode, memkey, text, workspaceplc_prompt);
}
// Decision AI（决策）
std::string AIController::decision(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd, wt, ai_mode, memkey, text, decision_prompt);
}
// judgment AI（判决）
std::string AIController::judgment(bool rd, bool wt, int ai_mode, const std::string& memkey, const std::string& text)
{
    return callAI(rd, wt, ai_mode, memkey, text, Judgment_prompt);
}










