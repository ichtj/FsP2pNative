#include "IInfomationsCallback.h"
#include "fs_p2p/Packetizer.h"
#include <android/log.h>
#include "Logger.h"
//#include "ConvertTools.h"
#include "iTools.h"

// 全局 JavaVM
static JavaVM* g_jvm = nullptr;

// ======== 构造函数 / 析构函数 ========
IInfomationsCallback::IInfomationsCallback() = default;

IInfomationsCallback::~IInfomationsCallback() {
    if (globalRef && g_jvm) {
        JNIEnv* env = nullptr;
        if (g_jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK && env) {
            if (globalRef) env->DeleteGlobalRef(globalRef);
            if (callbackClass) env->DeleteGlobalRef(callbackClass);
            if (infoClass) env->DeleteGlobalRef(infoClass);
            if (typeEnumClass) env->DeleteGlobalRef(typeEnumClass);
            if (jTypeGateway) env->DeleteGlobalRef(jTypeGateway);
            if (jTypeService) env->DeleteGlobalRef(jTypeService);
            if (jTypeUnknown) env->DeleteGlobalRef(jTypeUnknown);
        }
    }
}

// ======== 设置回调 ========
void IInfomationsCallback::set(JNIEnv* env, jobject obj) {
    // 保存 JavaVM
    env->GetJavaVM(&g_jvm);

    // 清理旧的
    clear(env);

    // 创建全局引用
    globalRef = env->NewGlobalRef(obj);
    jclass localCls = env->GetObjectClass(obj);
    callbackClass = (jclass)env->NewGlobalRef(localCls);

    // 获取方法 ID
    mid_deivces = env->GetMethodID(callbackClass, "deivces", "(Ljava/util/List;)V");
    if (!mid_deivces) {
        LOGE("Failed to get deivces method ID");
    }

    // 缓存 Infomation 类
    jclass localInfoCls = env->FindClass("com/library/natives/Infomation");
    infoClass = (jclass)env->NewGlobalRef(localInfoCls);
    iTools::deleteLocalRefs(env, localInfoCls);

    // 缓存 Type 枚举类和值
    jclass localTypeCls = env->FindClass("com/library/natives/Type");
    typeEnumClass = (jclass)env->NewGlobalRef(localTypeCls);

    jfieldID fGateway = env->GetStaticFieldID(typeEnumClass, "Gateway", "Lcom/library/natives/Type;");
    jfieldID fService = env->GetStaticFieldID(typeEnumClass, "Service", "Lcom/library/natives/Type;");
    jfieldID fUnknown = env->GetStaticFieldID(typeEnumClass, "Unknown", "Lcom/library/natives/Type;");

    jTypeGateway = env->NewGlobalRef(env->GetStaticObjectField(typeEnumClass, fGateway));
    jTypeService = env->NewGlobalRef(env->GetStaticObjectField(typeEnumClass, fService));
    jTypeUnknown = env->NewGlobalRef(env->GetStaticObjectField(typeEnumClass, fUnknown));

    iTools::deleteLocalRefs(env, localCls, localTypeCls);
}

// ======== 清理回调 ========
void IInfomationsCallback::clear(JNIEnv* env) {
    if (globalRef && env) {
        env->DeleteGlobalRef(globalRef);
        globalRef = nullptr;
    }
    // 其他 GlobalRef 在析构中清理
}

// ======== 获取 JNIEnv*（自动 Attach）========
JNIEnv* IInfomationsCallback::getEnv(JavaVM* jvm, bool& attached) {
    if (!jvm) return nullptr;
    JNIEnv* env = nullptr;
    if (jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) {
        attached = false;
        return env;
    }
    JavaVMAttachArgs args = {JNI_VERSION_1_6, nullptr, nullptr};
    if (jvm->AttachCurrentThread(&env, &args) != JNI_OK) {
        LOGE("AttachCurrentThread failed");
        return nullptr;
    }
    attached = true;
    return env;
}

