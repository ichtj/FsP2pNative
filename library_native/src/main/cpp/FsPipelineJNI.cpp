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
#include "Logger.h"
#include <memory>
#include "fs_p2p/MessagePipeline.h"

static std::shared_ptr<fs::p2p::MessagePipeline> s_mp;
static std::map<std::string, fs::p2p::InfomationManifest> devList;

extern "C" {
/**
 * 程序加载初始化 不要做耗时操作
 * @param jvm
 * @param reserved
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    gJavaVM = jvm;
    JNIEnv *env;
    if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR; // 返回错误代码表示加载失败
    }
    testFunction();
    callbackClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_IOTCALLBACK));
    requestClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_REQUEST));
    responseClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_RESPONSE));
    actionCls = (jclass) env->NewGlobalRef(env->FindClass(CLASS_ACTION));
    methodClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_METHOD));
    eventClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_EVENT));
    payloadClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_PAYLOAD));
    serviceClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_SERVICE));
    deviceClass = (jclass) env->NewGlobalRef(env->FindClass(CLASS_DEVICE));
    gMethodConnectStatus = (*env).GetMethodID(callbackClass, "connectStatus", "(Z)V");
    gMethodPrintLog = (*env).GetMethodID(callbackClass, "pipelineLog", "(ILjava/lang/String;)V");
    gReceiveCallback = (*env).GetMethodID(callbackClass, "request",
                                          "(Lcom/library/natives/Request;)V");
    gPushCallback = (*env).GetMethodID(callbackClass, "response",
                                       "(Lcom/library/natives/Response;)V");
    gMErrCallback = (*env).GetMethodID(callbackClass, "errCallback",
                                       "(ILjava/lang/String;)V");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
Java_com_library_natives_FsPipelineJNI_logEnable(JNIEnv *env, jclass clz,
                                                 jboolean isEnable) {
    setLoggingEnabled(isEnable);
}

JNIEXPORT jboolean JNICALL
Java_com_library_natives_FsPipelineJNI_isLogEnable(JNIEnv *env, jclass clz) {
    return getLoggingEnabled();
}

JNIEXPORT jboolean JNICALL
Java_com_library_natives_FsPipelineJNI_getConnectState(JNIEnv *env, jclass clz) {
    return connected;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_init(JNIEnv *env, jclass clazz,
                                            jobject conn_params) {
    // 获取ConnParams类引用
    jclass connParamsClass = env->GetObjectClass(conn_params);
    if (connParamsClass == NULL) {
        return -1;
    }
    jfieldID devModelFieldID = env->GetFieldID(connParamsClass, "subDev",
                                               "Lcom/library/natives/SubDev;");
    // 获取devModel对象
    jobject devModelObj = env->GetObjectField(conn_params, devModelFieldID);
    jclass devModelClass = env->GetObjectClass(devModelObj);

    // 获取字段ID
    jfieldID snField = env->GetFieldID(devModelClass, "sn", "Ljava/lang/String;");
    jfieldID nameField = env->GetFieldID(devModelClass, "name", "Ljava/lang/String;");
    jfieldID modelField = env->GetFieldID(devModelClass, "model", "Ljava/lang/String;");
    jfieldID typeField = env->GetFieldID(devModelClass, "type", "Lcom/library/natives/Type;");
    jfieldID versionField = env->GetFieldID(devModelClass, "version", "I");
    jfieldID productIdField = env->GetFieldID(devModelClass, "product_id", "Ljava/lang/String;");
    jfieldID jsonProtocolField = env->GetFieldID(connParamsClass, "json_protocol",
                                                 "Ljava/lang/String;");
    jfieldID userNameField = env->GetFieldID(connParamsClass, "userName", "Ljava/lang/String;");
    jfieldID passWordField = env->GetFieldID(connParamsClass, "passWord", "Ljava/lang/String;");
    jfieldID hostField = env->GetFieldID(connParamsClass, "host", "Ljava/lang/String;");
    jfieldID portField = env->GetFieldID(connParamsClass, "port", "I");

    // 获取字段值
    jstring model = (jstring) env->GetObjectField(devModelObj, modelField);
    jstring sn = (jstring) env->GetObjectField(devModelObj, snField);
    jstring name = (jstring) env->GetObjectField(devModelObj, nameField);
    jobject typeObj = env->GetObjectField(devModelObj, typeField);

    int version = env->GetIntField(devModelObj, versionField);
    jclass typeClass = env->GetObjectClass(typeObj);
    jmethodID ordinalMethodID = env->GetMethodID(typeClass, "ordinal", "()I");
    jstring productId = (jstring) env->GetObjectField(devModelObj, productIdField);
    jstring jsonProtocol = (jstring) env->GetObjectField(conn_params, jsonProtocolField);
    jstring userName = (jstring) env->GetObjectField(conn_params, userNameField);
    jstring passWord = (jstring) env->GetObjectField(conn_params, passWordField);
    jstring host = (jstring) env->GetObjectField(conn_params, hostField);
    jint port = env->GetIntField(conn_params, portField);

    const char *snStr = env->GetStringUTFChars(sn, nullptr);
    const char *nameStr = env->GetStringUTFChars(name, nullptr);
    const char *modelStr = env->GetStringUTFChars(model, nullptr);
    const jint type = env->CallIntMethod(typeObj, ordinalMethodID);
    const char *productIdStr = env->GetStringUTFChars(productId, nullptr);
    const char *jsonProtocolStr = env->GetStringUTFChars(jsonProtocol, nullptr);
    const char *userNameStr = env->GetStringUTFChars(userName, nullptr);
    const char *passWordStr = env->GetStringUTFChars(passWord, nullptr);
    const char *hostStr = env->GetStringUTFChars(host, nullptr);

    LOGD("version>>%d,type>>%d", version, type);

    // 构建C++结构体
    globalConnParams = ConnParams(snStr, nameStr, productIdStr, jsonProtocolStr, userNameStr,
                                  passWordStr, hostStr, port);

    fs::p2p::InfomationManifest manifest;
    manifest.sn = globalConnParams.sn;
    manifest.product_id = globalConnParams.product_id;
    manifest.name = globalConnParams.name;
    manifest.model = modelStr;
    manifest.type = type;
    manifest.version = version;
    s_mp.reset(new fs::p2p::MessagePipeline(manifest));

    s_mp->setConnectStateCallback([](bool state) {
        connected=state;
        JNIEnv *env;
        int attached = 0;
        if (gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            if (gJavaVM->AttachCurrentThread(&env, NULL) != 0) {
                // 处理无法附加线程的情况
                return;
            }
            attached = 1;
        }
        for (auto &call: callbacks) {
            // 在这里使用env进行JNI操作
            (*env).CallVoidMethod((jobject) call, gMethodConnectStatus, state);
        }
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
        LOGI("setConnectStateCallback:state>>%d", state);
    });

    s_mp->setErrorCallback([](int error_code, const std::string &error_string) {
        JNIEnv *env;
        int attached = 0;
        if (gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_6) != JNI_OK) {
            if (gJavaVM->AttachCurrentThread(&env, NULL) != 0) {
                // 处理无法附加线程的情况
                return;
            }
            attached = 1;
        }
        jstring javaStr = env->NewStringUTF(error_string.c_str());
        // 遍历向量中的每个RequestCallback对象，并回调它们
        for (auto &call: callbacks) {
            // 在这里使用env对象进行JNI操作
            (*env).CallVoidMethod((jobject) call, gMErrCallback, error_code, javaStr);
        }
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
        LOGI("setErrorCallback>>%s", error_string.c_str());
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
        // 遍历向量中的每个RequestCallback对象，并回调它们
        for (auto &call: callbacks) {
            // 在这里使用env对象进行JNI操作
            (*env).CallVoidMethod((jobject) call, gMethodPrintLog, level, javaStr);
        }
        if (attached) {
            gJavaVM->DetachCurrentThread();
        }
        LOGI("setLogCallback>>%s", str.c_str());
    });

    if (s_mp != NULL) {
        // 请求接入
        s_mp->setRequestCallback([](const fs::p2p::Request &req) {
            std::map<std::string, fs::p2p::Payload::Device> res_device_list;
            // 将 std::string 转换为 const char*
            LOGD("setRequestCallback>action>>%d", req.action);
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackRequest = convertRequestToJava(env, req);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gReceiveCallback, callbackRequest);
            }
            gJavaVM->DetachCurrentThread();
        });
    }

    // 设备上线通知
    s_mp->setDeviceStartupCallback([](const fs::p2p::InfomationManifest &info) {
        // xcore是云边同步的模型名称，需要往这里注入物模型，使product_id和物模型绑定
        LOGD("setDeviceStartupCallback>>%s", info.model.c_str());
        devList[info.sn] = info;
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

    s_mp->setDeviceHeartbeatCallback([](const fs::p2p::InfomationManifest &info) {
        LOGD("setDeviceHeartbeatCallback>>%s", info.sn.c_str());
        addInfomationManifest(info);
    });

    // 释放字符串资源
    env->ReleaseStringUTFChars(model, modelStr);
    env->ReleaseStringUTFChars(sn, snStr);
    env->ReleaseStringUTFChars(name, nameStr);
    env->ReleaseStringUTFChars(productId, productIdStr);
    env->ReleaseStringUTFChars(jsonProtocol, jsonProtocolStr);
    env->ReleaseStringUTFChars(userName, userNameStr);
    env->ReleaseStringUTFChars(passWord, passWordStr);
    env->ReleaseStringUTFChars(host, hostStr);

    env->DeleteLocalRef(connParamsClass);
    env->DeleteLocalRef(devModelObj);
    env->DeleteLocalRef(typeObj);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_addPipelineCallback(JNIEnv *env, jclass clz,
                                                           jobject callback) {
    jobject callbackObj = (*env).NewGlobalRef(callback);
    if (callback == NULL) {
        return -1;
    }// 检查 callback 是否已经存在于 callbacks 中
    for (const jobject &cb: callbacks) {
        if (env->IsSameObject(cb, callback)) {
            // 如果找到相同的 callback，则可以返回相应的错误码或进行其他处理
            LOGD("registerCallback:Duplicate object");
            return -1;
        }
    }
    // 如果不存在相同的 callback，则将其添加到 callbacks 中
    callbacks.push_back(env->NewGlobalRef(callback));
    LOGD("registerCallback:size>>%d", callbacks.size());
    (*env).DeleteGlobalRef(callbackObj);
    return 0;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_unRegisterCallback(JNIEnv *env, jclass clz,
                                                          jobject callback) {
    jobject callbackObj = (*env).NewGlobalRef(callback);
    if (callback == NULL) {
        return -1;
    }
    removeCallback(env, callbackObj);
    LOGD("unRegisterCallback:size>>%d", callbacks.size());
    (*env).DeleteGlobalRef(callbackObj);
    return 0;
}

JNIEXPORT void JNICALL
Java_com_library_natives_FsPipelineJNI_close(JNIEnv *env, jclass clz) {
    if (s_mp != NULL) {
        s_mp->close();
    }
    for (jobject cb: callbacks) {
        (*env).DeleteGlobalRef(cb);
    }
    callbacks.clear();
}

JNIEXPORT jobject JNICALL
Java_com_library_natives_FsPipelineJNI_getDevModelList(JNIEnv *env, jclass clz) {
    jclass listClass = env->FindClass("java/util/ArrayList");
    jmethodID listConstructor = env->GetMethodID(listClass, "<init>", "()V");
    jmethodID listAdd = env->GetMethodID(listClass, "add", "(Ljava/lang/Object;)Z");

    jobject arrayList = env->NewObject(listClass, listConstructor);

    jclass devModelClass = env->FindClass("com/library/natives/SubDev");
    jmethodID devModelConstructor = env->GetMethodID(devModelClass, "<init>",
                                                     "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Lcom/library/natives/Type;I)V");
    jclass typeClass = env->FindClass("com/library/natives/Type");
    jfieldID gatewayField = env->GetStaticFieldID(typeClass, "Gateway",
                                                  "Lcom/library/natives/Type;");
    jfieldID serviceField = env->GetStaticFieldID(typeClass, "Service",
                                                  "Lcom/library/natives/Type;");
    jfieldID unknownField = env->GetStaticFieldID(typeClass, "Unknown",
                                                  "Lcom/library/natives/Type;");
    jobject gatewayEnum = env->GetStaticObjectField(typeClass, gatewayField);
    jobject serviceEnum = env->GetStaticObjectField(typeClass, serviceField);
    jobject unknownEnum = env->GetStaticObjectField(typeClass, unknownField);

    for (const auto &manifest: subDevList) {
        jstring sn = env->NewStringUTF(manifest.sn.c_str());
        jstring product_id = env->NewStringUTF(manifest.product_id.c_str());
        jstring name = env->NewStringUTF(manifest.name.c_str());
        jstring model = env->NewStringUTF(manifest.model.c_str());
        jobject type;
        if (manifest.type == fs::p2p::InfomationManifest::Gateway) {
            type = gatewayEnum;
        } else if (manifest.type == fs::p2p::InfomationManifest::Service) {
            type = serviceEnum;
        } else {
            type = unknownEnum; // 设置为NULL或者其他默认值，视情况而定
        }
        jint version = static_cast<jint>(manifest.version);
        jobject devModel = env->NewObject(devModelClass, devModelConstructor, sn, product_id, name,
                                          model, type, version);
        env->CallBooleanMethod(arrayList, listAdd, devModel);
        //对象释放
        env->DeleteLocalRef(sn);
        env->DeleteLocalRef(product_id);
        env->DeleteLocalRef(name);
        env->DeleteLocalRef(model);
    }
    //对象释放
    env->DeleteLocalRef(listClass);
    env->DeleteLocalRef(devModelClass);
//    env->DeleteLocalRef(arrayList);

    env->DeleteLocalRef(gatewayEnum);
    env->DeleteLocalRef(serviceEnum);
    env->DeleteLocalRef(unknownEnum);

    return arrayList;
}

JNIEXPORT void JNICALL
Java_com_library_natives_FsPipelineJNI_connect(JNIEnv *env, jclass clz) {
    if (s_mp != NULL) {
        s_mp->open(globalConnParams.host, globalConnParams.port, globalConnParams.userName,
                   globalConnParams.passWord);
    }
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postOnLine(JNIEnv *env, jclass clz) {
    std::string iid;
    if (s_mp) {
        iid = s_mp->postStartup();
    }
    return iid.empty() ? -1 : 0;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_postHeartbeat(JNIEnv *env, jclass clz) {
    std::string iid;
    if (s_mp) {
        iid = s_mp->postHeartbeat();
    }
    return iid.empty() ? -1 : 0;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_replyBody(JNIEnv *env, jclass clz, jobject request,
                                                 jobject deviceMap) {
    fs::p2p::Request convertRequest = getRequest(env, request);
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
        jstring key = (jstring) env->CallObjectMethod(entry, getKeyMethod);
        jobject deviceObj = env->CallObjectMethod(entry, getValueMethod);

        // 创建 C++ 的 Device 对象
        fs::p2p::Payload::Device device;

        // 将 Java 字符串转换为 C++ 字符串
        const char *keyStr = env->GetStringUTFChars(key, nullptr);
        device.sn = keyStr;

        // 获取 Device 对象的其他字段值
        jstring productId = (jstring) env->GetObjectField(deviceObj, productIdField);
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
    env->DeleteLocalRef(mapClass);
    env->DeleteLocalRef(setClass);
    env->DeleteLocalRef(iteratorClass);
    env->DeleteLocalRef(entryClass);
    env->DeleteLocalRef(deviceClass);

    env->DeleteLocalRef(entrySet);
    env->DeleteLocalRef(iterator);

    return s_mp != NULL ? s_mp->response(convertRequest, deviceMapCpp) : -1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_replyMethod(JNIEnv *env, jclass clz, jobject request,
                                                   jobject params) {
    fs::p2p::Request convertRequest = getRequest(env, request);
    std::map<std::string, fs::p2p::Payload::Device> res_device_list;
    for (auto &item: convertRequest.payload.devices) {
        fs::p2p::Payload::Device res_device;
        res_device.sn = item.second.sn;
        res_device.product_id = item.second.product_id;
        const fs::p2p::Method &firstMethod = item.second.methods.front();
        fs::p2p::Method customizeMaps;
        customizeMaps.name = firstMethod.name;
        customizeMaps.params = convertOrderedJsons(env, params);
        customizeMaps.reason_code = firstMethod.reason_code;
        customizeMaps.reason_string = firstMethod.reason_string;
        res_device.methods.clear();
        res_device.methods.push_back(customizeMaps);
        if (res_device.methods.size() > 0) {
            res_device_list[res_device.sn] = res_device;
        }
    }
    //释放对象
    env->DeleteLocalRef(request);
    env->DeleteLocalRef(params);

    return s_mp != NULL ? s_mp->response(convertRequest, res_device_list) : -1;
}

JNIEXPORT jint JNICALL
Java_com_library_natives_FsPipelineJNI_replyService(JNIEnv *env, jclass clz, jobject request,
                                                    jobject params) {
    fs::p2p::Request convertRequest = getRequest(env, request);
    if (!convertRequest.payload.devices.empty()) {
        auto it = convertRequest.payload.devices.begin();
        fs::p2p::Payload::Device &firstDevice = it->second;
        std::string sn = firstDevice.sn;
        auto &services = firstDevice.services.front();
        std::map<std::string, ordered_json> newPropertys = convertOrderedJsons(env, params);
        services.propertys = newPropertys;
        int result =
                s_mp != NULL ? s_mp->response(convertRequest, convertRequest.payload.devices) : -1;
        env->DeleteLocalRef(request);
        env->DeleteLocalRef(params);
        return result;
    }
    return -1;
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushEvents(JNIEnv *env, jclass clz, jobject eventsList) {
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    fdevice.sn = s_mp->infomationManifest().sn;
    fdevice.product_id = s_mp->infomationManifest().product_id;
    fdevice.events = convertJavaToEvents(env, eventsList);
    list[s_mp->infomationManifest().sn] = fdevice;

    std::string iid;
    if (s_mp) {
        iid = s_mp->postEvent(list, [](const fs::p2p::Response &res, void *) {
            LOGD("postEvent>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        });
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushMethods(JNIEnv *env, jclass clz, jstring sn,
                                                   jobject out) {
    std::string snStr = jstringToString(env, sn);
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    auto &dev = devList[snStr];
    fdevice.sn = dev.sn;
    fdevice.product_id = dev.product_id;
    fdevice.methods = convertJavaToMethods(env, out);
    list[dev.sn] = fdevice;

    std::string iid;
    if (s_mp) {
        iid = s_mp->postMethod(list, [](const fs::p2p::Response &res, void *) {
            LOGD("postMethod>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        }, NULL, dev.getSubscribeTopic());
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushMethod(JNIEnv *env, jclass clz, jstring sn,
                                                  jobject out) {
    std::string snStr = jstringToString(env, sn);
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    auto &dev = devList[snStr];
    fdevice.sn = dev.sn;
    fdevice.product_id = dev.product_id;
    fdevice.methods.push_back(convertJavaToMethod(env, out));
    list[dev.sn] = fdevice;

    std::string iid;
    if (s_mp) {
        iid = s_mp->postMethod(list,[](const fs::p2p::Response &res, void *)
        {
            LOGD("postMethod>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        }, NULL,dev.getSubscribeTopic());
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushEvent(JNIEnv *env, jclass clz, jobject out) {
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    fdevice.sn = s_mp->infomationManifest().sn;
    fdevice.product_id = s_mp->infomationManifest().product_id;
    fdevice.events.push_back(convertJavaEvent(env, out));
    list[s_mp->infomationManifest().sn] = fdevice;

    std::string iid;
    if (s_mp) {
        iid = s_mp->postEvent(list, [](const fs::p2p::Response &res, void *)
        {
            LOGD("postEvent>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        });
        LOGD("postEvent>>iid>>%s", iid.c_str());
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushNotify(JNIEnv *env, jclass clz, jobject out) {
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    fdevice.sn = s_mp->infomationManifest().sn;
    fdevice.product_id = s_mp->infomationManifest().product_id;
    fdevice.services.push_back(convertJavaToService(env, out));
    list[s_mp->infomationManifest().sn] = fdevice;
    std::string iid;
    if (s_mp) {
        iid = s_mp->postNotify(list);
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushNotifyList(JNIEnv *env, jclass clz, jobject out) {
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    fdevice.sn = s_mp->infomationManifest().sn;
    fdevice.product_id = s_mp->infomationManifest().product_id;
    fdevice.services = convertJavaToServices(env, out);
    list[s_mp->infomationManifest().sn] = fdevice;
    std::string iid;
    if (s_mp) {
        iid = s_mp->postNotify(list);
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushReadList(JNIEnv *env, jclass clz, jstring sn,
                                                    jobject out) {
    std::string snStr = jstringToString(env, sn);
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    auto &dev = devList[snStr];
    fdevice.sn = dev.sn;
    fdevice.product_id = dev.product_id;
    fdevice.services = convertJavaToServices(env, out);
    list[snStr] = fdevice;
    std::string iid;
    if (s_mp) {
        iid = s_mp->postRead(list, [](const fs::p2p::Response &res, void *)
        {
            LOGD("postRead>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        }, NULL,dev.getSubscribeTopic());
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushRead(JNIEnv *env, jclass clz, jstring sn, jobject out) {
    std::string snStr = jstringToString(env, sn);
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    auto &dev = devList[snStr];
    fdevice.sn = dev.sn;
    fdevice.product_id = dev.product_id;
    fdevice.services.push_back(convertJavaToService(env, out));
    list[snStr] = fdevice;

    std::string iid;
    if (s_mp) {
        iid = s_mp->postRead(list, [](const fs::p2p::Response &res, void *)
        {
            LOGD("postRead>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        }, NULL,dev.getSubscribeTopic());
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushWriteList(JNIEnv *env, jclass clz, jstring sn,
                                                     jobject out) {
    std::string snStr = jstringToString(env, sn);
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    auto &dev = devList[snStr];
    fdevice.sn = dev.sn;
    fdevice.product_id = dev.product_id;
    fdevice.services = convertJavaToServices(env, out);
    list[snStr] = fdevice;

    std::string iid;
    if (s_mp) {
        iid = s_mp->postWrite(list, [](const fs::p2p::Response &res, void *)
        {
            LOGD("postWrite>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        }, NULL,dev.getSubscribeTopic());
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_library_natives_FsPipelineJNI_pushWrite(JNIEnv *env, jclass clz, jstring sn, jobject out) {
    std::string snStr = jstringToString(env, sn);
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    auto &dev = devList[snStr];
    fdevice.sn = dev.sn;
    fdevice.product_id = dev.product_id;
    fdevice.services.push_back(convertJavaToService(env, out));
    list[snStr] = fdevice;
    std::string iid;
    if (s_mp) {
        iid = s_mp->postWrite(list, [](const fs::p2p::Response &res, void *)
        {
            LOGD("postWrite>>deviceSize>>%d", res.payload.devices.size());
            JNIEnv *env;
            if (gJavaVM->AttachCurrentThread(&env, NULL) != JNI_OK) {
                // 处理附加失败的情况
                return;
            }
            jobject callbackResponse = convertResponseToJava(env, res);
            // 遍历向量中的每个RequestCallback对象，并回调它们
            for (auto &call: callbacks) {
                // 在这里使用env对象进行JNI操作
                env->CallVoidMethod((jobject) call, gPushCallback, callbackResponse);
            }
            gJavaVM->DetachCurrentThread();
        }, NULL,dev.getSubscribeTopic());
    }
    return iid.empty()?env->NewStringUTF(""):env->NewStringUTF(iid.c_str());
}
}
