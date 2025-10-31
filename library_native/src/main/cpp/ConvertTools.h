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

jobject convertInfomationToJava(std::vector<fs::p2p::InfomationManifest> list, JNIEnv* env) {
    jclass infoClass = env->FindClass("com/library/natives/Infomation");
    jmethodID constructor = env->GetMethodID(infoClass, "<init>",
                                            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lcom/library/natives/Type;I)V");

    jclass enumCls = env->FindClass("com/library/natives/Type");
    jmethodID enumValuesMid = env->GetStaticMethodID(enumCls, "values", "()[Lcom/library/natives/Type;");
    jobjectArray enumValues = (jobjectArray)env->CallStaticObjectMethod(enumCls, enumValuesMid);

    jclass listClass = env->FindClass("java/util/ArrayList");
    jmethodID listConstructor = env->GetMethodID(listClass, "<init>", "()V");
    jobject javaList = env->NewObject(listClass, listConstructor);
    jmethodID addMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    for (const auto& info : list) {
        jobject jTypeObject = env->GetObjectArrayElement(enumValues, info.type);

        jstring jsn = env->NewStringUTF(info.sn.c_str());
        jstring jproductId = env->NewStringUTF(info.product_id.c_str());
        jstring jname = env->NewStringUTF(info.name.c_str());
        jstring jmodel = env->NewStringUTF(info.model.c_str());

        jobject infoObject = env->NewObject(infoClass, constructor,
                                            jsn, jproductId, jname, jmodel,
                                            jTypeObject, static_cast<jint>(info.version));
        env->CallBooleanMethod(javaList, addMethod, infoObject);

        env->DeleteLocalRef(jsn);
        env->DeleteLocalRef(jproductId);
        env->DeleteLocalRef(jname);
        env->DeleteLocalRef(jmodel);
        env->DeleteLocalRef(jTypeObject);
        env->DeleteLocalRef(infoObject);
    }

    env->DeleteLocalRef(enumValues);
    env->DeleteLocalRef(enumCls);
    env->DeleteLocalRef(infoClass);
    return javaList;
}

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


