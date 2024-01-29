#ifndef PACKETIZER_H
#define PACKETIZER_H

#include <stdint.h>
#include <string>
#include <list>
#include <map>

#include <nlohmann/json.hpp>
using namespace nlohmann;

namespace fs {
namespace p2p {

/**
 * 网关or服务的信息清单，用于广播告知
 */
struct InfomationManifest
{
    enum Type
    {
        Gateway,
        Service,
        Unknown
    };

    std::string sn;
    std::string product_id;
    std::string name;
    std::string model;
    int type = Unknown;
    int version = -1;

    /**
     * 发布的主题，用于事件发布、通知上报
     */
    std::string getPublishTopic() const { return "publish/"+sn; }
    /**
     * 订阅的主题，用于接收方法调用、属性读写的请求
     */
    std::string getSubscribeTopic() const { return "subscribe/"+sn; }
};

/**
 * 设备属性组
 */
struct Service
{
    std::string name;
    std::map<std::string, ordered_json> propertys;

    // response
    int reason_code = 0;
    std::string reason_string;
};

/**
 * 设备方法
 */
struct Method
{
    std::string name;
    std::map<std::string, ordered_json> params;

    // response
    int reason_code = 0;
    std::string reason_string;
};

/**
 * 设备事件
 */
struct Event
{
    std::string name;
    std::map<std::string, ordered_json> params;
};

struct Payload {
    struct Device {
        /**
         * 设备序列号
         * 填充 方法/读写属性 的请求时，SN为目标设备
         * 填充 方法/读写属性 的响应时，SN为当前设备
         * 填充 事件/广播 的请求时，SN为当前设备
         */
        std::string sn;
        /**
         * 产品ID
         */
        std::string product_id;
        /**
         * 服务列表
         */
        std::list<Service> services;
        /**
         * 方法列表
         */
        std::list<Method> methods;
        /**
         * 事件列表
         */
        std::list<Event> events;
    };

    /**
     * 响应/请求的设备列表
     */
    std::map<std::string, Device> devices;
};

struct Request
{
    enum Action {
        Action_Unknown = 0x00,
        Action_Method,
        Action_Read,
        Action_Write,
        Action_Notify,
        Action_Event,
        Action_Broadcast,
    };

    /**
     * 每次请求都会生成一个唯一的事务iid，用于分辨响应
     */
    std::string iid;
    /**
     * 本次请求的类型
     */
    int action = Action_Unknown;
    /**
     * 用于应答的响应主题
     */
    std::string ack;
    /**
     * UTC时间戳
     */
    std::string time;

    Payload payload;
};

struct Response
{
    enum Action {
        Action_Unknown = 0x10,
        Action_Method,
        Action_Read,
        Action_Write,
    };

    /**
     * 响应iid对应的请求
     */
    std::string iid;
    /**
     * 本次响应的类型
     */
    int action = Action_Unknown;
    /**
     * UTC时间戳
     */
    std::string time;

    Payload payload;
};

/**
 * 用于打包/解包数据的接口类
 */
class IPacketizer
{
public:
    virtual ~IPacketizer() {}

    virtual int packRequest(const Request &in, std::string &out) = 0;
    virtual int unpackRequest(const std::string &in, Request &out) = 0;

    virtual int packResponse(const Response &in, std::string &out) = 0;
    virtual int unpackResponse(const std::string &in, Response &out) = 0;

    virtual int packStartup(const InfomationManifest &in, Payload::Device &out) = 0;
    virtual int unpackStartup(const Payload::Device &in, InfomationManifest &out) = 0;

    virtual int packShutdown(const InfomationManifest &in, Payload::Device &out) = 0;
    virtual int unpackShutdown(const Payload::Device &in, InfomationManifest &out) = 0;

    virtual int packHeartbeat(const InfomationManifest &in, Payload::Device &out) = 0;
    virtual int unpackHeartbeat(const Payload::Device &in, InfomationManifest &out) = 0;
};

}
}

#endif // PACKETIZER_H
