#include "createprojectdialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QLabel>

CreateProjectDialog::CreateProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("创建工程");
    resize(400, 200);

    QLabel* labelPath = new QLabel("保存路径:", this);
    QLabel* labelName = new QLabel("工程名称:", this);

    editPath = new QLineEdit(this);
    editName = new QLineEdit(this);

    btnBrowse = new QPushButton("浏览", this);
    btnOk = new QPushButton("确定", this);
    btnCancel = new QPushButton("取消", this);

    QHBoxLayout* pathLayout = new QHBoxLayout;
    pathLayout->addWidget(editPath);
    pathLayout->addWidget(btnBrowse);

    QHBoxLayout* btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    btnLayout->addWidget(btnOk);
    btnLayout->addWidget(btnCancel);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(labelPath);
    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(labelName);
    mainLayout->addWidget(editName);
    mainLayout->addLayout(btnLayout);

    connect(btnBrowse, &QPushButton::clicked, this, [this]()
    {
        QString dir = QFileDialog::getExistingDirectory(
            this,
            "选择保存目录"
        );

        if (!dir.isEmpty())
        {
            editPath->setText(dir);
        }
    });

    connect(btnOk, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancel, &QPushButton::clicked, this, &QDialog::reject);
}

std::string CreateProjectDialog::getFullPath() const
{
    QString dir = editPath->text();
    QString name = editName->text();

    if (dir.isEmpty() || name.isEmpty())
        return "";

    QString full = dir + "/" + name + ".db";
    return full.toStdString();
}
