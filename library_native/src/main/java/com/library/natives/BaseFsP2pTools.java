package com.library.natives;

import java.util.List;
import java.util.Map;

public class BaseFsP2pTools {
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
     * 获取所有信息设备
     * @return
     */
    public static native void getInfomationList(IInfomationsCallback icallback);

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
     * iot平台回复
     * @param iPutType
     * @param iid
     * @param node
     * @param dataMap
     * @return
     */
    public static boolean putIotReply(@IPutType int iPutType, String iid, String node, Map<String, Object> dataMap) {
        return putIotReply (iPutType, iid, node, dataMap,0,"");
    }

    /**
     * iot平台回复
     * @param iPutType
     * @param iid
     * @param node
     * @param dataMap
     * @return
     */
    public static native boolean putIotReply(@IPutType int iPutType, String iid, String node, Map<String, Object> dataMap,int statusCode,String statusDesc);

    /**
     * 向管道发送发布事件 属性等 属于fsp2p协议栈的一部分  这里只代理iot平台相关的:事件 属性上报
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
