#include "PipelineCallback.h"

#include "BaseDataConverter.h"
#include "Logger.h"

PipelineCallback::PipelineCallback()
        : javaVm(nullptr),
          globalRef(nullptr),
          baseDataClass(nullptr),
          mid_p2p_connState(nullptr),
          mid_iot_connState(nullptr),
          mid_msgArrives(nullptr),
          mid_pushed(nullptr),
          mid_iotReplyed(nullptr),
          mid_pushFail(nullptr),
          mid_subscribed(nullptr),
          mid_subscribeFail(nullptr) {}

PipelineCallback::~PipelineCallback() = default;

void PipelineCallback::set(JNIEnv* env, jobject obj) {
    if (!env || !obj) return;

    JavaVM* nextVm = nullptr;
    env->GetJavaVM(&nextVm);
    jobject nextGlobalRef = env->NewGlobalRef(obj);
    jclass callbackClass = env->GetObjectClass(obj);
    jclass localBaseDataClass = env->FindClass("com/library/natives/BaseData");
    jclass nextBaseDataClass = localBaseDataClass
            ? static_cast<jclass>(env->NewGlobalRef(localBaseDataClass))
            : nullptr;

    jmethodID nextP2pConnState = callbackClass
            ? env->GetMethodID(callbackClass, "p2pConnState", "(ZLjava/lang/String;)V")
            : nullptr;
    jmethodID nextIotConnState = callbackClass
            ? env->GetMethodID(callbackClass, "iotConnState", "(ZLjava/lang/String;)V")
            : nullptr;
    jmethodID nextMsgArrives = callbackClass
            ? env->GetMethodID(callbackClass, "msgArrives", "(Lcom/library/natives/BaseData;)V")
            : nullptr;
    jmethodID nextPushed = callbackClass
            ? env->GetMethodID(callbackClass, "pushed", "(Lcom/library/natives/BaseData;)V")
            : nullptr;
    jmethodID nextIotReplyed = callbackClass
            ? env->GetMethodID(callbackClass, "iotReplyed", "(Ljava/lang/String;Ljava/lang/String;)V")
            : nullptr;
    jmethodID nextPushFail = callbackClass
            ? env->GetMethodID(callbackClass, "pushFail", "(Lcom/library/natives/BaseData;Ljava/lang/String;)V")
            : nullptr;
    jmethodID nextSubscribed = callbackClass
            ? env->GetMethodID(callbackClass, "subscribed", "(Ljava/lang/String;)V")
            : nullptr;
    jmethodID nextSubscribeFail = callbackClass
            ? env->GetMethodID(callbackClass, "subscribeFail", "(Ljava/lang/String;Ljava/lang/String;)V")
            : nullptr;

    const bool valid = nextVm && nextGlobalRef && nextBaseDataClass &&
                       nextP2pConnState && nextIotConnState && nextMsgArrives &&
                       nextPushed && nextIotReplyed && nextPushFail &&
                       nextSubscribed && nextSubscribeFail && !env->ExceptionCheck();

    if (callbackClass) env->DeleteLocalRef(callbackClass);
    if (localBaseDataClass) env->DeleteLocalRef(localBaseDataClass);

    if (!valid) {
        clearPendingException(env, "initialize pipeline callback");
        if (nextGlobalRef) env->DeleteGlobalRef(nextGlobalRef);
        if (nextBaseDataClass) env->DeleteGlobalRef(nextBaseDataClass);
        return;
    }

    jobject previousGlobalRef = nullptr;
    jclass previousBaseDataClass = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        previousGlobalRef = globalRef;
        previousBaseDataClass = baseDataClass;
        javaVm = nextVm;
        globalRef = nextGlobalRef;
        baseDataClass = nextBaseDataClass;
        mid_p2p_connState = nextP2pConnState;
        mid_iot_connState = nextIotConnState;
        mid_msgArrives = nextMsgArrives;
        mid_pushed = nextPushed;
        mid_iotReplyed = nextIotReplyed;
        mid_pushFail = nextPushFail;
        mid_subscribed = nextSubscribed;
        mid_subscribeFail = nextSubscribeFail;
    }

    if (previousGlobalRef) env->DeleteGlobalRef(previousGlobalRef);
    if (previousBaseDataClass) env->DeleteGlobalRef(previousBaseDataClass);
}

