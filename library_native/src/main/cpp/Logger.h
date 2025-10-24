#ifndef LOGGER_H
#define LOGGER_H

#include <jni.h>
#include <android/log.h>

#define TAG "BaseXLink"

extern bool g_enableLogging;

void setLoggingEnabled(bool enable);
jboolean getLoggingEnabled();

#define LOGI(...) if (g_enableLogging) __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__)
#define LOGD(...) if (g_enableLogging) __android_log_print(ANDROID_LOG_DEBUG, TAG, __VA_ARGS__)
#define LOGW(...) if (g_enableLogging) __android_log_print(ANDROID_LOG_WARN,  TAG, __VA_ARGS__)
#define LOGE(...) if (g_enableLogging) __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__)

#endif // LOGGER_H