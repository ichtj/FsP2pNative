
#include <jni.h>
#include <string>
#include <memory>
#include "fs_p2p/MessagePipeline.h"
#include "PipelineCallback.h"
#include "Timer.h"
#include "RequestManager.h"
#include "iTools.h"
#include "Logger.h"
//#include "BlackBeanConverter.h"
#include "SubscribeInfomation.h"
#include "IInfomationsCallback.h"
#include "PutTypeTool.h"
#include "IBlackCallback.h"

static std::unique_ptr<fs::p2p::MessagePipeline> s_mp;
static std::mutex s_mp_mutex;
static Timer g_timer;  // 全局定时器对象
static int iot_connect_state_value = 0;//无连接-1 连接中0 已连接1
static bool isP2pConnect = false;
static bool isIotSubscribed = false;
PipelineCallback g_i_mqtt_callback;
IInfomationsCallback infomationsCallback;
static IBlackCallback* iblackcall = nullptr;
static fs::p2p::InfomationManifest xcore_manifest;
JavaVM* gJvm = nullptr;


jint JNI_OnLoad(JavaVM* vm, void*) {
    gJvm = vm;
    return JNI_VERSION_1_6;
}

inline fs::p2p::MessagePipeline* getGlobalPipeline() {
    std::lock_guard<std::mutex> lk(s_mp_mutex);
    return s_mp.get();
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_library_natives_BaseFsP2pTools_logEnable(JNIEnv *env, jclass clz,
                                                  jboolean isEnable) {
    setLoggingEnabled(isEnable);
}

JNIEXPORT jboolean JNICALL
Java_com_library_natives_BaseFsP2pTools_isLogEnable(JNIEnv *env, jclass clz) {
    return getLoggingEnabled();
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_getConnectStatus
        (JNIEnv* env, jclass /*clazz*/)
{
    return isP2pConnect&&iot_connect_state_value==1;
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseFsP2pTools_connect
        (JNIEnv* env, jclass , jobject information, jobject xCoreBean,jstring jProtocol,
         jobject i_pipeline_callback)
{
    if (iot_connect_state_value==1&&isP2pConnect){
        g_i_mqtt_callback.callP2pConnState(gJvm,true,"Connected");
        g_i_mqtt_callback.callIotConnState(gJvm,true,"Connected");
        return;
    }
    PutTypeTool::init(gJvm);
    g_i_mqtt_callback.set(env, i_pipeline_callback);
    jclass xCoreBeanCls = env->GetObjectClass(xCoreBean);
    jmethodID mid_getHost = env->GetMethodID(xCoreBeanCls, "getHost", "()Ljava/lang/String;");
    jmethodID mid_getPort = env->GetMethodID(xCoreBeanCls, "getPort", "()I");
    jmethodID mid_getUser = env->GetMethodID(xCoreBeanCls, "getUsername", "()Ljava/lang/String;");
    jmethodID mid_getPass = env->GetMethodID(xCoreBeanCls, "getPassword", "()Ljava/lang/String;");

    jstring jhost = mid_getHost ? (jstring)env->CallObjectMethod(xCoreBean, mid_getHost) : nullptr;
    jint jport = mid_getPort ? env->CallIntMethod(xCoreBean, mid_getPort) : 0;
    jstring juser = mid_getUser ? (jstring)env->CallObjectMethod(xCoreBean, mid_getUser) : nullptr;
    jstring jpass = mid_getPass ? (jstring)env->CallObjectMethod(xCoreBean, mid_getPass) : nullptr;
    std::string protocol = iTools::jstrToStd(env, jProtocol);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        iTools::deleteLocalRefs(env,jhost,juser,jpass,jProtocol);
        return ;
    }
    fs::p2p::InfomationManifest manifest = iTools::convertToCppInfomation(env, information);
    // 限定锁的作用域到 pipeline 重建部分code review
    std::lock_guard<std::mutex> lk(s_mp_mutex);
    s_mp.reset(new fs::p2p::MessagePipeline(manifest));
    const std::string host = iTools::jstrToStd(env, jhost);
    const unsigned short port = static_cast<unsigned short>(jport);
    const std::string userName = iTools::jstrToStd(env, juser);
    const std::string passWord = iTools::jstrToStd(env, jpass);

    if (s_mp) {
        iot_connect_state_value=0;
        s_mp->open(host, port, userName, passWord);
        s_mp->setConnectStateCallback([](bool isConnected){
            isP2pConnect=isConnected;
            g_i_mqtt_callback.callP2pConnState(gJvm,isConnected,"connection state changed5");
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
            LOGD("setDeviceHeartbeatCallback>>%s", info.model.c_str());
        });
        s_mp->setDeviceStartupCallback([](const fs::p2p::InfomationManifest &info) {
            // xcore是云边同步的模型名称，需要往这里注入物模型，使product_id和物模型绑定
            LOGD("setDeviceStartupCallback>>%s", info.model.c_str());
        });
        s_mp->setErrorCallback([](int error_code, const std::string &error_string) {
            LOGD( "Error Code: %d, Description: %s", error_code, error_string.c_str());
        });
        s_mp->setBroadcastCallback([protocol](const fs::p2p::Request &req) {
            LOGD( "setBroadcastCallback iid=%s,action>>%d", req.ack.c_str(),req.action);
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
                    if (method.name=="iot_connect_state"){
                        xcore_manifest.sn = device_sn;
                        xcore_manifest.product_id = device.product_id;
                        if (!isIotSubscribed) {
                            std::string topic=xcore_manifest.getPublishTopic();
                            std::string iid=s_mp->postMethod(
                                {{xcore_manifest.sn, {
                                        xcore_manifest.sn,
                                        xcore_manifest.product_id,
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
                        [](const fs::p2p::Response req, void *){},
                                    NULL, xcore_manifest.getSubscribeTopic());
                            int result=s_mp->subscribe(xcore_manifest, nullptr);
                            LOGD("setBroadcastCallback>>subscribe>>%d,iid>>%s",result,iid.c_str());
                            if (result==0){
                                g_i_mqtt_callback.callSubscribed(gJvm,topic);
                                isIotSubscribed= true;
                                SubscribeInfomation::getInstance().addManifest(xcore_manifest);
                            }else{
                                isIotSubscribed=false;
                                g_i_mqtt_callback.callSubscribeFail(gJvm,topic,"subscribe failed");
                            }
                        }
                        std::string state= iTools::getValue(method.params,"state","0");
                        std::string description= iTools::getValue(method.params,"desc","");
                        iot_connect_state_value=(state=="1")?1:-1;
                        g_i_mqtt_callback.callIotConnState(gJvm,state=="1",description);
                    }
                }
            }
        });
        s_mp->setLogCallback([](int level, const std::string &str){
            LOGD( "MessagePipeline Log Level: %d, Message: %s", level, str.c_str());
        });
        s_mp->setIncomingMethodCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req);
            LOGD( "setRequestCallback Action_Method iid=%s", req.iid.c_str());
            // --- 遍历 Methods ---
            for (const auto& method : device.methods) {
                for (const auto& param_pair : method.params) {
                }
                BaseData baseData={PutTypeTool::METHOD(),req.iid,method.name, method.params};
                g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
            }
        });
        s_mp->setIncomingReadCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req);
            LOGD( "setRequestCallback Action_Read iid=%s", req.iid.c_str());
            // --- 遍历 Services ---
            for (const auto& service : device.services) {
                for (const auto& prop_pair : service.propertys) {
                    BaseData baseData={PutTypeTool::GETPERTIES(),req.iid,service.name/*+"-"+prop_pair.first*/, service.propertys};
                    g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
                }
            }
        });
        s_mp->setIncomingWriteCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req);
            LOGD( "setRequestCallback Action_Write iid=%s", req.iid.c_str());
            // --- 遍历 Services ---
            for (const auto& service : device.services) {
                for (const auto& prop_pair : service.propertys) {//network-net_type
                    BaseData baseData={PutTypeTool::SETPERTIES(),req.iid,service.name/*+"-"+prop_pair.first*/, service.propertys};
                    g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
                }
            }
        });
        s_mp->setIncomingEventCallback([env](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            RequestManager::getInstance().addRequest(req);
            // --- 遍历 Events ---
            for (const auto& event : device.events) {
                for (const auto& param_pair : event.params) {

                }
                LOGD( "setRequestCallback Action_Event eventName=%s", event.name.c_str());
                std::string description= iTools::getValue(event.params,"desc","");
                if (event.name=="iot_disconnect"){
                    g_i_mqtt_callback.callIotConnState(gJvm, false,description);
                    iot_connect_state_value= -1;
                }else if (event.name=="iot_connect"){
                    g_i_mqtt_callback.callIotConnState(gJvm, true,description);
                    iot_connect_state_value= 1;
                }else{
                    BaseData baseData={PutTypeTool::EVENT(),req.iid,event.name, event.params};
                    g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
                }
            }
        });
    }
    iTools::deleteLocalRefs(env,jhost,juser,jpass,jProtocol,xCoreBeanCls);
}

