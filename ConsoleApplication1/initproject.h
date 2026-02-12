#pragma once

#include <QWidget>
#include "ui_initproject.h"

class initproject : public QWidget
{
	Q_OBJECT

public:
	initproject(QWidget *parent = nullptr);
	~initproject();

private:
	Ui::initprojectClass ui;
};

