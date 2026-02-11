#pragma once

#include <QWidget>
#include <QResizeEvent>
#include <string>
#include <windows.h>
#include <thread>
#include <memory>
#include "ui_qtmain.h"
#include "chatpanel.h"

#include <QTreeWidget>
#include "initpanel.h"
struct UiState
{
    int live2dEnabled;     // 是否允许启动 Live2D
};

class qtmain : public QWidget
{
    Q_OBJECT
public:
    explicit qtmain(
        UiState& stateRef, 
        QWidget* parent = nullptr
    );
    ~qtmain();
    initpanel* getInitPanel();
    // 外部调用：显示一行文本到聊天面板 
    void appendText(const std::string& text);
    void updateRenderState();

signals:
    // UI 输入文本，交给外部（delegate / AI）
    void uiTextSubmitted(const std::string& text);
protected:
    // Live2D 嵌入相关
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    Ui::qtmain ui;
    UiState& uiState;   // 引用，由 plcai 管理
    //中间聊天面板 
    chatpanel* chat = nullptr;
    // 左侧导航树
    QTreeWidget* navTree = nullptr;
    // 中间页面
    initpanel* init = nullptr;
    // 当前页面指针
    QWidget* currentPage = nullptr;
    //Live2D 相关 
    HWND hostHwnd = nullptr;
    HWND liveHwnd = nullptr;
    PROCESS_INFORMATION livePi{};
};
