#include "SubscribeInfomation.h"

// 获取单例实例
SubscribeInfomation &SubscribeInfomation::getInstance() {
    static SubscribeInfomation instance;
    return instance;
}

// 添加设备信息
void SubscribeInfomation::addManifest(const fs::p2p::InfomationManifest &info) {
    std::lock_guard<std::mutex> lock(mMutex);
    mManifestMap[info.sn] = info;
    LOGI("Add manifest: SN=%s, ProductID=%s, Type=%d",
         info.sn.c_str(), info.product_id.c_str(), info.type);
}

// 获取设备信息
const fs::p2p::InfomationManifest *SubscribeInfomation::getManifest(const std::string &sn) {
    std::lock_guard<std::mutex> lock(mMutex);
    auto it = mManifestMap.find(sn);
    if (it != mManifestMap.end()) {
        return &it->second;
    }
    return nullptr;
}

// 删除设备
void SubscribeInfomation::removeManifest(const std::string &sn) {
    std::lock_guard<std::mutex> lock(mMutex);
    mManifestMap.erase(sn);
    LOGI("Remove manifest: SN=%s", sn.c_str());
}

// 获取所有设备
std::vector<fs::p2p::InfomationManifest> SubscribeInfomation::getAllManifests() {
    std::lock_guard<std::mutex> lock(mMutex);
    std::vector<fs::p2p::InfomationManifest> list;
    for (const auto &pair : mManifestMap) {
        list.push_back(pair.second);
    }
    return list;
}

// 打印所有设备
void SubscribeInfomation::printAll() {
    std::lock_guard<std::mutex> lock(mMutex);
    for (const auto &pair : mManifestMap) {
        const auto &info = pair.second;
        LOGI("SN=%s, ProductID=%s, Type=%d, PublishTopic=%s",
             info.sn.c_str(), info.product_id.c_str(), info.type,
             info.getPublishTopic().c_str());
    }
}
