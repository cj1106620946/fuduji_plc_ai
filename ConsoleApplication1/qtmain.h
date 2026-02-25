#pragma once

#include <QWidget>
#include <QResizeEvent>
#include <QShowEvent>
#include <QVBoxLayout>
#include <QTreeWidgetItem>
#include <QFile>
#include <QCoreApplication>
#include <QDebug>
#include <QMessageBox>
#include <QCursor>
#include <QHBoxLayout>
#include <QLabel>
#include <windows.h>
#include <QTimer>
#include <QScreen>
#include <string>
#include <thread>
#include <memory>
#include "ui_qtmain.h"
#include "chatpanel.h"
#include <QTreeWidget>
#include "initpanel.h"
#include"projectpanel.h"
#include "initpersona.h"
#include"initproject.h"
class initpersona;
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
	initproject* getInitProject();
    // 外部调用：显示一行文本到聊天面板 
    void updateProjectMirror(const std::vector<std::string>& vars);
    void appendText(const std::string& text, int renderType);
    void showError(const std::string& text);
    void showMiniTip(const std::string& text);
    void updateRenderState();
    void updatePersonaMirror(
        const std::vector<std::string>& rows,
        int writeType
    );

signals:
    // UI 输入文本，交给外部（delegate / AI）
    void uiTextSubmitted(const std::string& text);
signals:
    void personaMirrorEdited(
        const std::string& key,
        const std::string& content
    );

protected:
    // Live2D 嵌入相关
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QLabel* plcStatusLabel;
    QLabel* aiStatusLabel;
    QLabel* liveStatusLabel;

    Ui::qtmain ui;
    UiState& uiState;   // 引用，由 plcai 管理
    //中间聊天面板 
    chatpanel* chat = nullptr;
    initpersona* persona;

    // 左侧导航树
    QTreeWidget* navTree = nullptr;
    initproject* project = nullptr;
    // 中间页面
    initpanel* init = nullptr;
    // 当前页面指针
    QWidget* currentPage = nullptr;
    //Live2D 相关 
    HWND hostHwnd = nullptr;
    HWND liveHwnd = nullptr;
    PROCESS_INFORMATION livePi{};
};
