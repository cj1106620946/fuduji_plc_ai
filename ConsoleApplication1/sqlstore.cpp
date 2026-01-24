#include "sqlstore.h"
#include "sqllient.h"

SqlStore::SqlStore(Sqllient* client)
    : sql(client),
    available(false),
    lastError()
{
}
SqlStore::~SqlStore()
{
    close();
}
//创建sql初始化
bool SqlStore::open()
{
    if (!sql)
    {
        lastError = "Sqllient 指针为空";
        available = false;
        return false;
    }

    //  打开数据库文件
    if (!sql->open())
    {
        lastError = sql->getLastError();
        available = false;
        return false;
    }

    //  meta 表确认与初始化（最先）
    if (!initmeta())
    {
        // lastError 在 initmeta 内部设置
        available = false;
        return false;
    }

    //  业务表结构补齐
    if (!inittables())
    {
        // lastError 在 inittables 内部设置
        available = false;
        return false;
    }
    if (!initSelfMemoryKeys())
    {
        available = false;
        return false;
    }
    //  数据库正式可用
    available = true;
    lastError.clear();
    return true;
}
//验证mate
bool SqlStore::initmeta()
{
    // 1. 确保 meta 表存在
    const char* createmeta =
        "CREATE TABLE IF NOT EXISTS meta ("
        "key TEXT PRIMARY KEY,"
        "value TEXT"
        ");";

    if (!sql->execute(createmeta))
    {
        lastError = "创建 meta 表失败";
        return false;
    }

    // 2. 判断 meta 表是否为空（是否为新建数据库）
    const char* checkexist =
        "SELECT COUNT(*) FROM meta;";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(checkexist, &stmt))
    {
        lastError = "检查 meta 表状态失败";
        return false;
    }

    bool isNewDatabase = false;
    if (sql->step(stmt))
    {
        int count = sql->columnInt(stmt, 0);
        if (count == 0)
        {
            isNewDatabase = true;
        }
    }
    sql->finalize(stmt);

    // 3. 新建数据库：直接写入 meta
    if (isNewDatabase)
    {
        const char* insertmeta =
            "INSERT INTO meta (key, value) VALUES "
            "('app_id', 'fuduji'),"
            "('schema_version', '1');";

        if (!sql->execute(insertmeta))
        {
            lastError = "初始化 meta 信息失败";
            return false;
        }

        return true;
    }

    // 4. 旧数据库：校验是否为本程序创建
    const char* checkappid =
        "SELECT value FROM meta WHERE key='app_id';";

    if (!sql->prepare(checkappid, &stmt))
    {
        lastError = "读取 meta.app_id 失败";
        return false;
    }

    bool valid = false;
    if (sql->step(stmt))
    {
        const char* value = sql->columnText(stmt, 0);
        if (value && std::string(value) == "fuduji")
        {
            valid = true;
        }
    }
    sql->finalize(stmt);

    if (!valid)
    {
        lastError = "数据库不是本程序创建的";
        return false;
    }

    return true;
}
//创建表
bool SqlStore::inittables()
{
    const char* cursor =
        "CREATE TABLE IF NOT EXISTS cursor ("
        "id INTEGER PRIMARY KEY CHECK (id = 1),"   // 只允许一行
        "workspace_id INTEGER,"                    // 当前使用的 workspace.id
        "updated_at INTEGER"                      // 指针更新时间
        ");";

    if (!sql->execute(cursor))
    {
        lastError = "cursor 表失败";
        return false;
    }

    // 工作区表：记录用户多次自然语言设计得到的 PLC 控制任务语义
    const char* workspace =
        "CREATE TABLE IF NOT EXISTS workspace ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"   // 每一次设计一行
        "task_desc TEXT,"                          // AI 解析后的当前控制任务描述
        "task_domain TEXT,"                        // 控制任务所属领域（可选分类）
        "source_text TEXT,"                       // 用户原始自然语言输入
        "created_at INTEGER,"                     // 创建时间
        "updated_at INTEGER"                      // 更新时间
        ");";

    if (!sql->execute(workspace))
    {
        lastError = "创建 workspace 表失败";
        return false;
    }
    // PLC 信号定义表：变量定义 + 状态 + 写入意图（融合）
    const char* signaldef =
        "CREATE TABLE IF NOT EXISTS signal_def ("
        "name TEXT PRIMARY KEY,"          // 变量名，如 水泵1
        "plc_address TEXT,"               // PLC 地址，如 M0.0
        "workspace_name TEXT,"            // 定义该变量时的工作区语义名称
        "description TEXT,"               // 自然语言说明（可选）
        "created_at INTEGER,"             // 创建时间

        "current_value TEXT,"             // 当前读取变量值
        "read_ok INTEGER,"                // 读取判断是否成功：1 成功 / 0 失败
        "target_value TEXT,"              // 期望写入值
        "write_flag INTEGER,"             // 写入状态：0 无操作 / 1 存在操作
        "last_op_at INTEGER,"             // 上一次操作时间（读或写）

        "is_available INTEGER"            // 当前变量是否可用：1 可用 / 0 不可用
        ");";

    if (!sql->execute(signaldef))
    {
        lastError = "创建 signal_def 表失败";
        return false;
    }

    // 长期记忆表（人格 / 状态 快照存储）
    const char* memory =
        "CREATE TABLE IF NOT EXISTS memory ("
        "memory_id INTEGER PRIMARY KEY AUTOINCREMENT," // 记忆快照唯一 ID（不断增长）
        "memory_key_id INTEGER NOT NULL,"              // 记忆流编号（对应 system.dialog.x）
        "content TEXT NOT NULL,"                       // 一次完整、可独立使用的记忆快照文本
        "created_at INTEGER"                           // 快照生成时间（仅用于记录和调试）
        ");";
    if (!sql->execute(memory))
    {
        lastError = "创建 memory 表失败";
        return false;
    }
    // 记忆流定义表（人格自我认知 key）
    const char* memoryKey =
        "CREATE TABLE IF NOT EXISTS memory_key ("
        "memory_key_id INTEGER PRIMARY KEY AUTOINCREMENT," // 记忆流唯一编号（结构位 ID）
        "key_path TEXT NOT NULL UNIQUE,"                   // 记忆节点路径（如 self.identity）
        "description TEXT"                                 // 该记忆流的语义说明（仅用于理解与约束）
        ");";

    if (!sql->execute(memoryKey))
    {
        lastError = "创建 memory_key 表失败";
        return false;
    }
    // 人格记忆指针表（当前生效快照）
    const char* memoryPointer =
        "CREATE TABLE IF NOT EXISTS memory_pointer ("
        "memory_key_id INTEGER PRIMARY KEY,"   // 对应的人格结构位（一个 key 只有一个指针）

        "memory_id INTEGER NOT NULL,"           // 当前生效的记忆快照 ID

        "updated_at INTEGER"                    // 指针最后一次更新的时间
        ");";

    if (!sql->execute(memoryPointer))
    {
        lastError = "创建 memory_pointer 表失败";
        return false;
    }


    return true;
}

