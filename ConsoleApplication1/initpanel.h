#pragma once

#include <QWidget>
#include "ui_initpanel.h"
#include <string>

class initpanel : public QWidget
{
    Q_OBJECT

public:
    explicit initpanel(QWidget* parent = nullptr);
    ~initpanel();

signals:
    void openRequested(const std::string& path);    // 打开已有工程
    void createRequested(const std::string& path);  // 创建新工程
    void closeRequested();

private slots:
    void onBrowseClicked();
    void onOpenClicked();
    void onCreateClicked();
    void onCloseClicked();

private:
    Ui::initpanelClass ui;
};