static std::string getValue(const std::map<std::string, ordered_json> &m,
                            const std::string &key,
                            const std::string &defaultValue = "") {
    auto it = m.find(key);
    if (it != m.end()) {
        try {
            const ordered_json &v = it->second;

            switch (v.type()) {
                case ordered_json::value_t::string:
                    return v.get<std::string>();
                case ordered_json::value_t::number_integer:
                case ordered_json::value_t::number_unsigned:
                case ordered_json::value_t::number_float:
                case ordered_json::value_t::boolean:
                    return v.dump(); // 转成字符串
                case ordered_json::value_t::null:
                    return "null";
                default:
                    return v.dump(); // 对象或数组也用 dump 转字符串
            }
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}



static std::vector<std::string> splitJString(JNIEnv *env, jstring jname) {
    std::vector<std::string> result;

    if (jname == nullptr) return result;

    const char *cStr = env->GetStringUTFChars(jname, nullptr);
    if (cStr == nullptr) return result;

    std::string name(cStr);
    env->ReleaseStringUTFChars(jname, cStr);

    // 查找分隔符 '-'
    size_t pos = name.find('-');
    if (pos != std::string::npos) {
        result.push_back(name.substr(0, pos));         // 前半部分
        result.push_back(name.substr(pos + 1));        // 后半部分
    } else {
        // 没有 '-' 就整个放进去
        result.push_back(name);
    }

    return result;
}


static std::string jstrToStd(JNIEnv* env, jstring s) {
    if (!s) return std::string();
    const char* cs = env->GetStringUTFChars(s, nullptr);
    std::string out(cs);
    env->ReleaseStringUTFChars(s, cs);
    return out;
}
int convertToRequestAction(int action) {
    switch (action) {
        case 0x100:
            return fs::p2p::Request::Action::Action_Method;
        case 0x102:
            return fs::p2p::Request::Action::Action_Event;
        case 0x104:
            return fs::p2p::Request::Action::Action_Write;
        case 0x105:
            return fs::p2p::Request::Action::Action_Read;
        case 0x106:
            return fs::p2p::Request::Action::Action_Broadcast;
        case 0x101:
            return fs::p2p::Request::Action::Action_Notify;
        default:
            return action;
    }
}
int convertToResponseAction(int action){
    switch (action) {
        case 0x100:
            return fs::p2p::Response::Action::Action_Method;
        case 0x105:
            return fs::p2p::Response::Action::Action_Read;
        case 0x104:
            return fs::p2p::Response::Action::Action_Write;
        case 0x102:
            return fs::p2p::Response::Action::Action_Event;
        default:
            return action;
    }
};


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

inline void deleteLocalRefs(JNIEnv* env) {}
template<typename T, typename... Args>
inline void deleteLocalRefs(JNIEnv* env, T first, Args... rest) {
    if (first && env) env->DeleteLocalRef(first);
    deleteLocalRefs(env, rest...);
}

fs::p2p::InfomationManifest convertToCppInfomation(JNIEnv* env, jobject information) {
    jclass manifestCls = env->GetObjectClass(information);
    jmethodID mid_getSn = env->GetMethodID(manifestCls, "getSn", "()Ljava/lang/String;");
    jmethodID mid_getProductId = env->GetMethodID(manifestCls, "getProductId", "()Ljava/lang/String;");
    jmethodID mid_getName = env->GetMethodID(manifestCls, "getName", "()Ljava/lang/String;");
    jmethodID mid_getModel = env->GetMethodID(manifestCls, "getModel", "()Ljava/lang/String;");
    jmethodID mid_getType = env->GetMethodID(manifestCls, "getType", "()Lcom/library/natives/Type;"); // 返回 Type 对象
    jmethodID mid_getVersion = env->GetMethodID(manifestCls, "getVersion", "()I");

//    jmethodID mid_Protocol = env->GetMethodID(xCoreBeanCls, "getProtocol", "()Ljava/lang/String;");

    jobject jTypeObject = mid_getType ? env->CallObjectMethod(information, mid_getType) : nullptr;

    jstring jsn = mid_getSn ? (jstring)env->CallObjectMethod(information, mid_getSn) : nullptr;
    jstring jproductId = mid_getProductId ? (jstring)env->CallObjectMethod(information, mid_getProductId) : nullptr;
    jstring jname = mid_getName ? (jstring)env->CallObjectMethod(information, mid_getName) : nullptr;
    jstring jmodel = mid_getModel ? (jstring)env->CallObjectMethod(information, mid_getModel) : nullptr;
    jint jtype = 0;
    if (jTypeObject) {
        jclass enumCls = env->FindClass("java/lang/Enum");
        jmethodID mid_ordinal = enumCls ? env->GetMethodID(enumCls, "ordinal", "()I") : nullptr;
        if (mid_ordinal) {
            jtype = env->CallIntMethod(jTypeObject, mid_ordinal);
        }
        if (enumCls) env->DeleteLocalRef(enumCls);
    }
    jint jversion = mid_getVersion ? env->CallIntMethod(information, mid_getVersion) : 0;

    fs::p2p::InfomationManifest manifest;
    manifest.sn = jstrToStd(env, jsn);
    manifest.product_id = jstrToStd(env, jproductId);
    manifest.name = jstrToStd(env, jname);
    manifest.model = jstrToStd(env, jmodel);
    manifest.type = static_cast<int>(jtype);
    manifest.version = static_cast<int>(jversion);

    deleteLocalRefs(env,jsn,jproductId,jname,jmodel,manifestCls,jTypeObject);
    return  manifest;
}

jobject convertToJavaInfomationList(JNIEnv *env, const std::vector<fs::p2p::InfomationManifest> &deviceList) {
    // 1. 获取 ArrayList 类和构造方法
    jclass arrayListClass = env->FindClass("java/util/ArrayList");
    jmethodID arrayListCtor = env->GetMethodID(arrayListClass, "<init>", "()V");
    jmethodID arrayListAdd  = env->GetMethodID(arrayListClass, "add", "(Ljava/lang/Object;)Z");

    // 创建一个新的 ArrayList 实例
    jobject javaList = env->NewObject(arrayListClass, arrayListCtor);

    // 2. 获取 com.library.natives.Infomation 类及其构造方法
    jclass infoClass = env->FindClass("com/library/natives/Infomation");
    jmethodID infoCtor = env->GetMethodID(
            infoClass,
            "<init>",
            "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lcom/library/natives/Type;I)V"
    );

    // 3. 获取 com.library.natives.Type 枚举类及其字段
    jclass typeClass = env->FindClass("com/library/natives/Type");
    jfieldID gatewayField = env->GetStaticFieldID(typeClass, "Gateway", "Lcom/library/natives/Type;");
    jfieldID serviceField = env->GetStaticFieldID(typeClass, "Service", "Lcom/library/natives/Type;");
    jfieldID unknownField = env->GetStaticFieldID(typeClass, "Unknown", "Lcom/library/natives/Type;");

    jobject typeGateway = env->GetStaticObjectField(typeClass, gatewayField);
    jobject typeService = env->GetStaticObjectField(typeClass, serviceField);
    jobject typeUnknown = env->GetStaticObjectField(typeClass, unknownField);

    // 4. 遍历 C++ 向量并逐个转换
    for (const auto &info : deviceList) {
        jstring jsn = env->NewStringUTF(info.sn.c_str());
        jstring jproductId = env->NewStringUTF(info.product_id.c_str());
        jstring jname = env->NewStringUTF(info.name.c_str());
        jstring jmodel = env->NewStringUTF(info.model.c_str());

        jobject jtype = nullptr;
        switch (info.type) {
            case fs::p2p::InfomationManifest::Gateway:
                jtype = typeGateway;
                break;
            case fs::p2p::InfomationManifest::Service:
                jtype = typeService;
                break;
            default:
                jtype = typeUnknown;
                break;
        }

        jobject infoObj = env->NewObject(infoClass, infoCtor,
                                         jsn, jproductId, jname, jmodel, jtype, info.version);

        env->CallBooleanMethod(javaList, arrayListAdd, infoObj);
        // 清理局部引用防止 JNI 局部表溢出
        deleteLocalRefs(env,jsn,jproductId,jname,jmodel,infoObj);
    }

    // 5. 返回结果
    return javaList;
}
