#pragma once

#include <QDialog>
#include <string>

class QLineEdit;
class QPushButton;

class CreateProjectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateProjectDialog(QWidget* parent = nullptr);

    std::string getFullPath() const;

private:
    QLineEdit* editPath;
    QLineEdit* editName;
    QPushButton* btnBrowse;
    QPushButton* btnOk;
    QPushButton* btnCancel;
};