void PipelineCallback::clear(JNIEnv* env) {
    if (!env) return;

    jobject previousGlobalRef = nullptr;
    jclass previousBaseDataClass = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        previousGlobalRef = globalRef;
        previousBaseDataClass = baseDataClass;
        globalRef = nullptr;
        baseDataClass = nullptr;
        mid_p2p_connState = nullptr;
        mid_iot_connState = nullptr;
        mid_msgArrives = nullptr;
        mid_pushed = nullptr;
        mid_iotReplyed = nullptr;
        mid_pushFail = nullptr;
        mid_subscribed = nullptr;
        mid_subscribeFail = nullptr;
    }

    if (previousGlobalRef) env->DeleteGlobalRef(previousGlobalRef);
    if (previousBaseDataClass) env->DeleteGlobalRef(previousBaseDataClass);
}

JNIEnv* PipelineCallback::getEnv(JavaVM* jvm, bool& attached) {
    attached = false;
    if (!jvm) return nullptr;

    JNIEnv* env = nullptr;
    if (jvm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
        return env;
    }
    if (jvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
        attached = true;
        return env;
    }
    return nullptr;
}

void PipelineCallback::clearPendingException(JNIEnv* env, const char* operation) {
    if (!env || !env->ExceptionCheck()) return;
    LOGE("Java callback failed: %s", operation);
    env->ExceptionDescribe();
    env->ExceptionClear();
}

void PipelineCallback::callP2pConnState(
        JavaVM* jvm, bool connected, const std::string& description) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    jobject callback = nullptr;
    jmethodID method = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && mid_p2p_connState) {
            callback = env->NewLocalRef(globalRef);
            method = mid_p2p_connState;
        }
    }
    if (callback) {
        jstring desc = env->NewStringUTF(description.c_str());
        env->CallVoidMethod(callback, method, static_cast<jboolean>(connected), desc);
        clearPendingException(env, "p2pConnState");
        if (desc) env->DeleteLocalRef(desc);
        env->DeleteLocalRef(callback);
    }
    if (attached) jvm->DetachCurrentThread();
}

void PipelineCallback::callIotConnState(
        JavaVM* jvm, bool connected, const std::string& description) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    jobject callback = nullptr;
    jmethodID method = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && mid_iot_connState) {
            callback = env->NewLocalRef(globalRef);
            method = mid_iot_connState;
        }
    }
    if (callback) {
        jstring desc = env->NewStringUTF(description.c_str());
        env->CallVoidMethod(callback, method, static_cast<jboolean>(connected), desc);
        clearPendingException(env, "iotConnState");
        if (desc) env->DeleteLocalRef(desc);
        env->DeleteLocalRef(callback);
    }
    if (attached) jvm->DetachCurrentThread();
}

namespace {

struct DataCallbackSnapshot {
    jobject callback = nullptr;
    jclass dataClass = nullptr;
    jmethodID method = nullptr;
};

} // namespace

void PipelineCallback::callMsgArrives(JavaVM* jvm, const BaseData& baseData) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    DataCallbackSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && baseDataClass && mid_msgArrives) {
            snapshot.callback = env->NewLocalRef(globalRef);
            snapshot.dataClass = static_cast<jclass>(env->NewLocalRef(baseDataClass));
            snapshot.method = mid_msgArrives;
        }
    }
    if (snapshot.callback && snapshot.dataClass) {
        jobject data = BaseDataConverter::toJavaObject(env, baseData, snapshot.dataClass);
        if (data) {
            env->CallVoidMethod(snapshot.callback, snapshot.method, data);
            clearPendingException(env, "msgArrives");
            env->DeleteLocalRef(data);
        }
    }
    if (snapshot.dataClass) env->DeleteLocalRef(snapshot.dataClass);
    if (snapshot.callback) env->DeleteLocalRef(snapshot.callback);
    if (attached) jvm->DetachCurrentThread();
}

