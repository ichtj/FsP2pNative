#ifndef JAVA_IMQTT_CALLBACK_H
#define JAVA_IMQTT_CALLBACK_H

#include <jni.h>

#include <mutex>
#include <string>

#include "BaseData.h"

class PipelineCallback {
public:
    PipelineCallback();
    ~PipelineCallback();

    PipelineCallback(const PipelineCallback&) = delete;
    PipelineCallback& operator=(const PipelineCallback&) = delete;

    void set(JNIEnv* env, jobject obj);
    void clear(JNIEnv* env);

    JNIEnv* getEnv(JavaVM* jvm, bool& attached);

    void callP2pConnState(JavaVM* jvm, bool connected, const std::string& description);
    void callIotConnState(JavaVM* jvm, bool connected, const std::string& description);
    void callMsgArrives(JavaVM* jvm, const BaseData& baseData);
    void callPushed(JavaVM* jvm, const BaseData& baseData);
    void callIotReplyed(const std::string& act, const std::string& iid);
    void callPushFail(JavaVM* jvm, const BaseData& baseData, const std::string& desc);
    void callSubscribed(JavaVM* jvm, const std::string& topic);
    void callSubscribeFail(JavaVM* jvm, const std::string& topic, const std::string& desc);

private:
    void clearPendingException(JNIEnv* env, const char* operation);

    mutable std::mutex callbackMutex;
    JavaVM* javaVm;
    jobject globalRef;
    jclass baseDataClass;
    jmethodID mid_p2p_connState;
    jmethodID mid_iot_connState;
    jmethodID mid_msgArrives;
    jmethodID mid_pushed;
    jmethodID mid_iotReplyed;
    jmethodID mid_pushFail;
    jmethodID mid_subscribed;
    jmethodID mid_subscribeFail;
};

#endif // JAVA_IMQTT_CALLBACK_H
