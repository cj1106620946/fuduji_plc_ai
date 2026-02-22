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

private:
    Ui::initprojectClass ui;
    QTreeWidget* tree = nullptr;
};