# fuduji PLC-AI

一个使用 **C++ / Qt / PLC / 大语言模型** 搭建的实验性 AI 上位机项目。

这个项目最初用于验证：能否让 AI 理解自然语言中的 PLC 操作意图，将其转换为结构化指令，再由本地 C++ 程序完成实际通信与控制。

它不是成熟的工业控制软件，而是一个保留了大量实验、调试和学习过程的个人 Demo。

> ⚠️ **安全说明：本项目仅用于学习、研究和 Demo。不要直接用于真实生产环境、安全关键设备或无人监管的工业控制。AI 不应拥有绕过 PLC 联锁、急停和确定性安全逻辑的权限。**

## 项目思路

```text
用户
 ↓
ChatAI
 ↓
任务分发 / 意图识别
 ├─ ExecuteAI   → PLC 读写操作
 ├─ WorkspaceAI → PLC / 变量配置
 ├─ DecisionAI  → 状态分析与建议
 └─ MemoryAI    → 实验性记忆模块
 ↓
ProjectManager
 ↓
PLC 状态镜像
 ↓
读取线程 / 写入线程
 ↓
Snap7
 ↓
Siemens PLC
```

AI 不直接调用 PLC 通信接口。以写入为例，AI 先生成结构化动作，本地程序解析后只登记写入意图：

```text
targetValue = ...
writeFlag = 1
```

真正的 PLC 写入由本地写线程执行。这样可以让 AI 的不确定输出与实际工业控制层保持一定隔离。

## 已实现 / 存在雏形的功能

- Qt Widgets 图形界面
- AI 普通对话
- 本地 Ollama 模型调用
- OpenAI / DeepSeek / Anthropic / Google 等云端 Provider 的实验性接口
- Snap7 PLC 连接、读取、写入
- PLC 变量状态镜像
- 周期读取 / 写入线程
- ExecuteAI：把自然语言转换为 `read` / `write` JSON 动作
- WorkspaceAI：根据自然语言创建 PLC 配置和变量定义
- DecisionAI：根据当前状态生成分析建议
- MemoryAI：实验性记忆模块
- AI Trace：记录 Prompt、输入、输出、成功状态与时间戳
- whisper.cpp + PortAudio 语音识别
- Piper TTS
- Live2D 相关实验代码
- 旧 Console 调试入口（A1 ~ A30）

## AI 指令格式

ExecuteAI 会把操作转换为类似：

```json
{
  "type": "ok",
  "message": "打开水泵",
  "actions": [
    {
      "op": "write",
      "address": "M0.0",
      "value": 1
    }
  ]
}
```

本地程序负责验证和解析这些字段，再决定是否登记实际操作。

项目早期还尝试过类似 Agent Loop 的流程：

```text
AI 判断
 ↓
执行
 ↓
读取结果
 ↓
任务未完成 / 返回失败
 ↓
结果重新交给 AI
 ↓
继续处理
```

后来没有继续采用无限自主循环，一方面是 Token 消耗很快，另一方面工业控制场景也不适合让 AI 在失败后无限尝试不同动作。

## 开发环境

从当前工程文件可以确认的原始环境大致为：

- Windows
- Visual Studio 2022
- MSVC v143
- C++17
- Qt 6.x + Qt Visual Studio Tools
- x64

当前 `Release|x64` 配置中保留了 **Qt 6.10.2** 的历史配置。

## 主要依赖

项目中使用或引用了：

- Qt 6
- Snap7
- libcurl
- SQLite3
- JsonCpp
- whisper.cpp
- PortAudio
- Piper

部分源码（例如 JsonCpp / SQLite）已经随工程参与编译，但部分 `.lib`、DLL、AI 模型、Whisper 模型和 Piper 运行文件没有上传。

## ⚠️ 当前仓库不能保证 Clone 后直接编译

这是目前最重要的已知问题。

原工程保留了开发机器上的绝对路径，例如：

