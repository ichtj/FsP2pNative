#include <jni.h>
#include <android/log.h>

#define TAG		"core_fsp2p"
bool g_enableLogging = true; // 全局布尔变量，用于控制日志是否打印

// 设置布尔变量控制日志是否打印的函数
void setLoggingEnabled(bool enable) {
    g_enableLogging = enable;
}

// 设置布尔变量控制日志是否打印的函数
jboolean getLoggingEnabled() {
    return g_enableLogging;
}

#define LOGI(...)	if (g_enableLogging) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)
#define LOGD(...)	if (g_enableLogging) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGW(...)	if (g_enableLogging) __android_log_print(ANDROID_LOG_WARN, TAG, __VA_ARGS__)
#define	LOGE(...)	if (g_enableLogging) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)