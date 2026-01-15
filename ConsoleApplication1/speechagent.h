// speechagent.h
#pragma once

#include <string>
#include <vector>
#include <deque>
#include <mutex>

// speechagent
// 提供三套并行能力：
// 1 getText：一体式 语音→文本（独立存在）
// 2 push：只录制一句语音，放入队列
// 3 pop：取出一条语音并进行处理
class speechagent
{
public:
    struct SpeechParams
    {
        int device_index = 1;
        // 麦克风设备序号
        // 0  表示系统默认输入设备
        // 1+ 表示指定的输入设备编号
        // 如果识别不到声音，优先检查这个值是否选对设备

        int channels = 1;
        // 采集声道数
        // 1 表示单声道（推荐）
        // 2 表示立体声（内部会混合为单声道）
        // 一般不建议改，除非你明确知道设备只支持双声道

        int sample_rate = 48000;
        // 采集采样率（Hz）
        // 当前设计固定为 48000
        // Whisper 推理前会自动降采样到 16000
        // 不建议修改，修改后需要同步调整整条音频链路

        float vad_threshold = 0.08f;
        // 静音判定阈值（RMS）
        // RMS < 该值 → 判定为静音
        // 值越大，越不容易触发“开始说话”
        // 环境噪声较大时可以适当调高（例如 0.1 ~ 0.15）

        int start_hit = 1;
        // 开始说话确认次数
        // 连续多少次检测满足开始条件，才进入“说话中”状态
        // 实际等待时间 ≈ start_hit × check_ms
        // 用于防止瞬时噪声误触发

        float start_rms_min = 0.08f;
        // 开始说话的最低 RMS 强度
        // RMS 必须 >= 该值，才允许触发说话
        // 用于过滤环境底噪、空调声等
        // 通常与 vad_threshold 保持接近

        int end_silence_ms = 500;
        // 结束说话的静音判定时长（ms）
        // 进入说话状态后，连续静音达到该值就结束录音
        // 值越大，允许说话中的停顿越长
        // 对“自然停顿多”的用户可适当调大（如 600~800）

        int check_ms = 100;
        // 检测间隔（ms）
        // 每隔多少毫秒检测一次 RMS
        // 越小 → 反应越快，但 CPU 占用越高
        // 一般 50~100 是比较平衡的范围

        int max_record_ms = 8000;
        // 最长等待时间（ms）
        // 一直检测不到说话时的超时上限
        // 防止录音线程永久阻塞
        // 可按交互场景调整，通常无需频繁修改

        int debug_rms_print_ms = 500;
        // RMS 调试打印间隔（ms）
        // 只影响日志输出频率
        // 不影响任何判定逻辑
        // 调试完成后可以调大或关闭

        const char* modlname = "ggml-small.bin";
        // Whisper 模型文件名或路径
        // 可使用相对路径或绝对路径
        // 模型越大，识别准确率越高，但速度和内存占用越大

        int prerpll = 200;
        // 触发说话时向前回退的时间（ms）
        // 用于保留语音前导，防止第一个字被截断
        // 一般 150~300 之间较为自然

        const char* language = "zh";
        // Whisper 识别语言
        // "auto" 表示自动检测语言
        // 也可以固定为 "zh"、"ja" 等
        // 固定语言可略微提升识别稳定性

        float temperature = 0.2f;
        // Whisper 输出随机性参数
        // 0.0 表示最稳定、最保守
        // 值越大，输出越容易发散
        // 对指令型、对话型系统强烈建议保持 0.0

        bool suppress_blank = true;
        // 是否抑制空白输出
        // true  可减少无意义的空结果
        // 对实时语音交互基本应保持开启

        int n_threads = 0;
        // Whisper 使用的线程数
        // >0 表示固定线程数
        // 0  表示自动（由程序根据 CPU 核心数计算）
        // 线程数越多，识别越快，但 CPU 占用越高

        bool print_progress = false;
        // 是否打印 Whisper 识别进度信息
        // 仅用于调试长音频
        // 实时交互场景建议关闭

        bool print_special = false;
        // 是否打印特殊标记
        // 例如 [MUSIC] [NOISE] 等
        // 普通文本对话场景通常不需要

        bool print_timestamps = false;
        // 是否打印时间戳信息
        // 用于字幕或时间轴分析
        // 对话系统中一般关闭
    };



    // 生命周期
    bool start();
    void stop();

    // 一体调用接口（与 push / pop 无关）
    std::string getText();
    bool pushtext();
    std::string poptext();

    // 调试控制
    static void setDebug(bool enable);
    static bool getDebug();
};
