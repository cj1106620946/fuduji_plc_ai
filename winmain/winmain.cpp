// winmain.cpp
// 调度器 宿主窗口 负责启动两个子进程 把 live2d 窗口嵌入到本窗口 并统一生命周期

#include <windows.h>

static const wchar_t* kToken = L"--from-launcher";

static PROCESS_INFORMATION gPlcPi{};
static PROCESS_INFORMATION gLivePi{};

static HWND gHostWnd = NULL;
static HWND gLiveWnd = NULL;

static bool hasTokenInCmdLine(const wchar_t* cmd)
{
    if (!cmd) return false;
    return wcsstr(cmd, kToken) != NULL;
}

static bool createChildProcess(const wchar_t* cmdLine, PROCESS_INFORMATION& pi)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    wchar_t buf[1024]{};
    wcsncpy_s(buf, cmdLine, _TRUNCATE);

    if (!CreateProcessW(
        NULL,
        buf,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi))
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

static BOOL CALLBACK enumFindMainWindow(HWND hwnd, LPARAM lParam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    DWORD targetPid = (DWORD)lParam;
    if (pid != targetPid)
        return TRUE;

    if (!IsWindowVisible(hwnd))
        return TRUE;

    if (GetWindow(hwnd, GW_OWNER) != NULL)
        return TRUE;

    gLiveWnd = hwnd;
    return FALSE;
}

static HWND findMainWindowByPid(DWORD pid, DWORD timeoutMs)
{
    DWORD start = GetTickCount();
    gLiveWnd = NULL;

    while (GetTickCount() - start < timeoutMs)
    {
        EnumWindows(enumFindMainWindow, (LPARAM)pid);
        if (gLiveWnd)
            return gLiveWnd;

        Sleep(50);
    }
    return NULL;
}

static void embedLive2dWindow(HWND host, HWND liveChild)
{
    if (!host || !liveChild)
        return;

    LONG_PTR style = GetWindowLongPtrW(liveChild, GWL_STYLE);
    style &= ~(WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    style |= WS_CHILD;
    SetWindowLongPtrW(liveChild, GWL_STYLE, style);

    LONG_PTR ex = GetWindowLongPtrW(liveChild, GWL_EXSTYLE);
    ex &= ~(WS_EX_APPWINDOW);
    ex |= WS_EX_TOOLWINDOW;
    SetWindowLongPtrW(liveChild, GWL_EXSTYLE, ex);

    SetParent(liveChild, host);

    RECT rc{};
    GetClientRect(host, &rc);
    MoveWindow(liveChild, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);

    ShowWindow(liveChild, SW_SHOW);
    SetForegroundWindow(host);
}

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

static LRESULT CALLBACK hostWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        if (gLiveWnd && IsWindow(gLiveWnd))
        {
            RECT rc{};
            GetClientRect(hWnd, &rc);
            MoveWindow(gLiveWnd, 0, 0, rc.right - rc.left, rc.bottom - rc.top, TRUE);
        }
        return 0;

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
        WS_OVERLAPPEDWINDOW,
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

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int)
{
    gHostWnd = createHostWindow(hInstance);
    if (!gHostWnd)
        return 1;

    // 先启动 plcai 再启动 live2d
    if (!createChildProcess(L"fuduij_ai_plc.exe --from-launcher", gPlcPi))
    {
        closeBoth();
        return 1;
    }

    if (!createChildProcess(L"Demo.exe --from-launcher", gLivePi))
    {
        closeBoth();
        return 1;
    }

    // 找到 live2d 主窗口并嵌入
    HWND live = findMainWindowByPid(gLivePi.dwProcessId, 8000);
    if (live)
    {
        embedLive2dWindow(gHostWnd, live);
    }

    // 消息循环 同时监控子进程退出
    MSG msg{};
    while (true)
    {
        HANDLE hs[2] = { gPlcPi.hProcess, gLivePi.hProcess };
        DWORD wait = MsgWaitForMultipleObjects(2, hs, FALSE, INFINITE, QS_ALLINPUT);

        if (wait == WAIT_OBJECT_0 || wait == WAIT_OBJECT_0 + 1)
        {
            // 任意一个退出 统一关闭
            closeBoth();
            PostMessageW(gHostWnd, WM_CLOSE, 0, 0);
            break;
        }

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                closeBoth();
                return (int)msg.wParam;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    // 再跑一段消息 让窗口销毁流程结束
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
