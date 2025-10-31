#ifndef PUT_TYPE_TOOL_H
#define PUT_TYPE_TOOL_H

#include <jni.h>
#include <string>

class PutTypeTool {
public:
    static void init(JavaVM* vm);
    static void release(JNIEnv* env);

    static int METHOD();
    static int UPLOAD();
    static int EVENT();
    static int UPGRADE();
    static int SETPERTIES();
    static int GETPERTIES();
    static int BROADCAST();

private:
    static JavaVM* gJvm;
    static jclass putTypeClass;

    static JNIEnv* getEnv(bool& attached);
    static int getStaticInt(const char* fieldName);
};

#endif // PUT_TYPE_TOOL_H
