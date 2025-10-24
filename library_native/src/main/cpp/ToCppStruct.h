#include <jni.h>
#include <string>
#include <map>
#include <type_traits>  // 注意要包含这个
#include "fs_p2p/Packetizer.h"
#include "nlohmann/json.hpp"   // for ordered_json
using ordered_json = nlohmann::ordered_json;

// --------------------------------------------
// 公共函数：Map<String,Object> 转换为 C++ map
// --------------------------------------------
static void fillJsonMap(JNIEnv* env, jobject jparams, std::map<std::string, ordered_json>& targetMap) {
    if (jparams == nullptr) return;

    jclass mapClass = env->GetObjectClass(jparams);
    jmethodID entrySetMid = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
    jobject entrySetObj = env->CallObjectMethod(jparams, entrySetMid);

    jclass setClass = env->GetObjectClass(entrySetObj);
    jmethodID iteratorMid = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
    jobject iteratorObj = env->CallObjectMethod(entrySetObj, iteratorMid);

    jclass iteratorClass = env->GetObjectClass(iteratorObj);
    jmethodID hasNextMid = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMid = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    jclass entryClass = nullptr;
    jmethodID getKeyMid = nullptr;
    jmethodID getValueMid = nullptr;

    while (env->CallBooleanMethod(iteratorObj, hasNextMid)) {
        jobject entryObj = env->CallObjectMethod(iteratorObj, nextMid);
        if (entryClass == nullptr) {
            entryClass = env->GetObjectClass(entryObj);
            getKeyMid = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
            getValueMid = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");
        }

        jstring jkey = (jstring)env->CallObjectMethod(entryObj, getKeyMid);
        jobject jvalue = env->CallObjectMethod(entryObj, getValueMid);

        const char* key_cstr = env->GetStringUTFChars(jkey, nullptr);
        std::string key = key_cstr ? key_cstr : "";
        env->ReleaseStringUTFChars(jkey, key_cstr);

        ordered_json value_json;
        if (jvalue != nullptr) {
            jclass clsString = env->FindClass("java/lang/String");
            jclass clsInteger = env->FindClass("java/lang/Integer");
            jclass clsBoolean = env->FindClass("java/lang/Boolean");
            jclass clsDouble = env->FindClass("java/lang/Double");
            jclass clsLong = env->FindClass("java/lang/Long");

            if (env->IsInstanceOf(jvalue, clsString)) {
                const char* str = env->GetStringUTFChars((jstring)jvalue, nullptr);
                value_json = str ? std::string(str) : "";
                env->ReleaseStringUTFChars((jstring)jvalue, str);
            } else if (env->IsInstanceOf(jvalue, clsInteger)) {
                jmethodID intValue = env->GetMethodID(clsInteger, "intValue", "()I");
                jint val = env->CallIntMethod(jvalue, intValue);
                value_json = static_cast<int>(val);
            } else if (env->IsInstanceOf(jvalue, clsBoolean)) {
                jmethodID boolValue = env->GetMethodID(clsBoolean, "booleanValue", "()Z");
                jboolean val = env->CallBooleanMethod(jvalue, boolValue);
                value_json = (bool)val;
            } else if (env->IsInstanceOf(jvalue, clsDouble)) {
                jmethodID doubleValue = env->GetMethodID(clsDouble, "doubleValue", "()D");
                jdouble val = env->CallDoubleMethod(jvalue, doubleValue);
                value_json = static_cast<double>(val);
            } else if (env->IsInstanceOf(jvalue, clsLong)) {
                jmethodID longValue = env->GetMethodID(clsLong, "longValue", "()J");
                jlong val = env->CallLongMethod(jvalue, longValue);
                value_json = static_cast<long long>(val);
            } else {
                value_json = "[UnsupportedType]";
            }
        }

        targetMap[key] = value_json;
        env->DeleteLocalRef(entryObj);
    }

    env->DeleteLocalRef(iteratorObj);
    env->DeleteLocalRef(entrySetObj);
}

// --------------------------------------------
// 各类型转换函数
// --------------------------------------------
fs::p2p::Method convertToMethod(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string) {
    fs::p2p::Method method;
    const char* c_name = env->GetStringUTFChars(jname, nullptr);
    method.name = c_name ? c_name : "";
    env->ReleaseStringUTFChars(jname, c_name);

    const char* c_reason_string = env->GetStringUTFChars(jreason_string, nullptr);
    method.reason_string = c_reason_string ? c_reason_string : "";
    env->ReleaseStringUTFChars(jreason_string, c_reason_string);

    method.reason_code = static_cast<int>(jreason_code);
    fillJsonMap(env, jparams, method.params);
    return method;
}

fs::p2p::Event convertToEvent(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string) {
    fs::p2p::Event event;
    const char* c_name = env->GetStringUTFChars(jname, nullptr);
    event.name = c_name ? c_name : "";
    env->ReleaseStringUTFChars(jname, c_name);

    const char* c_reason_string = env->GetStringUTFChars(jreason_string, nullptr);
    event.reason_string = c_reason_string ? c_reason_string : "";
    env->ReleaseStringUTFChars(jreason_string, c_reason_string);

    event.reason_code = static_cast<int>(jreason_code);
    fillJsonMap(env, jparams, event.params);
    return event;
}

fs::p2p::Service convertToService(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string) {
    fs::p2p::Service service;
    const char* c_name = env->GetStringUTFChars(jname, nullptr);
    service.name = c_name ? c_name : "";
    env->ReleaseStringUTFChars(jname, c_name);

    const char* c_reason_string = env->GetStringUTFChars(jreason_string, nullptr);
    service.reason_string = c_reason_string ? c_reason_string : "";
    env->ReleaseStringUTFChars(jreason_string, c_reason_string);

    service.reason_code = static_cast<int>(jreason_code);
    fillJsonMap(env, jparams, service.propertys);
    return service;
}