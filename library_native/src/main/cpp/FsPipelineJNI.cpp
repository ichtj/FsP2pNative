#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <jni.h>
#include "tools.h"
#include "include/log.h"
#include <memory>
#include "fs_p2p/MessagePipeline.h"


extern "C" {
/**
 * 程序加载初始化 不要做耗时操作
 * @param jvm
 * @param reserved
 * @return
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    gJavaVM = jvm;
    JNIEnv *env;
    if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // 返回错误代码表示加载失败
    }
    callbackClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_IOTCALLBACK));
    requestClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_REQUEST));
    methodClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_METHOD));
    eventClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_EVENT));
    payloadClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_PAYLOAD));
    serviceClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_SERVICE));
    deviceClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_DEVICE));
    return JNI_VERSION_1_6;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_init(JNIEnv *env, jclass clazz,
                                                     jobject conn_params, jobject callback) {
    callbackObj = (*env).NewGlobalRef(callback);
    if (callback==NULL){
        return -1;
    }
    gMethodConnectStatus = (*env).GetMethodID(callbackClass, "connectStatus", "(Z)V");
    gMethodPrintLog = (*env).GetMethodID(callbackClass, "pipelineLog", "(ILjava/lang/String;)V");
    gMethodRequest = (*env).GetMethodID(callbackClass, "callback",
                                        "(Lcom/library/natives/Request;)V");
    // 获取ConnParams类引用
    jclass connParamsClass = env->GetObjectClass(conn_params);
    if (connParamsClass==NULL){
        return -1;
    }
    // 获取字段ID
    jfieldID snField = env->GetFieldID(connParamsClass, "sn", "Ljava/lang/String;");
    jfieldID nameField = env->GetFieldID(connParamsClass, "name", "Ljava/lang/String;");
    jfieldID modelField = env->GetFieldID(connParamsClass, "model", "Ljava/lang/String;");
    jfieldID typeField = env->GetFieldID(connParamsClass, "type", "Lcom/library/natives/Type;");
    jclass typeClass = env->FindClass("com/library/natives/Type"); // 替换为实际的包名
    jfieldID ordinalField = env->GetFieldID(typeClass, "ordinal", "I");
    jfieldID versionField = env->GetFieldID(connParamsClass, "version", "I");
    jfieldID productIdField = env->GetFieldID(connParamsClass, "product_id", "Ljava/lang/String;");
    jfieldID jsonProtocolField = env->GetFieldID(connParamsClass, "json_protocol",
                                                 "Ljava/lang/String;");
    jfieldID userNameField = env->GetFieldID(connParamsClass, "userName", "Ljava/lang/String;");
    jfieldID passWordField = env->GetFieldID(connParamsClass, "passWord", "Ljava/lang/String;");
    jfieldID hostField = env->GetFieldID(connParamsClass, "host", "Ljava/lang/String;");
    jfieldID portField = env->GetFieldID(connParamsClass, "port", "I");

    // 获取字段值
    jstring model = (jstring) env->GetObjectField(conn_params, modelField);
    jstring sn = (jstring) env->GetObjectField(conn_params, snField);
    jstring name = (jstring) env->GetObjectField(conn_params, nameField);
    jobject typeObj = env->GetObjectField(conn_params, typeField);
    int ordinalTypeValue = env->GetIntField(typeObj, ordinalField);

    int version = env->GetIntField(conn_params, versionField);
    jstring productId = (jstring) env->GetObjectField(conn_params, productIdField);
    jstring jsonProtocol = (jstring) env->GetObjectField(conn_params, jsonProtocolField);
    jstring userName = (jstring) env->GetObjectField(conn_params, userNameField);
    jstring passWord = (jstring) env->GetObjectField(conn_params, passWordField);
    jstring host = (jstring) env->GetObjectField(conn_params, hostField);
    jint port = env->GetIntField(conn_params, portField);

    LOGD("version>>%d,type>>%d",version,ordinalTypeValue);

    // 将Java字符串转换为C++字符串
    const char *snStr = env->GetStringUTFChars(sn, nullptr);
    const char *nameStr = env->GetStringUTFChars(name, nullptr);
    const char *modelStr = env->GetStringUTFChars(model, nullptr);
    const char *productIdStr = env->GetStringUTFChars(productId, nullptr);
    const char *jsonProtocolStr = env->GetStringUTFChars(jsonProtocol, nullptr);
    const char *userNameStr = env->GetStringUTFChars(userName, nullptr);
    const char *passWordStr = env->GetStringUTFChars(passWord, nullptr);
    const char *hostStr = env->GetStringUTFChars(host, nullptr);

    // 构建C++结构体
    globalConnParams = ConnParams(snStr, nameStr, productIdStr, jsonProtocolStr, userNameStr,
                                  passWordStr, hostStr, port);

    // 释放字符串资源
    env->ReleaseStringUTFChars(sn, snStr);
    env->ReleaseStringUTFChars(productId, productIdStr);
    env->ReleaseStringUTFChars(jsonProtocol, jsonProtocolStr);
    env->ReleaseStringUTFChars(userName, userNameStr);
    env->ReleaseStringUTFChars(passWord, passWordStr);
    env->ReleaseStringUTFChars(host, hostStr);

    fs::p2p::InfomationManifest manifest;
    manifest.sn = globalConnParams.sn;
    manifest.product_id = globalConnParams.product_id;
    manifest.name = globalConnParams.name;
    manifest.model = modelStr;
    manifest.type = ordinalTypeValue;
    manifest.version = version;
    s_mp.reset(new fs::p2p::MessagePipeline(manifest));

    s_mp->setConnectStateCallback([](bool state) {
        if (state) {
            s_mp->postStartup();
            s_mp->postHeartbeat();
        }

        JNIEnv *env;
        int attached = 0;
        if (gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            if (gJavaVM->AttachCurrentThread(&env, NULL) != 0) {
                // 处理无法附加线程的情况
                return;
            }
            attached = 1;
        }
        // 在这里使用env进行JNI操作
        (*env).CallVoidMethod(callbackObj, gMethodConnectStatus, state);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
        LOGI("setConnectStateCallback>>%d", state);
    });

    s_mp->setLogCallback([](int level, const std::string &str) {
        JNIEnv *env;
        int attached = 0;
        if (gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            if (gJavaVM->AttachCurrentThread(&env, NULL) != 0) {
                // 处理无法附加线程的情况
                return;
            }
            attached = 1;
        }
        jstring javaStr = env->NewStringUTF(str.c_str());
        (*env).CallVoidMethod(callbackObj, gMethodPrintLog, level, javaStr);
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
        LOGI("setLogCallback>>%s", str.c_str());
    });

    // 设备上线通知
    s_mp->setDeviceStartupCallback([](const fs::p2p::InfomationManifest &info) {
        // xcore是云边同步的模型名称，需要往这里注入物模型，使product_id和物模型绑定
        LOGD("setDeviceStartupCallback>>%s", info.model.c_str());
        if (info.model == "xcore") {
            // 注入设备物模型
            s_mp->postMethod({{info.sn, {
                                     info.sn,
                                     info.product_id,
                                     {}, // services
                                     {
                                             {
                                                     "iot_json_protocol",
                                                     {
                                                             {"app_sn",
                                                              s_mp->infomationManifest().sn},
                                                             {"product_id",
                                                              s_mp->infomationManifest().product_id},
                                                             {"json_protocol",
                                                              globalConnParams.json_protocol}
                                                     }
                                             }
                                     }, // methods
                                     {} // events
                             }}
                             }, // std::map<std::string, Payload::Device>
                             [](const fs::p2p::Response &, void *) {},
                             NULL, info.getSubscribeTopic());
        }
    });

    // 请求接入
    s_mp->setRequestCallback([](const fs::p2p::Request &req) {
        const auto &self_info = s_mp->infomationManifest();
        std::map<std::string, fs::p2p::Payload::Device> res_device_list;
        // 将 std::string 转换为 const char*
        LOGD("setRequestCallback>action>>%d", req.action);
        switch (req.action) {
            case fs::p2p::Request::Action_Method: {

                for (auto &item: req.payload.devices) {

                    fs::p2p::Payload::Device res_device;
                    res_device.sn = item.second.sn;
                    res_device.product_id = item.second.product_id;

                    for (auto &method: item.second.methods) {
                        fs::p2p::Method out;
                        out.name = method.name;
                        out.reason_code = 0;

                        // 填充 out.param

                        out.reason_string = "";
                        res_device.methods.push_back(out);
                    }

                    if (res_device.methods.size() > 0)
                        res_device_list[res_device.sn] = res_device;
                }
                break;
            }
            case fs::p2p::Request::Action_Write:
            case fs::p2p::Request::Action_Read: {
                bool is_read = req.action == fs::p2p::Request::Action_Read;

                for (auto &item: req.payload.devices) {

                    fs::p2p::Payload::Device res_device;
                    res_device.sn = item.second.sn;
                    res_device.product_id = item.second.product_id;

                    for (auto &service: item.second.services) {
                        fs::p2p::Service out;
                        out.name = service.name;
                        out.reason_code = -1;
                        out.reason_string = "NOT FOUND";

                        // 填充 out.param

                        res_device.services.push_back(out);
                    }

                    res_device_list[res_device.sn] = res_device;
                }
                break;
            }
        }
        if (!res_device_list.empty()) {
            s_mp->response(req, res_device_list);
        }
        JNIEnv *env;
        if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
            // 处理附加失败的情况
            return;
        }
        // 在这里使用env对象进行JNI操作
        env->CallVoidMethod(callbackObj, gMethodRequest, convertRequestToJava(env, req));
        gJavaVM->DetachCurrentThread();
    });
    return 0;
}

JNIEXPORT void JNICALL
Java_com_library_natives_FsPipelineJNI_connect(JNIEnv *env, jclass clz) {
    if (s_mp != NULL) {
        s_mp->open(globalConnParams.host, globalConnParams.port, globalConnParams.userName,
                   globalConnParams.passWord);
    }
}

JNIEXPORT void JNICALL
Java_com_library_natives_FsPipelineJNI_close(JNIEnv *env, jclass clz) {
    if (s_mp != NULL) {
        s_mp->close();
    }
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postStartup(JNIEnv *env, jclass clz) {
    return s_mp!=NULL?s_mp->postStartup():-1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postHeartbeat(JNIEnv *env, jclass clz) {
    return s_mp!=NULL?s_mp->postHeartbeat():-1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postBody(JNIEnv *env, jclass clz, jobject request,
                                                jobject deviceMap) {
    fs::p2p::Request convertRequest= getRequest(env,request);
    // 获取 Map 类型的 Class 对象
    jclass mapClass = env->GetObjectClass(deviceMap);

    // 获取 entrySet 方法的 ID
    jmethodID entrySetMethod = env->GetMethodID(mapClass, "entrySet", "()Ljava/util/Set;");
    // 调用 entrySet 方法获取键值对集合
    jobject entrySet = env->CallObjectMethod(deviceMap, entrySetMethod);

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

    // 获取 Device 类型的 Class 对象
    jclass deviceClass = env->FindClass("com/library/natives/Device");
    // 获取 Device 类的字段 ID
    //jfieldID snField = env->GetFieldID(deviceClass, "sn", "Ljava/lang/String;");
    jfieldID productIdField = env->GetFieldID(deviceClass, "product_id", "Ljava/lang/String;");
    jfieldID servicesField = env->GetFieldID(deviceClass, "services", "Ljava/util/List;");
    jfieldID methodsField = env->GetFieldID(deviceClass, "methods", "Ljava/util/List;");
    jfieldID eventsField = env->GetFieldID(deviceClass, "events", "Ljava/util/List;");

    // 创建 C++ 的 std::map 对象
    std::map<std::string, fs::p2p::Payload::Device> deviceMapCpp;

    // 循环遍历 Map 的键值对
    while (env->CallBooleanMethod(iterator, hasNextMethod)) {
        // 调用 next 方法获取下一个键值对
        jobject entry = env->CallObjectMethod(iterator, nextMethod);

        // 获取键和值
        jstring key = (jstring)env->CallObjectMethod(entry, getKeyMethod);
        jobject deviceObj = env->CallObjectMethod(entry, getValueMethod);

        // 创建 C++ 的 Device 对象
        fs::p2p::Payload::Device device;

        // 将 Java 字符串转换为 C++ 字符串
        const char* keyStr = env->GetStringUTFChars(key, nullptr);
        device.sn = keyStr;

        // 获取 Device 对象的其他字段值
        jstring productId = (jstring)env->GetObjectField(deviceObj, productIdField);
        device.product_id = env->GetStringUTFChars(productId, nullptr);

        // 处理 services 字段
        jobject servicesList = env->GetObjectField(deviceObj, servicesField);
        processServiceList(env, servicesList, device.services);

        // 处理 methods 字段
        jobject methodsList = env->GetObjectField(deviceObj, methodsField);
        processMethodList(env, methodsList, device.methods);

        // 处理 events 字段
        jobject eventsList = env->GetObjectField(deviceObj, eventsField);
        processEventList(env, eventsList, device.events);

        // 将设备添加到 map 中
        deviceMapCpp[device.sn] = device;

        // 释放字符串资源
        env->ReleaseStringUTFChars(key, keyStr);
        env->ReleaseStringUTFChars(productId, device.product_id.c_str());
    }
    return s_mp!=NULL?s_mp->response(convertRequest,deviceMapCpp):-1;
}


JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postMethod(JNIEnv *env, jclass clz, jobject request,
                                                  jobject methodsList) {
    fs::p2p::Request convertRequest= getRequest(env,request);

    std::map<std::string, fs::p2p::Payload::Device> res_device_list;
    // 获取 Map 类型的 Class 对象
    for (auto &item: convertRequest.payload.devices) {
        fs::p2p::Payload::Device res_device;
        res_device.sn = item.second.sn;
        res_device.product_id = item.second.product_id;
        res_device.methods=convertJavaToMethods(env,methodsList);
        if (res_device.methods.size() > 0){
            res_device_list[res_device.sn] = res_device;
        }
    }
    return s_mp!=NULL?s_mp->response(convertRequest,res_device_list):-1;
}


JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postService(JNIEnv *env, jclass clz, jobject request,
                                                   jobject servicesList) {
    fs::p2p::Request convertRequest= getRequest(env,request);

    std::map<std::string, fs::p2p::Payload::Device> res_device_list;
    // 获取 Map 类型的 Class 对象
    for (auto &item: convertRequest.payload.devices) {
        fs::p2p::Payload::Device res_device;
        res_device.sn = item.second.sn;
        res_device.product_id = item.second.product_id;
        res_device.services=convertJavaToServices(env,servicesList);
        if (res_device.methods.size() > 0){
            res_device_list[res_device.sn] = res_device;
        }
    }
    return s_mp!=NULL?s_mp->response(convertRequest,res_device_list):-1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postEvent(JNIEnv *env, jclass clz, jobject request,
                                                 jobject eventsList) {
    fs::p2p::Request convertRequest= getRequest(env,request);

    std::map<std::string, fs::p2p::Payload::Device> res_device_list;
    // 获取 Map 类型的 Class 对象
    for (auto &item: convertRequest.payload.devices) {
        fs::p2p::Payload::Device res_device;
        res_device.sn = item.second.sn;
        res_device.product_id = item.second.product_id;
        res_device.events=convertJavaToEvents(env,eventsList);
        if (res_device.methods.size() > 0){
            res_device_list[res_device.sn] = res_device;
        }
    }
    return s_mp!=NULL?s_mp->response(convertRequest,res_device_list):-1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postNotify(JNIEnv *env, jclass clz, jobject request,
                                                  jobject list) {
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postRead(JNIEnv *env, jclass clz, jobject request,
                                                jobject list) {
    return -1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postWrite(JNIEnv *env, jclass clz, jobject request,
                                                 jobject list) {
    return -1;
}
}
