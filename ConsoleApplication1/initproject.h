#pragma once

#include <QWidget>
#include <QTreeWidgetItem>
#include <vector>
#include <string>
#include "ui_initproject.h"

class initproject : public QWidget
{
    Q_OBJECT

public:
    explicit initproject(QWidget* parent = nullptr);
    ~initproject();

    void refreshRows(const std::vector<std::string>& rows);

private:
    void clearTree();
    QTreeWidgetItem* addGroup(const QString& name);
    void addItem(
        QTreeWidgetItem* parentItem,
        const QString& name,
        const QString& value
    );
signals:
    void con1Clicked();
    void con2Clicked();
    void con3Clicked();
    void con4Clicked();
private slots:
    void onCon1Clicked();
    void onCon2Clicked();
    void onCon3Clicked();
    void onCon4Clicked();
private:
    Ui::initprojectClass ui;
    QTreeWidget* tree = nullptr;
};