#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui { class chatpanel; }
QT_END_NAMESPACE

class chatpanel : public QWidget
{
    Q_OBJECT

public:
    explicit chatpanel(QWidget* parent = nullptr);
    ~chatpanel();

    // 由外部调用，用于向输出区追加文本
    void appendOutput(const QString& text);

signals:
    // 用户在输入框按下回车后发出
    void inputSubmitted(const QString& text);

private slots:
    // 处理回车
    void onReturnPressed();

private:
    Ui::chatpanel* ui;
};
