package com.library.natives;

/**
 * fsp2p管道回调
 */
public interface PipelineCallback {
    /**
     * 连接状态回调
     * @param status true成功 false失败
     */
    void connectStatus(boolean status);

    /**
     * 管道日志回调
     * @param level 等级 D,E,V,I,W
     * @param str 日志内容
     */
    void pipelineLog(int level,String str);


    /**
     * 消息管道中的回调
     * @param request 回调的对象
     * 按需调用以下的方法 去回应调用者[有些方法不需要反馈]
     * -------PublishMessage.postAll(Request source, Map<String,Device> list);//回应总消息体
     * ---------------------.postMethod(Request source, List<Method> methods);//回应方法
     * ---------------------.postService(Request source, List<Service> events);//回应属性
     * ---------------------.postEvent(Request source, List<Event> events);//回应事件
     * ---------------------.postNotify(Request source, List<Service> events);//回应通知
     */
    void callback(Request request);
}
