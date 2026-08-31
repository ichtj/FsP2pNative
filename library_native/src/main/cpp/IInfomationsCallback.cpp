#include "IInfomationsCallback.h"

#include "Logger.h"

IInfomationsCallback::IInfomationsCallback() = default;
IInfomationsCallback::~IInfomationsCallback() = default;

void IInfomationsCallback::set(JNIEnv* env, jobject obj) {
    if (!env || !obj) return;

    JavaVM* nextVm = nullptr;
    env->GetJavaVM(&nextVm);
    jobject nextCallback = env->NewGlobalRef(obj);
    jclass callbackClass = env->GetObjectClass(obj);
    jmethodID nextMethod = callbackClass
            ? env->GetMethodID(callbackClass, "deivces", "(Ljava/util/List;)V")
            : nullptr;

    jclass localInfoClass = env->FindClass("com/library/natives/Infomation");
    jclass nextInfoClass = localInfoClass
            ? static_cast<jclass>(env->NewGlobalRef(localInfoClass))
            : nullptr;
    jclass typeClass = env->FindClass("com/library/natives/Type");
    jfieldID gatewayField = typeClass
            ? env->GetStaticFieldID(typeClass, "Gateway", "Lcom/library/natives/Type;")
            : nullptr;
    jfieldID serviceField = typeClass
            ? env->GetStaticFieldID(typeClass, "Service", "Lcom/library/natives/Type;")
            : nullptr;
    jfieldID unknownField = typeClass
            ? env->GetStaticFieldID(typeClass, "Unknown", "Lcom/library/natives/Type;")
            : nullptr;

    jobject localGateway = gatewayField
            ? env->GetStaticObjectField(typeClass, gatewayField)
            : nullptr;
    jobject localService = serviceField
            ? env->GetStaticObjectField(typeClass, serviceField)
            : nullptr;
    jobject localUnknown = unknownField
            ? env->GetStaticObjectField(typeClass, unknownField)
            : nullptr;
    jobject nextGateway = localGateway ? env->NewGlobalRef(localGateway) : nullptr;
    jobject nextService = localService ? env->NewGlobalRef(localService) : nullptr;
    jobject nextUnknown = localUnknown ? env->NewGlobalRef(localUnknown) : nullptr;

    if (callbackClass) env->DeleteLocalRef(callbackClass);
    if (localInfoClass) env->DeleteLocalRef(localInfoClass);
    if (typeClass) env->DeleteLocalRef(typeClass);
    if (localGateway) env->DeleteLocalRef(localGateway);
    if (localService) env->DeleteLocalRef(localService);
    if (localUnknown) env->DeleteLocalRef(localUnknown);

    const bool valid = nextVm && nextCallback && nextInfoClass && nextMethod &&
                       nextGateway && nextService && nextUnknown &&
                       !env->ExceptionCheck();
    if (!valid) {
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        if (nextCallback) env->DeleteGlobalRef(nextCallback);
        if (nextInfoClass) env->DeleteGlobalRef(nextInfoClass);
        if (nextGateway) env->DeleteGlobalRef(nextGateway);
        if (nextService) env->DeleteGlobalRef(nextService);
        if (nextUnknown) env->DeleteGlobalRef(nextUnknown);
        return;
    }

    jobject oldCallback = nullptr;
    jclass oldInfoClass = nullptr;
    jobject oldGateway = nullptr;
    jobject oldService = nullptr;
    jobject oldUnknown = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        oldCallback = globalRef;
        oldInfoClass = infoClass;
        oldGateway = typeGateway;
        oldService = typeService;
        oldUnknown = typeUnknown;
        javaVm = nextVm;
        globalRef = nextCallback;
        infoClass = nextInfoClass;
        midDevices = nextMethod;
        typeGateway = nextGateway;
        typeService = nextService;
        typeUnknown = nextUnknown;
    }
    if (oldCallback) env->DeleteGlobalRef(oldCallback);
    if (oldInfoClass) env->DeleteGlobalRef(oldInfoClass);
    if (oldGateway) env->DeleteGlobalRef(oldGateway);
    if (oldService) env->DeleteGlobalRef(oldService);
    if (oldUnknown) env->DeleteGlobalRef(oldUnknown);
}

