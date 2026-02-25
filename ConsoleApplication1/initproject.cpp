#include "initproject.h"
#include <QVBoxLayout>

initproject::initproject(QWidget* parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    tree = ui.tree;
    tree->setColumnCount(2);
    QStringList headers;
    headers << "name" << "value";
    // connect定义（放在构造函数中）
connect(ui.con1, &QPushButton::clicked, this, &initproject::onCon1Clicked);
connect(ui.con2, &QPushButton::clicked, this, &initproject::onCon2Clicked);
connect(ui.con3, &QPushButton::clicked, this, &initproject::onCon3Clicked);
connect(ui.con4, &QPushButton::clicked, this, &initproject::onCon4Clicked);
    tree->setHeaderLabels(headers);
}

initproject::~initproject()
{
}

void initproject::clearTree()
{
    tree->clear();
}

QTreeWidgetItem* initproject::addGroup(const QString& name)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();

    item->setText(0, name);
    item->setExpanded(true);

    tree->addTopLevelItem(item);

    return item;
}

void initproject::addItem(
    QTreeWidgetItem* parentItem,
    const QString& name,
    const QString& value
)
{
    if (!parentItem)
        return;

    QTreeWidgetItem* child = new QTreeWidgetItem();

    child->setText(0, name);
    child->setText(1, value);

    parentItem->addChild(child);
}
void initproject::refreshRows(const std::vector<std::string>& rows)
{
    clearTree();
    // 用栈来维护当前路径
    std::vector<QTreeWidgetItem*> stack;
    for (size_t i = 0; i < rows.size(); ++i)
    {
        std::string line = rows[i];
        if (line.empty())
            continue;

        // 检查是否是根节点标记
        if (line.rfind("#A#", 0) == 0)
        {
            std::string groupName = line.substr(3);
            // 清空栈，创建新的根节点
            stack.clear();
            QTreeWidgetItem* item = new QTreeWidgetItem(tree);
            item->setText(0, QString::fromStdString(groupName));
            stack.push_back(item);
            continue;
        }

        // 检查是否是子节点标记
        if (line.rfind("#B#", 0) == 0)
        {
            std::string groupName = line.substr(3);
            // 在当前最后一个节点下创建子节点
            if (!stack.empty())
            {
                QTreeWidgetItem* parent = stack.back();
                QTreeWidgetItem* item = new QTreeWidgetItem(parent);
                item->setText(0, QString::fromStdString(groupName));
                stack.push_back(item);
            }
            continue;
        }

        // 检查是否是回退标记
        if (line.rfind("#BACK#", 0) == 0)
        {
            // 弹出栈顶，回到上一级
            if (!stack.empty())
            {
                stack.pop_back();
            }
            continue;
        }

        // 普通条目行，格式 "名称|值"
        if (!stack.empty())
        {
            size_t pos = line.find('|');
            if (pos != std::string::npos)
            {
                std::string name = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                QTreeWidgetItem* item = new QTreeWidgetItem(stack.back());
                item->setText(0, QString::fromStdString(name));
                item->setText(1, QString::fromStdString(value));
            }
        }
    }

    tree->expandAll();
}
void initproject::onCon1Clicked()
{
    emit con1Clicked();  // 发射信号
    // TODO: 添加con1按钮点击处理逻辑
}

void initproject::onCon2Clicked()
{
    emit con2Clicked();  // 发射信号
    // TODO: 添加con2按钮点击处理逻辑
}

void initproject::onCon3Clicked()
{
    emit con3Clicked();  // 发射信号
    // TODO: 添加con3按钮点击处理逻辑
}

void initproject::onCon4Clicked()
{
    emit con4Clicked();  // 发射信号
    // TODO: 添加con4按钮点击处理逻辑
}