#include "RequestManager.h"
#include <android/log.h>
#include "Logger.h"

RequestManager& RequestManager::getInstance() {
    static RequestManager instance;
    return instance;
}

void RequestManager::addRequest(const fs::p2p::Request& req) {
    std::lock_guard<std::mutex> lock(mMutex);
    mRequestList.push_back(req);
    if (mRequestList.size() > 100) {
        mRequestList.pop_front();
    }
    LOGD("添加 Request: iid=%s, action=%d", req.iid.c_str(), (int)req.action);
}

std::list<fs::p2p::Request> RequestManager::getAllRequests() {
    return mRequestList;  // 返回一份拷贝，避免外部操作破坏内部数据
}

void RequestManager::dumpRequests() {
    std::lock_guard<std::mutex> lock(mMutex);
    for (const auto& r : mRequestList) {
        LOGD("Request: iid=%s, action=%d", r.iid.c_str(), (int)r.action);
    }
}

void RequestManager::clearByIid(const std::string& iid) {
    std::lock_guard<std::mutex> lock(mMutex);
    mRequestList.remove_if([&](const fs::p2p::Request& r) {
        return r.iid == iid;
    });
    LOGD("清空匹配 iid=%s 的请求", iid.c_str());
}

void RequestManager::clearByAction(fs::p2p::Request::Action action) {
    std::lock_guard<std::mutex> lock(mMutex);
    mRequestList.remove_if([&](const fs::p2p::Request& r) {
        return r.action == action;
    });
    LOGD("清空匹配 action=%d 的请求", (int)action);
}

void RequestManager::clearAll() {
    std::lock_guard<std::mutex> lock(mMutex);
    mRequestList.clear();
    LOGD("已清空所有请求");
}

size_t RequestManager::size() {
    std::lock_guard<std::mutex> lock(mMutex);
    return mRequestList.size();
}
