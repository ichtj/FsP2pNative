#include "PutTypeTool.h"
#include <iostream>

JavaVM* PutTypeTool::gJvm = nullptr;
jclass PutTypeTool::putTypeClass = nullptr;

void PutTypeTool::init(JavaVM* vm) {
    gJvm = vm;
    if (!vm) return;

    bool attached = false;
    JNIEnv* env = getEnv(attached);
    if (!env) return;

    jclass localClass = env->FindClass("com/library/natives/PutType");
    if (!localClass) {
        std::cerr << "❌ 找不到 com/library/natives/PutType 类" << std::endl;
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        if (attached) gJvm->DetachCurrentThread();
        return;
    }

    putTypeClass = (jclass)env->NewGlobalRef(localClass);
    env->DeleteLocalRef(localClass);

    if (attached) gJvm->DetachCurrentThread();

    std::cout << "✅ PutTypeTool 初始化成功" << std::endl;
}

void PutTypeTool::release(JNIEnv* env) {
    if (putTypeClass && env) {
        env->DeleteGlobalRef(putTypeClass);
        putTypeClass = nullptr;
    }
}

JNIEnv* PutTypeTool::getEnv(bool& attached) {
    attached = false;
    if (!gJvm) return nullptr;
    JNIEnv* env = nullptr;

    if (gJvm->GetEnv((void**)&env, JNI_VERSION_1_6) != JNI_OK) {
        if (gJvm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        } else {
            return nullptr;
        }
    }
    return env;
}

int PutTypeTool::getStaticInt(const char* fieldName) {
    if (!putTypeClass || !gJvm) {
        std::cerr << "❌ PutTypeTool 未初始化 (putTypeClass 或 gJvm 为空)" << std::endl;
        return 0;
    }

    bool attached = false;
    JNIEnv* env = getEnv(attached);
    if (!env) return 0;

    jfieldID fid = env->GetStaticFieldID(putTypeClass, fieldName, "I");
    if (!fid) {
        std::cerr << "❌ 找不到字段: " << fieldName << std::endl;
        if (env->ExceptionCheck()) {
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        if (attached) gJvm->DetachCurrentThread();
        return 0;
    }

    jint value = env->GetStaticIntField(putTypeClass, fid);
    if (attached) gJvm->DetachCurrentThread();
    return value;
}

int PutTypeTool::METHOD()     { return getStaticInt("METHOD"); }
int PutTypeTool::UPLOAD()     { return getStaticInt("UPLOAD"); }
int PutTypeTool::EVENT()      { return getStaticInt("EVENT"); }
int PutTypeTool::UPGRADE()    { return getStaticInt("UPGRADE"); }
int PutTypeTool::SETPERTIES() { return getStaticInt("SETPERTIES"); }
int PutTypeTool::GETPERTIES() { return getStaticInt("GETPERTIES"); }
int PutTypeTool::BROADCAST()  { return getStaticInt("BROADCAST"); }
