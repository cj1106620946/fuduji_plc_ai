#pragma once

#include <QWidget>
#include <vector>
#include <string>
#include "ui_initpersona.h"

class initpersona : public QWidget
{
    Q_OBJECT

public:
    initpersona(QWidget* parent = nullptr);
    ~initpersona();

    void refreshRows(const std::vector<std::string>& rows);
private slots:
    void onItemChanged(QTableWidgetItem* item);
signals:
    void personaContentChanged(
        const std::string& key,
        const std::string& content
    );

private:
    Ui::initpersonaClass ui;
};
