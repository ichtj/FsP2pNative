#include <jni.h>
#include "Logger.h"
#include <android/log.h>
#include "nlohmann/json.hpp"
using nlohmann::ordered_json;

/**
 * 前向声明（因为 cppMapToJavaMap 和 jsonToJavaObject 互相调用）
 */
jobject jsonToJavaObjectValue(JNIEnv* env, const ordered_json& value);
jobject cppMapToJavaMapValue(JNIEnv* env, const std::map<std::string, ordered_json>& cppMap);

/**
 * 前向声明
 */
std::map<std::string, ordered_json> javaMapToCppMapValue(JNIEnv* env, jobject jMap);
ordered_json javaObjectToJsonValue(JNIEnv* env, jobject jObj);


// 将 ordered_json 转换为 jobject（Java Object）
jobject jsonToJavaObjectValue(JNIEnv* env, const ordered_json& value) {
    if (value.is_null()) {
        return nullptr;
    } else if (value.is_string()) {
        return env->NewStringUTF(value.get<std::string>().c_str());
    } else if (value.is_boolean()) {
        jclass cls = env->FindClass("java/lang/Boolean");
        jmethodID mid = env->GetStaticMethodID(cls, "valueOf", "(Z)Ljava/lang/Boolean;");
        jobject obj = env->CallStaticObjectMethod(cls, mid, (jboolean)value.get<bool>());
        env->DeleteLocalRef(cls);
        return obj;
    } else if (value.is_number_integer()) {
        jclass cls = env->FindClass("java/lang/Integer");
        jmethodID mid = env->GetStaticMethodID(cls, "valueOf", "(I)Ljava/lang/Integer;");
        jobject obj = env->CallStaticObjectMethod(cls, mid, (jint)value.get<int>());
        env->DeleteLocalRef(cls);
        return obj;
    } else if (value.is_number_float()) {
        jclass cls = env->FindClass("java/lang/Double");
        jmethodID mid = env->GetStaticMethodID(cls, "valueOf", "(D)Ljava/lang/Double;");
        jobject obj = env->CallStaticObjectMethod(cls, mid, (jdouble)value.get<double>());
        env->DeleteLocalRef(cls);
        return obj;
    } else if (value.is_object()) {
        // 嵌套对象
        std::map<std::string, ordered_json> nested = value.get<std::map<std::string, ordered_json>>();
        return cppMapToJavaMapValue(env, nested);  // ✅ 递归调用
    } else if (value.is_array()) {
        jclass arrayListCls = env->FindClass("java/util/ArrayList");
        jmethodID ctor = env->GetMethodID(arrayListCls, "<init>", "()V");
        jmethodID add = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");
        jobject list = env->NewObject(arrayListCls, ctor);
        for (const auto& elem : value) {
            jobject jElem = jsonToJavaObjectValue(env, elem);
            env->CallBooleanMethod(list, add, jElem);
            env->DeleteLocalRef(jElem);
        }
        env->DeleteLocalRef(arrayListCls);
        return list;
    }

    // 其他类型转字符串
    std::string str = value.dump();
    return env->NewStringUTF(str.c_str());
}

