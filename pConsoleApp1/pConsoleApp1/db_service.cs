// 文件：db_service.cs

using System;
using System.Collections.Generic;
using Siemens.Engineering.SW;   // 只需要 PlcSoftware（不再用 PlcBlockType 等强类型）

namespace openness
{
    // 管理 DB（Data Block）
    // 当前实现：用 dynamic + 名称/类型字符串判断，把工程中“看起来像 DB 的块”列出来
    public sealed class db_service
    {
        // 真实的 PLC 软件对象
        // C++ 类比：PlcSoftware* _plc_software;
        private readonly PlcSoftware _plc_software;

        public db_service(object plc_software)
        {
            // C++ 类比：_plc_software = static_cast<PlcSoftware*>(plc_software);
            _plc_software = plc_software as PlcSoftware
                ?? throw new ArgumentException("plc_software 必须是 PlcSoftware 类型", nameof(plc_software));
        }

        // 获取所有“疑似 DB”的块
        // C++ 类比：std::vector<std::tuple<std::string, int, std::string>> get_all_db_basic();
        public IEnumerable<(string name, int? number, string type_name)> get_all_db_basic()
        {
            console_logger.debug("开始枚举 DB 块（动态实现）...");

            // C++ 类比：auto root = _plc_software->BlockGroup;
            dynamic root_group = _plc_software.BlockGroup;

            foreach (dynamic block in enumerate_all_blocks(root_group))
            {
                string name = safe_get_name(block);
                int? number = safe_get_number(block);
                string type_name = block != null ? block.GetType().Name : "(unknown-type)";

                string name_upper = (name ?? "").ToUpperInvariant();
                string type_upper = (type_name ?? "").ToUpperInvariant();

                // 简单规则：名称或类型字符串里包含 "DB" 或 "DATABLOCK" 的，先当做 DB
                // 例如：DB1, "GlobalDB", 类型名里含有 "PlcDBlock" 等
                if (name_upper.Contains("DB") || type_upper.Contains("DB") || type_upper.Contains("DATABLOCK"))
                {
                    yield return (name, number, type_name);
                }
            }
        }

        // 打印所有 DB 的基本信息
        public void print_all_db_basic()
        {
            foreach (var db in get_all_db_basic())
            {
                Console.WriteLine(
                    $"DB Name: {db.name}, Number: {db.number?.ToString() ?? "-"}, Type: {db.type_name}");
            }
        }

        // 递归遍历所有块组，返回里面的所有块（dynamic）
        // C++ 类比：
        // void enumerate_all_blocks(BlockGroup* group, std::vector<Block*>& out);
        private IEnumerable<dynamic> enumerate_all_blocks(dynamic group)
        {
            if (group == null)
                yield break;

            // ---- 1) 当前组里的 Blocks ----
            dynamic blocks = null;
            try
            {
                // 不在 try 里 yield，只拿引用
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

            // ---- 2) 子组 Groups ----
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

        // 安全获取块名
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

        // 安全获取块编号
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

        // ========= 下面预留创建/修改 DB 接口，后面再实现 =========

        public void create_db(string db_name)
        {
            // TODO：后续实现：使用 Openness 创建 DB
            throw new NotImplementedException("create_db 尚未实现");
        }

        public void update_db_source(object db_block, string source_code)
        {
            // TODO：后续实现：写入 DB 源码并编译
            throw new NotImplementedException("update_db_source 尚未实现");
        }
    }
}
