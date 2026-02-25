#include "qtmain.h"



// 构造函数
qtmain::qtmain(UiState& stateRef, QWidget* parent)
    : QWidget(parent),
    uiState(stateRef),
    hostHwnd(nullptr),
    liveHwnd(nullptr),
    persona(nullptr)
{
    // ================= 基础UI初始化 =================
    ui.setupUi(this);
    // ================= 初始化 splitter 比例 =================

    QList<int> sizes1;
    sizes1 << 10 << 70 << 20; 
    ui.splitter->setSizes(sizes1);

    QList<int> sizes2;
    sizes2 << 50 << 50;
    ui.splitter_2->setSizes(sizes2);

    QList<int> sizes3;
    sizes3 << 10 << 85<<5;
    ui.splitter_3->setSizes(sizes3);

    // ================= 顶部状态面板初始化 =================
    // 在 qtmain.cpp 的构造函数或初始化函数中添加
    if (!ui.statuspanel->layout()) {
        QHBoxLayout* statusLayout = new QHBoxLayout(ui.statuspanel);
        statusLayout->setContentsMargins(10, 0, 10, 0);
        statusLayout->setSpacing(15);
    }

    QHBoxLayout* statusLayout = qobject_cast<QHBoxLayout*>(ui.statuspanel->layout());

    // 创建带图片的 QLabel
    QLabel* plcStatusLabel = new QLabel(ui.statuspanel);
    QLabel* aiStatusLabel = new QLabel(ui.statuspanel);
    QLabel* liveStatusLabel = new QLabel(ui.statuspanel);

    // 调大尺寸，比如 48x48 或 64x64
    plcStatusLabel->setPixmap(QPixmap(":/new/prefix1/icons/png (7).png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    aiStatusLabel->setPixmap(QPixmap(":/new/prefix1/icons/png (8).png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    liveStatusLabel->setPixmap(QPixmap(":/new/prefix1/icons/png (9).png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    // 添加到布局
    statusLayout->addWidget(plcStatusLabel);
    statusLayout->addWidget(aiStatusLabel);
    statusLayout->addWidget(liveStatusLabel);
    statusLayout->addStretch();

    // ================= 左侧导航区域 =================
    navTree = new QTreeWidget(ui.navpanel);
    navTree->setHeaderHidden(true);


    // 若 navpanel 尚未设置布局，则创建垂直布局
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


    // ================= 构建导航树结构 =================
    QTreeWidgetItem* root = new QTreeWidgetItem(navTree);
    root->setText(0, u8"功能");

    QTreeWidgetItem* itemInit = new QTreeWidgetItem(root);
    itemInit->setText(0, u8"初始化");

    QTreeWidgetItem* itemProject = new QTreeWidgetItem(root);
    itemProject->setText(0, u8"工程镜像");

    QTreeWidgetItem* itemPersona = new QTreeWidgetItem(root);
    itemPersona->setText(0, u8"人格管理");


    navTree->expandAll();
    navTree->setCurrentItem(itemInit);
    // ================= 中间页面初始化（上半部分：业务页面） =================
    // init 页面
    init = new initpanel(ui.pageInit);
    if (!ui.pageInit->layout())
    {
        QVBoxLayout* layout = new QVBoxLayout(ui.pageInit);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }
    ui.pageInit->layout()->addWidget(init);

    // project 页面
    project = new initproject(ui.pageProject);
    if (!ui.pageProject->layout())
    {
        QVBoxLayout* layout = new QVBoxLayout(ui.pageProject);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }

	// persona 页面
    persona = new initpersona(ui.pagePersona);
    if (!ui.pagePersona->layout())
    {
        QVBoxLayout* layout = new QVBoxLayout(ui.pagePersona);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }
    ui.pagePersona->layout()->addWidget(persona);


    ui.pageProject->layout()->addWidget(project);
    // 默认显示聊天页
    ui.mainpanel->setCurrentWidget(ui.pageInit);
// ================= 下半部分：常驻输入输出界面 =================
    if (!ui.iointerface->layout())
    {
        QVBoxLayout* layout = new QVBoxLayout(ui.iointerface);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
    }

    chat = new chatpanel(ui.iointerface);
    ui.iointerface->layout()->addWidget(chat);

    // ================= 导航点击事件处理 =================
    connect(
        navTree,
        &QTreeWidget::itemClicked,
        this,
        [this](QTreeWidgetItem* item, int)
    {
        if (!item)
            return;

        QString name = item->text(0);
         if (name == u8"初始化")
        {
            ui.mainpanel->setCurrentWidget(ui.pageInit);
        }
        else if (name == u8"工程镜像")
        {
            ui.mainpanel->setCurrentWidget(ui.pageProject);
        }
        else if (name == u8"人格管理")
         {
             ui.mainpanel->setCurrentWidget(ui.pagePersona);
         }
    });

    // ================= 聊天输入信号转发 =================
    connect(
        chat,
        &chatpanel::inputSubmitted,
        this,
        [this](const QString& text)
    {
        emit uiTextSubmitted(text.toStdString());
    });
    connect(
        persona,
        &initpersona::personaContentChanged,
        this,
        [this](const std::string& key, const std::string& content)
    {
        emit personaMirrorEdited(key, content);
    });

    // ================= Live2D 承载窗口初始化 =================
    ui.rightpanel->setAttribute(Qt::WA_NativeWindow);
    ui.rightpanel->setAttribute(Qt::WA_DontCreateNativeAncestors);
    this->show();
    this->adjustSize();
}


// 析构函数
qtmain::~qtmain()
{
    // 若存在 Live2D 子窗口，解除父子关系
    if (liveHwnd)
    {
        SetParent(liveHwnd, nullptr);
        liveHwnd = nullptr;
    }
    // 若存在 Live2D 进程，终止并释放句柄
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
initproject* qtmain::getInitProject()
{
    return project;
}

//输出文本
void qtmain::appendText(const std::string& text, int renderType)
{
    if (!chat)
        return;
    updateRenderState();
    chat->appendOutput(
        QString::fromStdString(text),
        renderType
    );
}
//弹窗报错
void qtmain::showError(const std::string& text)
{
    QMessageBox::critical(
        this,
        QString::fromUtf8("错误"),
        QString::fromStdString(text),
        QMessageBox::Ok
    );
}
//迷你提示
void qtmain::showMiniTip(const std::string& text)
{
    QWidget* tip = new QWidget(nullptr);
    tip->setAttribute(Qt::WA_DeleteOnClose);
    tip->setWindowFlags(
        Qt::Tool |
        Qt::FramelessWindowHint |
        Qt::WindowStaysOnTopHint |
        Qt::WindowDoesNotAcceptFocus
    );
    tip->setAttribute(Qt::WA_TranslucentBackground);
    QLabel* label = new QLabel(QString::fromStdString(text), tip);
    label->setStyleSheet(
        "QLabel {"
        "background-color: rgba(40,40,40,220);"
        "color: white;"
        "padding: 8px 12px;"
        "border-radius: 8px;"
        "font-size: 13px;"
        "}"
    );
    QVBoxLayout* layout = new QVBoxLayout(tip);
    layout->addWidget(label);
    layout->setContentsMargins(0, 0, 0, 0);

    tip->adjustSize();

    QPoint cursorPos = QCursor::pos();
    int x = cursorPos.x() + 15;
    int y = cursorPos.y() + 20;

    tip->move(x, y);
    tip->show();

    QTimer::singleShot(2000, tip, &QWidget::close);
}

void qtmain::updatePersonaMirror(
    const std::vector<std::string>& rows,
    int mirrorType
)
{
    if (mirrorType == 1)
    {
        if (!persona)
            return;
        persona->refreshRows(rows);
    }
    else if (mirrorType == 2)
    {
        if (!project)
            return;
        project->refreshRows(rows);
    }
}




// 窗口尺寸变化时同步 Live2D
void qtmain::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    int h = ui.navpanel->height();
    int w = ui.navpanel->width();

    int base = h < w ? h : w;

    // 固定基准
    int fontSize = 12;

    // 只做小幅调整
    if (base > 500)
        fontSize = 14;
    if (base > 700)
        fontSize = 15;
    if (base > 900)
        fontSize = 16;

    if (fontSize > 16)
        fontSize = 16;

    QFont f = navTree->font();
    f.setPointSize(fontSize);
    navTree->setFont(f);

    navTree->setStyleSheet(
        QString("QTreeWidget::item { height: %1px; }")
        .arg(fontSize * 2 + 4)
    );

    if (uiState.live2dEnabled == 2)
    {
        updateRenderState();
    }
}
void qtmain::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    hostHwnd = (HWND)ui.rightpanel->winId();

    updateRenderState();
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
void qtmain::updateProjectMirror(const std::vector<std::string>& vars)
{

}
