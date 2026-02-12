#pragma once

#include <QWidget>
#include "ui_initpersona.h"

class initpersona : public QWidget
{
	Q_OBJECT

public:
	initpersona(QWidget *parent = nullptr);
	~initpersona();

private:
	Ui::initpersonaClass ui;
};

