#include "decisionai.h"
DecisionAI::DecisionAI(int AICODE,AIController& aiRef,AITrace& traceRef)
    :ai(aiRef), trace(traceRef), aicode(AICODE)
{
}
// 修正：实现函数时需加上返回类型 std::string，且不能有分号
std::string DecisionAI::runOnce(const std::string& user_input)
{
    return "no";
}
// 外部数据接口（占位，不实现）
void DecisionAI::feedExternalData(const std::string& data)
{
    // 预留：
    // - 爬虫结果
    // - HTTP / MQTT
    // - 传感器聚合数据
    // - 第三方系统输入
    // 后续可以通过协议发送给 ChatAI，例如：
    // chat.query_once("DECISION_EXTERNAL_DATA\n" + data);
}
