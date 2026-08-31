#ifndef JAVA_IINFOMATIONS_CALLBACK_H
#define JAVA_IINFOMATIONS_CALLBACK_H

#include <jni.h>

#include <mutex>
#include <vector>

#include "fs_p2p/Packetizer.h"

class IInfomationsCallback {
public:
    IInfomationsCallback();
    ~IInfomationsCallback();

    IInfomationsCallback(const IInfomationsCallback&) = delete;
    IInfomationsCallback& operator=(const IInfomationsCallback&) = delete;

    void set(JNIEnv* env, jobject obj);
    void clear(JNIEnv* env);
    void callDevices(JavaVM* jvm, const std::vector<fs::p2p::InfomationManifest>& infos);

private:
    JNIEnv* getEnv(JavaVM* jvm, bool& attached);

    mutable std::mutex callbackMutex;
    JavaVM* javaVm = nullptr;
    jobject globalRef = nullptr;
    jclass infoClass = nullptr;
    jmethodID midDevices = nullptr;
    jobject typeGateway = nullptr;
    jobject typeService = nullptr;
    jobject typeUnknown = nullptr;
};

#endif // JAVA_IINFOMATIONS_CALLBACK_H
