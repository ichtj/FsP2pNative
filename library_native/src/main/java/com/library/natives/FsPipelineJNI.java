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
     *
     * @param connParams       初始化参数
     * @param pipelineCallback 注册回调
     */
    public static native int init(ConnParams connParams, PipelineCallback pipelineCallback);

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
    public static native int postOnLine();

    /**
     * 发布心跳 时间周期 1分钟执行一次
     */
    public static native int postHeartbeat();

    /**
     * 回应总消息体
     *
     * @param out     输出参数
     * @return 是否成功
     */
    public static native int replyBody(Request request, Map<String, Device> out);

    /**
     * 回应消息 回复
     *
     * @param out     输出参数
     * @return 是否成功
     */
    public static native int replyMethod(Request request,Map<String, String> out);

    /**
     * 回应服务-属性
     *
     * @param out     输出参数
     * @return 是否成功
     */
    public static native int replyServices(Request request,List<Service> out);

    /**
     * 回应服务
     *
     * @param out     输出参数
     * @return 是否成功
     */
    public static native int replyService(Request request, Service out);

    /**
     * 发布事件
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int postEvents(List<Event> out);

    /**
     * 发布事件
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int postEvent(Event out);

    /**
     * 主动通知
     */
    public static native int postNotify(Service out);

    /**
     * 主动通知
     */
    public static native int postNotifyList(List<Service> out);

    /**
     * 主动请求读
     */
    public static native int postReadList(String sn,List<Service> out);

    /**
     * 主动请求读
     */
    public static native int postRead(String sn,Service out);

    /**
     * 主动请求写
     */
    public static native int postWriteList(String sn,List<Service> out);

    /**
     * 主动请求写
     */
    public static native int postWrite(String sn,Service out);
}

