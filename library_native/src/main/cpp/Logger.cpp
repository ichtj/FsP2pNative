#include "Logger.h"

// 定义全局变量（这里是唯一的定义处）
bool g_enableLogging = true;

// 实现接口
void setLoggingEnabled(bool enable) {
    g_enableLogging = enable;
}

jboolean getLoggingEnabled() {
    return g_enableLogging;
}
