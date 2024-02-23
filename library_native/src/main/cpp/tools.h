#include <string>
#include "fs_p2p/MessagePipeline.h"

struct ConnParams {
    std::string sn;
    std::string name;
    std::string product_id;
    std::string json_protocol;
    std::string userName;
    std::string passWord;
    std::string host;
    int port;

    ConnParams(const std::string &sn, const std::string &name, const std::string &product_id,
               const std::string &json_protocol,
               const std::string &userName, const std::string &passWord, const std::string &host,
               int port)
            : sn(sn), name(name), product_id(product_id), json_protocol(json_protocol),
              userName(userName), passWord(passWord), host(host), port(port) {}

    // 默认构造函数
    ConnParams() : port(0) {}
};


#define  CLASS_REQUEST "com/library/natives/Request"
#define  CLASS_METHOD "com/library/natives/Method"
#define  CLASS_EVENT "com/library/natives/Event"
#define  CLASS_SERVICE "com/library/natives/Service"
#define  CLASS_PAYLOAD "com/library/natives/Payload"
#define  CLASS_DEVICE "com/library/natives/Device"
#define  CLASS_IOTCALLBACK "com/library/natives/PipelineCallback"

static JavaVM *gJavaVM;
static jobject callbackObj;
static jclass callbackClass;
static jclass requestClass;
static jclass methodClass;
static jclass eventClass;
static jclass payloadClass;
static jclass serviceClass;
static jclass deviceClass;
static jmethodID gMethodConnectStatus;
static jmethodID gMethodPrintLog;
static jmethodID gMethodRequest;
static ConnParams globalConnParams;


