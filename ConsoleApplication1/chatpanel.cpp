#include "chatpanel.h"
#include "ui_chatpanel.h"

chatpanel::chatpanel(QWidget* parent)
    : QWidget(parent),
    ui(new Ui::chatpanel)
{
    ui->setupUi(this);

    // 输出区只读
    ui->plainTextEdit->setReadOnly(true);

    // 回车提交输入
    connect(
        ui->lineEdit,
        &QLineEdit::returnPressed,
        this,
        &chatpanel::onReturnPressed
    );
}

chatpanel::~chatpanel()
{
    delete ui;
}

void chatpanel::onReturnPressed()
{
    QString text = ui->lineEdit->text();
    if (text.isEmpty())
        return;

    // 清空输入框
    ui->lineEdit->clear();

    // 发出信号，交给外部（qtmain）处理
    emit inputSubmitted(text);
}

void chatpanel::appendOutput(const QString& text)
{
    // 向输出区追加一行
    ui->plainTextEdit->appendPlainText(text);
}
