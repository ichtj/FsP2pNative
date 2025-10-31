// ========== BaseDataConverter.cpp ==========
#include "BaseDataConverter.h"
#include "PipelineCallback.h"
#include "BaseData.h"

// 递归转换 ordered_json 到 Java Object
jobject jsonToJavaObject(JNIEnv* env, const ordered_json& json) {
    if (json.is_null()) {
        return nullptr;
    }

    if (json.is_string()) {
        jstring jstr = env->NewStringUTF(json.get<std::string>().c_str());
        return jstr;
    }

    if (json.is_number_integer()) {
        jint value = static_cast<jint>(json.get<int64_t>());
        return env->NewObject(env->FindClass("java/lang/Integer"),
                              env->GetMethodID(env->FindClass("java/lang/Integer"), "<init>", "(I)V"), value);
    }

    if (json.is_number_unsigned()) {
        jlong value = static_cast<jlong>(json.get<uint64_t>());
        return env->NewObject(env->FindClass("java/lang/Long"),
                              env->GetMethodID(env->FindClass("java/lang/Long"), "<init>", "(J)V"), value);
    }

    if (json.is_number_float()) {
        jdouble value = json.get<double>();
        return env->NewObject(env->FindClass("java/lang/Double"),
                              env->GetMethodID(env->FindClass("java/lang/Double"), "<init>", "(D)V"), value);
    }

    if (json.is_boolean()) {
        jboolean value = json.get<bool>();
        return env->NewObject(env->FindClass("java/lang/Boolean"),
                              env->GetMethodID(env->FindClass("java/lang/Boolean"), "<init>", "(Z)V"), value);
    }

    if (json.is_array()) {
        jclass arrayListClass = env->FindClass("java/util/ArrayList");
        jmethodID arrayListConstructor = env->GetMethodID(arrayListClass, "<init>", "()V");
        jobject arrayList = env->NewObject(arrayListClass, arrayListConstructor);
        jmethodID addMethod = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

        for (const auto& item : json) {
            jobject javaItem = jsonToJavaObject(env, item);
            env->CallBooleanMethod(arrayList, addMethod, javaItem);
        }
        return arrayList;
    }

    if (json.is_object()) {
        jclass hashMapClass = env->FindClass("java/util/HashMap");
        jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
        jobject hashMap = env->NewObject(hashMapClass, hashMapConstructor);
        jmethodID putMethod = env->GetMethodID(hashMapClass, "put",
                                               "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        for (const auto& [key, value] : json.items()) {
            jstring jkey = env->NewStringUTF(key.c_str());
            jobject jvalue = jsonToJavaObject(env, value);
            env->CallObjectMethod(hashMap, putMethod, jkey, jvalue);
        }
        return hashMap;
    }

    return nullptr;
}

// 将 std::map<std::string, ordered_json> 转换为 Java Map<String, Object>
jobject cppMapToJavaMap(JNIEnv* env, const std::map<std::string, ordered_json>& cppMaps) {
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID hashMapConstructor = env->GetMethodID(hashMapClass, "<init>", "()V");
    jobject javaMap = env->NewObject(hashMapClass, hashMapConstructor);
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put",
                                           "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    for (const auto& [key, value] : cppMaps) {
        jstring jkey = env->NewStringUTF(key.c_str());
        jobject jvalue = jsonToJavaObject(env, value);
        env->CallObjectMethod(javaMap, putMethod, jkey, jvalue);
    }

    return javaMap;
}


// 将C++ BaseData转换为Java BaseData对象
jobject BaseDataConverter::toJavaObject(JNIEnv* env, const BaseData& cppData,jclass baseDataClass) {
    if (env == nullptr) {
        LOGE("JNIEnv为空");
        return nullptr;
    }

    // 1. 查找Java BaseData类（修改为你的实际包名）
    if (baseDataClass == nullptr) {
        LOGE("找不到BaseData类");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    // 2. 转换字符串字段
    jstring jIid = env->NewStringUTF(cppData.iid.c_str());
    jstring jOperation = env->NewStringUTF(cppData.operation.c_str());

    if (jIid == nullptr || jOperation == nullptr) {
        LOGE("创建字符串失败");
        if (jIid) env->DeleteLocalRef(jIid);
        if (jOperation) env->DeleteLocalRef(jOperation);
        env->DeleteLocalRef(baseDataClass);
        return nullptr;
    }

    // 3. 创建Java HashMap存储maps
    jobject jMaps = cppMapToJavaMap(env, cppData.maps);
    if (jMaps == nullptr) {
        LOGE("创建HashMap失败");
        env->DeleteLocalRef(jIid);
        env->DeleteLocalRef(jOperation);
        env->DeleteLocalRef(baseDataClass);
        return nullptr;
    }

    // 4. 获取构造函数
    // 签名: (ILjava/lang/String;Ljava/lang/String;Ljava/util/Map;)V
    jmethodID constructor = env->GetMethodID(baseDataClass, "<init>",
                                             "(ILjava/lang/String;Ljava/lang/String;Ljava/util/Map;)V");

    if (constructor == nullptr) {
        LOGE("找不到BaseData构造函数");
        env->ExceptionDescribe();
        env->ExceptionClear();
        env->DeleteLocalRef(jIid);
        env->DeleteLocalRef(jOperation);
        env->DeleteLocalRef(jMaps);
        env->DeleteLocalRef(baseDataClass);
        return nullptr;
    }

    // 5. 创建Java BaseData对象
    jobject jBaseData = env->NewObject(baseDataClass, constructor,
                                       cppData.iPutType, jIid, jOperation, jMaps);

    // 6. 清理局部引用
    env->DeleteLocalRef(jIid);
    env->DeleteLocalRef(jOperation);
    env->DeleteLocalRef(jMaps);
//    env->DeleteLocalRef(baseDataClass);

    if (jBaseData == nullptr) {
        LOGE("创建Java BaseData对象失败");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return nullptr;
    }

    LOGD("成功转换为Java BaseData对象 (iPutType=%d, iid=%s)",
         cppData.iPutType, cppData.iid.c_str());

    return jBaseData;
}
