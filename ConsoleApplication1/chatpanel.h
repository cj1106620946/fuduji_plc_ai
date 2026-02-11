#pragma once

#include <QWidget>
#include <QString>

namespace Ui {
    class chatpanel;
}

class chatpanel : public QWidget
{
    Q_OBJECT

public:
    explicit chatpanel(QWidget* parent = nullptr);
    ~chatpanel();
    // 外部调用：显示一行文本
    void appendOutput(const QString& text);
signals:
    // 输入提交给外部（qtmain / delegate）
    void inputSubmitted(const QString& text);
private slots:
    void onReturnPressed();
private:
    Ui::chatpanel* ui;
};
