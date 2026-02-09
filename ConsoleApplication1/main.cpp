#include <QApplication>
#include "ai_plc_delegate.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    ai_plc_delegate delegate;
    delegate.run();
    return app.exec();
}

/*
#include <iostream>
#include <windows.h>
#include <string>
#include <thread>

#include "console.h"
#include "plcclient.h"
#include "aiclient.h"



int main(int argc, char* argv[])
{
    // 控制台程序入口
    Console app;
    // 主线程继续跑控制台（你原有逻辑）
    app.run();
    return 0;
}*/