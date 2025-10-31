// iTools.cpp
#include "iTools.h"
#include <android/log.h>
#include "PutTypeTool.h"

#define LOG_TAG "iTools"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ================== InfomationManifest ==================

fs::p2p::InfomationManifest iTools::convertToCppInfomation(JNIEnv* env, jobject information) {
    fs::p2p::InfomationManifest manifest;
    if (!information) return manifest;

    jclass manifestCls = env->GetObjectClass(information);

    jmethodID mid_getSn        = env->GetMethodID(manifestCls, "getSn", "()Ljava/lang/String;");
    jmethodID mid_getProductId = env->GetMethodID(manifestCls, "getProductId", "()Ljava/lang/String;");
    jmethodID mid_getName      = env->GetMethodID(manifestCls, "getName", "()Ljava/lang/String;");
    jmethodID mid_getModel     = env->GetMethodID(manifestCls, "getModel", "()Ljava/lang/String;");
    jmethodID mid_getType      = env->GetMethodID(manifestCls, "getType", "()Lcom/library/natives/Type;");
    jmethodID mid_getVersion   = env->GetMethodID(manifestCls, "getVersion", "()I");

    jstring jsn        = mid_getSn        ? (jstring)env->CallObjectMethod(information, mid_getSn)        : nullptr;
    jstring jproductId = mid_getProductId ? (jstring)env->CallObjectMethod(information, mid_getProductId) : nullptr;
    jstring jname      = mid_getName      ? (jstring)env->CallObjectMethod(information, mid_getName)      : nullptr;
    jstring jmodel     = mid_getModel     ? (jstring)env->CallObjectMethod(information, mid_getModel)     : nullptr;
    jint    jversion   = mid_getVersion   ? env->CallIntMethod(information, mid_getVersion)               : 0;

    // Type → ordinal
    int jtype = 2; // 默认 Unknown
    if (mid_getType) {
        jobject jTypeObj = env->CallObjectMethod(information, mid_getType);
        if (jTypeObj) {
            jclass enumCls = env->FindClass("java/lang/Enum");
            jmethodID mid_ordinal = env->GetMethodID(enumCls, "ordinal", "()I");
            if (mid_ordinal) jtype = env->CallIntMethod(jTypeObj, mid_ordinal);
            deleteLocalRefs(env, jTypeObj, enumCls);
        }
    }

    manifest.sn         = jstrToStd(env, jsn);
    manifest.product_id = jstrToStd(env, jproductId);
    manifest.name       = jstrToStd(env, jname);
    manifest.model      = jstrToStd(env, jmodel);
    manifest.type       = jtype;
    manifest.version    = jversion;

    deleteLocalRefs(env, jsn, jproductId, jname, jmodel, manifestCls);
    return manifest;
}

jobject iTools::convertToJavaInfomationList(JNIEnv* env, const std::vector<fs::p2p::InfomationManifest>& deviceList) {
    jclass arrayListCls = env->FindClass("java/util/ArrayList");
    jmethodID ctor = env->GetMethodID(arrayListCls, "<init>", "()V");
    jmethodID add  = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");
    jobject javaList = env->NewObject(arrayListCls, ctor);
    if (!javaList) return nullptr;

    jclass infoCls = env->FindClass("com/library/natives/Infomation");
    jmethodID infoCtor = env->GetMethodID(infoCls, "<init>",
                                          "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lcom/library/natives/Type;I)V");

    jclass typeCls = env->FindClass("com/library/natives/Type");
    jfieldID fGateway = env->GetStaticFieldID(typeCls, "Gateway", "Lcom/library/natives/Type;");
    jfieldID fService = env->GetStaticFieldID(typeCls, "Service", "Lcom/library/natives/Type;");
    jfieldID fUnknown = env->GetStaticFieldID(typeCls, "Unknown", "Lcom/library/natives/Type;");

    jobject typeGateway = env->GetStaticObjectField(typeCls, fGateway);
    jobject typeService = env->GetStaticObjectField(typeCls, fService);
    jobject typeUnknown = env->GetStaticObjectField(typeCls, fUnknown);

    for (const auto& info : deviceList) {
        jstring jsn        = env->NewStringUTF(info.sn.c_str());
        jstring jproductId = env->NewStringUTF(info.product_id.c_str());
        jstring jname      = env->NewStringUTF(info.name.c_str());
        jstring jmodel     = env->NewStringUTF(info.model.c_str());

        jobject jtype = (info.type == 0) ? typeGateway :
                        (info.type == 1) ? typeService : typeUnknown;

        jobject infoObj = env->NewObject(infoCls, infoCtor, jsn, jproductId, jname, jmodel, jtype, info.version);
        env->CallBooleanMethod(javaList, add, infoObj);

        deleteLocalRefs(env, jsn, jproductId, jname, jmodel, infoObj);
    }

    deleteLocalRefs(env, arrayListCls, infoCls, typeCls, typeGateway, typeService, typeUnknown);
    return javaList;
}

