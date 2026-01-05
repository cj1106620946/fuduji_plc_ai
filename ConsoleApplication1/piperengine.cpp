#include "piperengine.h"
#include <windows.h>
#include <fstream>

// 写入文本文件
static bool writetextfile(const std::string& path, const std::string& text)
{
    std::ofstream out(path.c_str(), std::ios::binary);
    if (!out.is_open())
        return false;

    out << text;
    out.close();
    return true;
}

bool PiperEngine::init(const std::string& workdir)
{
    m_workdir = workdir;
    m_inputtxt = workdir + "\\input.txt";
    m_outputwav = workdir + "\\output.wav";
    return true;
}

bool PiperEngine::speak(const std::string& utf8text)
{
    if (!writetextfile(m_inputtxt, utf8text))
        return false;

    std::string cmd =
        "cmd.exe /c \""
        "\"\\piper\\piper.exe\" "
        "--model voices\\zh_CN\\huayan\\medium\\zh_CN-huayan-medium.onnx "
        "--config voices\\zh_CN\\huayan\\medium\\zh_CN-huayan-medium.onnx.json "
        "--output_file output.wav < input.txt"
        "\"";

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        NULL,
        cmd.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        m_workdir.c_str(),
        &si,
        &pi
    );

    if (!ok)
        return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    PlaySoundA(m_outputwav.c_str(), NULL, SND_FILENAME | SND_SYNC);
    return true;
}