```text
D:\project\c-cpp\include
D:\project\c-cpp\lib
D:\Qt\6.10.2\msvc2022_64\lib
D:\project\run\
```

因此直接 Clone 后大概率需要重新配置依赖路径。

### Visual Studio 中需要检查

```text
项目属性
→ C/C++
→ 常规
→ 附加包含目录
```

```text
项目属性
→ 链接器
→ 常规
→ 附加库目录
```

```text
项目属性
→ 链接器
→ 输入
→ 附加依赖项
```

当前工程中可以看到这些链接依赖：

```text
snap7.lib
libcurl_a_debug.lib
libcurl_a.lib
ws2_32.lib
winmm.lib
wldap32.lib
Crypt32.lib
Normaliz.lib
whisper.lib
portaudio.lib
```

请按照自己的实际安装位置重新配置。

## 推荐重新整理的目录结构

如果要重新搭建环境，建议把第三方库整理到仓库附近，并逐渐去掉绝对路径，例如：

```text
fuduji_plc_ai/
├─ ConsoleApplication1/
├─ third_party/
│  ├─ snap7/
│  ├─ curl/
│  ├─ whisper/
│  └─ portaudio/
├─ models/
│  └─ ggml-small.bin
├─ piper/
│  ├─ piper.exe
│  └─ voices/
└─ runtime/
```

## Qt 配置

建议安装：

- Visual Studio 2022
- Qt 6.x MSVC 64-bit
- Qt Visual Studio Tools

然后在 Visual Studio 中配置 Qt Version。

如果出现：

```text
QtMsBuild: could not locate qt.targets
```

通常表示 Qt Visual Studio Tools / QtMsBuild 尚未正确配置。

## AI 模型

程序支持本地 Ollama 和实验性的云端 Provider。

### 本地 Ollama

默认接口：

```text
http://127.0.0.1:11434/api/chat
```

代码中的默认模型名为：

```text
qwen2.5:7b-instruct-q4_K_M
```

如果本机模型名称不同，需要在程序配置或源码中修改。

### 云端模型

代码中存在这些 Provider：

- OpenAI
- DeepSeek
- Anthropic
- Google

这些接口属于实验性实现。随着各厂商 API 变化，当前代码不保证仍然全部兼容。

请不要把 API Key 提交到公开仓库。

## Whisper 语音识别

当前语音模块使用：

```text
whisper.cpp + PortAudio
```

默认模型文件：

```text
ggml-small.bin
```

默认采集参数中包含：

```text
采样率：48000 Hz
声道：1
语言：zh
```

程序内部会把音频降采样到 Whisper 使用的 16 kHz。

如果语音模块无法启动，优先检查：

- `ggml-small.bin` 路径
- `device_index`
- 麦克风输入设备
- PortAudio
- whisper 动态 / 静态库是否正确链接

不同电脑的输入设备编号可能不同。

## Piper TTS

Piper 运行文件和语音模型没有完整上传。

当前代码曾使用：

```text
\piper\piper.exe
```

以及：

```text
voices\zh_CN\huayan\medium\zh_CN-huayan-medium.onnx
voices\zh_CN\huayan\medium\zh_CN-huayan-medium.onnx.json
```

重新部署时建议把这些路径改成相对于程序目录的路径，而不是继续使用历史硬编码。

## PLC 环境

PLC 通信主要基于 Snap7，面向 Siemens S7 系列设备。

运行前请确保：

- PC 与 PLC 网络可达
- PLC IP 正确
- Rack / Slot 正确
- PLC 允许对应的 S7 通信
- 测试地址确实存在并且允许读写

例如：

```text
IP   : 192.168.0.1
Rack : 0
Slot : 1
```

实际参数请按照 PLC 型号和项目配置调整。

## 编译

推荐先尝试：

```text
Release | x64
```

打开：

```text
ConsoleApplication1.sln
```

依次确认：

1. Qt 能被 Visual Studio 正确识别
2. Include 路径正确
3. Library 路径正确
4. 所需 `.lib` 可以找到
5. 需要的运行时 DLL 已准备