// ================== JSON ↔ Java ==================

jobject iTools::jsonToJavaObjectValue(JNIEnv* env, const ordered_json& value) {
    if (value.is_null()) return nullptr;
    if (value.is_string()) return env->NewStringUTF(value.get<std::string>().c_str());
    if (value.is_boolean()) {
        jclass cls = env->FindClass("java/lang/Boolean");
        jmethodID mid = env->GetStaticMethodID(cls, "valueOf", "(Z)Ljava/lang/Boolean;");
        jobject obj = env->CallStaticObjectMethod(cls, mid, (jboolean)value.get<bool>());
        deleteLocalRefs(env, cls);
        return obj;
    }
    if (value.is_number_integer()) {
        jclass cls = env->FindClass("java/lang/Integer");
        jmethodID mid = env->GetStaticMethodID(cls, "valueOf", "(I)Ljava/lang/Integer;");
        jobject obj = env->CallStaticObjectMethod(cls, mid, (jint)value.get<int64_t>());
        deleteLocalRefs(env, cls);
        return obj;
    }
    if (value.is_number_float()) {
        jclass cls = env->FindClass("java/lang/Double");
        jmethodID mid = env->GetStaticMethodID(cls, "valueOf", "(D)Ljava/lang/Double;");
        jobject obj = env->CallStaticObjectMethod(cls, mid, (jdouble)value.get<double>());
        deleteLocalRefs(env, cls);
        return obj;
    }
    if (value.is_object()) {
        auto map = value.get<std::map<std::string, ordered_json>>();
        return cppMapToJavaMapValue(env, map);
    }
    if (value.is_array()) {
        jclass listCls = env->FindClass("java/util/ArrayList");
        jmethodID ctor = env->GetMethodID(listCls, "<init>", "()V");
        jmethodID add  = env->GetMethodID(listCls, "add", "(Ljava/lang/Object;)Z");
        jobject list = env->NewObject(listCls, ctor);
        for (const auto& e : value) {
            jobject elem = jsonToJavaObjectValue(env, e);
            env->CallBooleanMethod(list, add, elem);
            deleteLocalRefs(env, elem);
        }
        deleteLocalRefs(env, listCls);
        return list;
    }
    return env->NewStringUTF(value.dump().c_str());
}

