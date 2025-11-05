package com.library.natives;

import java.util.Arrays;

public class BlackBean {
    private String[] devices_array;
    private String[] model_array;
    private String desc;

    public BlackBean() {
    }

    public BlackBean(String[] devices_array, String[] model_array, String desc) {
        this.devices_array = devices_array;
        this.model_array = model_array;
        this.desc = desc;
    }

    public String[] getDevices_array() {
        return devices_array;
    }

    public void setDevices_array(String[] devices_array) {
        this.devices_array = devices_array;
    }

    public String[] getModel_array() {
        return model_array;
    }

    public void setModel_array(String[] model_array) {
        this.model_array = model_array;
    }

    public String getDesc() {
        return desc;
    }

    public void setDesc(String desc) {
        this.desc = desc;
    }

    @Override
    public String toString() {
        return "BlackBean{" +
                "devices_array=" + Arrays.toString (devices_array) +
                ", model_array=" + Arrays.toString (model_array) +
                ", desc='" + desc + '\'' +
                '}';
    }
}
