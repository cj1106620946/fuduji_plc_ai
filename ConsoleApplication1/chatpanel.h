#pragma once

#include <QWidget>

class Ui_chatpanel;
class QVBoxLayout;

class chatpanel : public QWidget
{
    Q_OBJECT

public:
    explicit chatpanel(QWidget* parent = nullptr);
    ~chatpanel();

    void appendOutput(const QString& text, int renderType);

signals:
    void inputSubmitted(const QString& text);

private slots:
    void onReturnPressed();
    QWidget* thinkingBubble = nullptr;
    int thinkingActive = 0;
private:
    Ui_chatpanel* ui;

    QWidget* messageContainer;
    QVBoxLayout* messageLayout;
};
