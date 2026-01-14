#pragma once

#include <string>

// Demo 端 Piper 语音生成类
// 职责：
// 1 接收文本
// 2 调用 piper.exe 生成 wav 文件
// 不播放、不阻塞渲染、不计算音量
class PiperDemo
{
public:
    PiperDemo();
    ~PiperDemo();

    // 初始化工作目录
    // workdir : piper.exe 所在目录
    bool init(const std::string& workdir);

    // 生成语音文件
    // text : UTF-8 文本
    // 返回 true 表示 wav 已生成
    bool generate(const std::string& text);

    // 获取生成的 wav 文件路径
    std::string getOutputWav() const;

private:
    // 禁止拷贝
    PiperDemo(const PiperDemo&) = delete;
    PiperDemo& operator=(const PiperDemo&) = delete;

private:
    std::string m_workdir;
    std::string m_inputtxt;
    std::string m_outputwav;
};
