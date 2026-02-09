#pragma once

#include <QWidget>
#include <QResizeEvent>

#include <windows.h>
#include <thread>
#include <memory>

#include "ui_qtmain.h"
#include "chatpanel.h"
#include "console.h"

class qtmain : public QWidget
{
    Q_OBJECT

public:
    explicit qtmain(QWidget* parent = nullptr);
    ~qtmain();

protected:
    // Live2D 嵌入相关
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void startConsole();

private:
    Ui::qtmain ui;

    // ================= 中间聊天面板 =================
    chatpanel* chat = nullptr;

    // ================= Console =================
    bool consoleStarted = false;
    std::unique_ptr<Console> consoleApp;
    std::thread consoleThread;

    // ================= Live2D 相关 =================
    HWND hostHwnd = nullptr;
    HWND liveHwnd = nullptr;
    PROCESS_INFORMATION livePi{};
};
