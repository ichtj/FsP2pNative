// ========== BaseDataConverter.h ==========
#ifndef BASE_DATA_CONVERTER_H
#define BASE_DATA_CONVERTER_H

#include <jni.h>
#include <string>
#include <map>
#include <memory>
#include <android/log.h>
#include "BaseData.h"
#include "Logger.h"

#include <nlohmann/json.hpp>
using namespace nlohmann;

// 类型标记结构（用于记录void*的实际类型）
enum class DataType {
    INT,
    LONG,
    DOUBLE,
    BOOL,
    STRING,
    UNKNOWN
};

struct TypedData {
    DataType type;
    std::shared_ptr<void> value;

    template<typename T>
    static TypedData create(const T& val);
};

class BaseDataConverter {
public:
    /**
     * C++ BaseData 转换为 Java BaseData对象
     * @param env JNI环境
     * @param cppData C++的BaseData对象
     * @return Java BaseData对象（jobject）
     */
    static jobject toJavaObject(JNIEnv* env, const BaseData& cppData,jclass baseDataClass);

    // 创建Java HashMap
    /**
     * Java BaseData对象 转换为 C++ BaseData
     * @param env JNI环境
     * @param jBaseData Java BaseData对象
     * @return C++的BaseData对象
     */
    static BaseData fromJavaObject(JNIEnv* env, jobject jBaseData);

private:

    // 解析Java HashMap
    static std::map<std::string, std::shared_ptr<void>> parseJavaHashMap(JNIEnv* env,
                                                                         jobject jHashMap);

    // void指针转Java对象
    static jobject voidPtrToJavaObject(JNIEnv* env, ordered_json ptr);

    // Java对象转void指针
    static std::shared_ptr<void> javaObjectToVoidPtr(JNIEnv* env, jobject jValue);
};

#endif // BASE_DATA_CONVERTER_H