// 辅助函数：将 C++ 中的 Service 转换为 Java 对象
jobject convertToJavaServices(JNIEnv *env, std::list<fs::p2p::Service> services) {
    // 获取 List 类的引用
    jclass listClass = env->FindClass("java/util/ArrayList");
    // 获取 List 的构造函数
    jmethodID listConstructor = env->GetMethodID(listClass, "<init>", "()V");
    jobject javaList = env->NewObject(listClass, listConstructor);
    // 获取 List.add 方法的 ID
    jmethodID addMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    for (const auto &service: services) {

        // 获取 Service 类的构造函数
        jmethodID constructor = env->GetMethodID(serviceClass, "<init>",
                                                 "(Ljava/lang/String;Ljava/util/Map;ILjava/lang/String;)V");

        // 创建 Map 对象
        jclass mapClass = env->FindClass("java/util/HashMap");
        jmethodID mapConstructor = env->GetMethodID(mapClass, "<init>", "()V");
        jobject javaPropertysMap = env->NewObject(mapClass, mapConstructor);

        // 获取 Map.put 方法的 ID
        jmethodID putMethod = env->GetMethodID(mapClass, "put",
                                               "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        // 遍历 C++ 中的 propertys，将其转换为 Java 中的对象
        for (const auto &entry: service.propertys) {
            std::string val = entry.second.dump();
            // 将键值对放入 Map 中
            env->CallObjectMethod(javaPropertysMap, putMethod,
                                  env->NewStringUTF(entry.first.c_str()),
                                  env->NewStringUTF(val.c_str()));
        }

        // 创建 Service 对象并设置字段值
        jobject javaService = env->NewObject(serviceClass, constructor,
                                             env->NewStringUTF(service.name.c_str()),
                                             javaPropertysMap, service.reason_code,
                                             env->NewStringUTF(service.reason_string.c_str()));
        // 添加到 List 中
        env->CallBooleanMethod(javaList, addMethod, javaService);
    }
    return javaList;
}

// 辅助函数：将 C++ 中的 Service 转换为 Java 对象
jobject convertToJavaMethods(JNIEnv *env, std::list<fs::p2p::Method> methods) {
    // 获取 List 类的引用
    jclass listClass = env->FindClass("java/util/ArrayList");
    // 获取 List 的构造函数
    jmethodID listConstructor = env->GetMethodID(listClass, "<init>", "()V");
    jobject javaList = env->NewObject(listClass, listConstructor);
    // 获取 List.add 方法的 ID
    jmethodID addMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    for (const auto &method: methods) {
        // 将 C++ 中的 Service 对象转换为 Java 对象

        // 获取 Service 类的构造函数
        jmethodID constructor = env->GetMethodID(methodClass, "<init>",
                                                 "(Ljava/lang/String;Ljava/util/Map;ILjava/lang/String;)V");

        // 创建 Map 对象
        jclass mapClass = env->FindClass("java/util/HashMap");
        jmethodID mapConstructor = env->GetMethodID(mapClass, "<init>", "()V");
        jobject javaPropertysMap = env->NewObject(mapClass, mapConstructor);

        // 获取 Map.put 方法的 ID
        jmethodID putMethod = env->GetMethodID(mapClass, "put",
                                               "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        // 遍历 C++ 中的 propertys，将其转换为 Java 中的对象
        for (const auto &entry: method.params) {
            std::string val = entry.second.dump();
            // 将键值对放入 Map 中
            env->CallObjectMethod(javaPropertysMap, putMethod,
                                  env->NewStringUTF(entry.first.c_str()),
                                  env->NewStringUTF(val.c_str()));
        }

        // 创建 Service 对象并设置字段值
        jobject javaService = env->NewObject(methodClass, constructor,
                                             env->NewStringUTF(method.name.c_str()),
                                             javaPropertysMap, method.reason_code,
                                             env->NewStringUTF(method.reason_string.c_str()));
        // 添加到 List 中
        env->CallBooleanMethod(javaList, addMethod, javaService);
    }
    return javaList;
}


// 辅助函数：将 C++ 中的 Service 转换为 Java 对象
jobject convertToJavaEvents(JNIEnv *env, std::list<fs::p2p::Event> events) {
    // 获取 List 类的引用
    jclass listClass = env->FindClass("java/util/ArrayList");
    // 获取 List 的构造函数
    jmethodID listConstructor = env->GetMethodID(listClass, "<init>", "()V");
    jobject javaList = env->NewObject(listClass, listConstructor);
    // 获取 List.add 方法的 ID
    jmethodID addMethod = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    for (const auto &event: events) {
        // 获取 Service 类的构造函数
        jmethodID constructor = env->GetMethodID(eventClass, "<init>",
                                                 "(Ljava/lang/String;Ljava/util/Map;)V");
        // 创建 Map 对象
        jclass mapClass = env->FindClass("java/util/HashMap");
        jmethodID mapConstructor = env->GetMethodID(mapClass, "<init>", "()V");
        jobject javaPropertysMap = env->NewObject(mapClass, mapConstructor);

        // 获取 Map.put 方法的 ID
        jmethodID putMethod = env->GetMethodID(mapClass, "put",
                                               "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

        // 遍历 C++ 中的 propertys，将其转换为 Java 中的对象
        for (const auto &entry: event.params) {
            std::string val = entry.second.dump();
            // 将键值对放入 Map 中
            env->CallObjectMethod(javaPropertysMap, putMethod,
                                  env->NewStringUTF(entry.first.c_str()),
                                  env->NewStringUTF(val.c_str()));
        }

        // 创建 Service 对象并设置字段值
        jobject javaService = env->NewObject(eventClass, constructor,
                                             env->NewStringUTF(event.name.c_str()),
                                             javaPropertysMap);
        // 添加到 List 中
        env->CallBooleanMethod(javaList, addMethod, javaService);
    }
    return javaList;
}


// 在C++中将C++对象转换为Java对象
jobject convertRequestToJava(JNIEnv *env, const fs::p2p::Request &request) {
    jmethodID reqConstructor = env->GetMethodID(requestClass, "<init>",
                                                "(Ljava/lang/String;ILjava/lang/String;Ljava/lang/String;Lcom/library/natives/Payload;)V");

    jmethodID payloadConstructor = env->GetMethodID(payloadClass, "<init>", "()V");
    jobject javaPayload = env->NewObject(payloadClass, payloadConstructor);
    // 获取 Payload 对象的字段 ID
    jfieldID devicesField = env->GetFieldID(payloadClass, "devices", "Ljava/util/Map;");

    // 获取 Map 类的引用和构造函数
    jclass mapClass = env->FindClass("java/util/HashMap");
    jmethodID mapConstructor = env->GetMethodID(mapClass, "<init>", "()V");
    jobject javaDevicesMap = env->NewObject(mapClass, mapConstructor);

    for (const auto &entry: request.payload.devices) {
        // 创建 Device 对象

        jmethodID deviceConstructor = env->GetMethodID(deviceClass, "<init>",
                                                       "(Ljava/lang/String;Ljava/lang/String;Ljava/util/List;Ljava/util/List;Ljava/util/List;)V");

        // 获取 List 类的引用
        jclass listClass = env->FindClass("java/util/ArrayList");
        jobject javaDevice = env->NewObject(deviceClass, deviceConstructor,
                                            env->NewStringUTF(entry.second.sn.c_str()),
                                            env->NewStringUTF(entry.second.product_id.c_str()),
                                            convertToJavaServices(env, entry.second.services),
                                            convertToJavaMethods(env, entry.second.methods),
                                            convertToJavaEvents(env, entry.second.events));

        // 将 Device 对象放入 Map 中
        env->CallObjectMethod(javaDevicesMap, env->GetMethodID(mapClass, "put",
                                                               "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;"),
                              env->NewStringUTF(entry.first.c_str()), javaDevice);
    }
    //设置 Payload 对象的 devices 字段
    env->SetObjectField(javaPayload, devicesField, javaDevicesMap);


    const char *iidStr = request.iid.c_str();
    int actionStr = request.action;
    const char *ackStr = request.ack.c_str();
    const char *timeStr = request.time.c_str();
    jobject javaRequest = env->NewObject(requestClass,
                                         reqConstructor,
                                         env->NewStringUTF(iidStr),
                                         actionStr,
                                         env->NewStringUTF(ackStr),
                                         env->NewStringUTF(timeStr),
                                         javaPayload);
    return (*env).NewGlobalRef(javaRequest);
}

// 处理 params 字段
void processParams(JNIEnv *env, jobject paramsObj, std::map<std::string, ordered_json> &params) {
    // 获取 Map 类型的 Class 对象
    jclass mapClass = env->GetObjectClass(paramsObj);

    // 获取 entrySet 方法的 ID
    jmethodID entrySetMethod = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
    // 调用 entrySet 方法获取键值对集合
    jobject entrySet = env->CallObjectMethod(paramsObj, entrySetMethod);

    // 获取 Set 类型的 Class 对象
    jclass setClass = env->GetObjectClass(entrySet);

    // 获取 iterator 方法的 ID
    jmethodID iteratorMethod = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
    // 调用 iterator 方法获取迭代器
    jobject iterator = env->CallObjectMethod(entrySet, iteratorMethod);

    // 获取 Iterator 类型的 Class 对象
    jclass iteratorClass = env->GetObjectClass(iterator);

    // 获取 hasNext 和 next 方法的 ID
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // 获取 Map.Entry 类型的 Class 对象
    jclass entryClass = env->FindClass("java/util/Map$Entry");
    // 获取 getKey 和 getValue 方法的 ID
    jmethodID getKeyMethod = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
    jmethodID getValueMethod = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");

    // 循环遍历 Map 的键值对
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // 调用 next 方法获取下一个键值对
        jobject entry = env->CallObjectMethod(iterator, nextMethod);
        // 获取键和值
        jstring keyObj = (jstring) env->CallObjectMethod(entry, getKeyMethod);
        jstring valueObj = (jstring) env->CallObjectMethod(entry, getValueMethod);

        // 将键值对添加到 C++ 的 std::map 中
        const char *key = env->GetStringUTFChars(keyObj, nullptr);
        const char *value = env->GetStringUTFChars(valueObj, nullptr);

        // 添加到 params 中
        params[key] = value;

        // 释放字符串资源
        env->ReleaseStringUTFChars(keyObj, key);
        env->ReleaseStringUTFChars(valueObj, value);
    }
}


// 在C++中将C++对象转换为Java对象
fs::p2p::Payload convertJavaToPayload(JNIEnv *env, jobject &payload) {
    // 获取 Payload 类的引用
    jclass payloadClass = env->GetObjectClass(payload);

    // 获取 devices 字段的 ID
    jfieldID devicesFieldID = env->GetFieldID(payloadClass, "devices", "Ljava/util/Map;");

    // 获取 devices 字段的值
    jobject devicesObj = env->GetObjectField(payload, devicesFieldID);

    // 获取 Map 的类信息
    jclass mapClass = env->GetObjectClass(devicesObj);

    // 获取 Map 的 entrySet 方法 ID
    jmethodID entrySetMethodID = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");

    // 调用 entrySet 方法获取 Map 的 entrySet
    jobject entrySetObj = env->CallObjectMethod(devicesObj, entrySetMethodID);

    // 获取 Set 的类信息
    jclass setClass = env->GetObjectClass(entrySetObj);

    // 获取 Set 的 iterator 方法 ID
    jmethodID iteratorMethodID = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");

    // 调用 iterator 方法获取 Set 的 iterator
    jobject iteratorObj = env->CallObjectMethod(entrySetObj, iteratorMethodID);

    // 获取 Iterator 的类信息
    jclass iteratorClass = env->GetObjectClass(iteratorObj);

    // 获取 Iterator 的 hasNext 方法 ID 和 next 方法 ID
    jmethodID hasNextMethodID = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethodID = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");
    // 将 Device 对象添加到 Payload 中
    // 这里可以根据需要，将 Device 对象添加到 C++ 的 Payload 结构体中
    // 例如，将 Device 对象放入 C++ Payload 结构体的 devices 字段中
    fs::p2p::Payload getPayload;  // 请确保在适当的位置创建 Payload 对象

    // 遍历 Map 的 entrySet
    while (env->CallBooleanMethod(iteratorObj, hasNextMethodID)) {
        // 调用 next 方法获取 Map.Entry 对象
        jobject entryObj = env->CallObjectMethod(iteratorObj, nextMethodID);

        // 获取 Map.Entry 的类信息
        jclass entryClass = env->GetObjectClass(entryObj);

        // 获取 Map.Entry 的 getKey 和 getValue 方法 ID
        jmethodID getKeyMethodID = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
        jmethodID getValueMethodID = env->GetMethodID(entryClass, "getValue",
                                                      "()Ljava/lang/Object;");

        // 调用 getKey 和 getValue 方法获取 key 和 value 对象
        jstring keyObj = (jstring) env->CallObjectMethod(entryObj, getKeyMethodID);
        jobject deviceObj = env->CallObjectMethod(entryObj, getValueMethodID);

        // 将 key 和 device 对象转换为 C++ 字符串和 Device 对象
        const char *key = env->GetStringUTFChars(keyObj, nullptr);

        // 获取 Device 类的引用
        jclass deviceClass = env->GetObjectClass(deviceObj);

        // 获取 Device 类的字段 ID
        jfieldID snFieldID = env->GetFieldID(deviceClass, "sn", "Ljava/lang/String;");
        jfieldID productIdFieldID = env->GetFieldID(deviceClass, "product_id",
                                                    "Ljava/lang/String;");

        // 获取 Device 对象的字段值
        jstring snObj = (jstring) env->GetObjectField(deviceObj, snFieldID);
        jstring productIdObj = (jstring) env->GetObjectField(deviceObj, productIdFieldID);

        // 将 Java 字符串转换为 C++ 字符串
        const char *sn = env->GetStringUTFChars(snObj, nullptr);
        const char *productId = env->GetStringUTFChars(productIdObj, nullptr);

        // 创建 C++ 的 Device 对象并填充数据
        fs::p2p::Payload::Device device;
        device.sn = sn;
        device.product_id = productId;

        getPayload.devices[key] = device;
        // 释放本地引用
        env->ReleaseStringUTFChars(keyObj, key);
        env->DeleteLocalRef(keyObj);
        env->ReleaseStringUTFChars(snObj, sn);
        env->DeleteLocalRef(snObj);
        env->ReleaseStringUTFChars(productIdObj, productId);
        env->DeleteLocalRef(productIdObj);
        env->DeleteLocalRef(deviceObj);
        env->DeleteLocalRef(deviceClass);
        env->DeleteLocalRef(entryObj);
        env->DeleteLocalRef(entryClass);
    }

    // 释放本地引用
    env->DeleteLocalRef(payloadClass);
    env->DeleteLocalRef(devicesObj);
    env->DeleteLocalRef(mapClass);
    env->DeleteLocalRef(entrySetObj);
    env->DeleteLocalRef(setClass);
    env->DeleteLocalRef(iteratorObj);
    env->DeleteLocalRef(iteratorClass);
    return getPayload;
}

fs::p2p::Service
processServiceList(JNIEnv *env, jobject &servicesList, std::list<fs::p2p::Service> services) {
// 获取 List 类型的 Class 对象
    jclass listClass = env->GetObjectClass(servicesList);

    // 获取 iterator 方法的 ID
    jmethodID iteratorMethod = env->GetMethodID(listClass, "iterator", "()Ljava/util/Iterator;");
    // 调用 iterator 方法获取迭代器
    jobject iterator = env->CallObjectMethod(servicesList, iteratorMethod);

    // 获取 Iterator 类型的 Class 对象
    jclass iteratorClass = env->GetObjectClass(iterator);

    // 获取 hasNext 和 next 方法的 ID
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // 获取 Method 类型的 Class 对象
    jclass methodClass = env->FindClass("com/library/natives/Service");
    // 获取 Method 类的字段 ID
    jfieldID nameField = env->GetFieldID(methodClass, "name", "Ljava/lang/String;");
    jfieldID paramsField = env->GetFieldID(methodClass, "propertys", "Ljava/util/Map;");
    jfieldID reasonCodeField = env->GetFieldID(methodClass, "reason_code", "I");
    jfieldID reasonStringField = env->GetFieldID(methodClass, "reason_string",
                                                 "Ljava/lang/String;");

    // 循环遍历 List 的元素
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // 调用 next 方法获取下一个元素
        jobject methodObj = env->CallObjectMethod(iterator, nextMethod);

        // 创建 C++ 的 Method 对象
        fs::p2p::Service service;

        // 获取 name 字段
        jstring name = (jstring) env->GetObjectField(methodObj, nameField);
        service.name = env->GetStringUTFChars(name, nullptr);

        // 获取 params 字段
        jobject paramsObj = env->GetObjectField(methodObj, paramsField);
        processParams(env, paramsObj, service.propertys);

        // 获取 reason_code 字段
        service.reason_code = env->GetIntField(methodObj, reasonCodeField);

        // 获取 reason_string 字段
        jstring reasonString = (jstring) env->GetObjectField(methodObj, reasonStringField);
        service.reason_string = env->GetStringUTFChars(reasonString, nullptr);

        // 将 Method 对象添加到列表中
        services.push_back(service);

        // 释放字符串资源
        env->ReleaseStringUTFChars(name, service.name.c_str());
        env->ReleaseStringUTFChars(reasonString, service.reason_string.c_str());
    }
}

fs::p2p::Method
processMethodList(JNIEnv *env, jobject &methodsList, std::list<fs::p2p::Method> methods) {
    // 获取 List 类型的 Class 对象
    jclass listClass = env->GetObjectClass(methodsList);

    // 获取 iterator 方法的 ID
    jmethodID iteratorMethod = env->GetMethodID(listClass, "iterator", "()Ljava/util/Iterator;");
    // 调用 iterator 方法获取迭代器
    jobject iterator = env->CallObjectMethod(methodsList, iteratorMethod);

    // 获取 Iterator 类型的 Class 对象
    jclass iteratorClass = env->GetObjectClass(iterator);

    // 获取 hasNext 和 next 方法的 ID
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // 获取 Method 类型的 Class 对象
    jclass methodClass = env->FindClass("com/library/natives/Method");
    // 获取 Method 类的字段 ID
    jfieldID nameField = env->GetFieldID(methodClass, "name", "Ljava/lang/String;");
    jfieldID paramsField = env->GetFieldID(methodClass, "params", "Ljava/util/Map;");
    jfieldID reasonCodeField = env->GetFieldID(methodClass, "reason_code", "I");
    jfieldID reasonStringField = env->GetFieldID(methodClass, "reason_string",
                                                 "Ljava/lang/String;");

    // 循环遍历 List 的元素
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // 调用 next 方法获取下一个元素
        jobject methodObj = env->CallObjectMethod(iterator, nextMethod);

        // 创建 C++ 的 Method 对象
        fs::p2p::Method method;

        // 获取 name 字段
        jstring name = (jstring) env->GetObjectField(methodObj, nameField);
        method.name = env->GetStringUTFChars(name, nullptr);

        // 获取 params 字段
        jobject paramsObj = env->GetObjectField(methodObj, paramsField);
        processParams(env, paramsObj, method.params);

        // 获取 reason_code 字段
        method.reason_code = env->GetIntField(methodObj, reasonCodeField);

        // 获取 reason_string 字段
        jstring reasonString = (jstring) env->GetObjectField(methodObj, reasonStringField);
        method.reason_string = env->GetStringUTFChars(reasonString, nullptr);

        // 将 Method 对象添加到列表中
        methods.push_back(method);

        // 释放字符串资源
        env->ReleaseStringUTFChars(name, method.name.c_str());
        env->ReleaseStringUTFChars(reasonString, method.reason_string.c_str());
    }
}

