#include "qtmain.h"

#include <QPushButton>
#include <QShowEvent>
#include <windows.h>

#include "console.h"

// 构造函数
qtmain::qtmain(QWidget* parent)
    : QWidget(parent),
    consoleStarted(false),
    hostHwnd(nullptr),
    liveHwnd(nullptr)
{
    // 初始化 UI
    ui.setupUi(this);

    // 右侧 Live2D 承载区域必须是原生窗口
    ui.rightpanel->setAttribute(Qt::WA_NativeWindow);
    ui.rightpanel->setAttribute(Qt::WA_DontCreateNativeAncestors);

    // 启动控制台按钮
    connect(
        ui.btnStartConsole,
        &QPushButton::clicked,
        this,
        &qtmain::startConsole
    );
}

// 析构函数
qtmain::~qtmain()
{
    if (consoleThread.joinable())
        consoleThread.detach();

    if (livePi.hProcess)
    {
        TerminateProcess(livePi.hProcess, 0);
        CloseHandle(livePi.hProcess);
        livePi.hProcess = nullptr;
    }
}

// 启动控制台程序
void qtmain::startConsole()
{
    if (consoleStarted)
        return;

    consoleStarted = true;

    consoleApp.reset(new Console);

    consoleThread = std::thread([this]()
    {
        consoleApp->run();
    });

    consoleThread.detach();
}

// 窗口显示后嵌入 Live2D
void qtmain::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    static bool done = false;
    if (done)
        return;
    done = true;

    // 1 获取右侧 widget 的 HWND
    hostHwnd = (HWND)ui.rightpanel->winId();
    if (!hostHwnd)
        return;

    // 2 启动 Live2D 独立 exe
    STARTUPINFOW si{};
    si.cb = sizeof(si);

    ZeroMemory(&livePi, sizeof(livePi));

    wchar_t cmd[] = L"Demo.exe --from-launcher";

    if (!CreateProcessW(
        nullptr,
        cmd,
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &si,
        &livePi))
    {
        return;
    }

    if (livePi.hThread)
    {
        CloseHandle(livePi.hThread);
        livePi.hThread = nullptr;
    }

    // 3 查找 Live2D 主窗口
    DWORD start = GetTickCount();
    liveHwnd = nullptr;

    while (GetTickCount() - start < 8000)
    {
        EnumWindows([](HWND hwnd, LPARAM lParam)->BOOL {
            qtmain* self = (qtmain*)lParam;

            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid != self->livePi.dwProcessId)
                return TRUE;

            if (!IsWindowVisible(hwnd))
                return TRUE;

            if (GetWindow(hwnd, GW_OWNER) != NULL)
                return TRUE;

            self->liveHwnd = hwnd;
            return FALSE;
        }, (LPARAM)this);

        if (liveHwnd)
            break;

        Sleep(50);
    }

    if (!liveHwnd)
        return;

    // 关键：先隐藏 Live2D 窗口
    ShowWindow(liveHwnd, SW_HIDE);

    // 4 修改样式并嵌入 Qt
    LONG_PTR style = GetWindowLongPtrW(liveHwnd, GWL_STYLE);
    style &= ~(WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME |
        WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
    style |= WS_CHILD;
    SetWindowLongPtrW(liveHwnd, GWL_STYLE, style);

    SetParent(liveHwnd, hostHwnd);

    RECT rc{};
    GetClientRect(hostHwnd, &rc);
    MoveWindow(
        liveHwnd,
        0, 0,
        rc.right - rc.left,
        rc.bottom - rc.top,
        TRUE
    );

    ShowWindow(liveHwnd, SW_SHOW);
}
void qtmain::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (!hostHwnd || !liveHwnd)
        return;

    RECT rc{};
    GetClientRect(hostHwnd, &rc);

    MoveWindow(
        liveHwnd,
        0,
        0,
        rc.right - rc.left,
        rc.bottom - rc.top,
        TRUE
    );
}