jobject iTools::cppMapToJavaMapValue(JNIEnv* env, const std::map<std::string, ordered_json>& cppMap) {
    jclass mapCls = env->FindClass("java/util/HashMap");
    jmethodID init = env->GetMethodID(mapCls, "<init>", "()V");
    jmethodID put  = env->GetMethodID(mapCls, "put", "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    jobject hashMap = env->NewObject(mapCls, init);

    for (const auto& [k, v] : cppMap) {
        jstring jKey = env->NewStringUTF(k.c_str());
        jobject jVal = jsonToJavaObjectValue(env, v);
        env->CallObjectMethod(hashMap, put, jKey, jVal);
        deleteLocalRefs(env, jKey, jVal);
    }
    deleteLocalRefs(env, mapCls);
    return hashMap;
}

ordered_json iTools::javaObjectToJsonValue(JNIEnv* env, jobject jObj) {
    if (!jObj) return nullptr;

    jclass cls = env->GetObjectClass(jObj);

    // String
    jclass strCls = env->FindClass("java/lang/String");
    if (env->IsInstanceOf(jObj, strCls)) {
        const char* c = env->GetStringUTFChars((jstring)jObj, nullptr);
        std::string s(c ? c : "");
        env->ReleaseStringUTFChars((jstring)jObj, c);
        deleteLocalRefs(env, strCls, cls);
        return s;
    }

    // Boolean
    jclass boolCls = env->FindClass("java/lang/Boolean");
    if (env->IsInstanceOf(jObj, boolCls)) {
        jmethodID mid = env->GetMethodID(boolCls, "booleanValue", "()Z");
        bool val = env->CallBooleanMethod(jObj, mid);
        deleteLocalRefs(env, boolCls, cls);
        return val;
    }

    // Integer
    jclass intCls = env->FindClass("java/lang/Integer");
    if (env->IsInstanceOf(jObj, intCls)) {
        jmethodID mid = env->GetMethodID(intCls, "intValue", "()I");
        int val = env->CallIntMethod(jObj, mid);
        deleteLocalRefs(env, intCls, cls);
        return val;
    }

    // Double
    jclass dblCls = env->FindClass("java/lang/Double");
    if (env->IsInstanceOf(jObj, dblCls)) {
        jmethodID mid = env->GetMethodID(dblCls, "doubleValue", "()D");
        double val = env->CallDoubleMethod(jObj, mid);
        deleteLocalRefs(env, dblCls, cls);
        return val;
    }

    // Map
    jclass mapCls = env->FindClass("java/util/Map");
    if (env->IsInstanceOf(jObj, mapCls)) {
        auto m = javaMapToCppMapValue(env, jObj);
        deleteLocalRefs(env, mapCls, cls);
        return ordered_json(m);
    }

    // List
    jclass listCls = env->FindClass("java/util/List");
    if (env->IsInstanceOf(jObj, listCls)) {
        jmethodID sizeMid = env->GetMethodID(listCls, "size", "()I");
        jmethodID getMid  = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");
        int size = env->CallIntMethod(jObj, sizeMid);
        ordered_json arr = ordered_json::array();
        for (int i = 0; i < size; ++i) {
            jobject elem = env->CallObjectMethod(jObj, getMid, i);
            arr.push_back(javaObjectToJsonValue(env, elem));
            deleteLocalRefs(env, elem);
        }
        deleteLocalRefs(env, listCls, cls);
        return arr;
    }

    // toString()
    jmethodID toStr = env->GetMethodID(cls, "toString", "()Ljava/lang/String;");
    jstring jStr = (jstring)env->CallObjectMethod(jObj, toStr);
    std::string s = jstrToStd(env, jStr);
    deleteLocalRefs(env, jStr, cls);
    return s;
}

std::map<std::string, ordered_json> iTools::javaMapToCppMapValue(JNIEnv* env, jobject jMap) {
    std::map<std::string, ordered_json> cppMap;
    if (!jMap) return cppMap;

    jclass mapCls = env->FindClass("java/util/Map");
    jmethodID entrySet = env->GetMethodID(mapCls, "entrySet", "()Ljava/util/Set;");
    jobject setObj = env->CallObjectMethod(jMap, entrySet);

    jclass setCls = env->FindClass("java/util/Set");
    jmethodID iterMid = env->GetMethodID(setCls, "iterator", "()Ljava/util/Iterator;");
    jobject iterObj = env->CallObjectMethod(setObj, iterMid);

    jclass iterCls = env->FindClass("java/util/Iterator");
    jmethodID hasNext = env->GetMethodID(iterCls, "hasNext", "()Z");
    jmethodID next = env->GetMethodID(iterCls, "next", "()Ljava/lang/Object;");

    jclass entryCls = env->FindClass("java/util/Map$Entry");
    jmethodID getKey = env->GetMethodID(entryCls, "getKey", "()Ljava/lang/Object;");
    jmethodID getVal = env->GetMethodID(entryCls, "getValue", "()Ljava/lang/Object;");

    while (env->CallBooleanMethod(iterObj, hasNext)) {
        jobject entry = env->CallObjectMethod(iterObj, next);
        jobject keyObj = env->CallObjectMethod(entry, getKey);
        jobject valObj = env->CallObjectMethod(entry, getVal);

        std::string key = jstrToStd(env, (jstring)keyObj);
        ordered_json val = javaObjectToJsonValue(env, valObj);
        cppMap[key] = val;

        deleteLocalRefs(env, entry, keyObj, valObj);
    }

    deleteLocalRefs(env, mapCls, setCls, iterCls, entryCls, setObj, iterObj);
    return cppMap;
}

// ================== 工具函数 ==================

void iTools::fillJsonMap(JNIEnv* env, jobject jparams, std::map<std::string, ordered_json>& targetMap) {
    if (!jparams) return;
    auto m = javaMapToCppMapValue(env, jparams);
    targetMap = std::move(m);
}

std::string iTools::getValue(const std::map<std::string, ordered_json>& m, const std::string& key, const std::string& defaultValue) {
    auto it = m.find(key);
    if (it == m.end()) return defaultValue;
    try {
        const auto& v = it->second;
        if (v.is_string()) return v.get<std::string>();
        if (v.is_number() || v.is_boolean()) return v.dump();
        if (v.is_null()) return "null";
        return v.dump();
    } catch (...) { return defaultValue; }
}

std::vector<std::string> iTools::splitJString(JNIEnv* env, jstring jname) {
    std::vector<std::string> result;
    if (!jname) return result;
    const char* c = env->GetStringUTFChars(jname, nullptr);
    if (!c) return result;
    std::string s(c);
    env->ReleaseStringUTFChars(jname, c);

    size_t pos = s.find('-');
    if (pos != std::string::npos) {
        result.push_back(s.substr(0, pos));
        result.push_back(s.substr(pos + 1));
    } else {
        result.push_back(s);
    }
    return result;
}

std::string iTools::jstrToStd(JNIEnv* env, jstring s) {
    if (!s) return "";
    const char* c = env->GetStringUTFChars(s, nullptr);
    std::string out(c ? c : "");
    env->ReleaseStringUTFChars(s, c);
    return out;
}

int iTools::convertToRequestAction(JNIEnv* env,int action) {
    if (action==PutTypeTool::METHOD(env)){
        return fs::p2p::Request::Action::Action_Method;
    }else if(action==PutTypeTool::EVENT(env)){
        return fs::p2p::Request::Action::Action_Event;
    }else if(action==PutTypeTool::SETPERTIES(env)){
        return fs::p2p::Request::Action::Action_Write;
    }else if(action==PutTypeTool::GETPERTIES(env)){
        return fs::p2p::Request::Action::Action_Read;
    }else if(action==PutTypeTool::BROADCAST(env)){
        return fs::p2p::Request::Action::Action_Broadcast;
    }else if(action==PutTypeTool::UPLOAD(env)){
        return fs::p2p::Request::Action::Action_Notify;
    }else {
        return action;
    }
}

int iTools::convertToResponseAction(JNIEnv* env,int action) {
    if (action==PutTypeTool::METHOD(env)){
        return fs::p2p::Response::Action::Action_Method;
    }else if(action==PutTypeTool::EVENT(env)){
        return fs::p2p::Response::Action::Action_Event;
    }else if(action==PutTypeTool::SETPERTIES(env)){
        return fs::p2p::Response::Action::Action_Write;
    }else if(action==PutTypeTool::GETPERTIES(env)){
        return fs::p2p::Response::Action::Action_Read;
    }else {
        return action;
    }
}

fs::p2p::Method iTools::convertToMethod(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string) {
    fs::p2p::Method m;
    m.name = jstrToStd(env, jname);
    m.reason_string = jstrToStd(env, jreason_string);
    m.reason_code = jreason_code;
    fillJsonMap(env, jparams, m.params);
    return m;
}

fs::p2p::Event iTools::convertToEvent(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string) {
    fs::p2p::Event e;
    e.name = jstrToStd(env, jname);
    e.reason_string = jstrToStd(env, jreason_string);
    e.reason_code = jreason_code;
    fillJsonMap(env, jparams, e.params);
    return e;
}

fs::p2p::Service iTools::convertToService(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string) {
    fs::p2p::Service s;
    s.name = jstrToStd(env, jname);
    s.reason_string = jstrToStd(env, jreason_string);
    s.reason_code = jreason_code;
    fillJsonMap(env, jparams, s.propertys);
    return s;
}