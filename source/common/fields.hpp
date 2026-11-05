#pragma once
#include <unordered_map>
#include <string>

// 请求的字段宏
#define KEY_METHOD "method"
#define KEY_PRC_PARAMS "rpc_parametetrs"
#define KEY_TOPIC_KEY "topic_name"
#define KEY_OPTYPE "optype"
#define KEY_TOPIC_MSY "topic_message"
#define KEY_HOST "host"
#define KEY_HOST_IP "ip"
#define KEY_HOST_PORT "port"
#define KEY_RCODE "rcode"
#define KEY_RESULT "result"

namespace Json_rpc
{

    // 消息类型的定义
    enum class MType
    {
        REQ_RPC = 0,
        RSP_RPC,
        REQ_TOPIC,
        RSP_TOPIC,
        REQ_SERVICE,
        RSP_SERVICE
    };

    // 响应码类型的定义
    enum class RCode
    {
        RCODE_SUCCESS = 0,
        RCODE_PARSE_FAILED,
        RCODE_FAILED_MSGTYPE,
        RCODE_INVALID_MSY,
        RCOED_DISCONNETED,
        RCODE_INVALID_PARAMS,
        RCODE_NOT_FOUND_RPCSERVICE,
        RCODE_INVALID_OPTYPE,
        RCODE_NOT_FOUND_TOPIC,
        RCODE_INVALID_TOPIC,
        RCODE_INTERNAL_ERROR,
        RCODE_UNKNOWN_ERROR
    };

    static std::string errReason(RCode rcode)
    {

        static std::unordered_map<RCode, std::string> err_map = {
            {RCode::RCODE_SUCCESS, "成功处理！"},
            {RCode::RCODE_PARSE_FAILED, "消息解析失败！"},
            {RCode::RCODE_FAILED_MSGTYPE, "消息类型错误！"},
            {RCode::RCODE_INVALID_MSY, "⽆效消息"},
            {RCode::RCOED_DISCONNETED, "连接已断开！"},
            {RCode::RCODE_INVALID_PARAMS, "⽆效的Rpc参数！"},
            {RCode::RCODE_NOT_FOUND_RPCSERVICE, "没有找到对应的服务！"},
            {RCode::RCODE_INVALID_OPTYPE, "⽆效的操作类型"},
            {RCode::RCODE_NOT_FOUND_TOPIC, "没有找到对应的主题！"},
            {RCode::RCODE_INVALID_TOPIC, "无效主题！"},
            {RCode::RCODE_INTERNAL_ERROR, "内部错误！"},
            {RCode::RCODE_UNKNOWN_ERROR, "未知错误！"}};
        auto it = err_map.find(rcode);
        return it->second;
    }

    // RPC请求类型的定义
    enum class RPC_Rtype
    {
        REQ_ASYNC = 0,
        REQ_CALLBACK
    };

    // 主题操作类型的定义
    enum class TOPIC_Optype
    {
        TOPIC_CREATE = 0,
        TOPIC_REMOVE,
        TOPIC_SUBSCRIBE,
        TOPIC_CANCEL,
        TOPIC_PUBLISH
    };

    // 服务操作类型的定义
    enum class SERVICE_Optype
    {
        SERVICE_REGISTRY = 0,
        SERVICE_DISCOVERY,
        SERVICE_ONLINE,
        SERVICE_OFFLINE,
        SERVICE_UNKNOW
    };

}