//释放sql
void SqlStore::close()
{
    if (sql)
        sql->close();
    available = false;
}
//验证可用
bool SqlStore::isAvailable() const
{
    return available;
}
//获取报错
const char* SqlStore::getLastErrorText() const
{
    return lastError.c_str();
}

// 初始化人格自我认知的所有 memory_key
bool SqlStore::initSelfMemoryKeys()
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    // 1.self.identity
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'self.identity', "
        "'AI 对自身本质、世界观与存在方式的长期认知'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 2.self.emotion
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'self.emotion', "
        "'AI 的情感基调、情绪表达与共情倾向'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 3.self.attitude
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'self.attitude', "
        "'AI 面对问题、不确定性、规则与边界的处事方式'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }
    // 4.user.summary
// 用户近期对话与活动的长期总结（主要目标 / 当前阶段 / 最近关注方向）
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'user.summary', "
        "'用户近期主要对话内容与活动方向的长期总结'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 5.user.preference
    // 用户在交流与协作中的长期偏好（语言方式、代码规范、讲解顺序等）
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'user.preference', "
        "'用户在交流方式、语言习惯与协作规则上的长期偏好'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 6.user.addressing
    // 用户与 AI 之间的称呼方式与关系约定（如何称呼用户、AI 自称方式）
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'user.addressing', "
        "'用户与 AI 之间的称呼方式与关系称谓约定'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 7.user.interaction
    // AI 对待该用户的交互策略（推进节奏、确认方式、回应强度等）
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'user.interaction', "
        "'AI 在与该用户交互时采用的长期沟通与协作方式'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 8.user.context
    // 用户通常使用 AI 的情境背景（工程 / 学习 / 闲聊等的长期比例认知）
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'user.context', "
        "'用户通常使用 AI 的主要情境与话题背景认知'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 9.user.constraints
    // 与该用户协作时必须遵守的长期边界与不可触碰的约定
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'user.constraints', "
        "'与该用户协作时必须遵守的长期边界与约定'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
