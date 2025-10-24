// JavaIMqttCallback.cpp

#include "JavaIMqttCallback.h"
#include <jni.h>
#include "BaseDataConverter.h"

JavaIMqttCallback::JavaIMqttCallback()
        : globalRef(nullptr), mid_connState(nullptr), mid_msgArrives(nullptr), mid_pushed(nullptr),
          mid_iotReplyed(nullptr), mid_pushFail(nullptr), mid_subscribed(nullptr), mid_subscribeFail(nullptr) {}

JavaIMqttCallback::~JavaIMqttCallback() {

}

void JavaIMqttCallback::set(JNIEnv* env, jobject obj) {
    clear(env);
    if (!obj) return;
    globalRef = env->NewGlobalRef(obj);
    jclass cls = env->GetObjectClass(obj);
    jclass localBaseDataClass = env->FindClass("com/library/natives/BaseData");
    if (localBaseDataClass) {
        baseDataClass = (jclass)env->NewGlobalRef(localBaseDataClass);
        env->DeleteLocalRef(localBaseDataClass);
        LOGD("BaseData类缓存成功");
    } else {
        LOGE("找不到BaseData类");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }
    // 获取方法ID
    mid_connState = env->GetMethodID(cls, "connState", "(ZLjava/lang/String;)V");
    mid_msgArrives = env->GetMethodID(cls, "msgArrives", "(Lcom/library/natives/BaseData;)V");
    mid_pushed = env->GetMethodID(cls, "pushed", "(Lcom/library/natives/BaseData;)V");
    mid_iotReplyed = env->GetMethodID(cls, "iotReplyed", "(Ljava/lang/String;Ljava/lang/String;)V");
    mid_pushFail = env->GetMethodID(cls, "pushFail", "(Lcom/library/natives/BaseData;Ljava/lang/String;)V");
    mid_subscribed = env->GetMethodID(cls, "subscribed", "(Ljava/lang/String;)V");
    mid_subscribeFail = env->GetMethodID(cls, "subscribeFail", "(Ljava/lang/String;Ljava/lang/String;)V");

    env->DeleteLocalRef(cls);
}

void JavaIMqttCallback::clear(JNIEnv* env) {
    if (globalRef) {
        env->DeleteGlobalRef(globalRef);
        globalRef = nullptr;
    }

    mid_connState = mid_msgArrives = mid_pushed = mid_iotReplyed = mid_pushFail = mid_subscribed = mid_subscribeFail = nullptr;
}

JNIEnv* JavaIMqttCallback::getEnv(JavaVM* gJvm,bool& attached) {
    attached = false;
    JNIEnv* env = nullptr;
    if (!gJvm) return nullptr;
    if (gJvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
        return env;
    }
    if (gJvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
        attached = true;
        return env;
    }
    return nullptr;
}

// 回调方法实现
void JavaIMqttCallback::callSubscribed(JavaVM* gJvm,const std::string &topic) {
    if (!globalRef || !mid_subscribed) return;
    bool attached = false;
    JNIEnv* env = getEnv(gJvm,attached);
    if (!env) return;
    jstring jtopic = env->NewStringUTF(topic.c_str());
    env->CallVoidMethod(globalRef, mid_subscribed, jtopic);
    if (jtopic) env->DeleteLocalRef(jtopic);
    if (attached) gJvm->DetachCurrentThread();
}

// 回调方法实现
void JavaIMqttCallback::callSubscribeFail(JavaVM *gJvm, const std::string &topic,
                                          const std::string &desc) {
    if (!globalRef || !mid_subscribed) return;
    bool attached = false;
    JNIEnv* env = getEnv(gJvm,attached);
    if (!env) return;
    jstring jtopic = env->NewStringUTF(topic.c_str());
    jstring jdesc = env->NewStringUTF(desc.c_str());
    env->CallVoidMethod(globalRef, mid_subscribeFail, jtopic,jdesc);
    if (jtopic) env->DeleteLocalRef(jtopic);
    if (attached) gJvm->DetachCurrentThread();
}

// 回调方法实现
void JavaIMqttCallback::callConnState(JavaVM* gJvm,bool connected, const std::string& description) {
    if (!globalRef || !mid_connState) return;
    bool attached = false;
    JNIEnv* env = getEnv(gJvm,attached);
    if (!env) return;
    jstring jdesc = env->NewStringUTF(description.c_str());
    env->CallVoidMethod(globalRef, mid_connState, (jboolean)connected, jdesc);
    if (jdesc) env->DeleteLocalRef(jdesc);
    if (attached) gJvm->DetachCurrentThread();
}

