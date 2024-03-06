#ifndef MESSAGEPIPELINE_H
#define MESSAGEPIPELINE_H

#include "Packetizer.h"
#include "Transmitter.h"
#include <memory>
#include <functional>

#ifdef OS_WIN
#ifdef BUILD_FSP2P_STATIC
#define FSP2P_EXPORT
#else
#if defined(BUILD_FSP2P_LIB)
#  undef FSP2P_EXPORT
#  define FSP2P_EXPORT __declspec(dllexport)
#else
#  undef FSP2P_EXPORT
#  define FSP2P_EXPORT __declspec(dllimport)
#endif
#endif //BUILD_FSP2P_STATIC
#else
#define FSP2P_EXPORT
#endif

namespace fs {
namespace p2p {

using RequestCallback = std::function<void(const Request &)>;
using ResponseCallback = std::function<void(const Response &, void *)>;
using DeviceCallback = std::function<void(const InfomationManifest &)>;
using IncomingCallback = std::function<void(const Request &, const Payload::Device &)>;

FSP2P_EXPORT std::string version();

class MessagePipelinePrivate;
class FSP2P_EXPORT MessagePipeline
{
public:
    MessagePipeline(const InfomationManifest &v);
    virtual ~MessagePipeline();

    void open(const std::string &host, unsigned short port,
              const std::string &username = std::string(),
              const std::string &password = std::string());
    void close();

    InfomationManifest &infomationManifest() const;
    std::shared_ptr<IPacketizer> packerizer() const;
    std::shared_ptr<ITransmitter> transmitter() const;

    /**
     * 调用方法
     * @param list 请求的设备列表
     * @param cb 响应的回调函数
     * @param topic 目标设备的订阅主题
     * @return 非0失败
     */
    int postMethod(const std::map<std::string, Payload::Device> &list,
                   ResponseCallback cb, void *opaque,
                   const std::string &topic);
    int postRead(const std::map<std::string, Payload::Device> &list,
                 ResponseCallback cb, void *opaque,
                 const std::string &topic);
    int postWrite(const std::map<std::string, Payload::Device> &list,
                  ResponseCallback cb, void *opaque,
                  const std::string &topic);

    int postNotify(const std::map<std::string, Payload::Device> &list);
    int postEvent(const std::map<std::string, Payload::Device> &list);
    int postBroadcast(const std::map<std::string, Payload::Device> &list);
    int postRequest(Request::Action action,
                    const std::map<std::string, Payload::Device> &list,
                    ResponseCallback cb, void *opaque,
                    const std::string &topic = std::string());

    /**
     * 订阅设备消息，用于接收notify/event等
     * @param target 目标设备的信息清单
     * @param cb 目标设备的请求回调
     * @return 非0失败
     */
    int subscribe(const InfomationManifest &target, RequestCallback cb);
    int unsubscribe(const InfomationManifest &target);

    /**
     * 设备上线消息
     */
    int postStartup();
    /**
     * 设备离线消息
     */
    int postShutdown();
    /**
     * 设备心跳消息，需定时触发
     */
    int postHeartbeat();

    /**
     * @brief 回应请求
     * @param source 接收的请求包
     * @param list 设备列表
     * @return 非0失败
     */
    int response(const Request &source, const std::map<std::string, Payload::Device> &list);

    void setLogCallback(LogCallback cb);
    void setErrorCallback(ErrorCallback cb);

    /**
     * 收到请求时的回调
     */
    void setRequestCallback(RequestCallback cb);
    int  appendRequestCallback(RequestCallback cb);
    void removeRequestCallback(int cb_id);
    /**
     * 收到广播时的回调
     */
    void setBroadcastCallback(RequestCallback cb);
    /**
     * 连接状态发生变化
     */
    void setConnectStateCallback(ConnectStateCallback cb);

    void setDeviceStartupCallback(DeviceCallback cb);
    void setDeviceShutdownCallback(DeviceCallback cb);
    void setDeviceHeartbeatCallback(DeviceCallback cb);

    /**
     * 方法请求传入
     */
    void setIncomingMethodCallback(IncomingCallback cb);
    /**
     * 读请求传入
     */
    void setIncomingReadCallback(IncomingCallback cb);
    /**
     * 写请求传入
     */
    void setIncomingWriteCallback(IncomingCallback cb);
    /**
     * 通知传入
     */
    void setIncomingNotifyCallback(IncomingCallback cb);
    /**
     * 事件传入
     */
    void setIncomingEventCallback(IncomingCallback cb);

private:
    std::shared_ptr<MessagePipelinePrivate> d;
};

}
}

#endif // MESSAGEPIPELINE_H
