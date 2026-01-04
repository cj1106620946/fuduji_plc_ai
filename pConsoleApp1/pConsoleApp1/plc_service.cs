// 文件：plc_service.cs

using Siemens.Engineering;        // Project, Device
using Siemens.Engineering.HW;     // DeviceItem
using Siemens.Engineering.HW.Features;
using Siemens.Engineering.SW;     // Software, PlcSoftware, SoftwareContainer
using System;

namespace openness
{
    // 负责从工程中找到 PLC Software
    // C++ 类比：从 Project 里遍历所有设备，找到挂在 CPU 上的 plc 程序对象
    public sealed class plc_service
    {
        private readonly project_service _project_service;

        // 缓存找到的 PLC 程序
        // C++ 类比：PlcSoftware* cached_plc;
        public PlcSoftware current_plc_software { get; private set; }

        public plc_service(project_service project_srv)
        {
            _project_service = project_srv ?? throw new ArgumentNullException(nameof(project_srv));
        }

        // 获取工程中的第一个 PLC Software
        // C++ 类比：
        // PlcSoftware* get_first_plc_software() {
        //     if (cached_plc) return cached_plc;
        //     // 遍历 project->Devices → device->DeviceItems → SoftwareContainer → Software
        // }
        public PlcSoftware get_first_plc_software()
        {
            if (current_plc_software != null)
            {
                console_logger.debug("已缓存 PLC Software，直接返回。");
                return current_plc_software;
            }

            if (_project_service.current_project == null)
            {
                throw new InvalidOperationException("工程尚未打开，无法查找 PLC Software。");
            }

            Project project = _project_service.current_project;

            console_logger.debug("开始在工程中查找 PLC Software...");

            // 遍历工程里的每一个 Device（硬件设备）
            // C++ 类比：for (auto& device : project->Devices)
            foreach (Device device in project.Devices)
            {
                // 遍历 DeviceItem（可能是机架、CPU 插槽等）
                // C++ 类比：for (auto& item : device->DeviceItems)
                foreach (DeviceItem device_item in device.DeviceItems)
                {
                    // 从 DeviceItem 获取 SoftwareContainer 服务
                    // GetService<T>() 的 T 必须实现 IEngineeringService，
                    // SoftwareContainer 正是一个“服务”类型。
                    // C++ 类比：auto container = device_item->GetService<SoftwareContainer>();
                    SoftwareContainer container = device_item.GetService<SoftwareContainer>();

                    if (container == null)
                    {
                        // 这个 DeviceItem 上没有软件容器，跳过
                        continue;
                    }

                    // 绝大多数 TIA 版本，这里是一个单独的 Software 属性
                    // C++ 类比：Software* sw = container->Software;
                    Software sw = null;
                    try
                    {
                        sw = container.Software;
                    }
                    catch
                    {
                        // 如果这个版本没有 Software 属性（极少数情况），
                        // 直接跳过这个 device_item。
                        console_logger.debug("当前 SoftwareContainer 没有 Software 属性，跳过该设备项。");
                        continue;
                    }

                    if (sw == null)
                    {
                        // 容器上没有挂任何软件（比如还没下载程序）
                        continue;
                    }

                    // 判断这个软件是不是 PLC 程序
                    // C++ 类比：
                    // if (auto plc = dynamic_cast<PlcSoftware*>(sw)) { ... }
                    if (sw is PlcSoftware plc)
                    {
                        current_plc_software = plc;

                        console_logger.info("✔ 在工程中找到 PLC Software。");
                        console_logger.debug($"PLC 名称: {get_plc_software_display_name(plc)}");

                        return plc;
                    }
                }
            }

            // 如果走到这里，说明整个工程都扫描完了，没有 PLC
            console_logger.error("✘ 未在工程中找到任何 PLC Software（请检查工程是否包含 CPU 及其程序）。");
            return null;
        }

        // 获取PLC名字，用于调试
        // C++ 类比：std::string get_plc_name(PlcSoftware* plc);
        public string get_plc_software_display_name(PlcSoftware plc)
        {
            if (plc == null)
            {
                return "(null)";
            }

            try
            {
                // 大多数版本都有 Name 属性
                return plc.Name;
            }
            catch
            {
                // 作为保险，退回 ToString()
                return plc.ToString();
            }
        }
    }
}
