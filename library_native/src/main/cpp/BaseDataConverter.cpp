#include "BaseDataConverter.h"

namespace {

jobject boxedInteger(JNIEnv* env, jint value) {
    jclass cls = env->FindClass("java/lang/Integer");
    if (!cls) return nullptr;
    jmethodID valueOf = env->GetStaticMethodID(cls, "valueOf", "(I)Ljava/lang/Integer;");
    jobject result = valueOf ? env->CallStaticObjectMethod(cls, valueOf, value) : nullptr;
    env->DeleteLocalRef(cls);
    return result;
}

jobject boxedLong(JNIEnv* env, jlong value) {
    jclass cls = env->FindClass("java/lang/Long");
    if (!cls) return nullptr;
    jmethodID valueOf = env->GetStaticMethodID(cls, "valueOf", "(J)Ljava/lang/Long;");
    jobject result = valueOf ? env->CallStaticObjectMethod(cls, valueOf, value) : nullptr;
    env->DeleteLocalRef(cls);
    return result;
}

jobject boxedDouble(JNIEnv* env, jdouble value) {
    jclass cls = env->FindClass("java/lang/Double");
    if (!cls) return nullptr;
    jmethodID valueOf = env->GetStaticMethodID(cls, "valueOf", "(D)Ljava/lang/Double;");
    jobject result = valueOf ? env->CallStaticObjectMethod(cls, valueOf, value) : nullptr;
    env->DeleteLocalRef(cls);
    return result;
}

jobject boxedBoolean(JNIEnv* env, jboolean value) {
    jclass cls = env->FindClass("java/lang/Boolean");
    if (!cls) return nullptr;
    jmethodID valueOf = env->GetStaticMethodID(cls, "valueOf", "(Z)Ljava/lang/Boolean;");
    jobject result = valueOf ? env->CallStaticObjectMethod(cls, valueOf, value) : nullptr;
    env->DeleteLocalRef(cls);
    return result;
}

jobject jsonToJavaObject(JNIEnv* env, const ordered_json& value);

jobject jsonArrayToJavaList(JNIEnv* env, const ordered_json& value) {
    jclass listClass = env->FindClass("java/util/ArrayList");
    if (!listClass) return nullptr;
    jmethodID constructor = env->GetMethodID(listClass, "<init>", "()V");
    jmethodID add = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");
    jobject list = constructor && add ? env->NewObject(listClass, constructor) : nullptr;
    if (list) {
        for (const auto& item : value) {
            jobject javaItem = jsonToJavaObject(env, item);
            env->CallBooleanMethod(list, add, javaItem);
            if (javaItem) env->DeleteLocalRef(javaItem);
        }
    }
    env->DeleteLocalRef(listClass);
    return list;
}

jobject jsonObjectToJavaMap(JNIEnv* env, const ordered_json& value) {
    jclass mapClass = env->FindClass("java/util/HashMap");
    if (!mapClass) return nullptr;
    jmethodID constructor = env->GetMethodID(mapClass, "<init>", "()V");
    jmethodID put = env->GetMethodID(
            mapClass, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jobject map = constructor && put ? env->NewObject(mapClass, constructor) : nullptr;
    if (map) {
        for (const auto& item : value.items()) {
            jstring key = env->NewStringUTF(item.key().c_str());
            jobject javaValue = jsonToJavaObject(env, item.value());
            env->CallObjectMethod(map, put, key, javaValue);
            if (key) env->DeleteLocalRef(key);
            if (javaValue) env->DeleteLocalRef(javaValue);
        }
    }
    env->DeleteLocalRef(mapClass);
    return map;
}

jobject jsonToJavaObject(JNIEnv* env, const ordered_json& value) {
    if (value.is_null()) return nullptr;
    if (value.is_string()) return env->NewStringUTF(value.get<std::string>().c_str());
    if (value.is_boolean()) return boxedBoolean(env, value.get<bool>());
    if (value.is_number_unsigned()) {
        return boxedLong(env, static_cast<jlong>(value.get<uint64_t>()));
    }
    if (value.is_number_integer()) {
        const int64_t number = value.get<int64_t>();
        if (number >= INT32_MIN && number <= INT32_MAX) {
            return boxedInteger(env, static_cast<jint>(number));
        }
        return boxedLong(env, static_cast<jlong>(number));
    }
    if (value.is_number_float()) return boxedDouble(env, value.get<double>());
    if (value.is_array()) return jsonArrayToJavaList(env, value);
    if (value.is_object()) return jsonObjectToJavaMap(env, value);
    return env->NewStringUTF(value.dump().c_str());
}

jobject cppMapToJavaMap(
        JNIEnv* env, const std::map<std::string, ordered_json>& values) {
    ordered_json object = ordered_json::object();
    for (const auto& entry : values) object[entry.first] = entry.second;
    return jsonObjectToJavaMap(env, object);
}

void clearException(JNIEnv* env, const char* operation) {
    if (!env->ExceptionCheck()) return;
    LOGE("BaseData conversion failed: %s", operation);
    env->ExceptionDescribe();
    env->ExceptionClear();
}

} // namespace

jobject BaseDataConverter::toJavaObject(
        JNIEnv* env, const BaseData& cppData, jclass baseDataClass) {
    if (!env || !baseDataClass) return nullptr;

    jmethodID constructor = env->GetMethodID(
            baseDataClass,
            "<init>",
            "(ILjava/lang/String;Ljava/lang/String;Ljava/util/Map;)V");
    if (!constructor) {
        clearException(env, "resolve BaseData constructor");
        return nullptr;
    }

    jstring iid = env->NewStringUTF(cppData.iid.c_str());
    jstring operation = env->NewStringUTF(cppData.operation.c_str());
    jobject maps = cppMapToJavaMap(env, cppData.maps);
    jobject result = nullptr;
    if (iid && operation && maps) {
        result = env->NewObject(
                baseDataClass, constructor, cppData.iPutType, iid, operation, maps);
    }

    if (iid) env->DeleteLocalRef(iid);
    if (operation) env->DeleteLocalRef(operation);
    if (maps) env->DeleteLocalRef(maps);
    clearException(env, "create BaseData");
    return result;
}