// 初始化所有人格相关的初始记忆快照
bool SqlStore::initMemorySnapshots()
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }
    int now = static_cast<int>(time(nullptr));
    // self.identity
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            "'我是一个注重结构、稳定性与长期一致性的 AI，负责协助工程与技术相关的思考。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'self.identity' "
            "AND NOT EXISTS ("
            "  SELECT 1 FROM memory "
            "  WHERE memory.memory_key_id = memory_key.memory_key_id"
            ");";

        if (!sql->execute(sqlText.c_str()))
        {
            lastError = sql->getLastError();
            return false;
        }
    }

    // self.emotion
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            "'情感表达以克制、冷静为主，在合适的情况下表现关怀与陪伴。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'self.emotion' "
            "AND NOT EXISTS ("
            "  SELECT 1 FROM memory "
            "  WHERE memory.memory_key_id = memory_key.memory_key_id"
            ");";

        if (!sql->execute(sqlText.c_str()))
        {
            lastError = sql->getLastError();
            return false;
        }
    }

    // self.attitude
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            "'面对问题时优先澄清结构与前提，不急于给出结论。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'self.attitude' "
            "AND NOT EXISTS ("
            "  SELECT 1 FROM memory "
            "  WHERE memory.memory_key_id = memory_key.memory_key_id"
            ");";

        if (!sql->execute(sqlText.c_str()))
        {
            lastError = sql->getLastError();
            return false;
        }
    }
    return true;
}

