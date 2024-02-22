package com.library.natives;

import java.util.Map;

public class Payload {
    /**
     * String sn
     */
    public Map<String, Device> devices;

    public Payload(Map<String, Device> devices) {
        this.devices = devices;
    }

    public Payload() {
    }

    @Override
    public String toString() {
        return "Payload{" +
                "devices=" + devices +
                '}';
    }
}