fs::p2p::Event
processEventList(JNIEnv *env, jobject &eventsList, std::list<fs::p2p::Event> events) {
// 获取 List 类型的 Class 对象
    jclass listClass = env->GetObjectClass(eventsList);

    // 获取 iterator 方法的 ID
    jmethodID iteratorMethod = env->GetMethodID(listClass, "iterator", "()Ljava/util/Iterator;");
    // 调用 iterator 方法获取迭代器
    jobject iterator = env->CallObjectMethod(eventsList, iteratorMethod);

    // 获取 Iterator 类型的 Class 对象
    jclass iteratorClass = env->GetObjectClass(iterator);

    // 获取 hasNext 和 next 方法的 ID
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // 获取 Method 类型的 Class 对象
    jclass methodClass = env->FindClass("com/library/natives/Method");
    // 获取 Method 类的字段 ID
    jfieldID nameField = env->GetFieldID(methodClass, "name", "Ljava/lang/String;");
    jfieldID paramsField = env->GetFieldID(methodClass, "params", "Ljava/util/Map;");

    // 循环遍历 List 的元素
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // 调用 next 方法获取下一个元素
        jobject methodObj = env->CallObjectMethod(iterator, nextMethod);

        // 创建 C++ 的 Method 对象
        fs::p2p::Event method;

        // 获取 name 字段
        jstring name = (jstring) env->GetObjectField(methodObj, nameField);
        method.name = env->GetStringUTFChars(name, nullptr);

        // 获取 params 字段
        jobject paramsObj = env->GetObjectField(methodObj, paramsField);
        processParams(env, paramsObj, method.params);

        // 将 Method 对象添加到列表中
        events.push_back(method);

        // 释放字符串资源
        env->ReleaseStringUTFChars(name, method.name.c_str());
    }
}


