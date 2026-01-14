// subtitlewindow.h
#pragma once

#include <windows.h>
#include <string>

// 独立字幕窗口（置顶、无边框、半透明）
// 注意：对外接口允许跨线程调用，内部会转发到窗口线程执行
class SubtitleWindow
{
public:
    SubtitleWindow();
    ~SubtitleWindow();

    // 创建字幕窗口（必须在创建 Live2D 窗口的同一线程调用一次）
    bool create(HINSTANCE hInstance);

    // 设置字幕文本（允许跨线程调用）
    void setText(const std::string& text);

    // 清空字幕（允许跨线程调用）
    void clearText();

    // 设置位置与宽度（允许跨线程调用）
    void setPosition(int x, int y, int width);

    // 销毁（允许跨线程调用）
    void destroy();

    // 是否已创建
    bool isValid() const { return hwnd != NULL; }

private:
    // 窗口过程
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // 真正执行绘制
    void onPaint();

    // 仅在窗口线程执行的内部方法
    void setTextInternal(const std::wstring& wtext);
    void clearTextInternal();
    void setPositionInternal(int x, int y, int width);

private:
    HWND hwnd = NULL;
    DWORD uiThreadId = 0;

    // 当前字幕
    std::wstring currentText;

    // 位置参数
    int posX = 100;
    int posY = 600;
    int posW = 800;
    int posH = 120;

private:
    // 自定义消息
    static const UINT WM_SUBTITLE_SETTEXT = WM_APP + 101;
    static const UINT WM_SUBTITLE_CLEARTEXT = WM_APP + 102;
    static const UINT WM_SUBTITLE_SETPOS = WM_APP + 103;
    static const UINT WM_SUBTITLE_DESTROY = WM_APP + 104;

    // 辅助：UTF-8 转 UTF-16，失败返回空
    static bool utf8ToWide(const std::string& s, std::wstring& out);
};
