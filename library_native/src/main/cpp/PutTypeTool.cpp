#include "PutTypeTool.h"
#include <iostream>

static const char *PUT_TYPE_CLASS_PATH = "com/library/natives/PutType";

jclass PutTypeTool::getClass(JNIEnv *env) {
    jclass cls = env->FindClass(PUT_TYPE_CLASS_PATH);
    if (!cls) {
        std::cerr << "PutType class not found: " << PUT_TYPE_CLASS_PATH << std::endl;
    }
    return cls;
}

int PutTypeTool::getStaticInt(JNIEnv *env, const char *fieldName) {
    jclass cls = getClass(env);
    if (!cls) return 0;

    jfieldID fid = env->GetStaticFieldID(cls, fieldName, "I");
    if (!fid) {
        std::cerr << "PutType field not found: " << fieldName << std::endl;
        return 0;
    }
    return env->GetStaticIntField(cls, fid);
}

int PutTypeTool::METHOD(JNIEnv *env) {
    return getStaticInt(env, "METHOD");
}

int PutTypeTool::UPLOAD(JNIEnv *env) {
    return getStaticInt(env, "UPLOAD");
}

int PutTypeTool::EVENT(JNIEnv *env) {
    return getStaticInt(env, "EVENT");
}

int PutTypeTool::UPGRADE(JNIEnv *env) {
    return getStaticInt(env, "UPGRADE");
}

int PutTypeTool::SETPERTIES(JNIEnv *env) {
    return getStaticInt(env, "SETPERTIES");
}

int PutTypeTool::GETPERTIES(JNIEnv *env) {
    return getStaticInt(env, "GETPERTIES");
}

int PutTypeTool::BROADCAST(JNIEnv *env) {
    return getStaticInt(env, "BROADCAST");
}