// ✅ 将 std::map<std::string, ordered_json> 转为 HashMap<String, Object>
jobject cppMapToJavaMapValue(JNIEnv* env, const std::map<std::string, ordered_json>& cppMap) {
    jclass mapClass = env->FindClass("java/util/HashMap");
    jmethodID init = env->GetMethodID(mapClass, "<init>", "()V");
    jmethodID put = env->GetMethodID(mapClass, "put",
                                     "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jobject hashMap = env->NewObject(mapClass, init);

    for (const auto& [key, value] : cppMap) {
        LOGD("Key=%s, Value=%s", key.c_str(), value.dump().c_str());  // ✅ 打印参数
        jstring jKey = env->NewStringUTF(key.c_str());
        jobject jValue = jsonToJavaObjectValue(env, value);
        env->CallObjectMethod(hashMap, put, jKey, jValue);
        env->DeleteLocalRef(jKey);
        if (jValue) env->DeleteLocalRef(jValue);
    }

    env->DeleteLocalRef(mapClass);
    return hashMap;
}
/**
 * ✅ 将 Java Object 转换为 ordered_json
 */
ordered_json javaObjectToJsonValue(JNIEnv* env, jobject jObj) {
    if (jObj == nullptr) {
        return nullptr;
    }

    // 基本类型判断
    jclass cls = env->GetObjectClass(jObj);

    // String
    jclass stringCls = env->FindClass("java/lang/String");
    if (env->IsInstanceOf(jObj, stringCls)) {
        const char* chars = env->GetStringUTFChars((jstring)jObj, nullptr);
        std::string str(chars);
        env->ReleaseStringUTFChars((jstring)jObj, chars);
        env->DeleteLocalRef(stringCls);
        env->DeleteLocalRef(cls);
        return ordered_json(str);
    }

    // Boolean
    jclass booleanCls = env->FindClass("java/lang/Boolean");
    if (env->IsInstanceOf(jObj, booleanCls)) {
        jmethodID mid = env->GetMethodID(booleanCls, "booleanValue", "()Z");
        jboolean val = env->CallBooleanMethod(jObj, mid);
        env->DeleteLocalRef(booleanCls);
        env->DeleteLocalRef(cls);
        return ordered_json((bool)val);
    }

    // Integer
    jclass integerCls = env->FindClass("java/lang/Integer");
    if (env->IsInstanceOf(jObj, integerCls)) {
        jmethodID mid = env->GetMethodID(integerCls, "intValue", "()I");
        jint val = env->CallIntMethod(jObj, mid);
        env->DeleteLocalRef(integerCls);
        env->DeleteLocalRef(cls);
        return ordered_json((int)val);
    }

    // Double
    jclass doubleCls = env->FindClass("java/lang/Double");
    if (env->IsInstanceOf(jObj, doubleCls)) {
        jmethodID mid = env->GetMethodID(doubleCls, "doubleValue", "()D");
        jdouble val = env->CallDoubleMethod(jObj, mid);
        env->DeleteLocalRef(doubleCls);
        env->DeleteLocalRef(cls);
        return ordered_json((double)val);
    }

    // Map（嵌套）
    jclass mapCls = env->FindClass("java/util/Map");
    if (env->IsInstanceOf(jObj, mapCls)) {
        auto nestedMap = javaMapToCppMapValue(env, jObj);
        env->DeleteLocalRef(mapCls);
        env->DeleteLocalRef(cls);
        return ordered_json(nestedMap);
    }

    // List / ArrayList（数组）
    jclass listCls = env->FindClass("java/util/List");
    if (env->IsInstanceOf(jObj, listCls)) {
        jmethodID sizeMid = env->GetMethodID(listCls, "size", "()I");
        jmethodID getMid = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
        jint size = env->CallIntMethod(jObj, sizeMid);

        ordered_json arr = ordered_json::array();
        for (int i = 0; i < size; ++i) {
            jobject elem = env->CallObjectMethod(jObj, getMid, i);
            arr.push_back(javaObjectToJsonValue(env, elem));
            if (elem) env->DeleteLocalRef(elem);
        }
        env->DeleteLocalRef(listCls);
        env->DeleteLocalRef(cls);
        return arr;
    }

    // 其他类型（未知） -> 字符串
    jmethodID toString = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
    jstring jStr = (jstring)env->CallObjectMethod(jObj, toString);
    const char* chars = env->GetStringUTFChars(jStr, nullptr);
    std::string str(chars);
    env->ReleaseStringUTFChars(jStr, chars);
    env->DeleteLocalRef(jStr);
    env->DeleteLocalRef(cls);

    return ordered_json(str);
}

/**
 * ✅ 将 Java Map<String, Object> 转为 std::map<std::string, ordered_json>
 */
std::map<std::string, ordered_json> javaMapToCppMapValue(JNIEnv* env, jobject jMap) {
    std::map<std::string, ordered_json> cppMap;

    if (jMap == nullptr) return cppMap;

    jclass mapCls = env->FindClass("java/util/Map");
    jmethodID entrySetMid = env->GetMethodID(mapCls, "entrySet", "()Ljava/util/Set;");
    jobject entrySetObj = env->CallObjectMethod(jMap, entrySetMid);

    jclass setCls = env->FindClass("java/util/Set");
    jmethodID iteratorMid = env->GetMethodID(setCls, "iterator", "()Ljava/util/Iterator;");
    jobject iteratorObj = env->CallObjectMethod(entrySetObj, iteratorMid);

    jclass iteratorCls = env->FindClass("java/util/Iterator");
    jmethodID hasNextMid = env->GetMethodID(iteratorCls, "hasNext", "()Z");
    jmethodID nextMid = env->GetMethodID(iteratorCls, "next", "()Ljava/lang/Object;");

    jclass entryCls = env->FindClass("java/util/Map$Entry");
    jmethodID getKeyMid = env->GetMethodID(entryCls, "getKey", "()Ljava/lang/Object;");
    jmethodID getValueMid = env->GetMethodID(entryCls, "getValue", "()Ljava/lang/Object;");

    while (env->CallBooleanMethod(iteratorObj, hasNextMid)) {
        jobject entryObj = env->CallObjectMethod(iteratorObj, nextMid);
        jobject keyObj = env->CallObjectMethod(entryObj, getKeyMid);
        jobject valObj = env->CallObjectMethod(entryObj, getValueMid);

        const char* keyChars = env->GetStringUTFChars((jstring)keyObj, nullptr);
        std::string key(keyChars);
        env->ReleaseStringUTFChars((jstring)keyObj, keyChars);

        ordered_json value = javaObjectToJsonValue(env, valObj);
        LOGD("Key=%s, Value=%s", key.c_str(), value.dump().c_str());
        cppMap[key] = value;

        env->DeleteLocalRef(entryObj);
        env->DeleteLocalRef(keyObj);
        if (valObj) env->DeleteLocalRef(valObj);
    }

    env->DeleteLocalRef(entryCls);
    env->DeleteLocalRef(iteratorCls);
    env->DeleteLocalRef(iteratorObj);
    env->DeleteLocalRef(setCls);
    env->DeleteLocalRef(entrySetObj);
    env->DeleteLocalRef(mapCls);

    return cppMap;
}