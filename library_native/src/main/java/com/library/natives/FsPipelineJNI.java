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
     * 设置log开关
     */
    public static native void logEnable(boolean isEnable);

    /**
     * 查看log开关是否开启
     */
    public static native boolean isLogEnable();

    /**
     * 初始化fs_p2p管道
     *
     * @param connParams 初始化参数
     */
    public static native int init(ConnParams connParams);

    /**
     * 注册回调
     *
     * @param pipelineCallback
     */
    public static native int addPipelineCallback(PipelineCallback pipelineCallback);

    /**
     * 注册回调
     *
     * @param pipelineCallback
     */
    public static native int unRegisterCallback(PipelineCallback pipelineCallback);

    /**
     * 获取设备列表
     */
    public static native List<SubDev> getDevModelList();

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


    /**--------------------------------------------回复其他设备 start--------------------------------------------------*/
    /**
     * 回应总消息体
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int replyBody(Request request, Map<String, Device> out);

    /**
     * 回应消息 回复
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int replyMethod(Request request, Map<String, String> out);

    /**
     * 回应服务-属性
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int replyServices(Request request, List<Service> out);

    /**
     * 回应服务
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int replyService(Request request, Service out);
    /**--------------------------------------------回复其他设备 end--------------------------------------------------*/


    /**-------------------------------------------向设备读写,事件 start------------------------------------------------*/
    /**
     * 发布方法
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int pushMethods(String sn, List<Method> out);

    /**
     * 发布方法
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int pushMethod(String sn, Method out);

    /**
     * 发布事件
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int pushEvents(List<Event> out);

    /**
     * 发布事件
     *
     * @param out 输出参数
     * @return 是否成功
     */
    public static native int pushEvent(Event out);

    /**
     * 主动通知
     */
    public static native int pushNotify(Service out);

    /**
     * 主动通知
     */
    public static native int pushNotifyList(List<Service> out);

    /**
     * 主动请求读
     */
    public static native int pushReadList(String sn, List<Service> out);

    /**
     * 主动请求读
     */
    public static native int pushRead(String sn, Service out);

    /**
     * 主动请求写
     */
    public static native int pushWriteList(String sn, List<Service> out);

    /**
     * 主动请求写
     */
    public static native int pushWrite(String sn, Service out);
    /**-------------------------------------------向设备读写,事件 end------------------------------------------------*/


}

