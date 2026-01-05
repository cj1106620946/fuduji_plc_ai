#include <iostream>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include "console.h"     // 你的 ConsoleApp 类
#include "plcclient.h"   // PLC 客户端
#include "deepseek.h"    // AI 客户端

int main()
{
    // 创建上位机管理类
    Console app;
    // 启动菜单系统
    app.run();
    return 0;
}