#pragma once

#include <windows.h>

// UI 管理类
class AiManagerUI
{
public:
    AiManagerUI();
    ~AiManagerUI();

    // 创建窗口
    bool create(HINSTANCE instance);
    // 显示窗口
    void show();
    // 消息循环
    void loop();

private:
    // 窗口过程
    static LRESULT CALLBACK wndproc(
        HWND hwnd,
        UINT msg,
        WPARAM wparam,
        LPARAM lparam
    );

private:
    HINSTANCE hinstance = nullptr;
    HWND hwnd = nullptr;
};
