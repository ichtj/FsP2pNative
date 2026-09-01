
#include <jni.h>
#include <chrono>
#include <condition_variable>
#include <atomic>
#include <cstdint>
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
std::atomic<uint64_t> s_connectRequestSequence{0};
std::atomic<uint64_t> s_connectionSequence{0};
std::atomic<uint64_t> s_activeConnectionId{0};
std::mutex s_iotMutex;
Timer g_timer;
PipelineCallback g_i_mqtt_callback;
fs::p2p::InfomationManifest xcore_manifest;
thread_local int s_pipelineCallbackDepth = 0;
thread_local int s_pipelineLeaseDepth = 0;

constexpr int64_t kConnectTimeoutMs = 45 * 1000;

const char* connectionStateName(ConnectionState state) {
    switch (state) {
        case ConnectionState::Disconnected:
            return "Disconnected";
        case ConnectionState::Connecting:
            return "Connecting";
        case ConnectionState::Connected:
            return "Connected";
        case ConnectionState::Stopping:
            return "Stopping";
    }
    return "Unknown";
}

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
    LOGD("[FsP2pDiag][Lifecycle] reject operations connectionId=%llu state=%s",
         static_cast<unsigned long long>(s_activeConnectionId.load()),
         connectionStateName(s_connectionState.load()));
    s_connectionState.store(ConnectionState::Stopping);
    std::lock_guard<std::mutex> lock(s_mp_mutex);
    s_mp_accepting = false;
}