void PipelineCallback::callPushed(JavaVM* jvm, const BaseData& baseData) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    DataCallbackSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && baseDataClass && mid_pushed) {
            snapshot.callback = env->NewLocalRef(globalRef);
            snapshot.dataClass = static_cast<jclass>(env->NewLocalRef(baseDataClass));
            snapshot.method = mid_pushed;
        }
    }
    if (snapshot.callback && snapshot.dataClass) {
        jobject data = BaseDataConverter::toJavaObject(env, baseData, snapshot.dataClass);
        if (data) {
            env->CallVoidMethod(snapshot.callback, snapshot.method, data);
            clearPendingException(env, "pushed");
            env->DeleteLocalRef(data);
        }
    }
    if (snapshot.dataClass) env->DeleteLocalRef(snapshot.dataClass);
    if (snapshot.callback) env->DeleteLocalRef(snapshot.callback);
    if (attached) jvm->DetachCurrentThread();
}

void PipelineCallback::callIotReplyed(const std::string& act, const std::string& iid) {
    JavaVM* jvm = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        jvm = javaVm;
    }

    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    jobject callback = nullptr;
    jmethodID method = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && mid_iotReplyed) {
            callback = env->NewLocalRef(globalRef);
            method = mid_iotReplyed;
        }
    }
    if (callback) {
        jstring action = env->NewStringUTF(act.c_str());
        jstring requestId = env->NewStringUTF(iid.c_str());
        env->CallVoidMethod(callback, method, action, requestId);
        clearPendingException(env, "iotReplyed");
        if (action) env->DeleteLocalRef(action);
        if (requestId) env->DeleteLocalRef(requestId);
        env->DeleteLocalRef(callback);
    }
    if (attached) jvm->DetachCurrentThread();
}

void PipelineCallback::callPushFail(
        JavaVM* jvm, const BaseData& baseData, const std::string& desc) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    DataCallbackSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && baseDataClass && mid_pushFail) {
            snapshot.callback = env->NewLocalRef(globalRef);
            snapshot.dataClass = static_cast<jclass>(env->NewLocalRef(baseDataClass));
            snapshot.method = mid_pushFail;
        }
    }
    if (snapshot.callback && snapshot.dataClass) {
        jobject data = BaseDataConverter::toJavaObject(env, baseData, snapshot.dataClass);
        jstring description = env->NewStringUTF(desc.c_str());
        if (data && description) {
            env->CallVoidMethod(snapshot.callback, snapshot.method, data, description);
            clearPendingException(env, "pushFail");
        }
        if (data) env->DeleteLocalRef(data);
        if (description) env->DeleteLocalRef(description);
    }
    if (snapshot.dataClass) env->DeleteLocalRef(snapshot.dataClass);
    if (snapshot.callback) env->DeleteLocalRef(snapshot.callback);
    if (attached) jvm->DetachCurrentThread();
}

void PipelineCallback::callSubscribed(JavaVM* jvm, const std::string& topic) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    jobject callback = nullptr;
    jmethodID method = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && mid_subscribed) {
            callback = env->NewLocalRef(globalRef);
            method = mid_subscribed;
        }
    }
    if (callback) {
        jstring value = env->NewStringUTF(topic.c_str());
        env->CallVoidMethod(callback, method, value);
        clearPendingException(env, "subscribed");
        if (value) env->DeleteLocalRef(value);
        env->DeleteLocalRef(callback);
    }
    if (attached) jvm->DetachCurrentThread();
}

void PipelineCallback::callSubscribeFail(
        JavaVM* jvm, const std::string& topic, const std::string& desc) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    jobject callback = nullptr;
    jmethodID method = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && mid_subscribeFail) {
            callback = env->NewLocalRef(globalRef);
            method = mid_subscribeFail;
        }
    }
    if (callback) {
        jstring topicValue = env->NewStringUTF(topic.c_str());
        jstring description = env->NewStringUTF(desc.c_str());
        env->CallVoidMethod(callback, method, topicValue, description);
        clearPendingException(env, "subscribeFail");
        if (topicValue) env->DeleteLocalRef(topicValue);
        if (description) env->DeleteLocalRef(description);
        env->DeleteLocalRef(callback);
    }
    if (attached) jvm->DetachCurrentThread();
}
