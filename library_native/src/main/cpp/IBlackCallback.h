#ifndef JAVA_IBLACK_CALLBACK_H
#define JAVA_IBLACK_CALLBACK_H

#include <jni.h>
#include <vector>
#include <string>
#include <memory>
#include <mutex>
#include "Logger.h"

// ==================== C++ 实体类 ====================
struct BlackBean {
    std::vector<std::string> devices_array;
    std::vector<std::string> model_array;
    std::string desc;
};

// ==================== 转换工具类 ====================
class BlackBeanConverter {
public:
    // 自动初始化（仅第一次 set 回调时）
    static void ensureInitialized(JNIEnv* env, jobject anyJavaObj);

    // 获取 JNIEnv（自动 attach/detach）
    static JNIEnv* getEnv(JavaVM* jvm, bool& attached);

    // Java String[] ↔ std::vector<std::string>
    static std::vector<std::string> convertJavaStringArray(JNIEnv* env, jobjectArray array);
    static jobjectArray convertStringVector(JNIEnv* env, const std::vector<std::string>& vec);

    // Java BlackBean ↔ C++ BlackBean
    static BlackBean fromJava(JNIEnv* env, jobject javaObject);
    static jobject toJava(JNIEnv* env, const BlackBean& bean);

    // Java List<BlackBean> ↔ std::vector<BlackBean>
    static std::vector<BlackBean> fromJavaList(JNIEnv* env, jobject javaList);
    static jobject toJavaList(JNIEnv* env, const std::vector<BlackBean>& vec);

private:
    static jclass findBlackBeanClass(JNIEnv* env);

    static jobject g_classLoader;
    static jmethodID g_loadClass;
    static JavaVM* g_jvm;
    static std::once_flag g_onceInit;
};

// ==================== 回调类 ====================
class IBlackCallback {
public:
    IBlackCallback();
    ~IBlackCallback();

    void set(JNIEnv* env, jobject obj);
    void clear(JNIEnv* env);

    void callOnBlack(JavaVM* jvm, const std::vector<BlackBean>& beanList);
    static JNIEnv* getEnv(JavaVM* jvm, bool& attached);

private:
    jobject globalRef = nullptr;
    jclass callbackClass = nullptr;
    jmethodID mid_onBlack = nullptr;
};

// 全局管理接口
void setGlobalBlackCallback(JNIEnv* env, jobject callback);
void clearGlobalBlackCallback(JNIEnv* env);
void callGlobalBlackCallback(JavaVM* jvm, const std::vector<BlackBean>& list);

#endif // JAVA_IBLACK_CALLBACK_H
