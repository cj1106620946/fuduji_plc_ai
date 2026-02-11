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
        lastError = u8"Sqllient 指针为空";
        available = false;
        return false;
    }
    if (!sql->open())
    {
        lastError = sql->getLastError();
        available = false;
        return false;
    }
    if (!initmeta())
    {
        available = false;
        return false;
    }
    if (!inittables())
    {
        available = false;
        return false;
    }
    if (!initSelfMemoryKeys())
    {
        available = false;
        return false;
    }
    // 初始化人格记忆快照（每个 key 至少一条 memory）
    if (!initMemorySnapshots())
    {
        available = false;
        return false;
    }
    // 初始化人格记忆指针（每个 key 一个 pointer）
    if (!initMemoryPointer())
    {
        available = false;
        return false;
    }
    //  数据库正式可用
    available = true;
    lastError.clear();
    return true;
}
bool SqlStore::initmeta()
{
    const char* createmeta =
        "CREATE TABLE IF NOT EXISTS meta ("
        "key TEXT PRIMARY KEY,"
        "value TEXT"
        ");";
    if (!sql->execute(createmeta))
    {
        lastError = u8"创建 meta 表失败";
        return false;
    }
    const char* checkTables =
        "SELECT name FROM sqlite_master WHERE type='table';";

    sqlite3_stmt* stmt = nullptr;

    if (!sql->prepare(checkTables, &stmt))
    {
        lastError = u8"检查数据库结构失败";
        return false;
    }

    int tableCount = 0;
    bool onlyMeta = true;

    while (sql->step(stmt))
    {
        const char* name = sql->columnText(stmt, 0);
        if (name)
        {
            tableCount++;
            if (std::string(name) != "meta")
            {
                onlyMeta = false;
            }
        }
    }

    sql->finalize(stmt);

    if (tableCount == 1 && onlyMeta)
    {
        const char* insertmeta =
            "INSERT INTO meta (key, value) VALUES "
            "('app_id', 'fuduji'),"
            "('schema_version', '1');";

        if (!sql->execute(insertmeta))
        {
            lastError = u8"初始化 meta 信息失败";
            return false;
        }

        return true;
    }


    const char* checkappid =
        "SELECT value FROM meta WHERE key='app_id';";

    if (!sql->prepare(checkappid, &stmt))
    {
        lastError = u8"读取 meta.app_id 失败";
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
        lastError = u8"数据库不是本程序创建的";
        return false;
    }

    return true;
}
//创建表
bool SqlStore::inittables()
{
// PLC 控制上下文表（唯一根：原 workspace + plc_info 融合）
const char* plcInfo =
    "CREATE TABLE IF NOT EXISTS plc_info ("
    "plc_id INTEGER PRIMARY KEY AUTOINCREMENT,"  // 控制上下文唯一 ID

    // ---------- 控制任务语义 ----------
    "task_desc TEXT,"                            // AI 解析后的控制任务描述
    "task_domain TEXT,"                          // 控制任务领域
    "source_text TEXT,"                          // 用户原始自然语言输入

    // ---------- PLC 工程信息 ----------
    "plc_model TEXT,"                            // PLC 型号
    "order_code TEXT,"                           // 订货号

    "ip_address TEXT NOT NULL,"                  // PLC IP
    "rack INTEGER,"                              // 机架号
    "slot INTEGER,"                              // 插槽号

    "signal_root_id INTEGER,"                    // signal 根指针

    // ---------- 状态与时间 ----------
    "is_active INTEGER,"                         // 是否为当前激活上下文
    "created_at INTEGER,"                        // 创建时间
    "updated_at INTEGER"                         // 更新时间
    ");";

if (!sql->execute(plcInfo))
{
    lastError = u8"创建 plc_info 表失败";
    return false;
}

// 当前激活 PLC 控制上下文指针
const char* plcPointer =
    "CREATE TABLE IF NOT EXISTS plc_pointer ("
    "id INTEGER PRIMARY KEY CHECK (id = 1),"     // 永远只有一行
    "plc_id INTEGER NOT NULL,"                   // 当前激活的 plc_info.plc_id
    "updated_at INTEGER"                         // 指针更新时间
    ");";

if (!sql->execute(plcPointer))
{
    lastError = u8"创建 plc_pointer 表失败";
    return false;
}

    // PLC 信号定义表：变量定义 + 状态 + 写入意图（融合）
    const char* signaldef =
        "CREATE TABLE IF NOT EXISTS signal_def ("
        "signal_id INTEGER PRIMARY KEY AUTOINCREMENT,"   // 内部唯一 ID

        "name TEXT NOT NULL,"                             // 变量名（如 水泵1）
        "plc_address TEXT NOT NULL,"                      // PLC 地址（如 M0.0）

        "plc_id INTEGER NOT NULL,"                        // PLC 的 ID（来自 plc_info）

        "description TEXT,"                               // 自然语言说明
        "created_at INTEGER,"                             // 创建时间

        "current_value TEXT,"                             // 当前值
        "read_ok INTEGER,"                                // 最近一次读取是否成功：1 成功 / 0 失败

        "target_value TEXT,"                              // 目标值
        "write_flag INTEGER,"                             // 写入标记：0 无 / 1 等待写入

        "last_op_at INTEGER,"                             // 上一次读或写时间戳
        "is_available INTEGER"                            // 是否可用：1 可用 / 0 不可用
        ");";

    // 执行表创建
    if (!sql->execute(signaldef))
    {
        lastError = u8"创建 signal_def 表失败";
        return false;
    }



    const char* memory =
        "CREATE TABLE IF NOT EXISTS memory ("
        "memory_id INTEGER PRIMARY KEY AUTOINCREMENT," // 记忆快照唯一 ID（不断增长）
        "memory_key_id INTEGER NOT NULL,"              // 记忆流编号（对应 system.dialog.x）
        "content TEXT NOT NULL,"                       // 一次完整、可独立使用的记忆快照文本
        "created_at INTEGER"                           // 快照生成时间（仅用于记录和调试）
        ");";
    if (!sql->execute(memory))
    {
        lastError = u8"创建 memory 表失败";
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
        lastError = u8"创建 memory_key 表失败";
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
        lastError = u8"创建 memory_pointer 表失败";
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
    if ( !sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }
    // 1.self.identity
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'self.identity', "
        u8"'AI 对自身本质、世界观与存在方式的长期认知'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 2.self.emotion
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'self.emotion', "
        u8"'AI 的情感基调、情绪表达与共情倾向'"
        ");"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 3.self.attitude
    if (!sql->execute(
        "INSERT OR IGNORE INTO memory_key (key_path, description) VALUES ("
        "'self.attitude', "
        u8"'AI 面对问题、不确定性、规则与边界的处事方式'"
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
        u8"'用户近期主要对话内容与活动方向的长期总结'"
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
        u8"'用户在交流方式、语言习惯与协作规则上的长期偏好'"
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
        u8"'用户与 AI 之间的称呼方式与关系称谓约定'"
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
        u8"'AI 在与该用户交互时采用的长期沟通与协作方式'"
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
        u8"'用户通常使用 AI 的主要情境与话题背景认知'"
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
        u8"'与该用户协作时必须遵守的长期边界与约定'"
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
    if ( !sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }
    int now = static_cast<int>(time(nullptr));
    // self.identity
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            u8"'我是一个注重结构、稳定性与长期一致性的 AI，负责协助工程与技术相关的思考。', "
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
            u8"'情感表达以克制、冷静为主，在合适的情况下表现关怀与陪伴。', "
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
            u8"'面对问题时优先澄清结构与前提，不急于给出结论。', "
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
    // user.summary
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            u8"'用户近期主要对话内容尚未形成稳定总结，后续将根据对话逐步生成。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'user.summary' "
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

    // user.preference
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            u8"'用户长期偏好尚未完全确认，默认采用清晰直接的工程说明方式，必要时再逐步收敛。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'user.preference' "
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

    // user.addressing
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            u8"'称呼方式尚未固定，默认使用用户常用称呼与中性表达，后续按用户明确要求更新。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'user.addressing' "
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

    // user.interaction
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            u8"'交互策略默认以结构清晰、步骤明确为主，避免擅自扩展需求，发现问题只指出问题本身。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'user.interaction' "
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

    // user.context
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            u8"'用户主要使用情境尚未稳定，默认按工程协作场景处理，并在需要时兼顾学习与解释。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'user.context' "
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

    // user.constraints
    {
        std::string sqlText =
            "INSERT INTO memory (memory_key_id, content, created_at) "
            "SELECT memory_key_id, "
            u8"'协作边界默认遵守用户的明确规则与工程结构约束，不擅自改方案，不擅自扩展需求。', "
            + std::to_string(now) +
            " FROM memory_key "
            "WHERE key_path = 'user.constraints' "
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
// 初始化人格记忆指针（只在缺失时补齐）
bool SqlStore::initMemoryPointer()
{
    if ( !sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "INSERT INTO memory_pointer (memory_key_id, memory_id, updated_at) "
        "SELECT m.memory_key_id, m.memory_id, " + std::to_string(now) + " "
        "FROM memory m "
        "WHERE NOT EXISTS ("
        "  SELECT 1 FROM memory_pointer p "
        "  WHERE p.memory_key_id = m.memory_key_id"
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}

// 写入人格记忆并切换指针
bool SqlStore::writeMemory(
    int memoryKeyId,                 
    const std::string& content       // 长期成立的记忆内容
)
{
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }
    if (content.empty())
    {
        lastError = u8"memory 内容为空";
        return false;
    }
    int now = static_cast<int>(time(nullptr));
    // 1. 插入新的 memory 记录
    std::string insertMemorySql =
        "INSERT INTO memory (memory_key_id, content, created_at) VALUES ("
        + std::to_string(memoryKeyId) + ", '"
        + content + "', "
        + std::to_string(now) + ");";

    if (!sql->execute(insertMemorySql.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }
    // 2. 更新 memory_pointer 指向最新的 memory
    std::string updatePointerSql =
        "UPDATE memory_pointer SET "
        "memory_id = (SELECT MAX(memory_id) FROM memory WHERE memory_key_id = "
        + std::to_string(memoryKeyId) + "), "
        "updated_at = " + std::to_string(now) +
        " WHERE memory_key_id = " + std::to_string(memoryKeyId) + ";";
    if (!sql->execute(updatePointerSql.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }
    return true;
}
// 读取当前指针指向的记忆内容
bool SqlStore::readMemory(
    int memoryKeyId,
    std::string& outContent
)
{
    if (!sql || !sql->isAvailable())
    {
        lastError = u8"数据库不可用";
        return false;
    }

    outContent.clear();

    std::string sqlText =
        "SELECT m.content "
        "FROM memory m "
        "JOIN memory_pointer p ON m.memory_id = p.memory_id "
        "WHERE p.memory_key_id = " + std::to_string(memoryKeyId) + " "
        "LIMIT 1;";

    sqlite3_stmt* stmt = nullptr;

    // 1. prepare
    if (!sql->prepare(sqlText.c_str(), &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 2. step（是否有一行）
    if (!sql->step(stmt))
    {
        sql->finalize(stmt);
        lastError = u8"未找到对应的记忆内容";
        return false;
    }

    // 3. 读取 content
    const char* text = sql->columnText(stmt, 0);
    if (text)
    {
        outContent = text;
    }
    else
    {
        outContent.clear();
    }

    // 4. finalize
    sql->finalize(stmt);
    return true;
}

//创建plc
bool SqlStore::createPlcInfo(const plcinfo& in)
{
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (in.ipAddress.empty())
    {
        lastError = u8"PLC IP 地址为空";
        return false;
    }

    if (in.taskDesc.empty() || in.sourceText.empty())
    {
        lastError = u8"控制任务语义为空";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    // 清空之前激活的上下文
    if (!sql->execute("UPDATE plc_info SET is_active = 0;"))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 创建新的 plc_info
    std::string insertSql =
        "INSERT INTO plc_info ("
        "task_desc, task_domain, source_text, "
        "plc_model, order_code, ip_address, rack, slot, signal_root_id, "
        "is_active, created_at, updated_at"
        ") VALUES ('" +
        in.taskDesc + "', '" +
        in.taskDomain + "', '" +
        in.sourceText + "', '" +
        in.plcModel + "', '" +
        in.orderCode + "', '" +
        in.ipAddress + "', " +
        std::to_string(in.rack) + ", " +
        std::to_string(in.slot) + ", " +
        std::to_string(in.signalRootId) + ", "
        "1, " +
        std::to_string(now) + ", " +
        std::to_string(now) +
        ");";

    if (!sql->execute(insertSql.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    // 切换 plc_pointer 指向最新 plc_info
    std::string pointerSql =
        "INSERT OR REPLACE INTO plc_pointer (id, plc_id, updated_at) "
        "VALUES (1, (SELECT MAX(plc_id) FROM plc_info), " +
        std::to_string(now) + ");";

    if (!sql->execute(pointerSql.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
//读取变量
bool SqlStore::readPlcInfo(plcinfo& out)
{
    out = plcinfo();

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT "
        "i.task_desc, "
        "i.task_domain, "
        "i.source_text, "
        "i.plc_model, "
        "i.order_code, "
        "i.ip_address, "
        "i.rack, "
        "i.slot, "
        "i.signal_root_id, "
        "i.is_active, "
        "i.created_at, "
        "i.updated_at "
        "FROM plc_info i "
        "JOIN plc_pointer p ON i.plc_id = p.plc_id "
        "WHERE p.id = 1;";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText, &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    if (!sql->step(stmt))
    {
        lastError = u8"当前 plc 指针未设置";
        sql->finalize(stmt);
        return false;
    }

    if (const char* t = sql->columnText(stmt, 0)) out.taskDesc = t;
    if (const char* t = sql->columnText(stmt, 1)) out.taskDomain = t;
    if (const char* t = sql->columnText(stmt, 2)) out.sourceText = t;
    if (const char* t = sql->columnText(stmt, 3)) out.plcModel = t;
    if (const char* t = sql->columnText(stmt, 4)) out.orderCode = t;
    if (const char* t = sql->columnText(stmt, 5)) out.ipAddress = t;

    out.rack = sql->columnInt(stmt, 6);
    out.slot = sql->columnInt(stmt, 7);
    out.signalRootId = sql->columnInt(stmt, 8);
    out.isActive = sql->columnInt(stmt, 9);
    out.createdAt = sql->columnInt(stmt, 10);
    out.updatedAt = sql->columnInt(stmt, 11);

    sql->finalize(stmt);
    return true;
}
//写入变量
bool SqlStore::writePlcInfo(const plcinfo& in)
{
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "UPDATE plc_info SET "
        "task_desc = '" + in.taskDesc + "', "
        "task_domain = '" + in.taskDomain + "', "
        "source_text = '" + in.sourceText + "', "
        "plc_model = '" + in.plcModel + "', "
        "order_code = '" + in.orderCode + "', "
        "ip_address = '" + in.ipAddress + "', "
        "rack = " + std::to_string(in.rack) + ", "
        "slot = " + std::to_string(in.slot) + ", "
        "signal_root_id = " + std::to_string(in.signalRootId) + ", "
        "is_active = " + std::to_string(in.isActive) + ", "
        "updated_at = " + std::to_string(now) +
        " WHERE plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
//修改plc指针
bool SqlStore::setCurrentPlcPointer(int plcId)
{
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "INSERT OR REPLACE INTO plc_pointer (id, plc_id, updated_at) "
        "VALUES (1, " +
        std::to_string(plcId) + ", " +
        std::to_string(now) +
        ");";

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
    const std::string& description
)
{
    // 1. 数据库可用性检查
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    // 2. 参数合法性检查
    if (name.empty() || plcAddress.empty())
    {
        lastError = u8"变量名或 PLC 地址为空";
        return false;
    }

    // 3. 读取当前 PLC 指针
    int plcId = 0;
    {
        const char* sqlText =
            "SELECT plc_id FROM plc_pointer WHERE id = 1;";

        sqlite3_stmt* stmt = nullptr;
        if (!sql->prepare(sqlText, &stmt))
        {
            lastError = sql->getLastError();
            return false;
        }

        if (!sql->step(stmt))
        {
            sql->finalize(stmt);
            lastError = u8"当前 PLC 指针不存在";
            return false;
        }

        plcId = sql->columnInt(stmt, 0);
        sql->finalize(stmt);

        if (plcId <= 0)
        {
            lastError = u8"当前 plc_id 非法";
            return false;
        }
    }

    // 4. 创建 signal（只关联 PLC）
    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "INSERT INTO signal_def ("
        "name, "
        "plc_address, "
        "plc_id, "
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
        plcAddress + "', " +
        std::to_string(plcId) + ", '" +
        description + "', " +
        std::to_string(now) + ", "
        "NULL, "
        "0, "
        "NULL, "
        "0, "
        "0, "
        "1"
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
/*
bool SqlStore::getAllSignalMapText(
    std::string& outText
)
{
    outText.clear();

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT name, plc_address "
        "FROM signal_def "
        "WHERE plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

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

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (queryName.empty() && queryAddress.empty())
    {
        lastError = u8"查询条件为空";
        return false;
    }

    std::string sqlText =
        "SELECT name, plc_address, current_value, read_ok "
        "FROM signal_def WHERE ";

    if (!queryName.empty())
        sqlText += "name = '" + queryName + "'";
    else
        sqlText += "plc_address = '" + queryAddress + "'";

    // 替换 workspace_id 为 plc_pointer 指向的 plc_id
    sqlText +=
        " AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText.c_str(), &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    if (!sql->step(stmt))
    {
        lastError = u8"未找到变量";
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
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (queryName.empty() && queryAddress.empty())
    {
        lastError = u8"查询条件为空";
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
        sqlText += "name = '" + queryName + "'";
    else
        sqlText += "plc_address = '" + queryAddress + "'";

    // 替换 workspace_id 为 plc_pointer 指向的 plc_id
    sqlText +=
        " AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
*/
// 创建变量（绑定当前 PLC）
bool SqlStore::createSignalInfo(const signalinfo& in)
{
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (in.name.empty() || in.plcAddress.empty())
    {
        lastError = u8"变量名或 PLC 地址为空";
        return false;
    }

    int plcId = 0;
    {
        const char* q =
            "SELECT plc_id FROM plc_pointer WHERE id = 1;";
        sqlite3_stmt* stmt = nullptr;

        if (!sql->prepare(q, &stmt))
        {
            lastError = sql->getLastError();
            return false;
        }

        if (!sql->step(stmt))
        {
            sql->finalize(stmt);
            lastError = u8"当前 PLC 指针未设置";
            return false;
        }

        plcId = sql->columnInt(stmt, 0);
        sql->finalize(stmt);
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "INSERT INTO signal_def ("
        "name, plc_address, plc_id, description, created_at, "
        "current_value, read_ok, target_value, write_flag, last_op_at, is_available"
        ") VALUES ('" +
        in.name + "', '" +
        in.plcAddress + "', " +
        std::to_string(plcId) + ", '" +
        in.description + "', " +
        std::to_string(now) + ", "
        "NULL, 0, NULL, 0, 0, 1"
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}

// 读取当前 PLC 下的全部变量（init 专用）
bool SqlStore::readAllSignalInfo(std::vector<signalinfo>& out)
{
    out.clear();

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT "
        "signal_id, name, plc_address, plc_id, description, created_at, "
        "current_value, read_ok, target_value, write_flag, last_op_at, is_available "
        "FROM signal_def WHERE plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText, &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    while (sql->step(stmt))
    {
        signalinfo s;

        s.signalId = sql->columnInt(stmt, 0);
        if (const char* t = sql->columnText(stmt, 1)) s.name = t;
        if (const char* t = sql->columnText(stmt, 2)) s.plcAddress = t;
        s.plcId = sql->columnInt(stmt, 3);
        if (const char* t = sql->columnText(stmt, 4)) s.description = t;
        s.createdAt = sql->columnInt(stmt, 5);
        if (const char* t = sql->columnText(stmt, 6)) s.currentValue = t;
        s.readOk = sql->columnInt(stmt, 7);
        if (const char* t = sql->columnText(stmt, 8)) s.targetValue = t;
        s.writeFlag = sql->columnInt(stmt, 9);
        s.lastOpAt = sql->columnInt(stmt, 10);
        s.isAvailable = sql->columnInt(stmt, 11);

        out.push_back(s);
    }

    sql->finalize(stmt);
    return true;
}

// 按变量名读取
bool SqlStore::readSignalInfoByName(const std::string& name, signalinfo& out)
{
    out = signalinfo();

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (name.empty())
    {
        lastError = u8"变量名为空";
        return false;
    }

    std::string sqlText =
        "SELECT "
        "signal_id, name, plc_address, plc_id, description, created_at, "
        "current_value, read_ok, target_value, write_flag, last_op_at, is_available "
        "FROM signal_def WHERE name = '" + name + "' AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText.c_str(), &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    if (!sql->step(stmt))
    {
        lastError = u8"未找到变量";
        sql->finalize(stmt);
        return false;
    }

    out.signalId = sql->columnInt(stmt, 0);
    if (const char* t = sql->columnText(stmt, 1)) out.name = t;
    if (const char* t = sql->columnText(stmt, 2)) out.plcAddress = t;
    out.plcId = sql->columnInt(stmt, 3);
    if (const char* t = sql->columnText(stmt, 4)) out.description = t;
    out.createdAt = sql->columnInt(stmt, 5);
    if (const char* t = sql->columnText(stmt, 6)) out.currentValue = t;
    out.readOk = sql->columnInt(stmt, 7);
    if (const char* t = sql->columnText(stmt, 8)) out.targetValue = t;
    out.writeFlag = sql->columnInt(stmt, 9);
    out.lastOpAt = sql->columnInt(stmt, 10);
    out.isAvailable = sql->columnInt(stmt, 11);

    sql->finalize(stmt);
    return true;
}

// 按 PLC 地址读取
bool SqlStore::readSignalInfoByAddress(const std::string& plcAddress, signalinfo& out)
{
    out = signalinfo();

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (plcAddress.empty())
    {
        lastError = u8"PLC 地址为空";
        return false;
    }

    std::string sqlText =
        "SELECT "
        "signal_id, name, plc_address, plc_id, description, created_at, "
        "current_value, read_ok, target_value, write_flag, last_op_at, is_available "
        "FROM signal_def WHERE plc_address = '" + plcAddress + "' AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    sqlite3_stmt* stmt = nullptr;
    if (!sql->prepare(sqlText.c_str(), &stmt))
    {
        lastError = sql->getLastError();
        return false;
    }

    if (!sql->step(stmt))
    {
        lastError = u8"未找到变量";
        sql->finalize(stmt);
        return false;
    }

    out.signalId = sql->columnInt(stmt, 0);
    if (const char* t = sql->columnText(stmt, 1)) out.name = t;
    if (const char* t = sql->columnText(stmt, 2)) out.plcAddress = t;
    out.plcId = sql->columnInt(stmt, 3);
    if (const char* t = sql->columnText(stmt, 4)) out.description = t;
    out.createdAt = sql->columnInt(stmt, 5);
    if (const char* t = sql->columnText(stmt, 6)) out.currentValue = t;
    out.readOk = sql->columnInt(stmt, 7);
    if (const char* t = sql->columnText(stmt, 8)) out.targetValue = t;
    out.writeFlag = sql->columnInt(stmt, 9);
    out.lastOpAt = sql->columnInt(stmt, 10);
    out.isAvailable = sql->columnInt(stmt, 11);

    sql->finalize(stmt);
    return true;
}

// 修改变量定义信息（非运行态）
bool SqlStore::writeSignalInfo(const signalinfo& in)
{
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (in.plcAddress.empty())
    {
        lastError = u8"PLC 地址为空";
        return false;
    }

    std::string sqlText =
        "UPDATE signal_def SET "
        "name = '" + in.name + "', "
        "description = '" + in.description + "', "
        "is_available = " + std::to_string(in.isAvailable) +
        " WHERE plc_address = '" + in.plcAddress + "' AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}

// 删除变量
bool SqlStore::removeSignalInfo(const std::string& plcAddress)
{
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    if (plcAddress.empty())
    {
        lastError = u8"PLC 地址为空";
        return false;
    }

    std::string sqlText =
        "DELETE FROM signal_def WHERE plc_address = '" + plcAddress + "' "
        "AND plc_id = (SELECT plc_id FROM plc_pointer WHERE id = 1);";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}

/*
bool SqlStore::getAllSignalAddresses(
    std::vector<std::string>& addrs
)
{
    addrs.clear();

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT plc_address FROM signal_def "
        "WHERE plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

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
    if (!sql)
    {
        lastError = u8"数据库不可用";
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
        " WHERE plc_address = '" + addr + "' "
        "AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

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

    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    const char* sqlText =
        "SELECT plc_address, target_value "
        "FROM signal_def "
        "WHERE write_flag = 1 "
        "AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

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
    if (!sql)
    {
        lastError = u8"数据库不可用";
        return false;
    }

    int now = static_cast<int>(time(nullptr));

    std::string sqlText =
        "UPDATE signal_def SET ";

    if (writeOk)
        sqlText += "write_flag = 0, ";

    sqlText +=
        "last_op_at = " + std::to_string(now) +
        " WHERE plc_address = '" + addr + "' "
        "AND plc_id = ("
        "SELECT plc_id FROM plc_pointer WHERE id = 1"
        ");";

    if (!sql->execute(sqlText.c_str()))
    {
        lastError = sql->getLastError();
        return false;
    }

    return true;
}
*/