#include "decisionai.h"
#include "chatagent.h"

// ================================
// 构造
// ================================
DecisionAI::DecisionAI(ChatAgent& agent)
    : chat(agent)
{
}

// ================================
// 执行一次决策
// ================================
std::string DecisionAI::runOnce()
{
    // 决策 AI 只发送协议，不关心任何内部配置
    // ChatAI 会根据上下文自行判断是否启用、是否合法
    return chat.query_once("DECISION_EXECUTE");
}

// ================================
// 外部数据接口（占位，不实现）
// ================================
void DecisionAI::feedExternalData(const std::string& data)
{
    // 预留：
    // - 爬虫结果
    // - HTTP / MQTT
    // - 传感器聚合数据
    // - 第三方系统输入
    //
    // 后续可以通过协议发送给 ChatAI，例如：
    // chat.query_once("DECISION_EXTERNAL_DATA\n" + data);
}
