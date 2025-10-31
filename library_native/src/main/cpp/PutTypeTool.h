#ifndef PUT_TYPE_TOOL_H
#define PUT_TYPE_TOOL_H

#include <jni.h>
#include <string>

class PutTypeTool {
public:
    static int METHOD(JNIEnv *env);
    static int UPLOAD(JNIEnv *env);
    static int EVENT(JNIEnv *env);
    static int UPGRADE(JNIEnv *env);
    static int SETPERTIES(JNIEnv *env);
    static int GETPERTIES(JNIEnv *env);
    static int BROADCAST(JNIEnv *env);

private:
    static jclass getClass(JNIEnv *env);
    static int getStaticInt(JNIEnv *env, const char *fieldName);
};

#endif // PUT_TYPE_TOOL_H