bool stopPipeline() {
    const uint64_t connectionId = s_activeConnectionId.load();
    LOGD("[FsP2pDiag][Lifecycle] stop begin connectionId=%llu state=%s iotState=%d subscribed=%d pending=%d",
         static_cast<unsigned long long>(connectionId),
         connectionStateName(s_connectionState.load()),
         iot_connect_state_value.load(),
         isIotSubscribed.load(),
         isIotSubscriptionPending.load());
    rejectPipelineOperations();
    g_timer.stop();

    std::shared_ptr<fs::p2p::MessagePipeline> pipeline;
    {
        std::unique_lock<std::mutex> lock(s_mp_mutex);
        s_mp_accepting = false;
        LOGD("[FsP2pDiag][Lifecycle] waiting for operations connectionId=%llu activeOperations=%zu hasPipeline=%d",
             static_cast<unsigned long long>(connectionId),
             s_mp_operations,
             s_mp ? 1 : 0);
        s_mp_idle.wait(lock, []() { return s_mp_operations == 0; });
        pipeline = s_mp;
    }
    // A connect callback that already held a lease may have restarted the timer
    // after the first stop. No new lease can start once accepting is false.
    g_timer.stop();

    // close() joins the MQTT worker before MessagePipeline clears its packetizer.
    bool closed = true;
    if (pipeline) {
        LOGD("[FsP2pDiag][Lifecycle] pipeline close begin connectionId=%llu",
             static_cast<unsigned long long>(connectionId));
        try {
            pipeline->close();
            LOGD("[FsP2pDiag][Lifecycle] pipeline close complete connectionId=%llu",
                 static_cast<unsigned long long>(connectionId));
        } catch (const std::exception& error) {
            LOGE("P2P close failed: %s", error.what());
            LOGE("[FsP2pDiag][Lifecycle] pipeline close exception connectionId=%llu error=%s",
                 static_cast<unsigned long long>(connectionId), error.what());
            closed = false;
        } catch (...) {
            LOGE("P2P close failed with an unknown native exception");
            LOGE("[FsP2pDiag][Lifecycle] pipeline close unknown exception connectionId=%llu",
                 static_cast<unsigned long long>(connectionId));
            closed = false;
        }
    }

    if (!closed) {
        // Keep the object alive. Its destructor clears the packetizer before joining MQTT.
        LOGE("[FsP2pDiag][Lifecycle] stop incomplete; pipeline retained connectionId=%llu",
             static_cast<unsigned long long>(connectionId));
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
    s_activeConnectionId.store(0);
    LOGD("[FsP2pDiag][Lifecycle] stop complete connectionId=%llu state=%s",
         static_cast<unsigned long long>(connectionId),
         connectionStateName(s_connectionState.load()));
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
        LOGD("[FsP2pDiag][Lifecycle] asynchronous stop requested connectionId=%llu",
             static_cast<unsigned long long>(s_activeConnectionId.load()));
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

            LOGD("[FsP2pDiag][Lifecycle] asynchronous stop executing connectionId=%llu",
                 static_cast<unsigned long long>(s_activeConnectionId.load()));
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
    LOGI("[FsP2pDiag][JNI] library loaded");
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM* vm, void*) {
    LOGI("[FsP2pDiag][JNI] library unload begin connectionId=%llu state=%s",
         static_cast<unsigned long long>(s_activeConnectionId.load()),
         connectionStateName(s_connectionState.load()));
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
    LOGI("[FsP2pDiag][JNI] library unload complete");

    if (attached) vm->DetachCurrentThread();
}

extern "C" {

JNIEXPORT void JNICALL
Java_com_library_natives_BaseFsP2pTools_logEnable(JNIEnv *env, jclass clz,
                                                   jboolean isEnable) {
    setLoggingEnabled(isEnable);
    if (isEnable) {
        LOGI("[FsP2pDiag][Logging] native logging enabled");
    }
}

JNIEXPORT jboolean JNICALL
Java_com_library_natives_BaseFsP2pTools_isLogEnable(JNIEnv *env, jclass clz) {
    return getLoggingEnabled();
}

JNIEXPORT jboolean JNICALL Java_com_library_natives_BaseFsP2pTools_getConnectStatus
        (JNIEnv* env, jclass /*clazz*/)
{
    const ConnectionState state = s_connectionState.load();
    const uint64_t connectionId = s_activeConnectionId.load();
    if (state == ConnectionState::Connected) {
        LOGD("[FsP2pDiag][StatusQuery] connectionId=%llu state=%s result=true iotState=%d subscribed=%d pending=%d",
             static_cast<unsigned long long>(connectionId),
             connectionStateName(state),
             iot_connect_state_value.load(),
             isIotSubscribed.load(),
             isIotSubscriptionPending.load());
        return true;
    }
    if (state != ConnectionState::Connecting) {
        LOGD("[FsP2pDiag][StatusQuery] connectionId=%llu state=%s result=false iotState=%d subscribed=%d pending=%d",
             static_cast<unsigned long long>(connectionId),
             connectionStateName(state),
             iot_connect_state_value.load(),
             isIotSubscribed.load(),
             isIotSubscriptionPending.load());
        return false;
    }

    const int64_t startedAt = s_connectStartedAtMs.load();
    const int64_t elapsedMs = startedAt > 0 ? monotonicTimeMs() - startedAt : -1;
    if (startedAt <= 0 || elapsedMs <= kConnectTimeoutMs) {
        LOGD("[FsP2pDiag][StatusQuery] connectionId=%llu state=%s result=true connectElapsedMs=%lld",
             static_cast<unsigned long long>(connectionId),
             connectionStateName(state),
             static_cast<long long>(elapsedMs));
        return true;
    }

    ConnectionState expected = ConnectionState::Connecting;
    if (s_connectionState.compare_exchange_strong(
            expected, ConnectionState::Disconnected)) {
        LOGE("P2P connection timed out after %lld ms",
             static_cast<long long>(kConnectTimeoutMs));
        LOGE("[FsP2pDiag][StatusQuery] connectionId=%llu state=Connecting result=false reason=timeout connectElapsedMs=%lld",
             static_cast<unsigned long long>(connectionId),
             static_cast<long long>(elapsedMs));
        return false;
    }
    const bool result = expected == ConnectionState::Connecting ||
                        expected == ConnectionState::Connected;
    LOGD("[FsP2pDiag][StatusQuery] connectionId=%llu stateAfterRace=%s result=%d connectElapsedMs=%lld",
         static_cast<unsigned long long>(connectionId),
         connectionStateName(expected),
         result,
         static_cast<long long>(elapsedMs));
    return result;
}

JNIEXPORT void JNICALL Java_com_library_natives_BaseFsP2pTools_connect
        (JNIEnv* env, jclass , jobject information, jobject xCoreBean,jstring jProtocol,
         jobject i_pipeline_callback)
{
    const uint64_t requestId = s_connectRequestSequence.fetch_add(1) + 1;
    const int64_t requestStartedAtMs = monotonicTimeMs();
    const bool inPipelineCallback = PipelineCallbackScope::isActive();
    const bool hasPipelineLease = PipelineLease::isActiveOnCurrentThread();
    LOGI("[FsP2pDiag][Connect] enter requestId=%llu activeConnectionId=%llu state=%s iotState=%d subscribed=%d pending=%d callbackContext=%d leaseContext=%d args={env:%d,information:%d,core:%d,protocol:%d,callback:%d}",
         static_cast<unsigned long long>(requestId),
         static_cast<unsigned long long>(s_activeConnectionId.load()),
         connectionStateName(s_connectionState.load()),
         iot_connect_state_value.load(),
         isIotSubscribed.load(),
         isIotSubscriptionPending.load(),
         inPipelineCallback,
         hasPipelineLease,
         env != nullptr,
         information != nullptr,
         xCoreBean != nullptr,
         jProtocol != nullptr,
         i_pipeline_callback != nullptr);

    if (!env || !information || !xCoreBean || !i_pipeline_callback) {
        LOGE("[FsP2pDiag][Connect] return requestId=%llu reason=invalid_arguments",
             static_cast<unsigned long long>(requestId));
        return;
    }

    if (inPipelineCallback || hasPipelineLease) {
        LOGE("Ignoring reentrant connect from a pipeline operation");
        LOGE("[FsP2pDiag][Connect] return requestId=%llu reason=reentrant callbackContext=%d leaseContext=%d",
             static_cast<unsigned long long>(requestId),
             inPipelineCallback,
             hasPipelineLease);
        return;
    }

    LOGD("[FsP2pDiag][Connect] wait lifecycle idle requestId=%llu",
         static_cast<unsigned long long>(requestId));
    s_lifecycleExecutor.waitUntilIdle();

    std::lock_guard<std::mutex> lifecycleLock(s_lifecycleMutex);
    const ConnectionState currentState = s_connectionState.load();
    LOGD("[FsP2pDiag][Connect] lifecycle acquired requestId=%llu waitElapsedMs=%lld state=%s activeConnectionId=%llu",
         static_cast<unsigned long long>(requestId),
         static_cast<long long>(monotonicTimeMs() - requestStartedAtMs),
         connectionStateName(currentState),
         static_cast<unsigned long long>(s_activeConnectionId.load()));
    if (currentState == ConnectionState::Connecting ||
        currentState == ConnectionState::Connected) {
        g_i_mqtt_callback.set(env, i_pipeline_callback);
        const bool connected = currentState == ConnectionState::Connected;
        const int iotState = iot_connect_state_value.load();
        LOGI("[FsP2pDiag][Connect] reuse requestId=%llu connectionId=%llu p2pState=%s p2pReplayConnected=%d iotState=%d",
             static_cast<unsigned long long>(requestId),
             static_cast<unsigned long long>(s_activeConnectionId.load()),
             connectionStateName(currentState),
             connected,
             iotState);
        PipelineCallbackScope callbackScope;
        g_i_mqtt_callback.callP2pConnState(
                gJvm, connected, connected ? "Connected" : "Connecting");
        if (iotState == 1) {
            LOGD("[FsP2pDiag][Connect] replay IOT callback requestId=%llu connectionId=%llu connected=true",
                 static_cast<unsigned long long>(requestId),
                 static_cast<unsigned long long>(s_activeConnectionId.load()));
            g_i_mqtt_callback.callIotConnState(gJvm, true, "Connected");
        } else {
            LOGW("[FsP2pDiag][Connect] skip IOT callback replay requestId=%llu connectionId=%llu reason=iot_not_connected iotState=%d",
                 static_cast<unsigned long long>(requestId),
                 static_cast<unsigned long long>(s_activeConnectionId.load()),
                 iotState);
        }
        LOGI("[FsP2pDiag][Connect] return requestId=%llu action=reused elapsedMs=%lld",
             static_cast<unsigned long long>(requestId),
             static_cast<long long>(monotonicTimeMs() - requestStartedAtMs));
        return;
    }

    jclass xCoreBeanCls = env->GetObjectClass(xCoreBean);
    if (!xCoreBeanCls) {
        LOGE("[FsP2pDiag][Connect] return requestId=%llu reason=core_class_unavailable",
             static_cast<unsigned long long>(requestId));
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
        LOGE("[FsP2pDiag][Connect] return requestId=%llu reason=read_connection_arguments_failed",
             static_cast<unsigned long long>(requestId));
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

    LOGI("[FsP2pDiag][Connect] arguments requestId=%llu sn=%s productId=%s host=%s port=%u usernameSet=%d passwordSet=%d protocolBytes=%zu",
         static_cast<unsigned long long>(requestId),
         manifest.sn.c_str(),
         manifest.product_id.c_str(),
         host.c_str(),
         port,
         !userName.empty(),
         !passWord.empty(),
         protocol.size());

    if (!stopPipeline()) {
        LOGE("Cannot reconnect because the previous pipeline did not stop safely");
        LOGE("[FsP2pDiag][Connect] return requestId=%llu reason=previous_pipeline_stop_failed",
             static_cast<unsigned long long>(requestId));
        return;
    }
    g_i_mqtt_callback.set(env, i_pipeline_callback);
    PutTypeTool::init(gJvm);

    const uint64_t connectionId = s_connectionSequence.fetch_add(1) + 1;
    s_activeConnectionId.store(connectionId);
    LOGI("[FsP2pDiag][Connect] create pipeline requestId=%llu connectionId=%llu",
         static_cast<unsigned long long>(requestId),
         static_cast<unsigned long long>(connectionId));
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
        LOGD("[FsP2pDiag][Connect] pipeline initialized requestId=%llu connectionId=%llu state=%s iotState=0 subscribed=0 pending=0",
             static_cast<unsigned long long>(requestId),
             static_cast<unsigned long long>(connectionId),
             connectionStateName(s_connectionState.load()));

        pipeline->setConnectStateCallback([connectionId](bool isConnected){
            PipelineCallbackScope callbackScope;
            const ConnectionState stateBefore = s_connectionState.load();
            if (stateBefore == ConnectionState::Stopping) {
                LOGW("[FsP2pDiag][P2P] ignore native state callback connectionId=%llu connected=%d reason=stopping",
                     static_cast<unsigned long long>(connectionId),
                     isConnected);
                return;
            }
            LOGI("[FsP2pDiag][P2P] native state callback connectionId=%llu connected=%d stateBefore=%s iotState=%d subscribed=%d pending=%d",
                 static_cast<unsigned long long>(connectionId),
                 isConnected,
                 connectionStateName(stateBefore),
                 iot_connect_state_value.load(),
                 isIotSubscribed.load(),
                 isIotSubscriptionPending.load());
            s_connectionState.store(isConnected
                    ? ConnectionState::Connected
                    : ConnectionState::Disconnected);
            s_connectStartedAtMs.store(0);
            LOGD("[FsP2pDiag][P2P] request Java callback connectionId=%llu connected=%d description=connection state changed5",
                 static_cast<unsigned long long>(connectionId),
                 isConnected);
            g_i_mqtt_callback.callP2pConnState(gJvm,isConnected,"connection state changed5");
            if (isConnected) {
                PipelineLease pipelineLease;
                if (!pipelineLease) {
                    LOGW("[FsP2pDiag][P2P] skip startup and heartbeat connectionId=%llu reason=pipeline_lease_unavailable",
                         static_cast<unsigned long long>(connectionId));
                    return;
                }
                std::string iid1=pipelineLease->postStartup();
                LOGD( "setConnectStateCallback iid1=%s",iid1.c_str());
                LOGD("[FsP2pDiag][P2P] startup posted connectionId=%llu iid=%s",
                     static_cast<unsigned long long>(connectionId),
                     iid1.c_str());
                g_timer.start(1*60*1000, [connectionId]() {
                    PipelineLease heartbeatPipeline;
                    if (!heartbeatPipeline) {
                        LOGW("[FsP2pDiag][Heartbeat] skipped connectionId=%llu reason=pipeline_lease_unavailable",
                             static_cast<unsigned long long>(connectionId));
                        return;
                    }
                    std::string iid2 = heartbeatPipeline->postHeartbeat();
                    LOGD( "setConnectStateCallback iid2=%s",iid2.c_str());
                    LOGD("[FsP2pDiag][Heartbeat] posted connectionId=%llu iid=%s note=not_a_connection_callback",
                         static_cast<unsigned long long>(connectionId),
                         iid2.c_str());
                });
            }else{
                g_timer.stop();
            }
            LOGD("setConnectStateCallback isConnected=%d",isConnected);
        });
        pipeline->setDeviceHeartbeatCallback([protocol, connectionId](const fs::p2p::InfomationManifest &info) {
            PipelineCallbackScope callbackScope;
            if (s_connectionState.load() == ConnectionState::Stopping) {
                LOGW("[FsP2pDiag][CoreMessage] ignore heartbeat connectionId=%llu deviceSn=%s reason=stopping",
                     static_cast<unsigned long long>(connectionId),
                     info.sn.c_str());
                return;
            }
            LOGD("setDeviceHeartbeatCallback>>%s", info.model.c_str());
            LOGD("[FsP2pDiag][CoreMessage] heartbeat connectionId=%llu deviceSn=%s model=%s",
                 static_cast<unsigned long long>(connectionId),
                 info.sn.c_str(),
                 info.model.c_str());
        });
        pipeline->setDeviceStartupCallback([connectionId](const fs::p2p::InfomationManifest &info) {
            PipelineCallbackScope callbackScope;
            if (s_connectionState.load() == ConnectionState::Stopping) {
                LOGW("[FsP2pDiag][CoreMessage] ignore startup connectionId=%llu deviceSn=%s reason=stopping",
                     static_cast<unsigned long long>(connectionId),
                     info.sn.c_str());
                return;
            }
            // xcore是云边同步的模型名称，需要往这里注入物模型，使product_id和物模型绑定
            LOGD("setDeviceStartupCallback>>%s", info.model.c_str());
            LOGD("[FsP2pDiag][CoreMessage] startup connectionId=%llu deviceSn=%s productId=%s model=%s",
                 static_cast<unsigned long long>(connectionId),
                 info.sn.c_str(),
                 info.product_id.c_str(),
                 info.model.c_str());
        });
        pipeline->setErrorCallback([connectionId](int error_code, const std::string &error_string) {
            PipelineCallbackScope callbackScope;
            if (s_connectionState.load() == ConnectionState::Stopping) {
                LOGW("[FsP2pDiag][PipelineError] ignored connectionId=%llu code=%d error=%s reason=stopping",
                     static_cast<unsigned long long>(connectionId),
                     error_code,
                     error_string.c_str());
                return;
            }
            LOGD( "Error Code: %d, Description: %s", error_code, error_string.c_str());
            LOGE("[FsP2pDiag][PipelineError] connectionId=%llu code=%d error=%s p2pState=%s iotState=%d",
                 static_cast<unsigned long long>(connectionId),
                 error_code,
                 error_string.c_str(),
                 connectionStateName(s_connectionState.load()),
                 iot_connect_state_value.load());
        });
        pipeline->setBroadcastCallback([protocol, connectionId](const fs::p2p::Request &req) {
            PipelineCallbackScope callbackScope;
            PipelineLease pipelineLease;
            if (!pipelineLease) {
                LOGW("[FsP2pDiag][CoreMessage] ignore broadcast connectionId=%llu ack=%s action=%d reason=pipeline_lease_unavailable",
                     static_cast<unsigned long long>(connectionId),
                     req.ack.c_str(),
                     req.action);
                return;
            }
            LOGD( "setBroadcastCallback iid=%s,action>>%d", req.ack.c_str(),req.action);
            std::map<std::string, fs::p2p::Payload::Device> res_device_list=req.payload.devices;
            LOGD("[FsP2pDiag][CoreMessage] broadcast connectionId=%llu ack=%s action=%d deviceCount=%zu",
                 static_cast<unsigned long long>(connectionId),
                 req.ack.c_str(),
                 req.action,
                 res_device_list.size());
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
                    LOGD("[FsP2pDiag][IOT] broadcast method received connectionId=%llu deviceSn=%s method=%s paramCount=%zu route=broadcast_method",
                         static_cast<unsigned long long>(connectionId),
                         device_sn.c_str(),
                         method.name.c_str(),
                         method.params.size());
                    if (method.name=="iot_connect_state"){
                        const std::string state = iTools::getValue(method.params,"state","0");
                        const std::string description = iTools::getValue(method.params,"desc","");
                        LOGI("[FsP2pDiag][IOT] state message connectionId=%llu deviceSn=%s state=%s connected=%d desc=%s subscribedBefore=%d pendingBefore=%d",
                             static_cast<unsigned long long>(connectionId),
                             device_sn.c_str(),
                             state.c_str(),
                             state == "1",
                             description.c_str(),
                             isIotSubscribed.load(),
                             isIotSubscriptionPending.load());
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
                            LOGI("[FsP2pDiag][IOT] subscription attempted connectionId=%llu targetSn=%s topic=%s protocolPostIid=%s result=%d",
                                 static_cast<unsigned long long>(connectionId),
                                 target.sn.c_str(),
                                 topic.c_str(),
                                 iid.c_str(),
                                 result);
                            if (result==0){
                                LOGD("[FsP2pDiag][IOT] request Java subscribed callback connectionId=%llu topic=%s",
                                     static_cast<unsigned long long>(connectionId),
                                     topic.c_str());
                                g_i_mqtt_callback.callSubscribed(gJvm,topic);
                                isIotSubscribed.store(true);
                                SubscribeInfomation::getInstance().addManifest(target);
                            }else{
                                isIotSubscribed.store(false);
                                LOGW("[FsP2pDiag][IOT] request Java subscribeFail callback connectionId=%llu topic=%s result=%d",
                                     static_cast<unsigned long long>(connectionId),
                                     topic.c_str(),
                                     result);
                                g_i_mqtt_callback.callSubscribeFail(gJvm,topic,"subscribe failed");
                            }
                            isIotSubscriptionPending.store(false);
                        } else {
                            LOGD("[FsP2pDiag][IOT] subscription skipped connectionId=%llu subscribed=%d pending=%d",
                                 static_cast<unsigned long long>(connectionId),
                                 isIotSubscribed.load(),
                                 isIotSubscriptionPending.load());
                        }
                        iot_connect_state_value.store((state=="1")?1:-1);
                        LOGI("[FsP2pDiag][IOT] request Java state callback connectionId=%llu source=broadcast_state connected=%d desc=%s iotStateNow=%d",
                             static_cast<unsigned long long>(connectionId),
                             state == "1",
                             description.c_str(),
                             iot_connect_state_value.load());
                        g_i_mqtt_callback.callIotConnState(gJvm,state=="1",description);
                    } else if (method.name=="iot_connect" ||
                               method.name=="iot_disconnect") {
                        LOGW("[FsP2pDiag][IOT] control method observed connectionId=%llu deviceSn=%s method=%s route=broadcast_method callbackDispatched=0 reason=no_state_callback_branch_for_broadcast_control_method",
                             static_cast<unsigned long long>(connectionId),
                             device_sn.c_str(),
                             method.name.c_str());
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
        pipeline->setIncomingEventCallback([connectionId](const fs::p2p::Request &req, const fs::p2p::Payload::Device &device) {
            PipelineCallbackScope callbackScope;
            PipelineLease pipelineLease;
            if (!pipelineLease) {
                LOGW("[FsP2pDiag][CoreMessage] ignore incoming event connectionId=%llu iid=%s reason=pipeline_lease_unavailable",
                     static_cast<unsigned long long>(connectionId),
                     req.iid.c_str());
                return;
            }
            RequestManager::getInstance().addRequest(req);
            // --- 遍历 Events ---
            for (const auto& event : device.events) {
                for (const auto& param_pair : event.params) {

                }
                LOGD( "setRequestCallback Action_Event eventName=%s", event.name.c_str());
                std::string description= iTools::getValue(event.params,"desc","");
                LOGD("[FsP2pDiag][IOT] incoming event connectionId=%llu iid=%s event=%s paramCount=%zu desc=%s route=incoming_event",
                     static_cast<unsigned long long>(connectionId),
                     req.iid.c_str(),
                     event.name.c_str(),
                     event.params.size(),
                     description.c_str());
                if (event.name=="iot_disconnect"){
                    LOGI("[FsP2pDiag][IOT] request Java state callback connectionId=%llu source=incoming_event connected=false desc=%s iotStateBefore=%d",
                         static_cast<unsigned long long>(connectionId),
                         description.c_str(),
                         iot_connect_state_value.load());
                    g_i_mqtt_callback.callIotConnState(gJvm, false,description);
                    iot_connect_state_value.store(-1);
                    LOGD("[FsP2pDiag][IOT] incoming event handled connectionId=%llu event=iot_disconnect callbackDispatched=1 iotStateNow=%d",
                         static_cast<unsigned long long>(connectionId),
                         iot_connect_state_value.load());
                }else if (event.name=="iot_connect"){
                    LOGI("[FsP2pDiag][IOT] request Java state callback connectionId=%llu source=incoming_event connected=true desc=%s iotStateBefore=%d",
                         static_cast<unsigned long long>(connectionId),
                         description.c_str(),
                         iot_connect_state_value.load());
                    g_i_mqtt_callback.callIotConnState(gJvm, true,description);
                    iot_connect_state_value.store(1);
                    LOGD("[FsP2pDiag][IOT] incoming event handled connectionId=%llu event=iot_connect callbackDispatched=1 iotStateNow=%d",
                         static_cast<unsigned long long>(connectionId),
                         iot_connect_state_value.load());
                }else{
                    BaseData baseData={PutTypeTool::EVENT(),req.iid,event.name, event.params};
                    g_i_mqtt_callback.callMsgArrives(gJvm,baseData);
                }
            }
        });

        // open() may report the connection immediately, so callbacks must exist first.
        LOGI("[FsP2pDiag][Connect] open begin requestId=%llu connectionId=%llu host=%s port=%u",
             static_cast<unsigned long long>(requestId),
             static_cast<unsigned long long>(connectionId),
             host.c_str(),
             port);
        pipeline->open(host, port, userName, passWord);
        LOGI("[FsP2pDiag][Connect] open returned requestId=%llu connectionId=%llu elapsedMs=%lld state=%s iotState=%d subscribed=%d pending=%d",
             static_cast<unsigned long long>(requestId),
             static_cast<unsigned long long>(connectionId),
             static_cast<long long>(monotonicTimeMs() - requestStartedAtMs),
             connectionStateName(s_connectionState.load()),
             iot_connect_state_value.load(),
             isIotSubscribed.load(),
             isIotSubscriptionPending.load());
    } catch (const std::exception& error) {
        LOGE("P2P connect failed: %s", error.what());
        LOGE("[FsP2pDiag][Connect] exception requestId=%llu connectionId=%llu error=%s",
             static_cast<unsigned long long>(requestId),
             static_cast<unsigned long long>(connectionId),
             error.what());
        PipelineCallbackScope callbackScope;
        g_i_mqtt_callback.callP2pConnState(gJvm, false, error.what());
        if (!stopPipeline()) {
            LOGE("Failed to stop pipeline after connect exception");
        }
    } catch (...) {
        LOGE("P2P connect failed with an unknown native exception");
        LOGE("[FsP2pDiag][Connect] unknown exception requestId=%llu connectionId=%llu",
             static_cast<unsigned long long>(requestId),
             static_cast<unsigned long long>(connectionId));
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
    const bool inPipelineCallback = PipelineCallbackScope::isActive();
    const bool hasPipelineLease = PipelineLease::isActiveOnCurrentThread();
    const uint64_t connectionId = s_activeConnectionId.load();
    LOGI("[FsP2pDiag][Disconnect] enter connectionId=%llu state=%s iotState=%d subscribed=%d pending=%d callbackContext=%d leaseContext=%d",
         static_cast<unsigned long long>(connectionId),
         connectionStateName(s_connectionState.load()),
         iot_connect_state_value.load(),
         isIotSubscribed.load(),
         isIotSubscriptionPending.load(),
         inPipelineCallback,
         hasPipelineLease);
    if (inPipelineCallback || hasPipelineLease) {
        LOGW("[FsP2pDiag][Disconnect] using asynchronous stop connectionId=%llu",
             static_cast<unsigned long long>(connectionId));
        s_lifecycleExecutor.requestStop();
        rejectPipelineOperations();
        g_i_mqtt_callback.clear(env);
        infomationsCallback.clear(env);
        clearGlobalBlackCallback(env);
        RequestManager::getInstance().clearAll();
        LOGI("[FsP2pDiag][Disconnect] return connectionId=%llu action=asynchronous_stop_requested callbacksCleared=1",
             static_cast<unsigned long long>(connectionId));
        return;
    }

    LOGD("[FsP2pDiag][Disconnect] using synchronous stop connectionId=%llu",
         static_cast<unsigned long long>(connectionId));
    std::lock_guard<std::mutex> lifecycleLock(s_lifecycleMutex);
    if (!stopPipeline()) {
        LOGE("Disconnect did not complete safely");
        LOGE("[FsP2pDiag][Disconnect] return connectionId=%llu result=failed reason=pipeline_stop_failed",
             static_cast<unsigned long long>(connectionId));
        return;
    }
    g_i_mqtt_callback.clear(env);
    infomationsCallback.clear(env);
    clearGlobalBlackCallback(env);
    RequestManager::getInstance().clearAll();
    LOGI("[FsP2pDiag][Disconnect] return connectionId=%llu result=complete callbacksCleared=1",
         static_cast<unsigned long long>(connectionId));
}

} // extern "C"
