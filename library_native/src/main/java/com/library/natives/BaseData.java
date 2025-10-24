package com.library.natives;


import java.util.Map;

public class BaseData {
    @IPutType
    public int iPutType;
    public String iid;
    public String operation;
    public Map<String, Object> maps;

    public BaseData(int iPutType, String iid, String operation, Map<String, Object> maps) {
        this.iPutType = iPutType;
        this.iid = iid;
        this.operation = operation;
        this.maps = maps;
    }

    @Override
    public String toString() {
        return "MsgData{" +
                "iPutType=" + iPutType +
                ", iid='" + iid + '\'' +
                ", operation='" + operation + '\'' +
                ", maps=" + maps +
                '}';
    }
}