void JavaIMqttCallback::callPushed(JavaVM* gJvm, const BaseData& baseData) {
    LOGD("callPushed调用: iPutType=%d, iid=%s", baseData.iPutType, baseData.iid.c_str());

    if (!globalRef || !mid_pushed){
        LOGE("回调对象或方法未初始化");
        return;
    }

    // 获取JNIEnv
    bool needDetach = false;
    bool attached = false;
    JNIEnv* env = getEnv(gJvm,attached);
    if (!env){
        LOGE("无法获取JNIEnv");
        return;
    }

    // 将C++ BaseData转换为Java BaseData对象
    jobject jBaseData = BaseDataConverter::toJavaObject(env, baseData,baseDataClass);

    if (jBaseData == nullptr) {
        LOGE("转换BaseData失败");
        if (needDetach) {
            gJvm->DetachCurrentThread();
        }
        return;
    }

    // 调用Java回调方法
    env->CallVoidMethod(globalRef, mid_pushed, jBaseData);

    // 检查Java异常
    if (env->ExceptionCheck()) {
        LOGE("Java回调方法抛出异常");
        env->ExceptionDescribe();
        env->ExceptionClear();
    } else {
        LOGD("Java回调成功");
    }

    // 清理局部引用
    env->DeleteLocalRef(jBaseData);

    // 如果是新attach的线程，需要detach
    if (needDetach) {
        gJvm->DetachCurrentThread();
        LOGD("线程已detach");
    }

}

void JavaIMqttCallback::callPushFail(JavaVM *gJvm, const BaseData &baseData,const std::string &desc) {
    LOGD("callPushed调用: iPutType=%d, iid=%s", baseData.iPutType, baseData.iid.c_str());

    if (!globalRef || !mid_pushed){
        LOGE("回调对象或方法未初始化");
        return;
    }

    // 获取JNIEnv
    bool needDetach = false;
    bool attached = false;
    JNIEnv* env = getEnv(gJvm,attached);
    if (!env){
        LOGE("无法获取JNIEnv");
        return;
    }

    // 将C++ BaseData转换为Java BaseData对象
    jobject jBaseData = BaseDataConverter::toJavaObject(env, baseData,baseDataClass);

    if (jBaseData == nullptr) {
        LOGE("转换BaseData失败");
        if (needDetach) {
            gJvm->DetachCurrentThread();
        }
        return;
    }

    jstring jdesc = env->NewStringUTF(desc.c_str());

    // 调用Java回调方法
    env->CallVoidMethod(globalRef, mid_pushed, jBaseData,jdesc);

    // 检查Java异常
    if (env->ExceptionCheck()) {
        LOGE("Java回调方法抛出异常");
        env->ExceptionDescribe();
        env->ExceptionClear();
    } else {
        LOGD("Java回调成功");
    }

    // 清理局部引用
    env->DeleteLocalRef(jBaseData);

    // 如果是新attach的线程，需要detach
    if (needDetach) {
        gJvm->DetachCurrentThread();
        LOGD("线程已detach");
    }

}

void JavaIMqttCallback::callMsgArrives(JavaVM* gJvm, const BaseData& baseData) {
    LOGD("callMsgArrives调用: iPutType=%d, iid=%s", baseData.iPutType, baseData.iid.c_str());

    if (!globalRef || !mid_msgArrives){
        LOGE("回调对象或方法未初始化");
        return;
    }

    // 获取JNIEnv
    bool needDetach = false;
    bool attached = false;
    JNIEnv* env = getEnv(gJvm,attached);
    if (!env){
        LOGE("无法获取JNIEnv");
        return;
    }

    // 将C++ BaseData转换为Java BaseData对象
    jobject jBaseData = BaseDataConverter::toJavaObject(env, baseData,baseDataClass);

    if (jBaseData == nullptr) {
        LOGE("转换BaseData失败");
        if (needDetach) {
            gJvm->DetachCurrentThread();
        }
        return;
    }

    // 调用Java回调方法
    env->CallVoidMethod(globalRef, mid_msgArrives, jBaseData);

    // 检查Java异常
    if (env->ExceptionCheck()) {
        LOGE("Java回调方法抛出异常");
        env->ExceptionDescribe();
        env->ExceptionClear();
    } else {
        LOGD("Java回调成功");
    }

    // 清理局部引用
    env->DeleteLocalRef(jBaseData);

    // 如果是新attach的线程，需要detach
    if (needDetach) {
        gJvm->DetachCurrentThread();
        LOGD("线程已detach");
    }
}
