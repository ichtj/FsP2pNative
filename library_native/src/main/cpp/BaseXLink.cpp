
#include <jni.h>
#include <string>
#include <memory>
#include "fs_p2p/MessagePipeline.h"
#include "JavaIMqttCallback.h"
#include "Timer.h"
#include "RequestManager.h"
#include "jsonTools.h"
#include "Logger.h"
#include "ToCppStruct.h"
static std::unique_ptr<fs::p2p::MessagePipeline> s_mp;
static std::mutex s_mp_mutex;
static Timer g_timer;  // 全局定时器对象
static bool isMqConnect = false;
static bool isSubscribed = false;

JavaIMqttCallback g_i_mqtt_callback;
JavaVM* gJvm = nullptr;

jint JNI_OnLoad(JavaVM* vm, void*) {
    gJvm = vm;
    return JNI_VERSION_1_6;
}

inline fs::p2p::MessagePipeline* getGlobalPipeline() {
    std::lock_guard<std::mutex> lk(s_mp_mutex);
    return s_mp.get();
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
        case 0x107:
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

extern "C" {

JNIEXPORT void JNICALL
Java_com_library_natives_BaseXLink_logEnable(JNIEnv *env, jclass clz,
                                                 jboolean isEnable) {
    setLoggingEnabled(isEnable);
}

JNIEXPORT jboolean JNICALL
Java_com_library_natives_BaseXLink_isLogEnable(JNIEnv *env, jclass clz) {
    return getLoggingEnabled();
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseXLink_getConnectStatus
        (JNIEnv* env, jclass /*clazz*/)
{
    fs::p2p::MessagePipeline *mp = getGlobalPipeline();
    return mp&&isMqConnect;
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseXLink_connect
        (JNIEnv* env, jclass /*clazz*/, jobject information, jobject i_mqtt_callback)
{
    g_i_mqtt_callback.set(env, i_mqtt_callback);
    if (!information){
        g_i_mqtt_callback.callConnState(gJvm,false,"connection state changed1");
        return;
    }
    jclass manifestCls = env->GetObjectClass(information);
    if (!manifestCls) {
        g_i_mqtt_callback.callConnState(gJvm,false,"connection state changed2");
        return;
    }

    auto deleteLocalRefIf = [env](jobject ref){
        if (ref) env->DeleteLocalRef(ref);
    };

    jmethodID mid_getSn = env->GetMethodID(manifestCls, "getSn", "()Ljava/lang/String;");
    jmethodID mid_getProductId = env->GetMethodID(manifestCls, "getProductId", "()Ljava/lang/String;");
    jmethodID mid_getName = env->GetMethodID(manifestCls, "getName", "()Ljava/lang/String;");
    jmethodID mid_getModel = env->GetMethodID(manifestCls, "getModel", "()Ljava/lang/String;");
    jmethodID mid_getType = env->GetMethodID(manifestCls, "getType", "()Lcom/library/natives/Type;"); // 返回 Type 对象
    jmethodID mid_getVersion = env->GetMethodID(manifestCls, "getVersion", "()I");
    jmethodID mid_getHost = env->GetMethodID(manifestCls, "getHost", "()Ljava/lang/String;");
    jmethodID mid_getPort = env->GetMethodID(manifestCls, "getPort", "()I");
    jmethodID mid_getUser = env->GetMethodID(manifestCls, "getUsername", "()Ljava/lang/String;");
    jmethodID mid_getPass = env->GetMethodID(manifestCls, "getPassword", "()Ljava/lang/String;");
    jmethodID mid_Protocol = env->GetMethodID(manifestCls, "getProtocol", "()Ljava/lang/String;");

    jobject jTypeObject = mid_getType ? env->CallObjectMethod(information, mid_getType) : nullptr;

    jstring jsn = mid_getSn ? (jstring)env->CallObjectMethod(information, mid_getSn) : nullptr;
    jstring jproductId = mid_getProductId ? (jstring)env->CallObjectMethod(information, mid_getProductId) : nullptr;
    jstring jname = mid_getName ? (jstring)env->CallObjectMethod(information, mid_getName) : nullptr;
    jstring jmodel = mid_getModel ? (jstring)env->CallObjectMethod(information, mid_getModel) : nullptr;
    jint jtype = 0;
    if (jTypeObject) {
        // 1. 获取 java.lang.Enum 类
        jclass enumCls = env->FindClass("java/lang/Enum");

        // 2. 获取 ordinal() 方法ID
        jmethodID mid_ordinal = enumCls ? env->GetMethodID(enumCls, "ordinal", "()I") : nullptr;

        if (mid_ordinal) {
            // 3. 调用 ordinal() 方法，获取整型值 (jint)
            jtype = env->CallIntMethod(jTypeObject, mid_ordinal);
        }

        // 4. 清理局部引用
        if (enumCls) env->DeleteLocalRef(enumCls);
    }
    jint jversion = mid_getVersion ? env->CallIntMethod(information, mid_getVersion) : 0;
    jstring jhost = mid_getHost ? (jstring)env->CallObjectMethod(information, mid_getHost) : nullptr;
    jint jport = mid_getPort ? env->CallIntMethod(information, mid_getPort) : 0;
    jstring juser = mid_getUser ? (jstring)env->CallObjectMethod(information, mid_getUser) : nullptr;
    jstring jpass = mid_getPass ? (jstring)env->CallObjectMethod(information, mid_getPass) : nullptr;
    jstring jProtocol = mid_Protocol ? (jstring)env->CallObjectMethod(information, mid_Protocol) : nullptr;
    std::string protocol = jstrToStd(env, jProtocol);
    // 检查 JNI 调用中是否产生异常，若有则清理并返回
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        deleteLocalRefIf(jsn);
        deleteLocalRefIf(jproductId);
        deleteLocalRefIf(jname);
        deleteLocalRefIf(jmodel);
        deleteLocalRefIf(jhost);
        deleteLocalRefIf(juser);
        deleteLocalRefIf(jpass);
        deleteLocalRefIf(jProtocol);
        deleteLocalRefIf(manifestCls);
        deleteLocalRefIf(jTypeObject); // 清理获取到的 Type 对象
        return ;
    }

    fs::p2p::InfomationManifest manifest;
    manifest.sn = jstrToStd(env, jsn);
    manifest.product_id = jstrToStd(env, jproductId);
    manifest.name = jstrToStd(env, jname);
    manifest.model = jstrToStd(env, jmodel);
    manifest.type = static_cast<int>(jtype);
    manifest.version = static_cast<int>(jversion);

    {
        // 限定锁的作用域到 pipeline 重建部分
        std::lock_guard<std::mutex> lk(s_mp_mutex);
        s_mp.reset(new fs::p2p::MessagePipeline(manifest));
    }

    const std::string host = jstrToStd(env, jhost);
    const unsigned short port = static_cast<unsigned short>(jport);
    const std::string userName = jstrToStd(env, juser);
    const std::string passWord = jstrToStd(env, jpass);

    if (s_mp) {
        s_mp->open(host, port, userName, passWord);
        s_mp->setConnectStateCallback([](bool isConnected){
            isMqConnect=isConnected;
            g_i_mqtt_callback.callConnState(gJvm,isConnected,"connection state changed");
            if (isConnected) {
                std::string iid1=s_mp->postStartup();
                LOGD( "setConnectStateCallback iid1=%s",iid1.c_str());
                g_timer.start(1*60*1000, []() {
                    std::string iid2 = s_mp->postHeartbeat();
                    LOGD( "setConnectStateCallback iid2=%s",iid2.c_str());
                });
            }else{
                g_timer.stop();
            }
            LOGD("setConnectStateCallback isConnected=%d",isConnected);
        });
        s_mp->setDeviceHeartbeatCallback([protocol](const fs::p2p::InfomationManifest &info) {
            LOGD("setDeviceHeartbeatCallback>>%s,infoMode>>%s", info.sn.c_str(),info.model.c_str());
            if (info.model == "xcore"&&!isSubscribed) {
                std::string topic=info.getPublishTopic();
                std::string iid=s_mp->postMethod({{info.sn, {
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
                                                                                  protocol}
                                                                         }
                                                                 }
                                                         }, // methods
                                                         {} // events
                                                 }}
                                                 }, // std::map<std::string, Payload::Device>
                                                 [](const fs::p2p::Response &, void *) {},
                                                 NULL, info.getSubscribeTopic());
                if (!iid.empty()){
                    g_i_mqtt_callback.callSubscribed(gJvm,topic);
                    isSubscribed= true;
                }else{
                    isSubscribed=false;
                    g_i_mqtt_callback.callSubscribeFail(gJvm,topic,"subscribe failed");
                }
                LOGD("setDeviceHeartbeatCallback iot_json_protocol iid=%s", iid.c_str());
            }

        });
        s_mp->setDeviceStartupCallback([](const fs::p2p::InfomationManifest &info) {
            // xcore是云边同步的模型名称，需要往这里注入物模型，使product_id和物模型绑定
            LOGD("setDeviceStartupCallback>>%s", info.model.c_str());
        });
        s_mp->setErrorCallback([](int error_code, const std::string &error_string) {
            LOGD( "Error Code: %d, Description: %s", error_code, error_string.c_str());
        });
        s_mp->setBroadcastCallback([](const fs::p2p::Request &req) {
//            LOGD( "setBroadcastCallback iid=%s,action>>%d", req.ack.c_str(),req.action);
            std::map<std::string, fs::p2p::Payload::Device> res_device_list=req.payload.devices;
            for (const auto& device_pair : res_device_list) {
                const std::string& device_sn = device_pair.first;
                const fs::p2p::Payload::Device& device = device_pair.second;
                // --- 遍历 Events ---
                for (const auto& event : device.events) {
                    for (const auto& param_pair : event.params) {
                    }
                    LOGD( "setBroadcastCallback:event device_sn=%s , eventName=%s",device_sn.c_str(),event.name.c_str());

                }
                // --- 遍历 Events ---
                for (const auto& service : device.services) {
                    for (const auto& param_pair : service.propertys) {
                    }
                    LOGD( "setBroadcastCallback:service device_sn=%s , serviceName=%s",device_sn.c_str(),service.name.c_str());

                }
                // --- 遍历 Events ---
                for (const auto& method : device.methods) {
                    for (const auto& param_pair : method.params) {
                    }
                    LOGD( "setBroadcastCallback:method device_sn=%s , methodName=%s",device_sn.c_str(),method.name.c_str());
                }
            }
        });

        s_mp->setIncomingMethodCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req); // ✅ 保存
            LOGD( "setRequestCallback Action_Method iid=%s", req.iid.c_str());
            // --- 遍历 Methods ---
            for (const auto& method : device.methods) {
                for (const auto& param_pair : method.params) {
                }
                BaseData baseData={0x100,req.iid,method.name, method.params};
                g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
            }
        });
        s_mp->setIncomingReadCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req); // ✅ 保存
            LOGD( "setRequestCallback Action_Read iid=%s", req.iid.c_str());
            // --- 遍历 Services ---
            for (const auto& service : device.services) {
                for (const auto& prop_pair : service.propertys) {
                    BaseData baseData={0x105,req.iid,service.name+"-"+prop_pair.first, service.propertys};
                    g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
                }
            }
        });
        s_mp->setIncomingWriteCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req); // ✅ 保存
            LOGD( "setRequestCallback Action_Write iid=%s", req.iid.c_str());
            // --- 遍历 Services ---
            for (const auto& service : device.services) {
                for (const auto& prop_pair : service.propertys) {
                    BaseData baseData={0x104,req.iid,service.name+"-"+prop_pair.first, service.propertys};
                    g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
                }
            }
        });
        s_mp->setIncomingEventCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req); // ✅ 保存
            LOGD( "setRequestCallback Action_Event iid=%s", req.iid.c_str());
            // --- 遍历 Events ---
            for (const auto& event : device.events) {
                for (const auto& param_pair : event.params) {
                }
                BaseData baseData={0x102,req.iid,event.name, event.params};
                g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
            }
        });

