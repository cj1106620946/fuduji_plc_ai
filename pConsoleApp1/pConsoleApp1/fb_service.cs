// 文件：fb_service.cs

using System;
using System.Collections.Generic;
using Siemens.Engineering.SW;   // 只需要 PlcSoftware

namespace openness
{
    // 管理 FB（Function Block）
    // 当前实现：用 dynamic + 名称/类型字符串判断，把“看起来像 FB 的块”列出来
    public sealed class fb_service
    {
        private readonly PlcSoftware _plc_software;

        public fb_service(object plc_software)
        {
            _plc_software = plc_software as PlcSoftware
                ?? throw new ArgumentException("plc_software 必须是 PlcSoftware 类型", nameof(plc_software));
        }

        // 获取所有“疑似 FB”的块
        // C++ 类比：std::vector<std::tuple<std::string, int, std::string>> get_all_fb_basic();
        public IEnumerable<(string name, int? number, string type_name)> get_all_fb_basic()
        {
            console_logger.debug("开始枚举 FB 块（动态实现）...");

            dynamic root_group = _plc_software.BlockGroup;

            foreach (dynamic block in enumerate_all_blocks(root_group))
            {
                string name = safe_get_name(block);
                int? number = safe_get_number(block);
                string type_name = block != null ? block.GetType().Name : "(unknown-type)";

                string name_upper = (name ?? "").ToUpperInvariant();
                string type_upper = (type_name ?? "").ToUpperInvariant();

                // 简单规则：名称或类型包含 "FB"
                if (name_upper.Contains("FB") || type_upper.Contains("FB") || type_upper.Contains("F BLOCK"))
                {
                    yield return (name, number, type_name);
                }
            }
        }

        public void print_all_fb_basic()
        {
            foreach (var fb in get_all_fb_basic())
            {
                Console.WriteLine(
                    $"FB Name: {fb.name}, Number: {fb.number?.ToString() ?? "-"}, Type: {fb.type_name}");
            }
        }

        // 递归遍历所有块组（和 db_service 同样逻辑）
        private IEnumerable<dynamic> enumerate_all_blocks(dynamic group)
        {
            if (group == null)
                yield break;

            // ---- 1) Blocks ----
            dynamic blocks = null;
            try
            {
                blocks = group.Blocks;
            }
            catch
            {
                console_logger.debug("当前 BlockGroup 上没有 Blocks 属性，跳过该组的块。");
            }

            if (blocks != null)
            {
                foreach (dynamic block in blocks)
                {
                    yield return block;
                }
            }

            // ---- 2) Groups ----
            dynamic groups = null;
            try
            {
                groups = group.Groups;
            }
            catch
            {
                console_logger.debug("当前 BlockGroup 上没有 Groups 属性，跳过子组遍历。");
            }

            if (groups != null)
            {
                foreach (dynamic sub_group in groups)
                {
                    foreach (dynamic sub_block in enumerate_all_blocks(sub_group))
                    {
                        yield return sub_block;
                    }
                }
            }
        }

        private string safe_get_name(dynamic block)
        {
            try
            {
                return (string)block.Name;
            }
            catch
            {
                return string.Empty;
            }
        }

        private int? safe_get_number(dynamic block)
        {
            try
            {
                return (int?)block.Number;
            }
            catch
            {
                return null;
            }
        }

        // ========= 预留：后面实现 create/update FB =========
        public void create_fb(string fb_name, string source_code)
        {
            throw new NotImplementedException("create_fb 尚未实现");
        }

        public void update_fb_source(object fb_block, string source_code)
        {
            throw new NotImplementedException("update_fb_source 尚未实现");
        }
    }
}
