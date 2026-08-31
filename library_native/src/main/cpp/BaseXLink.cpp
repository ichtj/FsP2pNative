
#include <jni.h>
#include <chrono>
#include <condition_variable>
#include <atomic>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
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

IInfomationsCallback infomationsCallback;
static IBlackCallback* iblackcall = nullptr;
JavaVM* gJvm = nullptr;

namespace {

enum class ConnectionState : int {
    Disconnected,
    Connecting,
    Connected,
    Stopping,
};

// The prebuilt SDK destroys its packetizer before joining MQTT. Keep the
// storage itself process-scoped so a failed close is retained instead of
// being released later by static destruction.
std::shared_ptr<fs::p2p::MessagePipeline>& s_mp =
        *new std::shared_ptr<fs::p2p::MessagePipeline>();
std::mutex s_lifecycleMutex;
std::mutex s_mp_mutex;
std::condition_variable s_mp_idle;
size_t s_mp_operations = 0;
bool s_mp_accepting = false;
std::atomic<ConnectionState> s_connectionState{ConnectionState::Disconnected};
std::atomic<int64_t> s_connectStartedAtMs{0};
std::atomic<int> iot_connect_state_value{0};
std::atomic<bool> isIotSubscribed{false};
std::atomic<bool> isIotSubscriptionPending{false};
std::mutex s_iotMutex;
Timer g_timer;
PipelineCallback g_i_mqtt_callback;
fs::p2p::InfomationManifest xcore_manifest;
thread_local int s_pipelineCallbackDepth = 0;
thread_local int s_pipelineLeaseDepth = 0;

constexpr int64_t kConnectTimeoutMs = 45 * 1000;

int64_t monotonicTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
}

class PipelineLease {
public:
    PipelineLease() {
        std::lock_guard<std::mutex> lock(s_mp_mutex);
        if (!s_mp_accepting || !s_mp ||
            s_connectionState.load() != ConnectionState::Connected) return;
        pipeline = s_mp;
        ++s_mp_operations;
        ++s_pipelineLeaseDepth;
    }

    ~PipelineLease() {
        if (!pipeline) return;
        pipeline.reset();
        --s_pipelineLeaseDepth;
        std::lock_guard<std::mutex> lock(s_mp_mutex);
        if (s_mp_operations > 0) --s_mp_operations;
        if (s_mp_operations == 0) s_mp_idle.notify_all();
    }

    PipelineLease(const PipelineLease&) = delete;
    PipelineLease& operator=(const PipelineLease&) = delete;

    explicit operator bool() const { return pipeline != nullptr; }
    fs::p2p::MessagePipeline* operator->() const { return pipeline.get(); }
    static bool isActiveOnCurrentThread() { return s_pipelineLeaseDepth > 0; }

private:
    std::shared_ptr<fs::p2p::MessagePipeline> pipeline;
};

class PipelineCallbackScope {
public:
    PipelineCallbackScope() { ++s_pipelineCallbackDepth; }
    ~PipelineCallbackScope() { --s_pipelineCallbackDepth; }

    PipelineCallbackScope(const PipelineCallbackScope&) = delete;
    PipelineCallbackScope& operator=(const PipelineCallbackScope&) = delete;

    static bool isActive() { return s_pipelineCallbackDepth > 0; }
};

void rejectPipelineOperations() {
    s_connectionState.store(ConnectionState::Stopping);
    std::lock_guard<std::mutex> lock(s_mp_mutex);
    s_mp_accepting = false;
}

bool stopPipeline() {
    rejectPipelineOperations();
    g_timer.stop();

    std::shared_ptr<fs::p2p::MessagePipeline> pipeline;
    {
        std::unique_lock<std::mutex> lock(s_mp_mutex);
        s_mp_accepting = false;
        s_mp_idle.wait(lock, []() { return s_mp_operations == 0; });
        pipeline = s_mp;
    }
    // A connect callback that already held a lease may have restarted the timer
    // after the first stop. No new lease can start once accepting is false.
    g_timer.stop();

    // close() joins the MQTT worker before MessagePipeline clears its packetizer.
    bool closed = true;
    if (pipeline) {
        try {
            pipeline->close();
        } catch (const std::exception& error) {
            LOGE("P2P close failed: %s", error.what());
            closed = false;
        } catch (...) {
            LOGE("P2P close failed with an unknown native exception");
            closed = false;
        }
    }

    if (!closed) {
        // Keep the object alive. Its destructor clears the packetizer before joining MQTT.
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(s_mp_mutex);
        if (s_mp == pipeline) s_mp.reset();
    }
    s_connectionState.store(ConnectionState::Disconnected);
    s_connectStartedAtMs.store(0);
    iot_connect_state_value.store(-1);
    isIotSubscribed.store(false);
    isIotSubscriptionPending.store(false);
    return true;
}

