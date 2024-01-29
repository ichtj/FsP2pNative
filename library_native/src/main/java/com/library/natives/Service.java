package com.library.natives;

import java.util.Map;

public class Service {
    public String name;
    public Map<String, String> propertys;
    public int reason_code = 0;
    public String reason_string;

    public Service(String name, Map<String, String> propertys, int reason_code, String reason_string) {
        this.name = name;
        this.propertys = propertys;
        this.reason_code = reason_code;
        this.reason_string = reason_string;
    }

    public Service() {
    }

    @Override
    public String toString() {
        return "Service{" +
                "name='" + name + '\'' +
                ", propertys=" + propertys +
                ", reason_code=" + reason_code +
                ", reason_string='" + reason_string + '\'' +
                '}';
    }
}
