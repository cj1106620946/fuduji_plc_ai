#include "initpersona.h"
#include <QTableWidgetItem>

initpersona::initpersona(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    // 设置两列
    ui.table->setColumnCount(2);
    QStringList headers;
    headers << "Key" << "Content";
    ui.table->setHorizontalHeaderLabels(headers);
    // 第一列自动适应内容
    ui.table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    // 第二列拉伸
    ui.table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui.table->verticalHeader()->setVisible(false);
    ui.table->setRowCount(0);
    connect(
        ui.table,
        &QTableWidget::itemChanged,
        this,
        &initpersona::onItemChanged
    );

}

initpersona::~initpersona()
{
}
void initpersona::onItemChanged(QTableWidgetItem* item)
{
    if (!item)
        return;

    int row = item->row();
    int col = item->column();

    // 只处理第二列（Content 列）
    if (col != 1)
        return;

    QString key = ui.table->item(row, 0)->text();
    QString content = item->text();

    // 发信号给上层
    emit personaContentChanged(
        key.toStdString(),
        content.toStdString()
    );
}

void initpersona::refreshRows(const std::vector<std::string>& rows)
{
    ui.table->clearContents();
    ui.table->setRowCount(static_cast<int>(rows.size()));

    for (int i = 0; i < rows.size(); ++i)
    {
        std::string line = rows[i];

        std::string key;
        std::string content;

        size_t pos = line.find('|');

        if (pos != std::string::npos)
        {
            key = line.substr(0, pos);
            content = line.substr(pos + 1);
        }
        else
        {
            key = line;
            content = "";
        }

        QTableWidgetItem* keyItem =
            new QTableWidgetItem(QString::fromStdString(key));

        QTableWidgetItem* contentItem =
            new QTableWidgetItem(QString::fromStdString(content));

        // 第一列禁止编辑
        keyItem->setFlags(keyItem->flags() & ~Qt::ItemIsEditable);

        ui.table->setItem(i, 0, keyItem);
        ui.table->setItem(i, 1, contentItem);
    }

    ui.table->resizeRowsToContents();
}
