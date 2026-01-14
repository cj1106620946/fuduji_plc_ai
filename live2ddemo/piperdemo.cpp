#include "piperdemo.h"
#include <windows.h>
#include <fstream>
#include <vector>

// 写入文本文件
static bool writetextfile(const std::string& path, const std::string& text)
{
    std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
        return false;

    out << text;
    out.close();
    return true;
}

// 构造
PiperDemo::PiperDemo()
{
}

// 析构
PiperDemo::~PiperDemo()
{
}

// 初始化
bool PiperDemo::init(const std::string& workdir)
{
    m_workdir = workdir;
    m_inputtxt = workdir + "\\input.txt";
    m_outputwav = workdir + "\\output.wav";
    return true;
}

// 生成语音 wav
bool PiperDemo::generate(const std::string& text)
{
    // 写入输入文本
    if (!writetextfile(m_inputtxt, text))
        return false;

    // 保存当前控制台代码页
    UINT originalOutputCP = GetConsoleOutputCP();
    UINT originalInputCP = GetConsoleCP();

    std::string cmd =
        "cmd.exe /c \""
        "\"\\piper\\piper.exe\" "
        "--model voices\\zh_CN\\huayan\\medium\\zh_CN-huayan-medium.onnx "
        "--config voices\\zh_CN\\huayan\\medium\\zh_CN-huayan-medium.onnx.json "
        "--output_file output.wav < input.txt"
        "\"";

    // CreateProcessA 需要可写的命令行缓冲区
    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    BOOL ok = CreateProcessA(
        NULL,
        cmdBuffer.data(),      // 关键修改点：可写 char*
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        m_workdir.c_str(),
        &si,
        &pi
    );

    if (!ok)
    {
        // 恢复代码页
        SetConsoleOutputCP(originalOutputCP);
        SetConsoleCP(originalInputCP);
        return false;
    }

    // 等待 piper.exe 生成 wav
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // 恢复代码页
    SetConsoleOutputCP(originalOutputCP);
    SetConsoleCP(originalInputCP);

    return true;
}

// 获取 wav 路径
std::string PiperDemo::getOutputWav() const
{
    return m_outputwav;
}