bool getInfomationListCallback(std::string action,std::map<std::string, ordered_json> params){
    std::string iid = s_mp->postMethod({{xcore_manifest.sn, {
                                               xcore_manifest.sn,
                                               xcore_manifest.product_id,
                                               {}, // services
                                               {{action,params}}, // methods
                                               {} // events
                                       }}
                                       },
                                       [action](const fs::p2p::Response &res, void *){
                                           LOGD("getInfomationDevsList>>iid>>%s,action>>%d",res.iid.c_str(),res.action);
                                           std::map<std::string, fs::p2p::Payload::Device> res_device_list=res.payload.devices;
                                           if (action=="fsp2p_devices"){
                                               std::vector<fs::p2p::InfomationManifest> infos;
                                               for (auto& device_pair : res_device_list) {
                                                   std::string device_sn = device_pair.first;
                                                   fs::p2p::Payload::Device& device = device_pair.second;
                                                   std::string pdid=device.product_id;
                                                   LOGD("getInfomationDevsList>>device_sn>>%s",device_sn.c_str());
                                                   fs::p2p::InfomationManifest infomationManifest;
                                                   infomationManifest.sn=device_sn;
                                                   infomationManifest.product_id=pdid;
                                                   infomationManifest.name="000";
                                                   infomationManifest.model="0";
                                                   infomationManifest.type=fs::p2p::InfomationManifest::Type::Unknown;
                                                   infomationManifest.version=1;
                                                   infos.push_back(infomationManifest);
                                               }
                                               infomationsCallback.callDevices(gJvm,infos);
                                           }else if (action=="get_iot_blacklist"){
                                               std::vector<BlackBean> beanList;
                                               for (auto& device_pair : res_device_list) {
                                                   std::string device_sn = device_pair.first;
                                                   fs::p2p::Payload::Device &device = device_pair.second;
                                                   for (auto& read : device.methods) {
                                                       BlackBean bean;
                                                       bean.desc= iTools::getValue(read.params,"desc");
                                                       bean.devices_array= iTools::getStringVector(read.params,"devices_array",{});
                                                       bean.model_array= iTools::getStringVector(read.params,"model_array",{});
                                                       std::string request_device_sn= iTools::getValue(read.params,"request_device_sn");
                                                       std::string update_time= iTools::getValue(read.params,"update_time");
                                                       LOGD("request_device_sn>>%s,update_time>>%s",request_device_sn.c_str(),update_time.c_str());
                                                       beanList.push_back(bean);
                                                   }

                                                   for (auto& read : device.events) {

                                                   }
                                                   for (auto& read : device.services) {

                                                   }
                                               }
                                               callGlobalBlackCallback(gJvm,beanList);
                                           }else if(action=="set_iot_blacklist"){
                                               LOGD("set_iot_blacklist>>success");
                                           }
                                       }, NULL,xcore_manifest.getSubscribeTopic());
    return !iid.empty();
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseFsP2pTools_getBlackList
        (JNIEnv* env, jclass, jobject icallback) {
    setGlobalBlackCallback(env,icallback);
    getInfomationListCallback("get_iot_blacklist",{});
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_setBlackList
        (JNIEnv* env, jclass, jobject in ) {
    std::map<std::string, ordered_json> params=iTools::javaMapToCppMapValue(env, in);
    return getInfomationListCallback("set_iot_blacklist",params);
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseFsP2pTools_getInfomationList
        (JNIEnv* env, jclass, jobject icallback) {
    infomationsCallback.set(env,icallback);
    getInfomationListCallback("fsp2p_devices",{});
}


JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_unSubscribe
        (JNIEnv* env, jclass, jobject information) {
    if (!information||!isP2pConnect){
        iTools::deleteLocalRefs(env,information);
        return false;
    }
    jclass manifestCls = env->GetObjectClass(information);
    if (!manifestCls) {
        iTools::deleteLocalRefs(env,information);
        return false;
    }
    jmethodID mid_getSn = env->GetMethodID(manifestCls, "getSn", "()Ljava/lang/String;");
    jmethodID mid_getProductId = env->GetMethodID(manifestCls, "getProductId", "()Ljava/lang/String;");
    jmethodID mid_getName = env->GetMethodID(manifestCls, "getName", "()Ljava/lang/String;");
    jmethodID mid_getModel = env->GetMethodID(manifestCls, "getModel", "()Ljava/lang/String;");
    jmethodID mid_getType = env->GetMethodID(manifestCls, "getType", "()Lcom/library/natives/Type;"); // 返回 Type 对象
    jmethodID mid_getVersion = env->GetMethodID(manifestCls, "getVersion", "()I");
    jstring jsn = mid_getSn ? (jstring)env->CallObjectMethod(information, mid_getSn) : nullptr;
    jstring jproductId = mid_getProductId ? (jstring)env->CallObjectMethod(information, mid_getProductId) : nullptr;
    jstring jname = mid_getName ? (jstring)env->CallObjectMethod(information, mid_getName) : nullptr;
    jstring jmodel = mid_getModel ? (jstring)env->CallObjectMethod(information, mid_getModel) : nullptr;
    jobject jTypeObject = mid_getType ? env->CallObjectMethod(information, mid_getType) : nullptr;
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
    fs::p2p::InfomationManifest target;
    target.sn = iTools::jstrToStd(env, jsn);
    target.product_id = iTools::jstrToStd(env, jproductId);
    target.name = iTools::jstrToStd(env, jname);
    target.model = iTools::jstrToStd(env, jmodel);
    target.type = static_cast<int>(jtype);
    target.version = static_cast<int>(jversion);
    int result= s_mp->unsubscribe(target);
    iTools::deleteLocalRefs(env,information,jsn,jproductId,jname,jmodel,manifestCls,jTypeObject);
    return result==0;
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_subscribe
        (JNIEnv* env, jclass, jobject information) {
    if (!information||!isP2pConnect){
        iTools::deleteLocalRefs(env,information);
        return false;
    }
    jclass manifestCls = env->GetObjectClass(information);
    if (!manifestCls) {
        iTools::deleteLocalRefs(env,information);
        return false;
    }
    jmethodID mid_getSn = env->GetMethodID(manifestCls, "getSn", "()Ljava/lang/String;");
    jmethodID mid_getProductId = env->GetMethodID(manifestCls, "getProductId", "()Ljava/lang/String;");
    jmethodID mid_getName = env->GetMethodID(manifestCls, "getName", "()Ljava/lang/String;");
    jmethodID mid_getModel = env->GetMethodID(manifestCls, "getModel", "()Ljava/lang/String;");
    jmethodID mid_getType = env->GetMethodID(manifestCls, "getType", "()Lcom/library/natives/Type;"); // 返回 Type 对象
    jmethodID mid_getVersion = env->GetMethodID(manifestCls, "getVersion", "()I");
    jstring jsn = mid_getSn ? (jstring)env->CallObjectMethod(information, mid_getSn) : nullptr;
    jstring jproductId = mid_getProductId ? (jstring)env->CallObjectMethod(information, mid_getProductId) : nullptr;
    jstring jname = mid_getName ? (jstring)env->CallObjectMethod(information, mid_getName) : nullptr;
    jstring jmodel = mid_getModel ? (jstring)env->CallObjectMethod(information, mid_getModel) : nullptr;
    jobject jTypeObject = mid_getType ? env->CallObjectMethod(information, mid_getType) : nullptr;
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

    fs::p2p::InfomationManifest target;
    target.sn = iTools::jstrToStd(env, jsn);
    target.product_id = iTools::jstrToStd(env, jproductId);
    target.name = iTools::jstrToStd(env, jname);
    target.model = iTools::jstrToStd(env, jmodel);
    target.type = static_cast<int>(jtype);
    target.version = static_cast<int>(jversion);
    int result= s_mp->subscribe(target,[env](const fs::p2p::Request &req) {
        LOGD("subscribe>>%s",req.iid.c_str());
    });
    if (result==0){
        SubscribeInfomation::getInstance().addManifest(target);
    }
    iTools::deleteLocalRefs(env,information,jsn,jproductId,jname,jmodel,manifestCls,jTypeObject);
    return result==0;
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_putIotReply
        (JNIEnv* env, jclass, jint i_put_type, jstring iid, jstring operation, jobject data_map,
         jint status_code, jstring status_desc)
{
    if (iot_connect_state_value!=1){
        iTools::deleteLocalRefs(env,iid,operation,data_map,status_desc);
        return false;
    }
    bool isComplete= false;
    BaseData baseData;
    baseData.iPutType=i_put_type;
    baseData.iid=iTools::jstrToStd(env, iid);
    baseData.operation=iTools::jstrToStd(env, operation);
    baseData.maps=iTools::javaMapToCppMapValue(env, data_map);
    std::string str_iid = iTools::jstrToStd(env, iid);
    std::list<fs::p2p::Request> requests=RequestManager::getInstance().getAllRequests(); // ✅ 保存
    for ( auto& r : requests) {
        if (r.iid==str_iid){
            std::map<std::string, fs::p2p::Payload::Device> res_device_list=r.payload.devices;
            for (auto& device_pair : res_device_list) {
                std::string device_sn = device_pair.first;
                fs::p2p::Payload::Device& device = device_pair.second;
                LOGD( "putCmd device_sn=%s", device_sn.c_str());

                int cppAction=iTools::convertToResponseAction(i_put_type);

                if (cppAction==fs::p2p::Response::Action::Action_Method){
                    LOGD( "putCmd Action_Method iid=%s", r.iid.c_str());
                    // --- 遍历 Methods ---
                    for (auto& read : device.methods) {
                        std::map<std::string, ordered_json> newMaps = iTools::javaMapToCppMapValue(env, data_map);
                        read.params = newMaps;
                        read.reason_code=status_code;
                        read.reason_string=iTools::jstrToStd(env, status_desc);
                    }
                    int result=s_mp->response(r,res_device_list);
                    isComplete=result==0;
                    if (isComplete){
                        g_i_mqtt_callback.callPushed(gJvm,baseData);
                    }else{
                        g_i_mqtt_callback.callPushFail(gJvm,baseData,"putCmd failed");
                    }
                }else if(cppAction==fs::p2p::Response::Action::Action_Read){
                    LOGD( "putCmd Action_Method iid=%s", r.iid.c_str());
                    // --- 遍历 Methods ---
                    for (auto& read : device.services) {
                        std::map<std::string, ordered_json> newMaps = iTools::javaMapToCppMapValue(env, data_map);
                        read.propertys = newMaps;
                        read.reason_code=status_code;
                        read.reason_string=iTools::jstrToStd(env, status_desc);
                    }
                    int result=s_mp->response(r,res_device_list);
                    isComplete=result==0;
                    if (isComplete){
                        g_i_mqtt_callback.callPushed(gJvm,baseData);
                    }else{
                        g_i_mqtt_callback.callPushFail(gJvm,baseData,"putCmd failed");
                    }
                }else if(cppAction==fs::p2p::Response::Action::Action_Write){
                    LOGD( "putCmd Action_Method iid=%s", r.iid.c_str());
                    // --- 遍历 Methods ---
                    for (auto& write : device.services) {
                        std::map<std::string, ordered_json> newMaps = iTools::javaMapToCppMapValue(env, data_map);
                        write.propertys = newMaps;
                        write.reason_code=status_code;
                        write.reason_string=iTools::jstrToStd(env, status_desc);
                    }
                    int result=s_mp->response(r,res_device_list);
                    isComplete=result==0;
                    if (isComplete){
                        g_i_mqtt_callback.callPushed(gJvm,baseData);
                    }else{
                        g_i_mqtt_callback.callPushFail(gJvm,baseData,"putCmd failed");
                    }
                }else{
                    LOGD( "putCmd Action_Unknown iid=%s", r.iid.c_str());

                }
            }
        }
    }
    iTools::deleteLocalRefs(env,iid,operation,data_map,status_desc);
    return isComplete;
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_postMsg
        (JNIEnv* env, jclass, jint i_put_type , jstring target_sn, jstring p_did , jstring jnode, jobject jparams )
{
    bool isComplete= false;
    std::string targetSnStr = iTools::jstrToStd(env, target_sn);
    std::string pdidStr = iTools::jstrToStd(env, p_did);
    jstring jreason_str = env->NewStringUTF("");
    std::map<std::string, fs::p2p::Payload::Device> list;
    fs::p2p::Payload::Device fdevice;
    fdevice.sn = targetSnStr;
    fdevice.product_id = pdidStr;
    int cppAction=iTools::convertToRequestAction(i_put_type);
    std::string iid;

    BaseData baseData;
    baseData.iPutType=i_put_type;
    baseData.operation=iTools::jstrToStd(env, jnode);
    baseData.maps=iTools::javaMapToCppMapValue(env, jparams);
    if (cppAction==fs::p2p::Request::Action::Action_Method){
        fs::p2p::Method method = iTools::convertToMethod(env, jnode, jparams, 0, jreason_str);
        fdevice.methods.push_back(method);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postMethod(list,
                                   [](const fs::p2p::Response &res, void *){
                                       LOGD("postMethod>>iid>>%s,action>>%d",res.iid.c_str(),res.action);
                }, NULL,"");
            isComplete=!iid.empty();
            if(isComplete){
                g_i_mqtt_callback.callPushed(gJvm,baseData);
            }else{
                g_i_mqtt_callback.callPushFail(gJvm,baseData,"postMethod failed");
            }
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Read){
        std::vector<std::string> parts = iTools::splitJString(env, jnode);
        fs::p2p::Service service = iTools::convertToService(env, env->NewStringUTF(parts[0].c_str()), jparams, 0, jreason_str);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postRead(list,[](const fs::p2p::Response &res, void *)
            {}, NULL,"");
            baseData.iid=iid;
            isComplete=!iid.empty();
            if(isComplete){
                g_i_mqtt_callback.callPushed(gJvm,baseData);
            }else{
                g_i_mqtt_callback.callPushFail(gJvm,baseData,"postRead failed");
            }
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Write){
        std::vector<std::string> parts = iTools::splitJString(env, jnode);
        fs::p2p::Service service = iTools::convertToService(env, env->NewStringUTF(parts[0].c_str()), jparams, 0, jreason_str);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postWrite(list,[](const fs::p2p::Response &res, void *)
            {}, NULL,"");
            baseData.iid=iid;
            isComplete=!iid.empty();
            if(isComplete){
                g_i_mqtt_callback.callPushed(gJvm,baseData);
            }else{
                g_i_mqtt_callback.callPushFail(gJvm,baseData,"postWrite failed");
            }
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Event){
        fs::p2p::Event event = iTools::convertToEvent(env, jnode, jparams, 0, jreason_str);
        fdevice.events.push_back(event);
        list[targetSnStr] = fdevice;

        if (s_mp) {
            iid = s_mp->postEvent(list,[](const fs::p2p::Response &res, void *)
            {}, NULL);
            baseData.iid=iid;
            isComplete=!iid.empty();
            if(isComplete){
                g_i_mqtt_callback.callPushed(gJvm,baseData);
            }else{
                g_i_mqtt_callback.callPushFail(gJvm,baseData,"postEvent failed");
            }
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Notify){
        std::vector<std::string> parts = iTools::splitJString(env, jnode);
        fs::p2p::Service service = iTools::convertToService(env, env->NewStringUTF(parts[0].c_str()), jparams, 0, jreason_str);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postNotify(list);
            baseData.iid=iid;
            isComplete=!iid.empty();
            if(isComplete){
                g_i_mqtt_callback.callPushed(gJvm,baseData);
            }else{
                g_i_mqtt_callback.callPushFail(gJvm,baseData,"postNotify failed");
            }
        }
    }else if(cppAction==fs::p2p::Request::Action::Action_Broadcast){
        fs::p2p::Event event = iTools::convertToEvent(env, jnode, jparams, 0, jreason_str);
        fdevice.events.push_back(event);
        list[targetSnStr] = fdevice;
        if (s_mp) {
            iid = s_mp->postBroadcast(list);
            baseData.iid=iid;
            isComplete=!iid.empty();
            if(isComplete){
                g_i_mqtt_callback.callPushed(gJvm,baseData);
            }else{
                g_i_mqtt_callback.callPushFail(gJvm,baseData,"postBroadcast failed");
            }
        }
    }
    iTools::deleteLocalRefs(env,jreason_str,target_sn,p_did,jnode,jparams);
    return isComplete;
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseFsP2pTools_disConnect
        (JNIEnv* env, jclass)
{
    std::lock_guard<std::mutex> lk(s_mp_mutex);
    if (s_mp) {
        s_mp->close();
    }
    g_i_mqtt_callback.clear(env);
    infomationsCallback.clear(env);
    g_timer.stop();
    iot_connect_state_value= -1;
    isP2pConnect= false;
}

} // extern "C"