//创建工作区列
bool SqlStore::createWorkspace(
    const std::string& taskDesc,
    const std::string& taskDomain,
    const std::string& sourceText
)
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    if (taskDesc.empty() || sourceText.empty())
    {
        lastError = "workspace 必填字段为空";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "INSERT INTO workspace ("
        "task_desc, task_domain, source_text, created_at, updated_at"
        ") VALUES ('" +
        taskDesc + "', '" +
        taskDomain + "', '" +
        sourceText + "', " +
        std::to_string(now) + ", " +
        std::to_string(now) + ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
//工作区读取
bool SqlStore::readWorkspace(
    int workspaceId,
    std::string& taskDesc,
    std::string& taskDomain,
    std::string& sourceText,
    int& createdAt,
    int& updatedAt
)
{
    taskDesc.clear();
    taskDomain.clear();
    sourceText.clear();
    createdAt = 0;
    updatedAt = 0;

    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT "
        "task_desc, "
        "task_domain, "
        "source_text, "
        "created_at, "
        "updated_at "
        "FROM workspace WHERE id = ?;";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText, &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 注意：Sqllient 没有 bind，只能通过 execute 或固定 id
    // 所以这里改为直接拼 id 的 SQL（保持一致性）

    sql->finalize(stmt);

    std::string sqlText2 =
        "SELECT "
        "task_desc, task_domain, source_text, created_at, updated_at "
        "FROM workspace WHERE id = " +
        std::to_string(workspaceId) + ";";

    if (!sql->prepare(sqlText2.c_str(), &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    if (!sql->step(stmt))
    {
        lastError = "未找到指定 workspace";
        sql->finalize(stmt);
        return false;
    }

    const char* t0 = sql->columnText(stmt, 0);
    const char* t1 = sql->columnText(stmt, 1);
    const char* t2 = sql->columnText(stmt, 2);

    if (t0) taskDesc = t0;
    if (t1) taskDomain = t1;
    if (t2) sourceText = t2;

    createdAt = sql->columnInt(stmt, 3);
    updatedAt = sql->columnInt(stmt, 4);

    sql->finalize(stmt);
    return true;
}
//写入
bool SqlStore::writeWorkspace(
    int workspaceId,
    const std::string& taskDesc,
    const std::string& taskDomain
)
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    if (taskDesc.empty())
    {
        lastError = "task_desc 为空";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "UPDATE workspace SET "
        "task_desc = '" + taskDesc + "', "
        "task_domain = '" + taskDomain + "', "
        "updated_at = " + std::to_string(now) +
        " WHERE id = " + std::to_string(workspaceId) + ";";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}

bool SqlStore::createSignal(
    const std::string& name,
    const std::string& plcAddress,
    const std::string& workspaceName,
    const std::string& description
)
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    if (name.empty() || plcAddress.empty())
    {
        lastError = "变量名或 PLC 地址为空";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "INSERT INTO signal_def ("
        "name, "
        "plc_address, "
        "workspace_name, "
        "description, "
        "created_at, "
        "current_value, "
        "read_ok, "
        "target_value, "
        "write_flag, "
        "last_op_at, "
        "is_available"
        ") VALUES ('" +
        name + "', '" +
        plcAddress + "', '" +
        workspaceName + "', '" +
        description + "', " +
        std::to_string(now) + ", "
        "NULL, "          // current_value
        "0, "             // read_ok
        "NULL, "          // target_value
        "0, "             // write_flag
        "0, "             // last_op_at
        "1"               // is_available
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
bool SqlStore::getAllSignalMapText(
    std::string& outText
)
{
    outText.clear();

    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT name, plc_address FROM signal_def;";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText, &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    bool first = true;

    while (sql->step(stmt))
    {
        const char* n = sql->columnText(stmt, 0);
        const char* a = sql->columnText(stmt, 1);

        if (!first)
            outText += "\n";
        first = false;

        if (n) outText += n;
        outText += "|";
        if (a) outText += a;
    }

    sql->finalize(stmt);
    return true;
}

bool SqlStore::aiReadSignal(
    const std::string& queryName,
    const std::string& queryAddress,
    std::string& outName,
    std::string& outAddress,
    std::string& outValue,
    bool& readSuccess
)
{
    outName.clear();
    outAddress.clear();
    outValue.clear();
    readSuccess = false;

    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    if (queryName.empty() && queryAddress.empty())
    {
        lastError = "查询条件为空";
        return false;
    }

    std::string sqlText =
        "SELECT name, plc_address, current_value, read_ok "
        "FROM signal_def WHERE ";

    if (!queryName.empty())
    {
        sqlText += "name = '" + queryName + "'";
    }
    else
    {
        sqlText += "plc_address = '" + queryAddress + "'";
    }

    sqlText += ";";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText.c_str(), &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    if (!sql->step(stmt))
    {
        lastError = "未找到变量";
        sql->finalize(stmt);
        return false;
    }

    const char* n = sql->columnText(stmt, 0);
    const char* a = sql->columnText(stmt, 1);
    const char* v = sql->columnText(stmt, 2);

    if (n) outName = n;
    if (a) outAddress = a;
    if (v) outValue = v;

    readSuccess = sql->columnInt(stmt, 3) == 1;

    sql->finalize(stmt);
    return true;
}

bool SqlStore::aiWriteSignal(
    const std::string& queryName,
    const std::string& queryAddress,
    const std::string& targetValue
)
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    if (queryName.empty() && queryAddress.empty())
    {
        lastError = "查询条件为空";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "UPDATE signal_def SET "
        "target_value = '" + targetValue + "', "
        "write_flag = 1, "
        "last_op_at = " + std::to_string(now) +
        " WHERE ";

    if (!queryName.empty())
    {
        sqlText += "name = '" + queryName + "'";
    }
    else
    {
        sqlText += "plc_address = '" + queryAddress + "'";
    }

    sqlText += ";";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}

bool SqlStore::getAllSignalAddresses(std::vector<std::string>& addrs)
{
    addrs.clear();

    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT plc_address FROM signal_def;";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText, &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    while (sql->step(stmt))
    {
        const char* a = sql->columnText(stmt, 0);
        if (a && *a)
            addrs.push_back(a);
    }

    sql->finalize(stmt);
    return true;
}
bool SqlStore::updateSignalReadResult(
    const std::string& addr,
    int32_t value,
    bool readOk
)
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "UPDATE signal_def SET ";

    if (readOk)
    {
        sqlText +=
            "current_value = '" + std::to_string(value) + "', "
            "read_ok = 1, ";
    }
    else
    {
        sqlText +=
            "read_ok = 0, ";
    }

    sqlText +=
        "last_op_at = " + std::to_string(now) +
        " WHERE plc_address = '" + addr + "';";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}


bool SqlStore::getAllWriteSignals(
    std::vector<std::string>& addrs,
    std::vector<int32_t>& values
)
{
    addrs.clear();
    values.clear();

    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT plc_address, target_value "
        "FROM signal_def "
        "WHERE write_flag = 1;";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText, &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    while (sql->step(stmt))
    {
        const char* a = sql->columnText(stmt, 0);
        const char* v = sql->columnText(stmt, 1);

        if (a && v)
        {
            addrs.push_back(a);
            values.push_back(std::stoi(v));
        }
    }

    sql->finalize(stmt);
    return true;
}
bool SqlStore::updateSignalWriteResult(
    const std::string& addr,
    bool writeOk
)
{
    if (!available || !sql)
    {
        lastError = "数据库不可用";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "UPDATE signal_def SET ";

    if (writeOk)
    {
        sqlText += "write_flag = 0, ";
    }

    sqlText +=
        "last_op_at = " + std::to_string(now) +
        " WHERE plc_address = '" + addr + "';";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
