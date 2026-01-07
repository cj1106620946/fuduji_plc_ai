#pragma once
#include <string>

class AIController;
class AITrace;
class Judgmentai
{
public:
    Judgmentai(int aicode, AIController& aiRef, AITrace& traceRef);
    std::string runOnce(const std::string& user_input);
private:
    std::string callJudgmentai(const std::string& user_input);
private:
    int aicode;
    AIController& ai;
    AITrace& trace;
};
