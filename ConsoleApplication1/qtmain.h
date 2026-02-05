#pragma once

#include <windows.h>
#include <QWidget>
#include <QShowEvent>
#include <QResizeEvent>
#include <thread>
#include <memory>

#include "ui_qtmain.h"

class Console;

class qtmain : public QWidget
{
    Q_OBJECT

public:
    explicit qtmain(QWidget* parent = nullptr);
    ~qtmain();

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::qtmainClass ui;

    bool consoleStarted;
    std::unique_ptr<Console> consoleApp;
    std::thread consoleThread;

    HWND hostHwnd;
    HWND liveHwnd;
    PROCESS_INFORMATION livePi;

private:
    void startConsole();
};