// 在C++中将C++对象转换为Java对象
std::list<fs::p2p::Method> convertJavaToMethods(JNIEnv *env, jobject &methodsList) {
    // Get List class and its iterator method
    jclass listClass = env->GetObjectClass(methodsList);
    jmethodID iteratorMethod = env->GetMethodID(listClass, "iterator", "()Ljava/util/Iterator;");

    // Get Iterator class and its hasNext and next methods
    jclass iteratorClass = env->FindClass("java/util/Iterator");
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // Get Method class and its fields
    jclass methodClass = env->FindClass("com/library/natives/Method");
    jfieldID nameField = env->GetFieldID(methodClass, "name", "Ljava/lang/String;");
    jfieldID paramsField = env->GetFieldID(methodClass, "params", "Ljava/util/Map;");
    jfieldID reasonCodeField = env->GetFieldID(methodClass, "reason_code", "I");
    jfieldID reasonStringField = env->GetFieldID(methodClass, "reason_string",
                                                 "Ljava/lang/String;");
    // Create C++ std::list<Method>
    std::list<fs::p2p::Method> methods;
    // Call List's iterator method to get an iterator
    jobject iterator = env->CallObjectMethod(methodsList, iteratorMethod);

    // Loop through the list using the iterator
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // Call iterator's next method to get the next object in the list
        jobject methodObj = env->CallObjectMethod(iterator, nextMethod);

        // Create C++ Method object
        fs::p2p::Method method;

        // Get values from Java Method object
        jstring name = (jstring) env->GetObjectField(methodObj, nameField);
        method.name = env->GetStringUTFChars(name, nullptr);

        jint reasonCode = env->GetIntField(methodObj, reasonCodeField);
        method.reason_code = static_cast<int>(reasonCode);

        jstring reasonString = (jstring) env->GetObjectField(methodObj, reasonStringField);
        method.reason_string = env->GetStringUTFChars(reasonString, nullptr);

        // Get Map object from params field
        jobject paramsObj = env->GetObjectField(methodObj, paramsField);

        // Process Map entries
        processParams(env, paramsObj, method.params);

        // Add the Method object to the std::list
        methods.push_back(method);

        // Release local references
        //env->ReleaseStringUTFChars(name, method.name.c_str());
        //env->ReleaseStringUTFChars(reasonString, method.reason_string.c_str());
    }

    // Release local references
    env->DeleteLocalRef(iterator);
    return methods;
}



