#pragma once
#include <string>
#include <json/json.h>

class AIController;
class AITrace;

struct PlcWorkspaceData
{
    std::string plc_name;
    std::string ip_address;
    int rack;
    int slot;
    std::string description;
};

struct SignalWorkspaceData
{
    std::string name;
    std::string plc_address;
    std::string description;
};

class WorkspaceAI
{
public:
    explicit WorkspaceAI(int AICODE, AIController& aiRef, AITrace& traceRef);

    std::string runPlcOnce(
        const std::string& user_input,
        std::string& plc_name,
        std::string& ip_address,
        int& rack,
        int& slot,
        std::string& description
    );

    std::string runSignalOnce(
        const std::string& user_input,
        std::vector<SignalWorkspaceData>& worksignals
    );
    std::string callPlcAI(const std::string& user_input);
    std::string callSignalAI(const std::string& user_input);

private:
    std::string parsePlcJson(
        const std::string& jsonText,
        std::string& plc_name,
        std::string& ip_address,
        int& rack,
        int& slot,
        std::string& description
    );

    std::string parseSignalJson(
        const std::string& jsonText,
        std::vector<SignalWorkspaceData>& worksignals
    );


private:
    int aicode;
    AIController& ai;
    AITrace& trace;
};
