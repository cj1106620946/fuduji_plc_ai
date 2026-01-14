// winmain.cpp
// 调度器 宿主窗口 负责启动两个子进程 把 live2d 窗口嵌入到本窗口 并统一生命周期

#include <windows.h>

static PROCESS_INFORMATION gPlcPi{};
static PROCESS_INFORMATION gLivePi{};

static HWND gHostWnd = NULL;
static HWND gLiveWnd = NULL;
static HWND gUiWnd = NULL;

// ------------------------------------------------------------

static bool createChildProcess(const wchar_t* cmdLine, PROCESS_INFORMATION& pi)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    wchar_t buf[1024]{};
    wcsncpy_s(buf, cmdLine, _TRUNCATE);

    if (!CreateProcessW(
        NULL, buf,
        NULL, NULL,
        FALSE, 0,
        NULL, NULL,
        &si, &pi))
    {
        return false;
    }

    if (pi.hThread)
    {
        CloseHandle(pi.hThread);
        pi.hThread = NULL;
    }
    return true;
}

// ------------------------------------------------------------

struct FindParam
{
    DWORD pid = 0;
    const wchar_t* titleSub = nullptr;
    HWND found = NULL;
};

static BOOL CALLBACK enumFindMainWindow(HWND hwnd, LPARAM lParam)
{
    FindParam* p = (FindParam*)lParam;
    if (!p) return TRUE;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid != p->pid)
        return TRUE;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    if (GetWindow(hwnd, GW_OWNER) != NULL)
        return TRUE;

    if (p->titleSub && p->titleSub[0])
    {
        wchar_t title[256]{};
        GetWindowTextW(hwnd, title, 255);
        if (!wcsstr(title, p->titleSub))
            return TRUE;
    }

    p->found = hwnd;
    return FALSE;
}

static HWND findMainWindowByPid(DWORD pid, DWORD timeoutMs, const wchar_t* titleSub)
{
    DWORD start = GetTickCount();
    FindParam param{ pid, titleSub, NULL };

    while (GetTickCount() - start < timeoutMs)
    {
        param.found = NULL;
        EnumWindows(enumFindMainWindow, (LPARAM)&param);
        if (param.found)
            return param.found;

        Sleep(50);
    }
    return NULL;
}

// ------------------------------------------------------------
// 关键修改点：嵌入子窗口后的“行为收敛”
static void embedChildWindow(HWND host, HWND child)
{
    if (!host || !child)
        return;

    // 改为真正的子窗口样式
    LONG_PTR style = GetWindowLongPtrW(child, GWL_STYLE);
    style &= ~(WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME |
        WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    style |= WS_CHILD;
    SetWindowLongPtrW(child, GWL_STYLE, style);

    LONG_PTR ex = GetWindowLongPtrW(child, GWL_EXSTYLE);
    ex &= ~(WS_EX_APPWINDOW | WS_EX_TOPMOST);
    ex |= WS_EX_TOOLWINDOW;
    SetWindowLongPtrW(child, GWL_EXSTYLE, ex);

    SetParent(child, host);

    // 不激活、不抢焦点、不改变 Z 顺序
    SetWindowPos(
        child,
        NULL,
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE |
        SWP_NOZORDER | SWP_NOACTIVATE |
        SWP_FRAMECHANGED
    );

    ShowWindow(child, SW_SHOW);

    // 强制一次重绘，防止第一次嵌入闪烁
    InvalidateRect(child, NULL, TRUE);
    UpdateWindow(child);
}

// ------------------------------------------------------------

static void layoutChildren(HWND host)
{
    RECT rc{};
    GetClientRect(host, &rc);

    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    if (gUiWnd && IsWindow(gUiWnd) && gLiveWnd && IsWindow(gLiveWnd))
    {
        MoveWindow(gUiWnd, 0, 0, w / 2, h, TRUE);
        MoveWindow(gLiveWnd, w / 2, 0, w - (w / 2), h, TRUE);
        return;
    }

    if (gLiveWnd && IsWindow(gLiveWnd))
    {
        MoveWindow(gLiveWnd, 0, 0, w, h, TRUE);
        return;
    }

    if (gUiWnd && IsWindow(gUiWnd))
    {
        MoveWindow(gUiWnd, 0, 0, w, h, TRUE);
        return;
    }
}

// ------------------------------------------------------------

static void closeBoth()
{
    if (gPlcPi.hProcess)
    {
        TerminateProcess(gPlcPi.hProcess, 0);
        CloseHandle(gPlcPi.hProcess);
        gPlcPi.hProcess = NULL;
    }
    if (gLivePi.hProcess)
    {
        TerminateProcess(gLivePi.hProcess, 0);
        CloseHandle(gLivePi.hProcess);
        gLivePi.hProcess = NULL;
    }
}

// ------------------------------------------------------------

static LRESULT CALLBACK hostWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        layoutChildren(hWnd);
        return 0;

        // 防止背景反复擦除造成闪烁
    case WM_ERASEBKGND:
        return 1;

    case WM_CLOSE:
        closeBoth();
        DestroyWindow(hWnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

// ------------------------------------------------------------

static HWND createHostWindow(HINSTANCE hInst)
{
    const wchar_t* cls = L"fudujiHostWindow";

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = hostWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = cls;

    RegisterClassExW(&wc);

    HWND hWnd = CreateWindowExW(
        0,
        cls,
        L"fuduji launcher",
        // 关键：裁剪子窗口，防止闪烁
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1100,
        700,
        NULL,
        NULL,
        hInst,
        NULL);

    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    return hWnd;
}

// ------------------------------------------------------------

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    gHostWnd = createHostWindow(hInstance);
    if (!gHostWnd)
        return 1;

    if (!createChildProcess(L"fuduij_ai_plc.exe --from-launcher", gPlcPi))
        return 1;

    if (!createChildProcess(L"Demo.exe --from-launcher", gLivePi))
        return 1;

    // 先嵌入 live2d
    gLiveWnd = findMainWindowByPid(gLivePi.dwProcessId, 8000, NULL);
    if (gLiveWnd)
    {
        embedChildWindow(gHostWnd, gLiveWnd);
        layoutChildren(gHostWnd);
    }

    HANDLE hUiReady = CreateEventW(
        nullptr,
        TRUE,
        FALSE,
        L"Global\\AIMANAGER_UI_READY"
    );

    bool uiEmbedded = false;
    MSG msg{};

    while (true)
    {
        HANDLE hs[3] = { gPlcPi.hProcess, gLivePi.hProcess, hUiReady };
        DWORD wait = MsgWaitForMultipleObjects(3, hs, FALSE, INFINITE, QS_ALLINPUT);

        if (wait == WAIT_OBJECT_0 || wait == WAIT_OBJECT_0 + 1)
        {
            closeBoth();
            break;
        }

        if (!uiEmbedded && wait == WAIT_OBJECT_0 + 2)
        {
            gUiWnd = findMainWindowByPid(gPlcPi.dwProcessId, 8000, L"AiManagerUI");
            if (gUiWnd)
            {
                embedChildWindow(gHostWnd, gUiWnd);
                layoutChildren(gHostWnd);
                uiEmbedded = true;
            }
        }

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                return 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return 0;
}
