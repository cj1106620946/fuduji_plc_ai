#include "Workspaceai.h"
#include "aicontroller.h"

#include <fstream>
#include <sstream>

// 构造 / 重置
WorkspaceAI::WorkspaceAI(int AICODE,AIController& aiRef, AITrace& traceRef)
    :ai(aiRef), trace(traceRef),aicode(AICODE)
{
}
void WorkspaceAI::reset()
{
    user_input.clear();
    ai_output.clear();
    workspace_json.clear();
    error_message.clear();
    workspace_ready = false;
}

bool WorkspaceAI::hasWorkspace() const
{
    return workspace_ready && !workspace_json.empty();
}
bool WorkspaceAI::runOnce(const std::string& userInput)
{
    error_message.clear();
    ai_output.clear();
    if (!user_input.empty())
        user_input += u8"\n";
    user_input += userInput;
    if (!callWorkspaceAI(user_input))
    {
        workspace_ready = false;
        return false;
    }
    if (ai_output.find("WS:ERR") != std::string::npos)
    {
        workspace_ready = false;
        error_message = ai_output;
        return false;
    }
    if (ai_output.find("WS:OK") == std::string::npos)
    {
        workspace_ready = false;
        error_message = u8"AI 返回格式错误：缺少 WS:OK";
        return false;
    }
    if (!extractWorkspaceJson(ai_output))
    {
        workspace_ready = false;
        return false;
    }
    workspace_ready = true;
    return true;
}

// 状态判断
bool WorkspaceAI::isReadyForDecision() const
{
    if (!hasWorkspace()) return false;
    return workspace_json.find("\"decision_model\"") != std::string::npos;
}

bool WorkspaceAI::isReadyForPLC() const
{
    if (!hasWorkspace()) return false;
    return workspace_json.find("\"plc\"") != std::string::npos;
}

// Getter
const std::string& WorkspaceAI::getUserInput() const
{
    return user_input;
}

const std::string& WorkspaceAI::getAiRawOutput() const
{
    return ai_output;
}

const std::string& WorkspaceAI::getWorkspaceJson() const
{
    return workspace_json;
}

const std::string& WorkspaceAI::getErrorMessage() const
{
    return error_message;
}

// 文件操作
bool WorkspaceAI::saveToFile(const std::string& path) const
{
    if (!hasWorkspace()) return false;

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) return false;

    ofs.write(workspace_json.data(),
        (std::streamsize)workspace_json.size());
    return true;
}
//读取操作
bool WorkspaceAI::loadFromFile(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs)
    {
        error_message = u8"无法打开 workspace 文件";
        return false;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();

    if (content.empty())
    {
        error_message = u8"workspace 文件为空";
        return false;
    }

    size_t p = content.find('{');
    if (p == std::string::npos)
    {
        error_message = u8"workspace 文件不是 JSON";
        return false;
    }

    std::string j;
    if (!extractFirstJsonObject(content, p, j))
    {
        error_message = u8"workspace JSON 解析失败";
        return false;
    }

    workspace_json = j;
    workspace_ready = true;
    user_input.clear();
    ai_output.clear();
    error_message.clear();
    return true;
}

// 调用 AI（唯一修改点）
bool WorkspaceAI::callWorkspaceAI(const std::string& userInput)
{
   ai_output = ai.workspace(aicode, userInput);

    if (ai_output.empty())
    {
        error_message = u8"AI 返回为空";
        return false;
    }
    return true;
}

// JSON 提取
bool WorkspaceAI::extractWorkspaceJson(const std::string& aiText)
{
    workspace_json.clear();

    size_t pos = aiText.find("WS:JSON");
    if (pos == std::string::npos)
    {
        pos = aiText.find("WS:OK");
        if (pos == std::string::npos)
        {
            error_message = u8"AI 返回格式错误：缺少 WS:OK / WS:JSON";
            return false;
        }
    }

    size_t brace = aiText.find('{', pos);
    if (brace == std::string::npos)
    {
        error_message = u8"AI 返回格式错误：未找到 JSON";
        return false;
    }

    std::string j;
    if (!extractFirstJsonObject(aiText, brace, j))
    {
        error_message = u8"JSON 括号不匹配";
        return false;
    }

    workspace_json = j;
    return true;
}

// JSON 括号提取
bool WorkspaceAI::extractFirstJsonObject(
    const std::string& s,
    size_t startPos,
    std::string& outJson)
{
    outJson.clear();

    if (startPos >= s.size() || s[startPos] != '{')
        return false;

    int depth = 0;
    for (size_t i = startPos; i < s.size(); ++i)
    {
        if (s[i] == '{') depth++;
        else if (s[i] == '}') depth--;

        if (depth == 0)
        {
            outJson = s.substr(startPos, i - startPos + 1);
            return true;
        }
    }
    return false;
}
