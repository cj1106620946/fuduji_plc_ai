// 文件：console_logger.cs
using System;
namespace openness   // ← 命名空间要放在 openness，因为其它服务类也在这个命名空间
{
    internal static class console_logger
    {
        public static void info(string message)
        {
            Console.WriteLine(message);
        }

        public static void debug(string message)
        {
            Console.WriteLine("[DEBUG] " + message);
        }

        public static void error(string message)
        {
            Console.Error.WriteLine("[ERROR] " + message);
        }
    }
}