void IInfomationsCallback::clear(JNIEnv* env) {
    if (!env) return;

    jobject oldCallback = nullptr;
    jclass oldInfoClass = nullptr;
    jobject oldGateway = nullptr;
    jobject oldService = nullptr;
    jobject oldUnknown = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        oldCallback = globalRef;
        oldInfoClass = infoClass;
        oldGateway = typeGateway;
        oldService = typeService;
        oldUnknown = typeUnknown;
        globalRef = nullptr;
        infoClass = nullptr;
        midDevices = nullptr;
        typeGateway = nullptr;
        typeService = nullptr;
        typeUnknown = nullptr;
    }

    if (oldCallback) env->DeleteGlobalRef(oldCallback);
    if (oldInfoClass) env->DeleteGlobalRef(oldInfoClass);
    if (oldGateway) env->DeleteGlobalRef(oldGateway);
    if (oldService) env->DeleteGlobalRef(oldService);
    if (oldUnknown) env->DeleteGlobalRef(oldUnknown);
}

JNIEnv* IInfomationsCallback::getEnv(JavaVM* jvm, bool& attached) {
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

void IInfomationsCallback::callDevices(
        JavaVM* jvm, const std::vector<fs::p2p::InfomationManifest>& infos) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env) return;

    jobject callback = nullptr;
    jclass info = nullptr;
    jobject gateway = nullptr;
    jobject service = nullptr;
    jobject unknown = nullptr;
    jmethodID method = nullptr;
    {
        std::lock_guard<std::mutex> lock(callbackMutex);
        if (globalRef && infoClass && midDevices) {
            callback = env->NewLocalRef(globalRef);
            info = static_cast<jclass>(env->NewLocalRef(infoClass));
            gateway = env->NewLocalRef(typeGateway);
            service = env->NewLocalRef(typeService);
            unknown = env->NewLocalRef(typeUnknown);
            method = midDevices;
        }
    }

    jclass listClass = env->FindClass("java/util/ArrayList");
    jmethodID listConstructor = listClass
            ? env->GetMethodID(listClass, "<init>", "()V")
            : nullptr;
    jmethodID listAdd = listClass
            ? env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z")
            : nullptr;
    jmethodID infoConstructor = info
            ? env->GetMethodID(
                    info,
                    "<init>",
                    "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lcom/library/natives/Type;I)V")
            : nullptr;
    jobject list = listConstructor && listAdd
            ? env->NewObject(listClass, listConstructor)
            : nullptr;

    if (callback && infoConstructor && list) {
        for (const auto& item : infos) {
            jstring sn = env->NewStringUTF(item.sn.c_str());
            jstring productId = env->NewStringUTF(item.product_id.c_str());
            jstring name = env->NewStringUTF(item.name.c_str());
            jstring model = env->NewStringUTF(item.model.c_str());
            jobject type = item.type == 0 ? gateway : item.type == 1 ? service : unknown;
            jobject javaInfo = env->NewObject(
                    info, infoConstructor, sn, productId, name, model, type, item.version);
            if (javaInfo) env->CallBooleanMethod(list, listAdd, javaInfo);
            if (sn) env->DeleteLocalRef(sn);
            if (productId) env->DeleteLocalRef(productId);
            if (name) env->DeleteLocalRef(name);
            if (model) env->DeleteLocalRef(model);
            if (javaInfo) env->DeleteLocalRef(javaInfo);
        }
        env->CallVoidMethod(callback, method, list);
    }

    if (env->ExceptionCheck()) {
        LOGE("Java callback failed: deivces");
        env->ExceptionDescribe();
        env->ExceptionClear();
    }

    if (list) env->DeleteLocalRef(list);
    if (listClass) env->DeleteLocalRef(listClass);
    if (callback) env->DeleteLocalRef(callback);
    if (info) env->DeleteLocalRef(info);
    if (gateway) env->DeleteLocalRef(gateway);
    if (service) env->DeleteLocalRef(service);
    if (unknown) env->DeleteLocalRef(unknown);
    if (attached) jvm->DetachCurrentThread();
}
