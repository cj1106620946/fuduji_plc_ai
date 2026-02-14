#include "projectpanel.h"

projectpanel::projectpanel(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

projectpanel::~projectpanel()
{}

void projectpanel::refreshVariables(const std::vector<std::string>& vars)
{
    ui.variableTable->clear();
    ui.variableTable->setRowCount(static_cast<int>(vars.size()));
    ui.variableTable->setColumnCount(1);
    ui.variableTable->setHorizontalHeaderLabels(QStringList() << "鍙橀噺");

    for (int i = 0; i < vars.size(); ++i)
    {
        ui.variableTable->setItem(
            i,
            0,
            new QTableWidgetItem(QString::fromStdString(vars[i]))
        );
    }

    ui.variableTable->resizeColumnsToContents();
}