// ======== 通用 callMethod ========
template <typename... Args>
void IInfomationsCallback::callMethod(jmethodID methodID, Args... args) {
    bool attached = false;
    JNIEnv* env = getEnv(g_jvm, attached);
    if (!env || !methodID) return;
    env->CallVoidMethod(globalRef, methodID, args...);
    if (attached) g_jvm->DetachCurrentThread();
}

// ======== 转换 C++ → Java Infomation ========
jobject IInfomationsCallback::toJavaInfomation(JNIEnv* env, const fs::p2p::InfomationManifest& info) {
    jmethodID init = env->GetMethodID(infoClass, "<init>", "()V");
    jobject jInfo = env->NewObject(infoClass, init);

    jfieldID fSn        = env->GetFieldID(infoClass, "sn", "Ljava/lang/String;");
    jfieldID fProductId = env->GetFieldID(infoClass, "productId", "Ljava/lang/String;");
    jfieldID fName      = env->GetFieldID(infoClass, "name", "Ljava/lang/String;");
    jfieldID fModel     = env->GetFieldID(infoClass, "model", "Ljava/lang/String;");
    jfieldID fType      = env->GetFieldID(infoClass, "type", "Lcom/library/natives/Type;");
    jfieldID fVersion   = env->GetFieldID(infoClass, "version", "I");

    jstring jsn        = env->NewStringUTF(info.sn.c_str());
    jstring jproductId = env->NewStringUTF(info.product_id.c_str());
    jstring jname      = env->NewStringUTF(info.name.c_str());
    jstring jmodel     = env->NewStringUTF(info.model.c_str());

    env->SetObjectField(jInfo, fSn, jsn);
    env->SetObjectField(jInfo, fProductId, jproductId);
    env->SetObjectField(jInfo, fName, jname);
    env->SetObjectField(jInfo, fModel, jmodel);
    env->SetIntField(jInfo, fVersion, info.version);

    // 设置 Type
    jobject jtype;
    switch (info.type) {
        case 0: jtype = jTypeGateway; break;
        case 1: jtype = jTypeService; break;
        default: jtype = jTypeUnknown; break;
    }
    env->SetObjectField(jInfo, fType, jtype);

    iTools::deleteLocalRefs(env, jsn, jproductId, jname, jmodel);
    return jInfo;
}

// ======== 回调：设备列表 ========
void IInfomationsCallback::callDevices(JavaVM* jvm, const std::vector<fs::p2p::InfomationManifest>& infos) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env || !mid_deivces) {
        return;  // 提前返回，不调用 ExceptionCheck
    }

    // 创建 List
    jclass arrayListCls = env->FindClass("java/util/ArrayList");
    jmethodID listInit = env->GetMethodID(arrayListCls, "<init>", "()V");
    jmethodID listAdd  = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");
    jobject jList = env->NewObject(arrayListCls, listInit);

    if (!arrayListCls || !listInit || !listAdd || !jList) {
        iTools::deleteLocalRefs(env, arrayListCls);
        if (attached) jvm->DetachCurrentThread();
        return;
    }

    // 填充数据
    for (const auto& info : infos) {
        jobject jInfo = toJavaInfomation(env, info);
        if (jInfo) {
            env->CallBooleanMethod(jList, listAdd, jInfo);
            iTools::deleteLocalRefs(env, jInfo);
        }
    }

    // 调用 Java 回调
    env->CallVoidMethod(globalRef, mid_deivces, jList);

    // ✅ 异常检查：必须在 Detach 之前！
    bool hasException = env->ExceptionCheck();
    if (hasException) {
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    // 清理
    iTools::deleteLocalRefs(env, jList, arrayListCls);

    // ✅ Detach 在最后！
    if (attached) {
        jvm->DetachCurrentThread();
    }
}

// ======== 全局回调 ========
static std::unique_ptr<IInfomationsCallback> g_callback;

void setGlobalInfomationsCallback(JNIEnv* env, jobject callback) {
    g_callback = std::make_unique<IInfomationsCallback>();
    g_callback->set(env, callback);
}

void clearGlobalInfomationsCallback(JNIEnv* env) {
    if (g_callback) {
        g_callback->clear(env);
        g_callback.reset();
    }
}