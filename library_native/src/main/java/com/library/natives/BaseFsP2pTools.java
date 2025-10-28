package com.library.natives;

import java.util.Map;

public class BaseXLink {
    static {
        System.loadLibrary("BaseXLink");
    }

    /**
     * 日志开关
     * @param isEnable
     */
    public static native void logEnable(boolean isEnable);

    /**
     * 获取日志开关状态
     * @return
     */
    public static native boolean isLogEnable();

    /**
     * 获取连接状态
     * @return
     */
    public static native boolean getConnectStatus();

    /**
     * 连接
     * @param infomation  连接参数
     * @param iPipelineCallback 回调
     */
    public static native void connect(Infomation infomation,XCoreBean coreBean,String protocol, IPipelineCallback iPipelineCallback);

    /**
     * 订阅
     * @param infomation
     * @return
     */
    public static native boolean subscribe(Infomation infomation);

    /**
     * 取消订阅
     * @param infomation
     * @return
     */
    public static native boolean unSubscribe(Infomation infomation);

    /**
     * 平台回复
     * @param iPutType
     * @param iid
     * @param node
     * @param dataMap
     * @return
     */
    public static native boolean putReply(@IPutType int iPutType, String iid, String node, Map<String, Object> dataMap) ;

    /**
     * 消息发布事件 属性 优化中... 属于fsp2p协议栈的一部分 与iot通讯无关
     * @param iPutType
     * @param targetSn
     * @param pDid
     * @param node
     * @param params
     * @return
     */
    public static native boolean postMsg(@IPutType int iPutType,String targetSn,String pDid,String node, Map<String, Object> params);

    /**
     * 断开连接
     */
    public static native void disConnect();
}
