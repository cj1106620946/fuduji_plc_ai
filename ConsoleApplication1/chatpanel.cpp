#include "chatpanel.h"
#include "ui_chatpanel.h"

#include <QLineEdit>
#include <QPlainTextEdit>

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
    const QString text = ui->lineEdit->text();
    if (text.isEmpty())
        return;
    // 清空输入框
    ui->lineEdit->clear();
    // 只发信号，不处理任何业务
    emit inputSubmitted(text);
}
void chatpanel::appendOutput(const QString& text)
{
    ui->plainTextEdit->appendPlainText(text);
}
