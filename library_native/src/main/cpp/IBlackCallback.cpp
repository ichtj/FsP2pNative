#include "IBlackCallback.h"

// ==================== 静态变量定义 ====================
jobject BlackBeanConverter::g_classLoader = nullptr;
jmethodID BlackBeanConverter::g_loadClass = nullptr;
JavaVM* BlackBeanConverter::g_jvm = nullptr;
std::once_flag BlackBeanConverter::g_onceInit;

// ==================== 自动初始化 ====================
void BlackBeanConverter::ensureInitialized(JNIEnv* env, jobject anyJavaObj) {
    std::call_once(g_onceInit, [env, anyJavaObj]() {
        if (!env || !anyJavaObj) return;
        env->GetJavaVM(&g_jvm);

        // 获取 ClassLoader
        jclass objCls = env->GetObjectClass(anyJavaObj);
        jclass clsClass = env->FindClass("java/lang/Class");
        jmethodID midGetClass = env->GetMethodID(objCls, "getClass", "()Ljava/lang/Class;");
        jobject clsObj = env->CallObjectMethod(anyJavaObj, midGetClass);
        jmethodID midGetClassLoader = env->GetMethodID(clsClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
        jobject loader = env->CallObjectMethod(clsObj, midGetClassLoader);

        g_classLoader = env->NewGlobalRef(loader);

        jclass loaderCls = env->FindClass("java/lang/ClassLoader");
        g_loadClass = env->GetMethodID(loaderCls, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        env->DeleteLocalRef(loaderCls);
        env->DeleteLocalRef(loader);
        env->DeleteLocalRef(clsClass);
        env->DeleteLocalRef(objCls);
        env->DeleteLocalRef(clsObj);

        LOGI("BlackBeanConverter auto-initialized via callback object");
    });
}

// ==================== 获取 JNIEnv ====================
JNIEnv* BlackBeanConverter::getEnv(JavaVM* jvm, bool& attached) {
    attached = false;
    if (!jvm) return nullptr;
    JNIEnv* env = nullptr;
    if (jvm->GetEnv((void**)&env, JNI_VERSION_1_6) == JNI_OK) return env;
    JavaVMAttachArgs args = {JNI_VERSION_1_6, nullptr, nullptr};
    if (jvm->AttachCurrentThread(&env, &args) == JNI_OK) {
        attached = true;
        return env;
    }
    return nullptr;
}

// ==================== 查找类 ====================
jclass BlackBeanConverter::findBlackBeanClass(JNIEnv* env) {
    if (g_classLoader && g_loadClass) {
        jstring className = env->NewStringUTF("com.library.natives.BlackBean");
        jclass cls = (jclass)env->CallObjectMethod(g_classLoader, g_loadClass, className);
        env->DeleteLocalRef(className);
        return cls;
    }
    return env->FindClass("com/library/natives/BlackBean");
}

// ==================== String[] ↔ vector<string> ====================
std::vector<std::string> BlackBeanConverter::convertJavaStringArray(JNIEnv* env, jobjectArray array) {
    std::vector<std::string> result;
    if (!array) return result;
    jsize len = env->GetArrayLength(array);
    result.reserve(len);
    for (jsize i = 0; i < len; ++i) {
        jstring jstr = (jstring)env->GetObjectArrayElement(array, i);
        const char* cstr = env->GetStringUTFChars(jstr, nullptr);
        result.emplace_back(cstr);
        env->ReleaseStringUTFChars(jstr, cstr);
        env->DeleteLocalRef(jstr);
    }
    return result;
}

jobjectArray BlackBeanConverter::convertStringVector(JNIEnv* env, const std::vector<std::string>& vec) {
    jclass stringCls = env->FindClass("java/lang/String");
    jobjectArray arr = env->NewObjectArray(vec.size(), stringCls, nullptr);
    for (jsize i = 0; i < (jsize)vec.size(); ++i) {
        jstring jstr = env->NewStringUTF(vec[i].c_str());
        env->SetObjectArrayElement(arr, i, jstr);
        env->DeleteLocalRef(jstr);
    }
    env->DeleteLocalRef(stringCls);
    return arr;
}

// ==================== BlackBean ↔ Object ====================
BlackBean BlackBeanConverter::fromJava(JNIEnv* env, jobject obj) {
    BlackBean bean;
    if (!obj) return bean;
    jclass cls = env->GetObjectClass(obj);
    jfieldID fDevices = env->GetFieldID(cls, "devices_array", "[Ljava/lang/String;");
    jfieldID fModels  = env->GetFieldID(cls, "model_array", "[Ljava/lang/String;");
    jfieldID fDesc    = env->GetFieldID(cls, "desc", "Ljava/lang/String;");

    jobjectArray jDevices = (jobjectArray)env->GetObjectField(obj, fDevices);
    jobjectArray jModels  = (jobjectArray)env->GetObjectField(obj, fModels);
    jstring jDesc         = (jstring)env->GetObjectField(obj, fDesc);

    bean.devices_array = convertJavaStringArray(env, jDevices);
    bean.model_array   = convertJavaStringArray(env, jModels);
    if (jDesc) {
        const char* descStr = env->GetStringUTFChars(jDesc, nullptr);
        bean.desc = descStr ? descStr : "";
        env->ReleaseStringUTFChars(jDesc, descStr);
        env->DeleteLocalRef(jDesc);
    }
    env->DeleteLocalRef(cls);
    return bean;
}

jobject BlackBeanConverter::toJava(JNIEnv* env, const BlackBean& bean) {
    jclass cls = findBlackBeanClass(env);
    if (!cls) return nullptr;
    jmethodID ctor = env->GetMethodID(cls, "<init>", "([Ljava/lang/String;[Ljava/lang/String;Ljava/lang/String;)V");
    jobjectArray jDevices = convertStringVector(env, bean.devices_array);
    jobjectArray jModels  = convertStringVector(env, bean.model_array);
    jstring jDesc = env->NewStringUTF(bean.desc.c_str());
    jobject obj = env->NewObject(cls, ctor, jDevices, jModels, jDesc);
    env->DeleteLocalRef(jDevices);
    env->DeleteLocalRef(jModels);
    env->DeleteLocalRef(jDesc);
    env->DeleteLocalRef(cls);
    return obj;
}

// ==================== List<BlackBean> ↔ vector<BlackBean> ====================
std::vector<BlackBean> BlackBeanConverter::fromJavaList(JNIEnv* env, jobject listObj) {
    std::vector<BlackBean> list;
    if (!listObj) return list;
    jclass listCls = env->FindClass("java/util/List");
    jmethodID midSize = env->GetMethodID(listCls, "size", "()I");
    jmethodID midGet  = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
    jint size = env->CallIntMethod(listObj, midSize);
    for (jint i = 0; i < size; ++i) {
        jobject item = env->CallObjectMethod(listObj, midGet, i);
        list.push_back(fromJava(env, item));
        env->DeleteLocalRef(item);
    }
    env->DeleteLocalRef(listCls);
    return list;
}

jobject BlackBeanConverter::toJavaList(JNIEnv* env, const std::vector<BlackBean>& vec) {
    jclass arrCls = env->FindClass("java/util/ArrayList");
    jmethodID ctor = env->GetMethodID(arrCls, "<init>", "()V");
    jmethodID add  = env->GetMethodID(arrCls, "add", "(Ljava/lang/Object;)Z");
    jobject listObj = env->NewObject(arrCls, ctor);
    for (const auto& bean : vec) {
        jobject jBean = toJava(env, bean);
        env->CallBooleanMethod(listObj, add, jBean);
        env->DeleteLocalRef(jBean);
    }
    env->DeleteLocalRef(arrCls);
    return listObj;
}

// ==================== 回调实现 ====================
static std::unique_ptr<IBlackCallback> g_callback;
static JavaVM* g_jvm = nullptr;

IBlackCallback::IBlackCallback() = default;
IBlackCallback::~IBlackCallback() = default;

void IBlackCallback::set(JNIEnv* env, jobject obj) {
    env->GetJavaVM(&g_jvm);
    clear(env);
    globalRef = env->NewGlobalRef(obj);
    jclass localCls = env->GetObjectClass(obj);
    callbackClass = (jclass)env->NewGlobalRef(localCls);
    env->DeleteLocalRef(localCls);

    mid_onBlack = env->GetMethodID(callbackClass, "onBlack", "(Ljava/util/List;)V");

    // 🚀 自动初始化 BlackBeanConverter
    BlackBeanConverter::ensureInitialized(env, obj);
}

void IBlackCallback::clear(JNIEnv* env) {
    if (globalRef) env->DeleteGlobalRef(globalRef);
    if (callbackClass) env->DeleteGlobalRef(callbackClass);
    globalRef = nullptr;
    callbackClass = nullptr;
}

JNIEnv* IBlackCallback::getEnv(JavaVM* jvm, bool& attached) {
    return BlackBeanConverter::getEnv(jvm, attached);
}

void IBlackCallback::callOnBlack(JavaVM* jvm, const std::vector<BlackBean>& list) {
    bool attached = false;
    JNIEnv* env = getEnv(jvm, attached);
    if (!env || !mid_onBlack) return;
    jobject jList = BlackBeanConverter::toJavaList(env, list);
    env->CallVoidMethod(globalRef, mid_onBlack, jList);
    if (env->ExceptionCheck()) { env->ExceptionDescribe(); env->ExceptionClear(); }
    env->DeleteLocalRef(jList);
    if (attached) jvm->DetachCurrentThread();
}

// ==================== 全局管理 ====================
void setGlobalBlackCallback(JNIEnv* env, jobject callback) {
    if (!g_callback) g_callback = std::make_unique<IBlackCallback>();
    g_callback->set(env, callback);
}

void clearGlobalBlackCallback(JNIEnv* env) {
    if (g_callback) {
        g_callback->clear(env);
        g_callback.reset();
    }
}

void callGlobalBlackCallback(JavaVM* jvm, const std::vector<BlackBean>& list) {
    if (g_callback) g_callback->callOnBlack(jvm, list);
}
