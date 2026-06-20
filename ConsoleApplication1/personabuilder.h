#pragma once

#include <unordered_map>
#include <string>
#include <vector>
#include <ctime>

#include "sqlstore.h"

// 前向声明
class SqlStore;

// 人格运行状态
struct PersonaState
{
    int life;        // 人格生命周期状态
    // 0：未初始化
    // 1：初始化中
    // 2：运行中（镜像有效）
    // 3：提交中（写入数据库）
    // 4：致命错误（不可恢复）

    int fatalReason; // 致命错误原因（仅 life == 4 时有效）
    // 0：无
    // 1：数据库不可用
    // 2：人格结构缺失
    // 3：内部逻辑错误
};

// 人格结构位快照（内存镜像）
struct PersonaSlot
{
    int memoryKeyId;            // 对应 memory_key_id
    std::string keyPath;        // 结构位路径（如 self.identity）
    std::string content;        // 当前生效的人格内容
    std::time_t updatedAt;      // 最近一次更新时间（镜像时间）
};

class PersonaBuilder
{
public:
    explicit PersonaBuilder(SqlStore& storeRef);
    ~PersonaBuilder();

    // 初始化人格（从数据库加载，构建镜像）
    bool init();

    // 重置人格状态（清空镜像，不写库）
    void reset();

    // 是否已就绪
    bool isReady() const;

    // 获取当前人格镜像文本（只读）
    bool getPersonaText(std::string& outText) const;

    // 更新镜像中的某个结构位（不写数据库）
    bool updatePersonaSlot(
        const std::string& keyPath,
        const std::string& newContent
    );

    // 在短期记忆边界处提交人格（镜像 -> 数据库）
    bool commit();

    // 最近一次错误
    const std::string& getLastError() const;

private:
    // 从数据库加载当前生效人格
    bool loadFromDatabase();

    // 构建镜像后的完整校验
    bool validatePersona();

    // 将镜像写入 memory / memory_pointer
    bool writeToDatabase();
    // 组装人格文本
    void buildPersonaText(std::string& outText) const;

private:
    SqlStore& store;

    PersonaState state;
    std::string lastError;

    // key_path -> 人格结构位镜像
    std::unordered_map<std::string, PersonaSlot> personaSlots;

    // 最近一次提交时间
    std::time_t lastCommitTime;
};
