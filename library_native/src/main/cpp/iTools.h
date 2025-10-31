// iTools.h
#pragma once

#include <jni.h>
#include <vector>
#include <string>
#include <map>
#include "nlohmann/json.hpp"
#include "fs_p2p/Packetizer.h"

using ordered_json = nlohmann::ordered_json;

/**
 * @brief JNI 工具类：C++ ↔ Java 数据转换
 */
class iTools {
public:
    // ---------- InfomationManifest ----------
    static fs::p2p::InfomationManifest convertToCppInfomation(JNIEnv* env, jobject information);
    static jobject convertToJavaInfomationList(JNIEnv* env, const std::vector<fs::p2p::InfomationManifest>& deviceList);

    // ---------- JSON ↔ Java Object ----------
    static jobject jsonToJavaObjectValue(JNIEnv* env, const ordered_json& value);
    static jobject cppMapToJavaMapValue(JNIEnv* env, const std::map<std::string, ordered_json>& cppMap);
    static ordered_json javaObjectToJsonValue(JNIEnv* env, jobject jObj);
    static std::map<std::string, ordered_json> javaMapToCppMapValue(JNIEnv* env, jobject jMap);

    // ---------- 工具函数 ----------
    static void fillJsonMap(JNIEnv* env, jobject jparams, std::map<std::string, ordered_json>& targetMap);
    static std::string getValue(const std::map<std::string, ordered_json>& m, const std::string& key, const std::string& defaultValue = "");
    static std::vector<std::string> splitJString(JNIEnv* env, jstring jname);
    static std::string jstrToStd(JNIEnv* env, jstring s);

    // ---------- Action 转换 ----------
    static int convertToRequestAction(JNIEnv* env,int action);
    static int convertToResponseAction(JNIEnv* env,int action);

    // ---------- Method / Event / Service ----------
    static fs::p2p::Method convertToMethod(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string);
    static fs::p2p::Event convertToEvent(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string);
    static fs::p2p::Service convertToService(JNIEnv* env, jstring jname, jobject jparams, jint jreason_code, jstring jreason_string);


    // 局部引用自动释放
    inline static void deleteLocalRefs(JNIEnv* env) {}
    template<typename T, typename... Args>
    inline static void deleteLocalRefs(JNIEnv* env, T first, Args... rest) {
        if (first && env) env->DeleteLocalRef(first);
        deleteLocalRefs(env, rest...);
    }

private:
};