//        s_mp->setRequestCallback([env](const fs::p2p::Request &req) {
//            RequestManager::getInstance().addRequest(req); // ✅ 保存
//            std::map<std::string, fs::p2p::Payload::Device> res_device_list=req.payload.devices;
//            for (const auto& device_pair : res_device_list) {
//                const std::string& device_sn = device_pair.first;
//                const fs::p2p::Payload::Device& device = device_pair.second;
//                LOGD( "device_pair device_sn=%s", device_sn.c_str());
//
//                int javaAction=convertToJavaAction(req.action);
//                switch (req.action) {
//                    case fs::p2p::Request::Action::Action_Method:
//                        LOGD( "setRequestCallback Action_Method iid=%s", req.iid.c_str());
//                        // --- 遍历 Methods ---
//                        for (const auto& method : device.methods) {
//                            for (const auto& param_pair : method.params) {
//                            }
//                            BaseData baseData={javaAction,req.iid,method.name, method.params};
//                            g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
//                        }
//                        break;
//                    case fs::p2p::Request::Action::Action_Read:
//                        LOGD( "setRequestCallback Action_Read iid=%s", req.iid.c_str());
//                        // --- 遍历 Services ---
//                        for (const auto& service : device.services) {
//                            for (const auto& prop_pair : service.propertys) {
//                            }
//                            BaseData baseData={javaAction,req.iid,service.name, service.propertys};
//                            g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
//                        }
//                        break;
//                    case fs::p2p::Request::Action::Action_Write:
//                        LOGD( "setRequestCallback Action_Write iid=%s", req.iid.c_str());
//                        // --- 遍历 Services ---
//                        for (const auto& service : device.services) {
//                            for (const auto& prop_pair : service.propertys) {
//                            }
//                            BaseData baseData={javaAction,req.iid,service.name, service.propertys};
//                            g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
//                        }
//                        break;
//                    case fs::p2p::Request::Action::Action_Event:
//                        LOGD( "setRequestCallback Action_Event iid=%s", req.iid.c_str());
//                        // --- 遍历 Events ---
//                        for (const auto& event : device.events) {
//                            for (const auto& param_pair : event.params) {
//                            }
//                            BaseData baseData={javaAction,req.iid,event.name, event.params};
//                            g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
//                        }
//                        break;
//                    default:
//                        LOGD( "setRequestCallback Action_Unknown iid=%s", req.iid.c_str());
//                        break;
//
//                }
//            }
//        });
    }
    deleteLocalRefIf(jsn);
    deleteLocalRefIf(jproductId);
    deleteLocalRefIf(jname);
    deleteLocalRefIf(jmodel);
    deleteLocalRefIf(jhost);
    deleteLocalRefIf(juser);
    deleteLocalRefIf(jpass);
    deleteLocalRefIf(jProtocol);
    deleteLocalRefIf(manifestCls);
    deleteLocalRefIf(jTypeObject); // 清理获取到的 Type 对象
}


JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseXLink_putReply
        (JNIEnv* env, jclass, jint i_put_type, jstring iid, jstring operation, jobject data_map )
{
    BaseData baseData;
    baseData.iPutType=i_put_type;
    baseData.iid=jstrToStd(env, iid);
    baseData.operation=jstrToStd(env, operation);
    baseData.maps=javaMapToCppMapValue(env, data_map);
    std::string str_iid = jstrToStd(env, iid);
    std::list<fs::p2p::Request> requests=RequestManager::getInstance().getAllRequests(); // ✅ 保存
    for ( auto& r : requests) {
        if (r.iid==str_iid){
            std::map<std::string, fs::p2p::Payload::Device> res_device_list=r.payload.devices;
            for (auto& device_pair : res_device_list) {
                std::string device_sn = device_pair.first;
                fs::p2p::Payload::Device& device = device_pair.second;
                LOGD( "putCmd device_sn=%s", device_sn.c_str());

                int cppAction=convertToResponseAction(i_put_type);

                if (cppAction==fs::p2p::Response::Action::Action_Method){
                    LOGD( "putCmd Action_Method iid=%s", r.iid.c_str());
                    // --- 遍历 Methods ---
                    for (auto& read : device.methods) {
                        std::map<std::string, ordered_json> newMaps = javaMapToCppMapValue(env, data_map);
                        read.params = newMaps;
                    }
                    int result=s_mp->response(r,res_device_list);
                    if (result==0){
                        g_i_mqtt_callback.callPushed(gJvm,baseData);
                    }else{
                        g_i_mqtt_callback.callPushFail(gJvm,baseData,"putCmd failed");
                    }
                }else if(cppAction==fs::p2p::Response::Action::Action_Read){
                    LOGD( "putCmd Action_Method iid=%s", r.iid.c_str());
                    // --- 遍历 Methods ---
                    for (auto& read : device.services) {
                        std::map<std::string, ordered_json> newMaps = javaMapToCppMapValue(env, data_map);
                        read.propertys = newMaps;
                    }
                    int result=s_mp->response(r,res_device_list);
                    if (result==0){
                        g_i_mqtt_callback.callPushed(gJvm,baseData);
                    }else{
                        g_i_mqtt_callback.callPushFail(gJvm,baseData,"putCmd failed");
                    }                }else if(cppAction==fs::p2p::Response::Action::Action_Write){
                    LOGD( "putCmd Action_Method iid=%s", r.iid.c_str());
                    // --- 遍历 Methods ---
                    for (auto& write : device.services) {
                        std::map<std::string, ordered_json> newMaps = javaMapToCppMapValue(env, data_map);
                        write.propertys = newMaps;
                    }
                    int result=s_mp->response(r,res_device_list);
                    if (result==0){
                        g_i_mqtt_callback.callPushed(gJvm,baseData);
                    }else{
                        g_i_mqtt_callback.callPushFail(gJvm,baseData,"putCmd failed");
                    }
                }else{
                    LOGD( "putCmd Action_Unknown iid=%s", r.iid.c_str());

                }
            }
            return JNI_TRUE;
        }
    }
}


JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseXLink_postMsg
        (JNIEnv* env, jclass, jint i_put_type , jstring target_sn, jstring p_did , jstring jname, jobject jparams )
{
    std::string targetSnStr = jstrToStd(env, target_sn);
    std::string pdidStr = jstrToStd(env, p_did);
    jstring jreason_str = env->NewStringUTF("");
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    fdevice.sn = targetSnStr;
    fdevice.product_id = pdidStr;
    int cppAction=convertToRequestAction(i_put_type);
    std::string iid;
    if (cppAction==fs::p2p::Request::Action::Action_Method){
        fs::p2p::Method method = convertToMethod(env, jname, jparams, 0, jreason_str);
        fdevice.methods.push_back(method);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postMethod(list,[](const fs::p2p::Response &res, void *)
            {}, NULL,"");
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Read){
        fs::p2p::Service service = convertToService(env, jname, jparams, 0, jreason_str);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postRead(list,[](const fs::p2p::Response &res, void *)
            {}, NULL,"");
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Write){
        fs::p2p::Service service = convertToService(env, jname, jparams, 0, jreason_str);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postWrite(list,[](const fs::p2p::Response &res, void *)
            {}, NULL,"");
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Event){
        fs::p2p::Event event = convertToEvent(env, jname, jparams, 0, jreason_str);
        fdevice.events.push_back(event);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postEvent(list,[](const fs::p2p::Response &res, void *)
            {}, NULL);
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Notify){
        fs::p2p::Event event = convertToEvent(env, jname, jparams, 0, jreason_str);
        fdevice.events.push_back(event);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postNotify(list);
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Broadcast){
        fs::p2p::Event event = convertToEvent(env, jname, jparams, 0, jreason_str);
        fdevice.events.push_back(event);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postBroadcast(list);
        }
    }
    return !iid.empty();
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseXLink_disConnect
        (JNIEnv* env, jclass)
{
    std::lock_guard<std::mutex> lk(s_mp_mutex);
    if (s_mp) {
        s_mp->close();
    }
    g_i_mqtt_callback.clear(env);
    g_timer.stop();
}

} // extern "C"