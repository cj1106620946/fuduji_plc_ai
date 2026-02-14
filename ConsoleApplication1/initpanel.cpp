#include "initpanel.h"
#include "createprojectdialog.h"

#include <QFileDialog>
#include <QPushButton>
#include <QLineEdit>

initpanel::initpanel(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);

    connect(ui.btnBrowse, &QPushButton::clicked,
        this, &initpanel::onBrowseClicked);
    connect(ui.btnOpen, &QPushButton::clicked,
        this, &initpanel::onOpenClicked);
    connect(ui.btnCreate, &QPushButton::clicked,
        this, &initpanel::onCreateClicked);
    connect(ui.btnClose,  &QPushButton::clicked,
        this, &initpanel::onCloseClicked);

}

initpanel::~initpanel()
{
}

void initpanel::onBrowseClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择数据库文件",
        "",
        "Database Files (*.db)"
    );

    if (!fileName.isEmpty())
    {
        ui.lineEditPath->setText(fileName);
    }
}

void initpanel::onOpenClicked()
{
    QString path = ui.lineEditPath->text();
    if (path.isEmpty())
        return;

    emit openRequested(path.toStdString());
}

void initpanel::onCreateClicked()
{
    CreateProjectDialog dlg(this);

    if (dlg.exec() == QDialog::Accepted)
    {
        std::string path = dlg.getFullPath();
        if (!path.empty())
        {
            emit createRequested(path);
        }
    }
}
void initpanel::onCloseClicked()
{
    emit closeRequested();
}
