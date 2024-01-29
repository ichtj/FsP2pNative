package com.library.natives;

import java.util.List;

public class Device {
    public String sn;
    /**
     * 产品ID
     */
    public String product_id;
    /**
     * 服务列表
     */
    public List<Service> services;
    /**
     * 方法列表
     */
    public List<Method> methods;
    /**
     * 事件列表
     */
    public List<Event> events;

    public Device(String sn, String product_id, List<Service> services, List<Method> methods, List<Event> events) {
        this.sn = sn;
        this.product_id = product_id;
        this.services = services;
        this.methods = methods;
        this.events = events;
    }

    public Device() {
    }

    @Override
    public String toString() {
        return "Device{" +
                "sn='" + sn + '\'' +
                ", product_id='" + product_id + '\'' +
                ", services=" + services +
                ", methods=" + methods +
                ", events=" + events +
                '}';
    }
}

