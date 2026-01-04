// 文件名：tia_session.cs

using Siemens.Engineering;   // C# 里的 TIA Openness 核心命名空间
using System;

namespace openness
{
    // 封装 TIA Portal 会话
    // C++ 类比：
    // class tia_session {
    // public:
    //     TiaPortal* portal;
    //     tia_session(bool with_ui);
    //     ~tia_session();
    // };
    public sealed class tia_session : IDisposable
    {
        // 真实的 TiaPortal 对象
        // C++ 类比：TiaPortal* portal;
        public TiaPortal raw_tia_portal { get; }

        // 是否带 UI（true = 打开博图界面）
        public bool with_ui { get; }

        public tia_session(bool with_ui)
        {
            this.with_ui = with_ui;

            // C++ 类比：printf("[DEBUG] ctor with_ui = %d\n", with_ui);
            console_logger.debug($"tia_session ctor: with_ui = {with_ui}");

            // 选择启动模式（有界面/无界面）
            // C++ 类比：auto mode = with_ui ? UI : NoUI;
            TiaPortalMode mode = with_ui
                ? TiaPortalMode.WithUserInterface
                : TiaPortalMode.WithoutUserInterface;

            try
            {
                // 真正启动 TIA Portal
                // C++ 类比：portal = new TiaPortal(mode);
                raw_tia_portal = new TiaPortal(mode);

                console_logger.info("✔ TIA Portal 已成功启动。");
            }
            catch (Exception ex)
            {
                // 启动失败时输出错误
                // C++ 类比：catch(...) { print typeid(e).name() 和 what(); }
                console_logger.error("✘ 启动 TIA Portal 失败。");
                console_logger.error("异常类型: " + ex.GetType().FullName);
                console_logger.error("异常信息: " + ex.Message);

                // 抛出去，让上层决定是否退出程序
                throw;
            }
        }

        // 释放资源
        // C++ 类比：~tia_session() { delete portal; }
        public void Dispose()
        {
            try
            {
                if (raw_tia_portal != null)
                {
                    console_logger.debug("正在关闭 TIA Portal 会话...");
                    raw_tia_portal.Dispose();      // C# 里等价于 delete + 关闭工程
                    console_logger.info("✔ TIA Portal 会话已关闭。");
                }
            }
            catch (Exception ex)
            {
                // 关闭时异常一般不致命，打印一下方便调试
                console_logger.error("关闭 TIA Portal 时发生异常（一般问题不大）：");
                console_logger.error("异常类型: " + ex.GetType().FullName);
                console_logger.error("异常信息: " + ex.Message);
            }
        }
    }
}
