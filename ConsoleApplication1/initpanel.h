#pragma once

#include <QWidget>
#include "ui_initpanel.h"

class initpanel : public QWidget
{
	Q_OBJECT

public:
	initpanel(QWidget *parent = nullptr);
	~initpanel();

private:
	Ui::initpanelClass ui;
};

