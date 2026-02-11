#include "qtmain.h"

#include <QShowEvent>
#include <QVBoxLayout>
#include <QTreeWidgetItem>

#include <windows.h>

#include "chatpanel.h"
#include "initpanel.h"

// 构造函数
qtmain::qtmain(UiState& stateRef, QWidget* parent)
    : QWidget(parent),
    uiState(stateRef),
    hostHwnd(nullptr),
    liveHwnd(nullptr)
{
    // 初始化 UI
    ui.setupUi(this);
    // ================= 左侧导航树 =================
    navTree = new QTreeWidget(ui.navpanel);
    navTree->setHeaderHidden(true);
    // navpanel 可能已经有布局，不能重复 new
    if (!ui.navpanel->layout())
    {
        QVBoxLayout* navLayout = new QVBoxLayout(ui.navpanel);
        navLayout->setContentsMargins(0, 0, 0, 0);
        navLayout->setSpacing(0);
        navLayout->addWidget(navTree);
    }
    else
    {
        ui.navpanel->layout()->addWidget(navTree);
    }
    // 根节点
    QTreeWidgetItem* root = new QTreeWidgetItem(navTree);
    root->setText(0, u8"功能");
    // 子节点
    QTreeWidgetItem* itemInit = new QTreeWidgetItem(root);
    itemInit->setText(0, u8"初始化");
    QTreeWidgetItem* itemChat = new QTreeWidgetItem(root);
    itemChat->setText(0, u8"聊天");

    navTree->expandAll();
    navTree->setCurrentItem(itemChat);
    // ================= 中间页面容器 =================
    chat = new chatpanel(ui.mainpanel);
    init = new initpanel(ui.mainpanel);
    // mainpanel 也可能已经有布局，不能重复 new
    if (!ui.mainpanel->layout())
    {
        QVBoxLayout* layout = new QVBoxLayout(ui.mainpanel);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(chat);
        layout->addWidget(init);
    }
    else
    {
        ui.mainpanel->layout()->addWidget(chat);
        ui.mainpanel->layout()->addWidget(init);
    }

    // 默认显示 chat
    chat->show();
    init->hide();
    currentPage = chat;
    // ================= 树点击：切换中间页面 =================
    connect(
        navTree,
        &QTreeWidget::itemClicked,
        this,
        [this](QTreeWidgetItem* item, int)
    {
        if (!item)
            return;
        QString name = item->text(0);
        // 点到根节点“功能”不切换
        if (name == u8"功能")
            return;
        if (name == u8"聊天")
        {
            if (currentPage)
                currentPage->hide();
            chat->show();
            currentPage = chat;
        }
        else if (name == u8"初始化")
        {
            if (currentPage)
                currentPage->hide();

            init->show();
            currentPage = init;
        }
    });

    // ================= chatpanel 输入：转发给外部 =================
    connect(
        chat,
        &chatpanel::inputSubmitted,
        this,
        [this](const QString& text)
    {
        emit uiTextSubmitted(text.toStdString());
    });

    // ================= 右侧 Live2D 承载 =================
    ui.rightpanel->setAttribute(Qt::WA_NativeWindow);
    ui.rightpanel->setAttribute(Qt::WA_DontCreateNativeAncestors);
}

    // 析构函数
    qtmain::~qtmain()
    {
        // 如果存在 Live2D 子窗口
        if (liveHwnd)
        {
            SetParent(liveHwnd, nullptr);
            liveHwnd = nullptr;
        }

        // 如果进程存在，终止进程
        if (livePi.hProcess)
        {
            TerminateProcess(livePi.hProcess, 0);
            CloseHandle(livePi.hProcess);
            livePi.hProcess = nullptr;
        }
    }

initpanel* qtmain::getInitPanel()
{
    return init;
}
// 给外部调用：显示文本（目前只输出到 chat）
void qtmain::appendText(const std::string& text)
{
    if (!chat)
        return;

    chat->appendOutput(QString::fromStdString(text));
}

// 窗口显示后嵌入 Live2D
void qtmain::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    hostHwnd = (HWND)ui.rightpanel->winId();

    updateRenderState();
}

// 窗口尺寸变化时同步 Live2D
void qtmain::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (uiState.live2dEnabled == 2)
    {
        updateRenderState();
    }
}

void qtmain::updateRenderState()
{
    if (uiState.live2dEnabled == 0)
    {
        if (livePi.hProcess)
        {
            TerminateProcess(livePi.hProcess, 0);
            CloseHandle(livePi.hProcess);
            livePi.hProcess = nullptr;
        }

        liveHwnd = nullptr;
        return;
    }
    if (uiState.live2dEnabled == 1)
    {
        if (liveHwnd)  // 已存在就不重复创建
            return;

        if (!hostHwnd)
            return;

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

        ShowWindow(liveHwnd, SW_HIDE);

        LONG_PTR style = GetWindowLongPtrW(liveHwnd, GWL_STYLE);
        style &= ~(WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME |
            WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
        style |= WS_CHILD;
        SetWindowLongPtrW(liveHwnd, GWL_STYLE, style);

        SetParent(liveHwnd, hostHwnd);

        return;
    }
    if (uiState.live2dEnabled!=1&&uiState.live2dEnabled == 2)
    {
        if (!liveHwnd || !hostHwnd)
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

        ShowWindow(liveHwnd, SW_SHOW);
    }
}
