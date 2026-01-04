// 文件名建议：program.cs
// 命名空间全部小写：tia_openness.cli

using System;
using openness;                 // 你自己的 Openness 封装命名空间（小写）
// C++ 类比：相当于 #include "openness/xxx.h"
using Siemens.Engineering.SW;   // 为了能直接用 PlcSoftware 类型

namespace tia_openness.cli
{
    // internal 类似 C++ 里只在本程序集中可见的 class
    // C++ 类比：在当前工程内可见的 class program
    internal class program
    {
        // Main 等价于 C++ 的 int main(int argc, char** argv)
        static int Main(string[] args)
        {
            // 如果没有参数：进入“手动调试模式”
            if (args.Length == 0)
            {
                return run_interactive();
            }

            // 有参数：进入“命令行模式”（给 C++ 调用用）
            return run_command_mode(args);
        }

        /// <summary>
        /// 交互模式：你自己在 VS 里运行 exe，手动输入路径做调试
        /// C++ 类比：在 main 里用 std::cout / std::cin 和用户交互
        /// </summary>
        private static int run_interactive()
        {
            console_logger.info("=== TIA Openness 工程管理器 Demo ===");
            console_logger.info("请输入工程完整路径（例如 D:\\plc\\demo.ap20）：");

            // C++ 类比：std::string project_path; std::getline(std::cin, project_path);
            string project_path = Console.ReadLine();

            if (string.IsNullOrWhiteSpace(project_path))
            {
                console_logger.error("工程路径为空，退出。");
                return (int)exit_code.invalid_arguments;
            }

            try
            {
                // using 很像 C++ 的 RAII：
                // 在作用域结束时自动调用 Dispose，相当于调用析构函数 ~tia_session()
                using (tia_session session = new tia_session(with_ui: true))
                {
                    console_logger.debug("已创建 tia_session 对象，准备打开工程。");

                    // C++ 类比：project_service project_srv(&session);
                    project_service project_srv = new project_service(session);

                    // 打开工程
                    project_srv.open_project(project_path);

                    console_logger.info("✔ 工程打开成功");
                    console_logger.info($"工程名称: {project_srv.current_project_name}");
                    console_logger.info($"工程路径: {project_srv.current_project_path}");

                    // C++ 类比：plc_service plc_srv(&project_srv);
                    plc_service plc_srv = new plc_service(project_srv);

                    // ⚠ 这里一定要用 PlcSoftware 类型，不能用 object
                    // C++ 类比：PlcSoftware* plc_software = plc_srv.get_first_plc_software();
                    PlcSoftware plc_software = plc_srv.get_first_plc_software();

                    if (plc_software == null)
                    {
                        console_logger.error("✘ 未找到 PLC Software。");
                        return (int)exit_code.no_plc_found;
                    }

                    // 这里参数类型正好是 PlcSoftware，不再是 object
                    console_logger.info(
                        $"✔ 找到 PLC Software: {plc_srv.get_plc_software_display_name(plc_software)}");

                    // 这里 db_service / fb_service 构造函数参数类型是 object，
                    // C# 会自动把 PlcSoftware 向上转换为 object（C++ 类比：PlcSoftware* → void*）
                    db_service db_srv = new db_service(plc_software);
                    fb_service fb_srv = new fb_service(plc_software);

                    console_logger.info("");
                    console_logger.info("=== DB 列表 ===");
                    db_srv.print_all_db_basic();

                    console_logger.info("");
                    console_logger.info("=== FB 列表 ===");
                    fb_srv.print_all_fb_basic();

                    console_logger.info("");
                    console_logger.info("操作完成。按回车退出...");
                    Console.ReadLine();
                }

                return (int)exit_code.success;
            }
            catch (Exception ex)
            {
                print_exception(ex);
                console_logger.info("按回车退出...");
                Console.ReadLine();
                return (int)exit_code.unhandled_exception;
            }
        }

        /// <summary>
        /// 命令行模式：给 C++ 调用用
        /// C++ 那边可以：
        ///   system("tia_openness.cli.exe list-blocks \"D:\\plc\\demo.ap20\"");
        /// 然后读取标准输出
        /// </summary>
        private static int run_command_mode(string[] args)
        {
            // args[0] = 命令名称, args[1] = 工程路径
            string command = args[0].ToLowerInvariant();

            if (args.Length < 2)
            {
                Console.Error.WriteLine("参数不足。示例：");
                Console.Error.WriteLine("  tia_openness.cli.exe list-blocks \"D:\\plc\\demo.ap20\"");
                return (int)exit_code.invalid_arguments;
            }

            string project_path = args[1];

            try
            {
                // C++ 类比：后台模式运行库，不弹 UI
                using (tia_session session = new tia_session(with_ui: false))
                {
                    console_logger.debug("已在无 UI 模式下创建 tia_session。");

                    project_service project_srv = new project_service(session);
                    project_srv.open_project(project_path);

                    plc_service plc_srv = new plc_service(project_srv);

                    // 同样这里用 PlcSoftware 类型
                    // C++ 类比：PlcSoftware* plc_software = plc_srv.get_first_plc_software();
                    PlcSoftware plc_software = plc_srv.get_first_plc_software();

                    if (plc_software == null)
                    {
                        Console.Error.WriteLine("未找到 PLC Software。");
                        return (int)exit_code.no_plc_found;
                    }

                    db_service db_srv = new db_service(plc_software);
                    fb_service fb_srv = new fb_service(plc_software);

                    switch (command)
                    {
                        case "list-blocks":
                            // 后续可以改成 JSON 输出，方便 C++ 解析
                            Console.WriteLine("=== DB ===");
                            db_srv.print_all_db_basic();

                            Console.WriteLine("=== FB ===");
                            fb_srv.print_all_fb_basic();
                            break;

                        // 未来扩展（C++ 通过参数区分不同操作）：
                        // case "create-db":
                        // case "update-db":
                        // case "create-fb":
                        // case "update-fb":
                        //     ...
                        //     break;

                        default:
                            Console.Error.WriteLine($"未知命令: {command}");
                            return (int)exit_code.invalid_arguments;
                    }
                }

                return (int)exit_code.success;
            }
            catch (Exception ex)
            {
                print_exception(ex);
                return (int)exit_code.unhandled_exception;
            }
        }

        /// <summary>
        /// 统一异常输出
        /// C++ 类比：在 catch(...) 里打印 typeid(e).name() 和 e.what()
        /// </summary>
        private static void print_exception(Exception ex)
        {
            Console.Error.WriteLine("✘ 程序异常");
            Console.Error.WriteLine("异常类型: " + ex.GetType().FullName);
            Console.Error.WriteLine("异常信息: " + ex.Message);
        }
    }

    // 退出码枚举：类似 C++ 里自定义的 enum error_code
    internal enum exit_code
    {
        success = 0,
        invalid_arguments = 1,
        no_plc_found = 2,
        project_open_failed = 3,
        unhandled_exception = 99,
    }
}