class LifecycleExecutor {
public:
    LifecycleExecutor() : worker([this]() { run(); }) {}

    ~LifecycleExecutor() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            shuttingDown = true;
        }
        condition.notify_all();
        if (worker.joinable()) worker.join();
    }

    void requestStop() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopRequested = true;
        }
        condition.notify_one();
    }

    void waitUntilIdle() {
        std::unique_lock<std::mutex> lock(mutex);
        idleCondition.wait(lock, [this]() {
            return !stopRequested && !processingStop;
        });
    }

private:
    void run() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(mutex);
                condition.wait(lock, [this]() {
                    return shuttingDown || stopRequested;
                });
                if (shuttingDown) return;
                stopRequested = false;
                processingStop = true;
            }

            {
                std::lock_guard<std::mutex> lifecycleLock(s_lifecycleMutex);
                stopPipeline();
            }

            {
                std::lock_guard<std::mutex> lock(mutex);
                processingStop = false;
            }
            idleCondition.notify_all();
        }
    }

    std::mutex mutex;
    std::condition_variable condition;
    std::condition_variable idleCondition;
    bool shuttingDown = false;
    bool stopRequested = false;
    bool processingStop = false;
    std::thread worker;
};

LifecycleExecutor s_lifecycleExecutor;

} // namespace


JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
    gJvm = vm;
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void*) {
    JNIEnv* env = nullptr;
    bool attached = false;
    if (vm && vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
        if (vm->AttachCurrentThread(&env, nullptr) == JNI_OK) {
            attached = true;
        }
    }

    s_lifecycleExecutor.waitUntilIdle();
    {
        std::lock_guard<std::mutex> lifecycleLock(s_lifecycleMutex);
        if (!stopPipeline()) {
            LOGE("Pipeline did not stop safely during JNI unload");
        }
    }

    if (env) {
        g_i_mqtt_callback.clear(env);
        infomationsCallback.clear(env);
        clearGlobalBlackCallback(env);
        PutTypeTool::release(env);
    }
    RequestManager::getInstance().clearAll();
    gJvm = nullptr;

    if (attached) vm->DetachCurrentThread();
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
    const ConnectionState state = s_connectionState.load();
    if (state == ConnectionState::Connected) return true;
    if (state != ConnectionState::Connecting) return false;

    const int64_t startedAt = s_connectStartedAtMs.load();
    if (startedAt <= 0 || monotonicTimeMs() - startedAt <= kConnectTimeoutMs) {
        return true;
    }

    ConnectionState expected = ConnectionState::Connecting;
    if (s_connectionState.compare_exchange_strong(
            expected, ConnectionState::Disconnected)) {
        LOGE("P2P connection timed out after %lld ms",
             static_cast<long long>(kConnectTimeoutMs));
        return false;
    }
    return expected == ConnectionState::Connecting || expected == ConnectionState::Connected;
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseFsP2pTools_connect
        (JNIEnv* env, jclass , jobject information, jobject xCoreBean,jstring jProtocol,
         jobject i_pipeline_callback)
{
    if (!env || !information || !xCoreBean || !i_pipeline_callback) return;

    if (PipelineCallbackScope::isActive() ||
        PipelineLease::isActiveOnCurrentThread()) {
        LOGE("Ignoring reentrant connect from a pipeline operation");
        return;
    }

    s_lifecycleExecutor.waitUntilIdle();

    std::lock_guard<std::mutex> lifecycleLock(s_lifecycleMutex);
    const ConnectionState currentState = s_connectionState.load();
    if (currentState == ConnectionState::Connecting ||
        currentState == ConnectionState::Connected) {
        g_i_mqtt_callback.set(env, i_pipeline_callback);
        const bool connected = currentState == ConnectionState::Connected;
        PipelineCallbackScope callbackScope;
        g_i_mqtt_callback.callP2pConnState(
                gJvm, connected, connected ? "Connected" : "Connecting");
        if (iot_connect_state_value.load() == 1) {
            g_i_mqtt_callback.callIotConnState(gJvm, true, "Connected");
        }
        return;
    }

    jclass xCoreBeanCls = env->GetObjectClass(xCoreBean);
    if (!xCoreBeanCls) {
        if (env->ExceptionCheck()) env->ExceptionClear();
        return;
    }
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
        iTools::deleteLocalRefs(env,jhost,juser,jpass,xCoreBeanCls);
        return ;
    }
    fs::p2p::InfomationManifest manifest = iTools::convertToCppInfomation(env, information);
    const std::string host = iTools::jstrToStd(env, jhost);
    const unsigned short port = static_cast<unsigned short>(jport);
    const std::string userName = iTools::jstrToStd(env, juser);
    const std::string passWord = iTools::jstrToStd(env, jpass);

    iTools::deleteLocalRefs(env,jhost,juser,jpass,xCoreBeanCls);

    if (!stopPipeline()) {
        LOGE("Cannot reconnect because the previous pipeline did not stop safely");
        return;
    }
    g_i_mqtt_callback.set(env, i_pipeline_callback);
    PutTypeTool::init(gJvm);

    try {
        auto pipeline = std::make_shared<fs::p2p::MessagePipeline>(manifest);
        {
            std::lock_guard<std::mutex> lock(s_mp_mutex);
            s_mp = pipeline;
            s_mp_accepting = true;
        }
        s_connectionState.store(ConnectionState::Connecting);
        s_connectStartedAtMs.store(monotonicTimeMs());
        iot_connect_state_value.store(0);
        isIotSubscribed.store(false);
        isIotSubscriptionPending.store(false);

        pipeline->setConnectStateCallback([](bool isConnected){
            PipelineCallbackScope callbackScope;
            if (s_connectionState.load() == ConnectionState::Stopping) return;
            s_connectionState.store(isConnected
                    ? ConnectionState::Connected
                    : ConnectionState::Disconnected);
            s_connectStartedAtMs.store(0);
            g_i_mqtt_callback.callP2pConnState(gJvm,isConnected,"connection state changed5");
            if (isConnected) {
                PipelineLease pipelineLease;
                if (!pipelineLease) return;
                std::string iid1=pipelineLease->postStartup();
                LOGD( "setConnectStateCallback iid1=%s",iid1.c_str());
                g_timer.start(1*60*1000, []() {
                    PipelineLease heartbeatPipeline;
                    if (!heartbeatPipeline) return;
                    std::string iid2 = heartbeatPipeline->postHeartbeat();
                    LOGD( "setConnectStateCallback iid2=%s",iid2.c_str());
                });
            }else{
                g_timer.stop();
            }
            LOGD("setConnectStateCallback isConnected=%d",isConnected);
        });
        pipeline->setDeviceHeartbeatCallback([protocol](const fs::p2p::InfomationManifest &info) {
            PipelineCallbackScope callbackScope;
            if (s_connectionState.load() == ConnectionState::Stopping) return;
            LOGD("setDeviceHeartbeatCallback>>%s", info.model.c_str());
        });
        pipeline->setDeviceStartupCallback([](const fs::p2p::InfomationManifest &info) {
            PipelineCallbackScope callbackScope;
            if (s_connectionState.load() == ConnectionState::Stopping) return;
            // xcore是云边同步的模型名称，需要往这里注入物模型，使product_id和物模型绑定
            LOGD("setDeviceStartupCallback>>%s", info.model.c_str());
        });
        pipeline->setErrorCallback([](int error_code, const std::string &error_string) {
            PipelineCallbackScope callbackScope;
            if (s_connectionState.load() == ConnectionState::Stopping) return;
            LOGD( "Error Code: %d, Description: %s", error_code, error_string.c_str());
        });
        pipeline->setBroadcastCallback([protocol](const fs::p2p::Request &req) {
            PipelineCallbackScope callbackScope;
            PipelineLease pipelineLease;
            if (!pipelineLease) return;
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
                        fs::p2p::InfomationManifest target;
                        {
                            std::lock_guard<std::mutex> lock(s_iotMutex);
                            xcore_manifest.sn = device_sn;
                            xcore_manifest.product_id = device.product_id;
                            target = xcore_manifest;
                        }

                        bool expectedPending = false;
                        if (!isIotSubscribed.load() &&
                            isIotSubscriptionPending.compare_exchange_strong(
                                    expectedPending, true)) {
                            std::string topic=target.getPublishTopic();
                            const fs::p2p::InfomationManifest source =
                                    pipelineLease->infomationManifest();
                            std::string iid=pipelineLease->postMethod(
                                {{target.sn, {
                                        target.sn,
                                        target.product_id,
                                            {}, // services
                                            {
                                                    {
                                                            "iot_json_protocol",
                                                            {
                                                                    {"app_sn",
                                                                     source.sn},
                                                                    {"product_id",
                                                                     source.product_id},
                                                                    {"json_protocol",
                                                                     protocol}
                                                            }
                                                    }
                                            }, // methods
                                            {} // events
                                    }}
                                    }, // std::map<std::string, Payload::Device>
                        [](const fs::p2p::Response &, void *){},
                                    nullptr, target.getSubscribeTopic());
                            int result=pipelineLease->subscribe(target, nullptr);
                            LOGD("setBroadcastCallback>>subscribe>>%d,iid>>%s",result,iid.c_str());
                            if (result==0){
                                g_i_mqtt_callback.callSubscribed(gJvm,topic);
                                isIotSubscribed.store(true);
                                SubscribeInfomation::getInstance().addManifest(target);
                            }else{
                                isIotSubscribed.store(false);
                                g_i_mqtt_callback.callSubscribeFail(gJvm,topic,"subscribe failed");
                            }
                            isIotSubscriptionPending.store(false);
                        }
                        std::string state= iTools::getValue(method.params,"state","0");
                        std::string description= iTools::getValue(method.params,"desc","");
                        iot_connect_state_value.store((state=="1")?1:-1);
                        g_i_mqtt_callback.callIotConnState(gJvm,state=="1",description);
                    }
                }
            }
        });
        pipeline->setLogCallback([](int level, const std::string &str){
            PipelineCallbackScope callbackScope;
            LOGD( "MessagePipeline Log Level: %d, Message: %s", level, str.c_str());
        });
        pipeline->setIncomingMethodCallback([](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            PipelineCallbackScope callbackScope;
            PipelineLease pipelineLease;
            if (!pipelineLease) return;
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
        pipeline->setIncomingReadCallback([](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            PipelineCallbackScope callbackScope;
            PipelineLease pipelineLease;
            if (!pipelineLease) return;
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
        pipeline->setIncomingWriteCallback([](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            PipelineCallbackScope callbackScope;
            PipelineLease pipelineLease;
            if (!pipelineLease) return;
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
        pipeline->setIncomingEventCallback([](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            PipelineCallbackScope callbackScope;
            PipelineLease pipelineLease;
            if (!pipelineLease) return;
            RequestManager::getInstance().addRequest(req);
            // --- 遍历 Events ---
            for (const auto& event : device.events) {
                for (const auto& param_pair : event.params) {

                }
                LOGD( "setRequestCallback Action_Event eventName=%s", event.name.c_str());
                std::string description= iTools::getValue(event.params,"desc","");
                if (event.name=="iot_disconnect"){
                    g_i_mqtt_callback.callIotConnState(gJvm, false,description);
                    iot_connect_state_value.store(-1);
                }else if (event.name=="iot_connect"){
                    g_i_mqtt_callback.callIotConnState(gJvm, true,description);
                    iot_connect_state_value.store(1);
                }else{
                    BaseData baseData={PutTypeTool::EVENT(),req.iid,event.name, event.params};
                    g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
                }
            }
        });

        // open() may report the connection immediately, so callbacks must exist first.
        pipeline->open(host, port, userName, passWord);
    } catch (const std::exception& error) {
        LOGE("P2P connect failed: %s", error.what());
        PipelineCallbackScope callbackScope;
        g_i_mqtt_callback.callP2pConnState(gJvm, false, error.what());
        if (!stopPipeline()) {
            LOGE("Failed to stop pipeline after connect exception");
        }
    } catch (...) {
        LOGE("P2P connect failed with an unknown native exception");
        PipelineCallbackScope callbackScope;
        g_i_mqtt_callback.callP2pConnState(
                gJvm, false, "unknown native exception");
        if (!stopPipeline()) {
            LOGE("Failed to stop pipeline after unknown connect exception");
        }
    }
}

bool getInfomationListCallback(std::string action,std::map<std::string, ordered_json> params){
    PipelineLease pipelineLease;
    if (!pipelineLease) return false;

    fs::p2p::InfomationManifest target;
    {
        std::lock_guard<std::mutex> lock(s_iotMutex);
        target = xcore_manifest;
    }
    if (target.sn.empty()) return false;

    std::string iid = pipelineLease->postMethod({{target.sn, {
                                               target.sn,
                                               target.product_id,
                                               {}, // services
                                               {{action,params}}, // methods
                                               {} // events
                                       }}
                                       },
                                       [action](const fs::p2p::Response &res, void *){
                                           PipelineCallbackScope callbackScope;
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
                                       }, nullptr,target.getSubscribeTopic());
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
    PipelineLease pipelineLease;
    if (!information || !pipelineLease){
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
    int result= pipelineLease->unsubscribe(target);
    iTools::deleteLocalRefs(env,information,jsn,jproductId,jname,jmodel,manifestCls,jTypeObject);
    return result==0;
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_subscribe
        (JNIEnv* env, jclass, jobject information) {
    PipelineLease pipelineLease;
    if (!information || !pipelineLease){
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
    int result= pipelineLease->subscribe(target,[](const fs::p2p::Request &req) {
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
    PipelineLease pipelineLease;
    if (iot_connect_state_value.load()!=1 || !pipelineLease){
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
                    int result=pipelineLease->response(r,res_device_list);
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
                    int result=pipelineLease->response(r,res_device_list);
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
                    int result=pipelineLease->response(r,res_device_list);
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
    PipelineLease pipelineLease;
    if (!env || !target_sn || !p_did || !jnode || !jparams || !pipelineLease) {
        iTools::deleteLocalRefs(env, target_sn, p_did, jnode, jparams);
        return false;
    }

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
        if (pipelineLease) {
            iid = pipelineLease->postMethod(list,
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
        if (parts.empty()) {
            iTools::deleteLocalRefs(env,jreason_str,target_sn,p_did,jnode,jparams);
            return false;
        }
        jstring serviceName = env->NewStringUTF(parts.front().c_str());
        fs::p2p::Service service = iTools::convertToService(env, serviceName, jparams, 0, jreason_str);
        iTools::deleteLocalRefs(env, serviceName);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (pipelineLease) {
            iid = pipelineLease->postRead(list,[](const fs::p2p::Response &res, void *)
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
        if (parts.empty()) {
            iTools::deleteLocalRefs(env,jreason_str,target_sn,p_did,jnode,jparams);
            return false;
        }
        jstring serviceName = env->NewStringUTF(parts.front().c_str());
        fs::p2p::Service service = iTools::convertToService(env, serviceName, jparams, 0, jreason_str);
        iTools::deleteLocalRefs(env, serviceName);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (pipelineLease) {
            iid = pipelineLease->postWrite(list,[](const fs::p2p::Response &res, void *)
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

        if (pipelineLease) {
            iid = pipelineLease->postEvent(list,[](const fs::p2p::Response &res, void *)
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
        if (parts.empty()) {
            iTools::deleteLocalRefs(env,jreason_str,target_sn,p_did,jnode,jparams);
            return false;
        }
        jstring serviceName = env->NewStringUTF(parts.front().c_str());
        fs::p2p::Service service = iTools::convertToService(env, serviceName, jparams, 0, jreason_str);
        iTools::deleteLocalRefs(env, serviceName);
        fdevice.services.push_back(service);
        list[targetSnStr] = fdevice;
        if (pipelineLease) {
            iid = pipelineLease->postNotify(list);
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
        if (pipelineLease) {
            iid = pipelineLease->postBroadcast(list);
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
    if (PipelineCallbackScope::isActive() ||
        PipelineLease::isActiveOnCurrentThread()) {
        s_lifecycleExecutor.requestStop();
        rejectPipelineOperations();
        g_i_mqtt_callback.clear(env);
        infomationsCallback.clear(env);
        clearGlobalBlackCallback(env);
        RequestManager::getInstance().clearAll();
        return;
    }

    std::lock_guard<std::mutex> lifecycleLock(s_lifecycleMutex);
    if (!stopPipeline()) {
        LOGE("Disconnect did not complete safely");
        return;
    }
    g_i_mqtt_callback.clear(env);
    infomationsCallback.clear(env);
    clearGlobalBlackCallback(env);
    RequestManager::getInstance().clearAll();
}

} // extern "C"
