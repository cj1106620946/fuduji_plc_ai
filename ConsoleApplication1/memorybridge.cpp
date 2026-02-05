#include "memorybridge.h"

memorybridge::memorybridge(SqlStore& storeRef)
    : store(storeRef)
{
}

memorybridge::~memorybridge()
{
}

// 初始化：只做“数据库 → 内存镜像”
bool memorybridge::init()
{
    std::string content;

    if (!store.readMemory(1, content)) return false;
    current.selfMemory[0] = { 1, "self.identity", content };

    if (!store.readMemory(2, content)) return false;
    current.selfMemory[1] = { 2, "self.emotion", content };

    if (!store.readMemory(3, content)) return false;
    current.selfMemory[2] = { 3, "self.attitude", content };

    if (!store.readMemory(4, content)) return false;
    current.userMemory[0] = { 4, "user.summary", content };

    if (!store.readMemory(5, content)) return false;
    current.userMemory[1] = { 5, "user.preference", content };

    if (!store.readMemory(6, content)) return false;
    current.userMemory[2] = { 6, "user.addressing", content };

    if (!store.readMemory(7, content)) return false;
    current.userMemory[3] = { 7, "user.interaction", content };

    if (!store.readMemory(8, content)) return false;
    current.userMemory[4] = { 8, "user.context", content };

    if (!store.readMemory(9, content)) return false;
    current.userMemory[5] = { 9, "user.constraints", content };

    return true;
}

// 读取：只读当前镜像
const CurrentMemoryState& memorybridge::read() const
{
    return current;
}

// 写入：只写数据库，不动 current，防污染
bool memorybridge::write(const MemoryWrite& req)
{
    if (req.content.empty())
        return false;
    return store.writeMemory(req.memoryKeyId, req.content);
}
