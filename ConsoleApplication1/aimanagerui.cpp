#include "aimanagerui.h"
#include <vector>
#include <string>

static const wchar_t* UI_CLASS_NAME = L"AiManagerUIClass";

// 顶部状态栏高度
static const int STATUS_BAR_HEIGHT = 48;

// 底部控制栏高度
static const int CONTROL_BAR_HEIGHT = 56;

// 中间输入区固定高度
static const int MAIN_INPUT_HEIGHT = 80;

// 顶部状态栏区域
static RECT rcStatus{};

// 中间主区域
static RECT rcMain{};

// 中间上部输出区域
static RECT rcMainOutput{};

// 中间下部输入区域
static RECT rcMainInput{};

// 底部控制区域
static RECT rcControl{};

// 输入框控件
static HWND hInputEdit = nullptr;

// 输出消息列表（等价于以前的控制台输出流）
static std::vector<std::wstring> gOutputLines;

AiManagerUI::AiManagerUI()
{
}

AiManagerUI::~AiManagerUI()
{
}

bool AiManagerUI::create(HINSTANCE instance)
{
    hinstance = instance;

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = AiManagerUI::wndproc;
    wc.hInstance = hinstance;
    wc.lpszClassName = UI_CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = NULL;

    if (!RegisterClassExW(&wc))
        return false;

    hwnd = CreateWindowExW(
        0,
        UI_CLASS_NAME,
        L"AiManagerUI",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        900,
        600,
        nullptr,
        nullptr,
        hinstance,
        nullptr
    );

    if (!hwnd)
        return false;

    // 创建输入框控件
    hInputEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT",
        L"",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        0, 0, 0, 0,
        hwnd,
        nullptr,
        hinstance,
        nullptr
    );

    // 初始化一次布局
    RECT rc{};
    GetClientRect(hwnd, &rc);
    SendMessageW(hwnd, WM_SIZE, 0, MAKELPARAM(rc.right, rc.bottom));

    return true;
}

void AiManagerUI::show()
{
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
}

void AiManagerUI::loop()
{
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

LRESULT CALLBACK AiManagerUI::wndproc(
    HWND hwnd,
    UINT msg,
    WPARAM wparam,
    LPARAM lparam)
{
    switch (msg)
    {
    case WM_ERASEBKGND:
        return 1;

    case WM_SIZE:
    {
        int width = LOWORD(lparam);
        int height = HIWORD(lparam);

        rcStatus = { 0, 0, width, STATUS_BAR_HEIGHT };
        rcControl = { 0, height - CONTROL_BAR_HEIGHT, width, height };
        rcMain = { 0, STATUS_BAR_HEIGHT, width, height - CONTROL_BAR_HEIGHT };

        rcMainOutput = {
            rcMain.left,
            rcMain.top,
            rcMain.right,
            rcMain.bottom - MAIN_INPUT_HEIGHT
        };

        rcMainInput = {
            rcMain.left,
            rcMainOutput.bottom,
            rcMain.right,
            rcMain.bottom
        };

        if (hInputEdit)
        {
            int margin = 10;
            MoveWindow(
                hInputEdit,
                rcMainInput.left + margin,
                rcMainInput.top + margin,
                (rcMainInput.right - rcMainInput.left) - margin * 2,
                (rcMainInput.bottom - rcMainInput.top) - margin * 2,
                TRUE
            );
        }

        InvalidateRect(hwnd, nullptr, TRUE);
        return 0;
    }

    case WM_KEYDOWN:
    {
        // 当输入框有焦点并且按下回车
        if (wparam == VK_RETURN && GetFocus() == hInputEdit)
        {
            wchar_t buffer[512]{};
            GetWindowTextW(hInputEdit, buffer, 512);

            if (buffer[0] != L'\0')
            {
                // 等价于以前的 pushUserInput
                gOutputLines.push_back(buffer);
                SetWindowTextW(hInputEdit, L"");
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            return 0;
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);

        // 顶部状态栏
        FillRect(hdc, &rcStatus, CreateSolidBrush(RGB(90, 130, 180)));

        // 输出区
        FillRect(hdc, &rcMainOutput, CreateSolidBrush(RGB(245, 247, 250)));

        // 输入区
        FillRect(hdc, &rcMainInput, CreateSolidBrush(RGB(225, 230, 235)));

        // 画输出文本
        int y = rcMainOutput.top + 10;
        for (const auto& line : gOutputLines)
        {
            TextOutW(hdc, rcMainOutput.left + 10, y, line.c_str(), (int)line.size());
            y += 20;
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}
