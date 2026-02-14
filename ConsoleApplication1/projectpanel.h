#pragma once

#include <QWidget>
#include "ui_projectpanel.h"

class projectpanel : public QWidget
{
	Q_OBJECT

public:
	projectpanel(QWidget *parent = nullptr);
	~projectpanel();
	void refreshVariables(const std::vector<std::string>& vars);

private:
	Ui::projectpanelClass ui;
};

