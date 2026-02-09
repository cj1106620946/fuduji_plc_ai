#include "ai_plc_delegate.h"

// 构造函数
// 只负责让对象处于“可进入 init/run 阶段”的状态
ai_plc_delegate::ai_plc_delegate()
    : ui(nullptr),
    sqlClient(nullptr),
    sqlStore(nullptr),
    chatAi(nullptr),
    memoryAi(nullptr),
    workspaceAi(nullptr),
    executeAi(nullptr),
    decisionAi(nullptr),
    project(nullptr),
    persona(nullptr),
    speech(nullptr),
    live2dWriter("live2dstate.json")
{
    // 骨架阶段：不做任何初始化
}

// 析构函数
// 骨架阶段不负责释放资源
ai_plc_delegate::~ai_plc_delegate()
{
    // 后续由你决定是否在这里释放
}

bool ai_plc_delegate::initQt()
{

    return true;
}

bool ai_plc_delegate::initSql()
{
    // 1. 创建 Sqllient（仅对象，不触发业务）
    //    数据库文件是否存在的判断，应在 Sqllient 或此处完成

    // 2. 创建 SqlStore（绑定 client）
    //    不做任何读取、不加载镜像

    // 3. 打开数据库
    //    - 若数据库不存在：创建新数据库 + 初始化 schema + 初始镜像
    //    - 若数据库存在：校验 meta + schema + 指针
    //    所有逻辑封装在 SqlStore::open() 内

    // 4. 打开失败：
    //    - 记录错误
    //    - 返回 false，主循环终止

    // 5. 打开成功：
    //    - 不主动修改任何数据
    //    - 不写入任何状态
    //    - 仅代表“工程世界已恢复”

    return true;
}

void ai_plc_delegate::run()
{
    if (!initQt())
        return;

}

// 错误上报接口（主循环统一处理）
// 骨架阶段不实现任何行为
void ai_plc_delegate::logError(
    const std::string& fromFunc,
    const std::string& reason
)
{
    // 之后统一在这里处理日志 / UI / 控制台
}
