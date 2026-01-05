#pragma once
#include <string>

class PiperEngine
{
public:
    bool init(const std::string& workdir);
    bool speak(const std::string& utf8text);

private:
    std::string m_workdir;
    std::string m_inputtxt;
    std::string m_outputwav;
};
