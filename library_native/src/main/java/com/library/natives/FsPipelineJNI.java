package com.library.natives;

import java.util.List;
import java.util.Map;

/**
 * 管道控制
 */
public class FsPipelineJNI {
    static {
        System.loadLibrary("FsPipelineJNI");
    }

    /**
     * 初始化fs_p2p管道
     * @param connParams 初始化参数
     * @param pipelineCallback 注册回调
     */
    public static native void init(ConnParams connParams,PipelineCallback pipelineCallback);

    /**
     * 开启连接
     */
    public static native void connect();

    /**
     * 关闭连接
     */
    public static native void close();

    /**
     * 上线事件
     */
    public static native int postStartup();

    /**
     * 发布心跳 时间周期 1分钟执行一次
     */
    public static native int postHeartbeat();

    /**
     * 发布总消息体
     */
    public static native int postBody(Request source, Map<String, Device> list);

    /**
     * 发布消息
     */
    public static native int postMethod(Request source, List<Method> methods);

    /**
     * 发布服务
     */
    public static native int postService(Request source, List<Service> events);

    /**
     * 发布事件
     */
    public static native int postEvent(Request source, List<Event> events);

    /**
     * 发布通知
     */
    public static native int postNotify(Request source, List<Service> events);

    /**
     * 主动请求读
     */
    public static native int postRead(Map<String, Device> list, List<Service> events);

    /**
     * 主动请求写
     */
    public static native int postWrite(Request source, List<Service> events);

}