// 将 Java 的 String 转换为 C++ 的 std::string
std::string jstringToString(JNIEnv *env, jstring jstr) {
    const char *cstr = env->GetStringUTFChars(jstr, nullptr);
    std::string result(cstr);
    env->ReleaseStringUTFChars(jstr, cstr);
    return result;
}


// 在C++中将C++对象转换为Java对象
std::map<std::string, ordered_json> convertJavaMap(JNIEnv *env, jobject &maps) {
    std::map<std::string, ordered_json> cppMap;
    // 获取 Java Map 的类和方法信息
    jclass mapClass = env->GetObjectClass(maps);
    jmethodID entrySetMethod = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
    jclass setClass = env->FindClass("java/util/Set");
    jmethodID iteratorMethod = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
    jclass iteratorClass = env->FindClass("java/util/Iterator");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");
    jclass entryClass = env->FindClass("java/util/Map$Entry");
    jmethodID getKeyMethod = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
    jmethodID getValueMethod = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");

    // 获取 Map 的迭代器
    jobject entrySet = env->CallObjectMethod(maps, entrySetMethod);
    jobject iterator = env->CallObjectMethod(entrySet, iteratorMethod);

    // 遍历 Map，并将数据转换为 C++ Map
    while (env->CallBooleanMethod(iterator, env->GetMethodID(iteratorClass, "hasNext", "()Z"))) {
        jobject entry = env->CallObjectMethod(iterator, nextMethod);
        jstring key = static_cast<jstring>(env->CallObjectMethod(entry, getKeyMethod));
        jstring value = static_cast<jstring>(env->CallObjectMethod(entry, getValueMethod));

        // 将 Java String 转换为 C++ std::string
        std::string cppKey = jstringToString(env, key);
        std::string cppValue = jstringToString(env, value);

        // 创建 ordered_json 对象并存入 C++ Map
        ordered_json jsonValue = ordered_json::parse(cppValue);
        cppMap[cppKey] = jsonValue;

        // 释放局部引用
        env->DeleteLocalRef(entry);
        env->DeleteLocalRef(key);
        env->DeleteLocalRef(value);
    }

    // 释放局部引用
    env->DeleteLocalRef(iterator);
    env->DeleteLocalRef(entrySet);

    return cppMap;
}

