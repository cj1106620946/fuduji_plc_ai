// subtitlewindow.cpp
#include "subtitlewindow.h"
#include <vector>

// 窗口类名
static const wchar_t* SUBTITLE_CLASS = L"SubtitleWindowClass";

SubtitleWindow::SubtitleWindow()
{
}

SubtitleWindow::~SubtitleWindow()
{
    destroy();
}

bool SubtitleWindow::create(HINSTANCE hInstance)
{
    // 记录创建线程
    uiThreadId = GetCurrentThreadId();

    // 注册窗口类（重复注册会失败，但不影响已注册类继续使用）
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SubtitleWindow::WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = SUBTITLE_CLASS;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassExW(&wc);

    // 创建无边框、置顶、半透明、工具窗口（不出现在任务栏）
    hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW,
        SUBTITLE_CLASS,
        L"",
        WS_POPUP,
        posX,
        posY,
        posW,
        posH,
        NULL,
        NULL,
        hInstance,
        this
    );

    if (!hwnd)
        return false;

    // 整体透明度（0~255）
    SetLayeredWindowAttributes(hwnd, 0, 220, LWA_ALPHA);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return true;
}

bool SubtitleWindow::utf8ToWide(const std::string& s, std::wstring& out)
{
    out.clear();

    if (s.empty())
    {
        out = L"";
        return true;
    }

    // 第一次获取所需长度（包含结尾 \0）
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, NULL, 0);
    if (len <= 0)
    {
        // 转换失败，直接返回 false
        return false;
    }

    // 分配空间（包含 \0）
    out.resize((size_t)len);

    // 第二次执行转换
    int ok = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    if (ok <= 0)
    {
        out.clear();
        return false;
    }

    // 去掉尾部 \0，避免 DrawTextW 处理时长度混乱
    if (!out.empty() && out.back() == L'\0')
        out.pop_back();

    return true;
}

void SubtitleWindow::setText(const std::string& text)
{
    if (!hwnd)
        return;

    std::wstring wtext;
    if (!utf8ToWide(text, wtext))
        return;

    // 跨线程调用时，转发到窗口线程执行
    if (GetCurrentThreadId() != uiThreadId)
    {
        std::wstring* heapText = new std::wstring(wtext);
        PostMessageW(hwnd, WM_SUBTITLE_SETTEXT, 0, (LPARAM)heapText);
        return;
    }

    setTextInternal(wtext);
}

void SubtitleWindow::clearText()
{
    if (!hwnd)
        return;

    if (GetCurrentThreadId() != uiThreadId)
    {
        PostMessageW(hwnd, WM_SUBTITLE_CLEARTEXT, 0, 0);
        return;
    }

    clearTextInternal();
}

void SubtitleWindow::setPosition(int x, int y, int width)
{
    if (!hwnd)
        return;

    if (width < 50)
        width = 50;

    // 跨线程：打包参数发给窗口线程
    if (GetCurrentThreadId() != uiThreadId)
    {
        // 低 16 位：x，高 16 位：y（y 可能超过 32767，所以这里用分两次更稳）
        // 这里用 heap 结构体最稳
        struct PosPack
        {
            int x;
            int y;
            int w;
        };
        PosPack* p = new PosPack();
        p->x = x;
        p->y = y;
        p->w = width;
        PostMessageW(hwnd, WM_SUBTITLE_SETPOS, 0, (LPARAM)p);
        return;
    }

    setPositionInternal(x, y, width);
}

void SubtitleWindow::destroy()
{
    if (!hwnd)
        return;

    // 跨线程销毁：转发到窗口线程
    if (GetCurrentThreadId() != uiThreadId)
    {
        PostMessageW(hwnd, WM_SUBTITLE_DESTROY, 0, 0);
        hwnd = NULL;
        return;
    }

    DestroyWindow(hwnd);
    hwnd = NULL;
}

LRESULT CALLBACK SubtitleWindow::WndProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
{
    SubtitleWindow* self = (SubtitleWindow*)GetWindowLongPtrW(h, GWLP_USERDATA);

    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        self = (SubtitleWindow*)cs->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)self);
        return TRUE;
    }

    if (!self)
    {
        return DefWindowProcW(h, msg, wParam, lParam);
    }

    switch (msg)
    {
    case WM_SUBTITLE_SETTEXT:
    {
        std::wstring* p = (std::wstring*)lParam;
        if (p)
        {
            self->setTextInternal(*p);
            delete p;
        }
        return 0;
    }
    case WM_SUBTITLE_CLEARTEXT:
    {
        self->clearTextInternal();
        return 0;
    }
    case WM_SUBTITLE_SETPOS:
    {
        struct PosPack
        {
            int x;
            int y;
            int w;
        };
        PosPack* p = (PosPack*)lParam;
        if (p)
        {
            self->setPositionInternal(p->x, p->y, p->w);
            delete p;
        }
        return 0;
    }
    case WM_SUBTITLE_DESTROY:
    {
        DestroyWindow(h);
        return 0;
    }
    case WM_PAINT:
    {
        self->onPaint();
        return 0;
    }
    case WM_ERASEBKGND:
    {
        // 防止闪烁
        return 1;
    }
    case WM_NCDESTROY:
    {
        SetWindowLongPtrW(h, GWLP_USERDATA, 0);
        return 0;
    }
    default:
        break;
    }

    return DefWindowProcW(h, msg, wParam, lParam);
}

void SubtitleWindow::setTextInternal(const std::wstring& wtext)
{
    currentText = wtext;
    InvalidateRect(hwnd, NULL, TRUE);
}

void SubtitleWindow::clearTextInternal()
{
    currentText.clear();
    InvalidateRect(hwnd, NULL, TRUE);
}

void SubtitleWindow::setPositionInternal(int x, int y, int width)
{
    posX = x;
    posY = y;
    posW = width;

    SetWindowPos(
        hwnd,
        HWND_TOPMOST,
        posX,
        posY,
        posW,
        posH,
        SWP_NOACTIVATE | SWP_SHOWWINDOW
    );
}

void SubtitleWindow::onPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc;
    GetClientRect(hwnd, &rc);

    // 黑色背景
    HBRUSH bg = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);

    // 白色文字
    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);

    HFONT font = CreateFontW(
        28, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Microsoft YaHei"
    );

    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    if (!currentText.empty())
    {
        DrawTextW(
            hdc,
            currentText.c_str(),
            -1,
            &rc,
            DT_CENTER | DT_VCENTER | DT_WORDBREAK
        );
    }

    SelectObject(hdc, oldFont);
    DeleteObject(font);

    EndPaint(hwnd, &ps);
}
