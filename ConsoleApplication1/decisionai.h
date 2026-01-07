#pragma once
#include <string>
#include "aicontroller.h"
#include "aitrace.h"

class DecisionAI
{
public:
    DecisionAI(int AICODE, AIController& aiRef, AITrace& traceRef);
    std::string runOnce(const std::string& user_input); // ÉùÃ÷
    void feedExternalData(const std::string& data);
private:
    AIController& ai;
    AITrace& trace;
    int aicode;
};