fs::p2p::Service convertJavaToService(JNIEnv* env, jobject serviceObject) {
    fs::p2p::Service cppService;

    // Get class
    jclass serviceClass = env->GetObjectClass(serviceObject);

    // Get name field
    jfieldID nameFieldID = env->GetFieldID(serviceClass, "name", "Ljava/lang/String;");
    jstring nameJava = (jstring)env->GetObjectField(serviceObject, nameFieldID);
    const char* nameCStr = env->GetStringUTFChars(nameJava, nullptr);
    cppService.name = std::string(nameCStr);
    env->ReleaseStringUTFChars(nameJava, nameCStr);

    // Get propertys Map
    jfieldID propertysFieldID = env->GetFieldID(serviceClass, "propertys", "Ljava/util/Map;");
    jobject propertysMap = env->GetObjectField(serviceObject, propertysFieldID);
    // Access and iterate over the Map as shown in the previous example

    // Assuming you have a similar function to the one used for Event to populate the map
    // populateCppMapFromJavaMap(env, propertysMap, cppService.propertys);

    // Get reason_code
    jfieldID reasonCodeFieldID = env->GetFieldID(serviceClass, "reason_code", "I");
    jint reasonCodeJava = env->GetIntField(serviceObject, reasonCodeFieldID);
    cppService.reason_code = reasonCodeJava;

    // Get reason_string
    jfieldID reasonStringFieldID = env->GetFieldID(serviceClass, "reason_string", "Ljava/lang/String;");
    jstring reasonStringJava = (jstring)env->GetObjectField(serviceObject, reasonStringFieldID);
    const char* reasonStringCStr = env->GetStringUTFChars(reasonStringJava, nullptr);
    cppService.reason_string = std::string(reasonStringCStr);
    env->ReleaseStringUTFChars(reasonStringJava, reasonStringCStr);

    return cppService;
}
// 在C++中将C++对象转换为Java对象
std::list<fs::p2p::Service> convertJavaToServices(JNIEnv *env, jobject &servicesList) {
    // Get List class and its iterator method
    jclass listClass = env->GetObjectClass(servicesList);
    jmethodID iteratorMethod = env->GetMethodID(listClass, "iterator", "()Ljava/util/Iterator;");

    // Get Iterator class and its hasNext and next methods
    jclass iteratorClass = env->FindClass("java/util/Iterator");
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // Get Method class and its fields
    jclass methodClass = env->FindClass("com/library/natives/Service");
    jfieldID nameField = env->GetFieldID(methodClass, "name", "Ljava/lang/String;");
    jfieldID paramsField = env->GetFieldID(methodClass, "propertys", "Ljava/util/Map;");
    jfieldID reasonCodeField = env->GetFieldID(methodClass, "reason_code", "I");
    jfieldID reasonStringField = env->GetFieldID(methodClass, "reason_string",
                                                 "Ljava/lang/String;");
    // Create C++ std::list<Method>
    std::list<fs::p2p::Service> servies;

    // Call List's iterator method to get an iterator
    jobject iterator = env->CallObjectMethod(servicesList, iteratorMethod);

    // Loop through the list using the iterator
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // Call iterator's next method to get the next object in the list
        jobject methodObj = env->CallObjectMethod(iterator, nextMethod);

        // Create C++ Method object
        fs::p2p::Service service;

        // Get values from Java Method object
        jstring name = (jstring) env->GetObjectField(methodObj, nameField);
        service.name = env->GetStringUTFChars(name, nullptr);

        jint reasonCode = env->GetIntField(methodObj, reasonCodeField);
        service.reason_code = static_cast<int>(reasonCode);

        jstring reasonString = (jstring) env->GetObjectField(methodObj, reasonStringField);
        service.reason_string = env->GetStringUTFChars(reasonString, nullptr);

        // Get Map object from params field
        jobject paramsObj = env->GetObjectField(methodObj, paramsField);

        // Process Map entries
        processParams(env, paramsObj, service.propertys);

        // Add the Method object to the std::list
        servies.push_back(service);

//        // Release local references
//        env->ReleaseStringUTFChars(name, method.name.c_str());
//        env->ReleaseStringUTFChars(reasonString, method.reason_string.c_str());
    }

    // Use the 'methods' list as needed...

    // Release local references
    //env->DeleteLocalRef(iterator);
    return servies;
}
// Function to convert Java Event to C++ struct
fs::p2p::Event convertJavaEvent(JNIEnv* env, jobject eventObject) {
    fs::p2p::Event cppEvent;

    // Get class
    jclass eventClass = env->GetObjectClass(eventObject);

    // Get name field
    jfieldID nameFieldID = env->GetFieldID(eventClass, "name", "Ljava/lang/String;");
    jstring nameJava = (jstring)env->GetObjectField(eventObject, nameFieldID);
    const char* nameCStr = env->GetStringUTFChars(nameJava, nullptr);
    cppEvent.name = std::string(nameCStr);
    env->ReleaseStringUTFChars(nameJava, nameCStr);

    // Get params Map
    jfieldID paramsFieldID = env->GetFieldID(eventClass, "params", "Ljava/util/Map;");
    jobject paramsMap = env->GetObjectField(eventObject, paramsFieldID);

    // Access Map and iterate over it
    jclass mapClass = env->FindClass("java/util/Map");
    jmethodID entrySetMethod = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
    jobject entrySet = env->CallObjectMethod(paramsMap, entrySetMethod);

    jclass setClass = env->FindClass("java/util/Set");
    jmethodID iteratorMethod = env->GetMethodID(setClass, "iterator", "()Ljava/util/Iterator;");
    jobject iterator = env->CallObjectMethod(entrySet, iteratorMethod);

    jclass iteratorClass = env->FindClass("java/util/Iterator");
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    jclass entryClass = env->FindClass("java/util/Map$Entry");
    jmethodID getKeyMethod = env->GetMethodID(entryClass, "getKey", "()Ljava/lang/Object;");
    jmethodID getValueMethod = env->GetMethodID(entryClass, "getValue", "()Ljava/lang/Object;");

    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        jobject entry = env->CallObjectMethod(iterator, nextMethod);

        jstring keyJava = (jstring)env->CallObjectMethod(entry, getKeyMethod);
        jstring valueJava = (jstring)env->CallObjectMethod(entry, getValueMethod);

        const char* keyCStr = env->GetStringUTFChars(keyJava, nullptr);
        const char* valueCStr = env->GetStringUTFChars(valueJava, nullptr);

        cppEvent.params[std::string(keyCStr)] = std::string(valueCStr);

        env->ReleaseStringUTFChars(keyJava, keyCStr);
        env->ReleaseStringUTFChars(valueJava, valueCStr);
    }

    return cppEvent;
}


