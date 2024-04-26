package com.library.natives;

import java.util.Map;

public class Method {
    public String name;
    public Map<String, Object> params;
    public int reason_code = 0;
    public String reason_string;

    public Method(String name, Map<String, Object> params, int reason_code, String reason_string) {
        this.name = name;
        this.params = params;
        this.reason_code = reason_code;
        this.reason_string = reason_string;
    }

    public Method() {
    }

    @Override
    public String toString() {
        return "Method{" +
                "name='" + name + '\'' +
                ", params=" + params +
                ", reason_code=" + reason_code +
                ", reason_string='" + reason_string + '\'' +
                '}';
    }
}
