#include "workspaceai.h"
#include "aicontroller.h"
#include "aitrace.h"

// 构造
WorkspaceAI::WorkspaceAI(int AICODE, AIController& aiRef, AITrace& traceRef)
    : aicode(AICODE),
    ai(aiRef),
    trace(traceRef)
{
}
std::string WorkspaceAI::runPlcOnce(
    const std::string& user_input,
    std::string& plc_name,
    std::string& ip_address,
    int& rack,
    int& slot,
    std::string& description
)
{
    trace.begin(
        "workspace_plc",
        aicode,
        user_input,
        ai.workspaceplcprompt_get()
    );

    std::string output = callPlcAI(user_input);

    std::string err = parsePlcJson(
        output,
        plc_name,
        ip_address,
        rack,
        slot,
        description
    );

    trace.end(err == "OK", output);
    return err;
}

std::string WorkspaceAI::runSignalOnce(
    const std::string& user_input,
    std::vector<SignalWorkspaceData>& worksignals
)
{
    trace.begin(
        "workspace_signal",
        aicode,
        user_input,
        ai.workspacesigprompt_get()
    );
    std::string output = callSignalAI(user_input);
    std::string err = parseSignalJson(output, worksignals);
    trace.end(err == "OK", output);
    return err;
}

std::string WorkspaceAI::parsePlcJson(
    const std::string& jsonText,
    std::string& plc_name,
    std::string& ip_address,
    int& rack,
    int& slot,
    std::string& description
)
{
    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(jsonText, root))
        return "json parse failed";

    if (!root.isMember("success") || !root["success"].isBool())
        return "missing success";

    if (!root["success"].asBool())
    {
        if (root.isMember("error") && root["error"].isString())
            return root["error"].asString();
        return "unknown error";
    }

    if (!root.isMember("plc_info") || !root["plc_info"].isObject())
        return "missing plc_info";

    Json::Value plc = root["plc_info"];

    plc_name = plc.isMember("plc_name") && plc["plc_name"].isString()
        ? plc["plc_name"].asString() : "";

    ip_address = plc.isMember("ip_address") && plc["ip_address"].isString()
        ? plc["ip_address"].asString() : "";

    rack = plc.isMember("rack") && plc["rack"].isInt()
        ? plc["rack"].asInt() : 0;

    slot = plc.isMember("slot") && plc["slot"].isInt()
        ? plc["slot"].asInt() : 0;

    description = plc.isMember("description") && plc["description"].isString()
        ? plc["description"].asString() : "";

    return "OK";
}

std::string WorkspaceAI::parseSignalJson(
    const std::string& jsonText,
    std::vector<SignalWorkspaceData>& worksignals
)
{
    worksignals.clear();

    Json::Value root;
    Json::Reader reader;

    if (!reader.parse(jsonText, root))
        return "json parse failed";

    if (!root.isMember("success") || !root["success"].isBool())
        return "missing success";

    if (!root["success"].asBool())
    {
        if (root.isMember("error") && root["error"].isString())
            return root["error"].asString();
        return "unknown error";
    }

    if (!root.isMember("worksignals") || !root["worksignals"].isArray())
        return "missing worksignals";

    const Json::Value& arr = root["worksignals"];
    if (arr.empty())
        return "worksignals empty";

    for (Json::ArrayIndex i = 0; i < arr.size(); ++i)
    {
        const Json::Value& sig = arr[i];
        if (!sig.isObject())
            continue;

        SignalWorkspaceData data;

        data.name =
            sig.isMember("name") && sig["name"].isString()
            ? sig["name"].asString()
            : "";

        data.plc_address =
            sig.isMember("plc_address") && sig["plc_address"].isString()
            ? sig["plc_address"].asString()
            : "";

        data.description =
            sig.isMember("description") && sig["description"].isString()
            ? sig["description"].asString()
            : "";

        worksignals.push_back(data);
    }

    if (worksignals.empty())
        return "no valid signal item";

    return "OK";
}

// 调用 PLC Workspace AI
std::string WorkspaceAI::callPlcAI(const std::string& user_input)
{
    return ai.allairun(
        true,
        true,
        aicode,
        "workspace_plc",
        user_input,
        ai.workspaceplcprompt_get()
    );
}

// 调用 Signal Workspace AI
std::string WorkspaceAI::callSignalAI(const std::string& user_input)
{
    return ai.allairun(
        true,
        true,
        aicode,
        "workspace_signal",
        user_input,
        ai.workspacesigprompt_get()
    );
}
