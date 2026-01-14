// ================= plcai main.cpp =================
#include <iostream>
#include <windows.h>
#include <string>
#include <thread>

#include "console.h"
#include "plcclient.h"
#include "aiclient.h"

// 判断是否携带启动令牌
static bool checkLaunchToken(int argc, char* argv[])
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--from-launcher")
            return true;
    }
    return false;
}
static void hideConsoleWindow()
{
    HWND h = GetConsoleWindow();
    if (h)
        ShowWindow(h, SW_HIDE);
}
int main(int argc, char* argv[])
{
    // 启动即校验令牌
    if (!checkLaunchToken(argc, argv))
    {
        // 非 launcher 启动，直接退出
        return 0;
    }
    // 控制台程序入口
    Console app;

    // 主线程继续跑控制台（你原有逻辑）
    app.run();

    return 0;
}