// 在C++中将C++对象转换为Java对象
std::list<fs::p2p::Event> convertJavaToEvents(JNIEnv *env, jobject &eventsList) {
    // Get List class and its iterator method
    jclass listClass = env->GetObjectClass(eventsList);
    jmethodID iteratorMethod = env->GetMethodID(listClass, "iterator", "()Ljava/util/Iterator;");

    // Get Iterator class and its hasNext and next methods
    jclass iteratorClass = env->FindClass("java/util/Iterator");
    jmethodID hasNextMethod = env->GetMethodID(iteratorClass, "hasNext", "()Z");
    jmethodID nextMethod = env->GetMethodID(iteratorClass, "next", "()Ljava/lang/Object;");

    // Get Method class and its fields
    jclass methodClass = env->FindClass("com/library/natives/Event");
    jfieldID nameField = env->GetFieldID(methodClass, "name", "Ljava/lang/String;");
    jfieldID paramsField = env->GetFieldID(methodClass, "params", "Ljava/util/Map;");

    // Create C++ std::list<Method>
    std::list<fs::p2p::Event> events;

    // Call List's iterator method to get an iterator
    jobject iterator = env->CallObjectMethod(eventsList, iteratorMethod);

    // Loop through the list using the iterator
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // Call iterator's next method to get the next object in the list
        jobject methodObj = env->CallObjectMethod(iterator, nextMethod);

        // Create C++ Method object
        fs::p2p::Event event;

        // Get values from Java Method object
        jstring name = (jstring) env->GetObjectField(methodObj, nameField);
        event.name = env->GetStringUTFChars(name, nullptr);


        // Get Map object from params field
        jobject paramsObj = env->GetObjectField(methodObj, paramsField);

        // Process Map entries
        processParams(env, paramsObj, event.params);

        // Add the Method object to the std::list
        events.push_back(event);

//        // Release local references
//        env->ReleaseStringUTFChars(name, event.name.c_str());
    }

    // Release local references
