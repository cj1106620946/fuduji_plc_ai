#pragma once

#include <string>
#include <vector>
#include <cstdint>

// AI 调用追踪与记录工具类
// 职责：
// 1 记录一次 AI 调用的完整生命周期
// 2 记录 debug / error 文本
// 3 在调用结束时，将完整过程保存为可读文本文件
// 不参与任何 AI 逻辑、不做解析、不做判断
class AITrace
{
public:
    AITrace();
    ~AITrace();
    // 设置根目录，例如：ai_trace
    // 实际文件将根据 ai_mode 写入子目录
    void setRootDir(const std::string& dir);
    // AI 调用开始
    // role_name   : chat / execute / workspace / decision
    // ai_mode     : 1 云端Chat / 2 本地Chat / 3 云端Reason / 4 本地Reason
    // user_text   : 用户原始输入
    // prompt_text : 本次调用使用的 prompt
    void begin(
        const std::string& role_name,
        int ai_mode,
        const std::string& user_text,
        const std::string& prompt_text
    );
    // 记录调试 / 错误信息
    // 任意可读字符串，按顺序保存
    void debug(const std::string& text);

    // AI 调用结束
    // ok          : 本次调用是否成功
    // output_text : AI 返回的原始文本
    // 调用该接口时会立即写文件
    void end(
        bool ok,
        const std::string& output_text
    );

    // 清空当前 trace 状态（不写文件）
    void reset();

private:
    // 单条 debug 记录
    struct DebugItem
    {
        uint64_t timestamp = 0;
        std::string text;
    };

    // 当前 trace 状态
    struct TraceContext
    {
        bool active = false;
        std::string role_name;//调用ai名称
        int ai_mode = 0;//选用模型
        std::string user_text;//原始输入内容
        std::string prompt_text;//prompt内容
        std::string output_text;//ai输出类容
        bool ok = false;//执行成功判断位

        uint64_t begin_time = 0;
        uint64_t end_time = 0;

        std::vector<DebugItem> debug_logs;
    };

private:
    // 当前正在记录的 trace
    TraceContext ctx;
    // trace 根目录
    std::string root_dir;
private:
    // 获取当前时间戳（毫秒）
    static uint64_t nowMs();

    // 根据 ai_mode 获取子目录名
    static std::string modeDir(int ai_mode);

    // 根据时间生成文件名
    static std::string makeFileName(uint64_t timestamp);

    // 将当前 trace 写入文件
    void writeToFile();
};
