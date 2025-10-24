#ifndef REQUEST_MANAGER_H
#define REQUEST_MANAGER_H

#include <list>
#include <mutex>
#include <string>
#include "fs_p2p/Packetizer.h"  // 你自己的Request结构

class RequestManager {
public:
    static RequestManager& getInstance();  // 单例

    // 添加请求
    void addRequest(const fs::p2p::Request& req);
    std::list<fs::p2p::Request> getAllRequests();
    // 遍历请求（只打印）
    void dumpRequests();

    // 按条件清除：例如匹配iid
    void clearByIid(const std::string& iid);

    // 按action清除
    void clearByAction(fs::p2p::Request::Action action);

    // 全部清空
    void clearAll();

    // 获取当前缓存数量
    size_t size();

private:
    RequestManager() = default;
    ~RequestManager() = default;
    RequestManager(const RequestManager&) = delete;
    RequestManager& operator=(const RequestManager&) = delete;
    // ✅ 这两个就是你问的变量
    std::list<fs::p2p::Request> m_requests;  // 存放请求
    std::mutex m_mutex;                      // 锁，保证线程安全

    std::list<fs::p2p::Request> mRequestList;
    std::mutex mMutex;
};

#endif // REQUEST_MANAGER_H