//    env->DeleteLocalRef(iterator);
    return events;
}


fs::p2p::Request getRequest(JNIEnv *env, jobject &request) {
    // 获取ConnParams类引用
    jclass connParamsClass = env->GetObjectClass(request);

    // 获取字段ID
    jfieldID iidField = env->GetFieldID(connParamsClass, "iid", "Ljava/lang/String;");
    jfieldID actionField = env->GetFieldID(connParamsClass, "action", "I");
    jfieldID ackField = env->GetFieldID(connParamsClass, "ack", "Ljava/lang/String;");
    jfieldID timeField = env->GetFieldID(connParamsClass, "time", "Ljava/lang/String;");
    jfieldID payloadField = env->GetFieldID(connParamsClass, "payload",
                                            "Lcom/library/natives/Payload;");

    // 获取字段值
    jstring iid = (jstring) env->GetObjectField(request, iidField);
    int action = (jint) env->GetIntField(request, actionField);
    jstring ack = (jstring) env->GetObjectField(request, ackField);
    jstring time = (jstring) env->GetObjectField(request, timeField);
    jobject payload = (jobject) env->GetObjectField(request, payloadField);

    // 将Java字符串转换为C++字符串
    const char *iidStr = env->GetStringUTFChars(iid, nullptr);
    const char *ackStr = env->GetStringUTFChars(ack, nullptr);
    const char *timeStr = env->GetStringUTFChars(time, nullptr);
    fs::p2p::Request iRequest;
    iRequest.iid = iidStr;
    iRequest.action = action;
    iRequest.ack = ackStr;
    iRequest.time = timeStr;
    iRequest.payload = convertJavaToPayload(env, payload);
    return iRequest;
}