#ifndef INFORMATION_MANIFEST_HELPER_H
#define INFORMATION_MANIFEST_HELPER_H

#include <string>
#include <map>
#include <mutex>
#include <vector>
#include <android/log.h>
#include "fs_p2p/Packetizer.h"

#define LOG_TAG "SubscribeInfomation"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * 全局单例管理类
 */
class SubscribeInfomation {
public:
    // 获取单例实例
    static SubscribeInfomation &getInstance();

    // 添加设备信息
    void addManifest(const fs::p2p::InfomationManifest &info);

    // 按 SN 获取设备信息（返回 nullptr 表示未找到）
    const fs::p2p::InfomationManifest *getManifest(const std::string &sn);

    // 删除设备信息
    void removeManifest(const std::string &sn);

    // 获取所有设备信息
    std::vector<fs::p2p::InfomationManifest> getAllManifests();

    // 打印所有设备
    void printAll();

private:
    // 构造函数设为私有
    SubscribeInfomation() = default;
    // 禁止拷贝
    SubscribeInfomation(const SubscribeInfomation &) = delete;
    SubscribeInfomation &operator=(const SubscribeInfomation &) = delete;

private:
    std::map<std::string, fs::p2p::InfomationManifest> mManifestMap;
    std::mutex mMutex;
};

#endif // INFORMATION_MANIFEST_HELPER_H
