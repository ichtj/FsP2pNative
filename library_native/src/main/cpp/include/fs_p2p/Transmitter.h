#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <string>
#include <functional>

namespace fs {
namespace p2p {

using MessageCallback = std::function<void(const std::string &topic, const std::string &payload)>;
using LogCallback = std::function<void(int level, const std::string &str)>;
using ErrorCallback = std::function<void(int error_code, const std::string &error_string)>;
using ConnectStateCallback = std::function<void(bool)>;

class ITransmitter
{
public:
    virtual ~ITransmitter() {}

    /**
     * 连接至服务
     * @param host 服务所在地址
     * @param port 服务所在端口
     * @param username 连接至服务的账号
     * @param password 连接至服务的密码
     * @return =0为成功，其他则失败
     */
    virtual int connectToHost(const std::string &host, unsigned short port,
                              const std::string &username = std::string(),
                              const std::string &password = std::string()) = 0;

    /**
     * 从服务断开
     * @return =0为成功，其他则失败
     */
    virtual int disconnectFromHost() = 0;

    /**
     * 阻塞等待其连接结果
     * @param msecs 超时时长
     * @return true为已连接，反之则尚未连接
     */
    virtual bool waitForConnected(int msecs = 30000) = 0;

    /**
     * 阻塞等待其断开
     * @param msecs 超时时长
     * @return true为已断开，反之则尚未断开
     */
    virtual bool waitForDisconnected(int msecs = 30000) = 0;

    /**
     * 开始后台循环
     * @return =0为成功，其他则失败
     */
    virtual int start() = 0;
    /**
     * 结束后台循环
     * @return =0为成功，其他则失败
     */
    virtual int stop() = 0;

    /**
     * 发布数据
     * @param topic 发布的主题
     * @param payload 要发布的数据
     * @return =0为成功，其他则失败
     */
    virtual int publish(const std::string &topic, const std::string &payload,
                        int qos = 0, bool retain = false) = 0;

    /**
     * 订阅主题
     * @param topic 订阅的主题
     * @param cb 有数据时的回调
     * @return =0为成功，其他则失败
     */
    virtual int subscribe(const std::string &topic, MessageCallback cb) = 0;

    /**
     * 取消订阅
     * @param topic 取消订阅的主题
     * @return
     */
    virtual int unsubscribe(const std::string &topic) = 0;

    /**
     * 设置日志回调
     * @param cb 回调函数
     * @return =0为成功，其他则失败
     */
    virtual int setLogCallback(LogCallback cb) = 0;

    /**
     * 设置错误信息回调
     * @param cb 回调函数
     * @return =0为成功，其他则失败
     */
    virtual int setErrorCallback(ErrorCallback cb) = 0;

    /**
     * 设置连接状态变化的回调
     * @param cb 回调函数
     * @return =0为成功，其他则失败
     */
    virtual int setConnectStateCallback(ConnectStateCallback cb) = 0;
};

}
}

#endif // TRANSMITTER_H
