#include "chatpanel.h"
#include "ui_chatpanel.h"

#include <QLineEdit>
#include <QVBoxLayout>
#include <QScrollBar>
#include <QLabel>
#include <QScrollArea>

chatpanel::chatpanel(QWidget* parent)
    : QWidget(parent),
    ui(new Ui::chatpanel),
    messageContainer(nullptr),
    messageLayout(nullptr)
{
    ui->setupUi(this);
    // ================= 输入框视觉优化 =================
    ui->lineEdit->setPlaceholderText(QString::fromUtf8("在这里输入内容..."));
    // 样式设置
    ui->lineEdit->setStyleSheet(
        "QLineEdit {"
        "background-color: rgba(255,255,255,230);"
        "border: 2px solid rgba(210,220,230,255);"
        "border-radius: 18px;"
        "padding: 10px 16px;"
        "font-size: 14px;"
        "color: rgba(40,45,55,255);"
        "}"
        "QLineEdit:focus {"
        "border: 2px solid rgba(180,195,215,255);"
        "}"
        "QLineEdit::placeholder {"
        "color: rgba(150,160,170,200);"
        "}"
    );

    // 回车提交输入
    connect(
        ui->lineEdit,
        &QLineEdit::returnPressed,
        this,
        &chatpanel::onReturnPressed
    );

    // 用滚动区域承载消息卡片
    messageContainer = new QWidget(ui->scrollArea);
    messageLayout = new QVBoxLayout(messageContainer);

    // 外层留白和消息间距
    messageLayout->setContentsMargins(18, 18, 18, 18);
    messageLayout->setSpacing(12);

    // 让消息从上往下堆叠
    messageLayout->addStretch(1);

    ui->scrollArea->setWidget(messageContainer);
    ui->scrollArea->setWidgetResizable(true);

    // 滚动区域本身无边框
    ui->scrollArea->setFrameShape(QFrame::NoFrame);
}

chatpanel::~chatpanel()
{
    delete ui;
}

void chatpanel::onReturnPressed()
{
    const QString text = ui->lineEdit->text();
    if (text.isEmpty())
        return;
    ui->lineEdit->clear();

    if (ui->modeBox)
    {
        mode = ui->modeBox->currentIndex(); // 0 聊天 1 指令
    }
    emit inputSubmitted(text, mode);
}

void chatpanel::appendOutput(const QString& text, int renderType)
{
    if (!messageLayout || !messageContainer)
        return;

    // ===== 如果不是思考气泡且当前存在思考气泡，隐藏它 =====
    if (renderType != 1 && thinkingActive == 1 && thinkingBubble)
    {
        thinkingBubble->hide();
        thinkingActive = 0;
    }

    // ===== 外层行容器 =====
    QWidget* row = new QWidget(messageContainer);
    QHBoxLayout* rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(0);

    // ===== 气泡卡片 =====
    QWidget* bubble = new QWidget(row);
    bubble->setObjectName("messagebubble");

    QVBoxLayout* bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(18, 14, 18, 14);
    bubbleLayout->setSpacing(0);

    QLabel* label = new QLabel(text, bubble);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // 默认样式
    QString textStyle =
        "QLabel {"
        "color: rgba(40,45,55,255);"
        "font-size: 14px;"
        "}";

    QString bubbleStyle =
        "#messagebubble {"
        "background-color: rgba(245,247,250,255);"
        "border: 2px solid rgba(195,205,220,255);"
        "border-radius: 22px;"
        "}";

    // ===== 根据 renderType 修改样式 =====
    if (renderType == 1) // AI 思考中
    {
        textStyle =
            "QLabel {"
            "color: rgba(120,130,150,255);"
            "font-size: 14px;"
            "}";

        bubbleStyle =
            "#messagebubble {"
            "background-color: rgba(235,238,245,255);"
            "border: 2px dashed rgba(180,190,210,255);"
            "border-radius: 22px;"
            "}";

        // 标记当前气泡为思考气泡
        thinkingBubble = row;
        thinkingActive = 1;
    }
    else if (renderType == 2) // 错误
    {
        textStyle =
            "QLabel {"
            "color: rgba(200,60,60,255);"
            "font-size: 14px;"
            "}";

        bubbleStyle =
            "#messagebubble {"
            "background-color: rgba(255,240,240,255);"
            "border: 2px solid rgba(220,120,120,255);"
            "border-radius: 22px;"
            "}";
    }
    else if (renderType == 3) // 系统提示
    {
        textStyle =
            "QLabel {"
            "color: rgba(90,90,90,255);"
            "font-size: 13px;"
            "}";

        bubbleStyle =
            "#messagebubble {"
            "background-color: rgba(240,240,240,255);"
            "border: 1px solid rgba(210,210,210,255);"
            "border-radius: 18px;"
            "}";
    }

    label->setStyleSheet(textStyle);

    bubble->setMaximumWidth(520);
    bubbleLayout->addWidget(label);
    bubble->setStyleSheet(bubbleStyle);

    rowLayout->addWidget(bubble, 0, Qt::AlignLeft);
    rowLayout->addStretch(1);

    int insertIndex = messageLayout->count() - 1;
    if (insertIndex < 0)
        insertIndex = 0;

    messageLayout->insertWidget(insertIndex, row);

    QScrollBar* bar = ui->scrollArea->verticalScrollBar();
    if (bar)
        bar->setValue(bar->maximum());
}
