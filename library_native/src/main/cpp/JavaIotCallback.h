#ifndef JAVA_IMQTT_CALLBACK_H
#define JAVA_IMQTT_CALLBACK_H

#include <jni.h>
#include <string>
#include <map>
#include <memory>
#include "BaseData.h"

// JavaIotCallback 类声明
class JavaIotCallback {
public:
    JavaIotCallback();
    ~JavaIotCallback();

    // 设置回调
    void set(JNIEnv* env, jobject obj);

    // 清理回调
    void clear(JNIEnv* env);

    // 获取 JNIEnv 并决定是否需要 detach
    JNIEnv* getEnv(JavaVM* gJvm,bool& attached);

    // 调用 JNI 方法
    template <typename... Args>
    void callMethod(jmethodID methodID, Args... args);

    // 各种回调方法
    void callP2pConnState(JavaVM* gJvm,bool connected, const std::string& description);
    void callIotConnState(JavaVM* gJvm,bool connected, const std::string& description);
    void callMsgArrives(JavaVM* gJvm,const BaseData& baseData);  // 修改为传递 C++ 类型 BaseData
    void callPushed(JavaVM* gJvm,const BaseData& baseData);  // 修改为传递 C++ 类型 BaseData
    void callIotReplyed(const std::string& act, const std::string& iid);
    void callPushFail(JavaVM* gJvm,const BaseData& baseData, const std::string& desc);
    void callSubscribed(JavaVM* gJvm,const std::string& topic);
    void callSubscribeFail(JavaVM* gJvm,const std::string& topic, const std::string& desc);

private:
    jobject globalRef;
    jclass baseDataClass;  // 新增：缓存BaseData类的全局引用
    jmethodID mid_p2p_connState,mid_iot_connState, mid_msgArrives, mid_pushed, mid_iotReplyed, mid_pushFail, mid_subscribed, mid_subscribeFail;

    // 转换 C++ BaseData 到 Java BaseData
    jobject toJavaBaseData(JNIEnv* env, const BaseData& baseData);
};

// 设置全局回调
void setGlobalCallback(JNIEnv* env, jobject callback);
void clearGlobalCallback(JNIEnv* env);

#endif // JAVA_IMQTT_CALLBACK_H
