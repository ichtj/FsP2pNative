#ifndef JAVA_IINFOMATIONS_CALLBACK_H
#define JAVA_IINFOMATIONS_CALLBACK_H

#include <jni.h>
#include <vector>
#include <string>
#include <memory>
#include "fs_p2p/Packetizer.h"

// IInfomationsCallback 类声明
class IInfomationsCallback {
public:
    IInfomationsCallback();
    ~IInfomationsCallback();

    // 设置回调
    void set(JNIEnv* env, jobject obj);

    // 清理回调
    void clear(JNIEnv* env);

    // 获取 JNIEnv 并决定是否需要 detach
    JNIEnv* getEnv(JavaVM* gJvm, bool& attached);

    // 调用 JNI 方法（通用模板）
    template <typename... Args>
    void callMethod(jmethodID methodID, Args... args);

    // 各种回调方法
    void callDevices(JavaVM* gJvm, const std::vector<fs::p2p::InfomationManifest>& infos);

private:
    jobject globalRef = nullptr;
    jclass  callbackClass = nullptr;
    jclass  infoClass = nullptr;           // Infomation 类
    jclass  typeEnumClass = nullptr;       // Type 枚举类
    jmethodID mid_deivces = nullptr;

    // 转换 C++ InfomationManifest 到 Java Infomation
    jobject toJavaInfomation(JNIEnv* env, const fs::p2p::InfomationManifest& info);

    // 缓存 Type 枚举值
    jobject jTypeGateway = nullptr;
    jobject jTypeService = nullptr;
    jobject jTypeUnknown = nullptr;
};

// 设置全局回调
void setGlobalInfomationsCallback(JNIEnv* env, jobject callback);
void clearGlobalInfomationsCallback(JNIEnv* env);

#endif // JAVA_IINFOMATIONS_CALLBACK_H