然后：

```text
生成 → 生成解决方案
```

### Post-Build Event

当前 `Release|x64` 还保留了历史命令：

```text
xcopy /y /d "$(TargetPath)" "D:\project\run\"
```

请删除或改成自己的运行目录，否则即使编译成功，也可能在 Build 的最后一步因为这个路径不存在而报错。

## 运行

当前主入口是：

```text
QApplication
 ↓
ai_plc_delegate
 ↓
Qt UI
```

第一次重新搭建环境时，不建议直接连接真实 PLC。

推荐测试顺序：

1. Qt UI 能启动
2. SQLite / 项目数据可以初始化
3. Ollama 或云端 AI 可以正常回复
4. PLC 可以手动连接
5. 测试单个地址读取
6. 测试单个地址写入
7. 检查 ExecuteAI 输出的 JSON
8. 最后再测试自然语言控制 PLC

## AI Trace

AI 调试记录默认写入：

```text
ai_trace/
```

记录内容包括：

- role
- AI mode
- 用户输入
- System Prompt
- Debug 信息
- AI 输出
- 调用开始 / 结束时间
- ok / failed 状态

这个功能主要用于排查 Prompt、JSON 格式和 AI 行为问题。

## 工控安全原则

这个项目的核心想法并不是“让 AI 接管 PLC”。

更合理的边界应该是：

```text
AI
│
│ 生成建议 / 操作请求
▼
确定性规则检查
│
├─ 地址是否合法
├─ 数值是否越界
├─ 当前状态是否允许操作
├─ 权限检查
└─ 是否要求人工确认
│
▼
PLC 执行层
```

以下逻辑不应该交给 AI 自由判断：

- 急停
- 安全联锁
- 温度 / 压力等硬限制
- 电机保护
- 人身安全相关动作
- 危险设备自动重试

PLC 和本地确定性程序应该始终保留最终控制权。

## 已知问题

当前分支属于实验和开发过程中的版本，技术债务很多，包括但不限于：

- 构建路径硬编码
- 第三方依赖没有统一管理
- 部分模型 / DLL / LIB 未上传
- 某些 AI 模块仍是实验性实现或只有雏形
- 多线程生命周期管理仍需重构
- 部分共享状态缺少完整同步
- 项目运行状态比较复杂
- 错误恢复逻辑较简单
- Provider API 可能已经变化
- 运行时目录结构尚未标准化
- PLC 写入安全规则仍不完整
- UI、AI、设备控制层仍存在耦合

因此目前更适合：

- 学习和代码阅读
- AI + PLC 架构讨论
- Demo 演示
- Prompt / Tool 调用实验
- 上位机原型验证

不适合直接用于生产部署。

## 项目定位

这个项目真正想验证的是：

> 能否在传统工业控制系统上增加自然语言和智能分析层，同时仍然让确定性程序与 PLC 掌握最终执行权。

PLC 更擅长：

- 实时控制
- 确定性逻辑
- 联锁
- 可靠执行

AI 更擅长：

- 自然语言理解
- 状态分析
- 信息整理
- 高层建议
- 人机交互

两者更适合互补，而不是互相替代。

## 关于这个仓库

这是一个个人实验项目，保留了明显的学习和试错痕迹。

开发方式基本是：

```text
先把想法跑起来
 ↓
验证结构
 ↓
发现问题
 ↓
再修改 / 重构
```

因此仓库中会同时存在正式逻辑、调试代码、旧实现、未完成模块和实验功能。

保留它的目的之一，就是记录一个 AI + PLC 上位机从想法逐渐变成可运行 Demo 的过程。

## License

本项目采用 [MIT License](LICENSE) 开源。

你可以使用、复制、修改、合并、发布和再分发本项目代码，但需保留原始版权声明和许可声明。

第三方库、模型、语音资源等仍遵循各自原有许可证，本项目的 MIT License 不会覆盖这些第三方组件。