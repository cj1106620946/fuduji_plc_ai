// 文件名：project_service.cs

using Siemens.Engineering;   // Project, TiaPortal
using System;
using System.IO;

namespace openness
{
    // 负责管理工程（打开工程 / 记录工程信息）
    // C++ 类比：
    // class project_service {
    //     TiaPortal* _sessionPortal;
    //     Project*   current_project;
    // };
    public sealed class project_service
    {
        private readonly tia_session _session;

        // 当前打开的工程对象
        // C++ 类比：Project* current_project;
        public Project current_project { get; private set; }

        // 工程名称（方便打印调试信息）
        public string current_project_name { get; private set; } = string.Empty;

        // 工程文件完整路径（包含 .ap20）
        public string current_project_path { get; private set; } = string.Empty;

        public project_service(tia_session session)
        {
            // C++ 类比：assert(session != nullptr);
            _session = session ?? throw new ArgumentNullException(nameof(session));
        }

        // 打开工程
        // 参数 project_path 可以是：
        //  1) 直接是 *.ap20 文件路径
        //  2) 或者是工程所在的文件夹（例如 C:\...\项目2）
        public void open_project(string project_path)
        {
            if (string.IsNullOrWhiteSpace(project_path))
            {
                // C++ 类比：throw std::invalid_argument("工程路径不能为空");
                throw new ArgumentException("工程路径不能为空", nameof(project_path));
            }

            // 去掉可能的引号（有时命令行传参会带双引号）
            // C++ 类比：strip quotes from std::string
            string trimmed = project_path.Trim('"');

            string final_project_file = null;

            // 第一种情况：用户给的是文件路径（包含 .ap20）
            // C++ 类比：if (is_regular_file(path)) { ... }
            if (System.IO.File.Exists(trimmed))
            {
                final_project_file = trimmed;
            }
            else if (Directory.Exists(trimmed))
            {
                // 第二种情况：用户给的是工程文件夹路径
                // 比如：C:\Users\...\项目2
                // 我们在这个文件夹里找第一个 .ap* 文件
                // C++ 类比：
                //   for (auto& f : directory_iterator(trimmed))
                //       if (ends_with(".ap20")) { use this; }

                console_logger.debug($"输入的是文件夹路径，开始在目录中搜索工程文件: {trimmed}");

                string[] candidates = Directory.GetFiles(trimmed, "*.ap*", SearchOption.TopDirectoryOnly);

                if (candidates.Length == 0)
                {
                    // 文件夹里没有工程文件
                    // C++ 类比：throw std::runtime_error("no project file in dir");
                    throw new FileNotFoundException("在该文件夹中没有找到 .apxx 工程文件", trimmed);
                }

                // 简单起见：取第一个匹配到的工程文件
                // 如果你一个文件夹只放一个 .ap20，这样足够用了
                final_project_file = candidates[0];
                console_logger.debug($"在文件夹中找到工程文件: {final_project_file}");
            }
            else
            {
                // 既不是文件，也不是文件夹 → 直接认为路径不正确
                throw new FileNotFoundException("工程文件或文件夹不存在", trimmed);
            }

            // 走到这里时，final_project_file 一定是一个存在的 .apxx 文件
            console_logger.debug($"准备通过 TIA Openness 打开工程文件: {final_project_file}");

            // 从 session 里拿到真正的 TiaPortal 对象
            // C++ 类比：
            //   TiaPortal* portal = session->portal;
            TiaPortal portal = _session.raw_tia_portal;

            try
            {
                // 用 FileInfo 包装路径
                // C++ 类比：std::filesystem::path p(final_project_file);
                FileInfo project_file_info = new FileInfo(final_project_file);

                // 真实 Openness 调用：打开工程
                // C++ 类比：
                //   Project* proj = portal->Projects.Open(p);
                Project proj = portal.Projects.Open(project_file_info);

                current_project = proj;
                current_project_name = proj.Name;                 // 工程名（比如“项目2”）
                current_project_path = proj.Path.FullName;        // 工程实际路径（一般是 .ap20 包文件）

                console_logger.info($"✔ 工程已成功打开: {current_project_name}");
                console_logger.debug($"工程实际路径: {current_project_path}");
            }
            catch (EngineeringException engEx)
            {
                // Openness 的特定异常
                // 例如：工程被其他 TIA 实例占用，或版本不兼容等
                console_logger.error("使用 TIA Openness 打开工程失败（EngineeringException）。");
                console_logger.error("异常类型: " + engEx.GetType().FullName);
                console_logger.error("异常信息: " + engEx.Message);

                // 封装成更通用的异常抛出，也可以定义自己的异常类型
                throw;
            }
            catch (Exception ex)
            {
                // 其它未知异常
                console_logger.error("使用 TIA Openness 打开工程失败（未知异常）。");
                console_logger.error("异常类型: " + ex.GetType().FullName);
                console_logger.error("异常信息: " + ex.Message);
                throw;
            }
        }
    }
}
