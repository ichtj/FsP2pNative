package com.library.natives;

public class DevModel {
    public String sn;
    public String name;
    public String product_id;
    public String model;
    public Type type;
    public int version;

    public DevModel() {
    }

    public DevModel(String sn, String name, String product_id, String model, Type type, int version) {
        this.sn = sn;
        this.name = name;
        this.product_id = product_id;
        this.model = model;
        this.type = type;
        this.version = version;
    }

    @Override
    public String toString() {
        return "DevModel{" +
                "sn='" + sn + '\'' +
                ", name='" + name + '\'' +
                ", product_id='" + product_id + '\'' +
                ", model='" + model + '\'' +
                ", type=" + type +
                ", version=" + version +
                '}';
    }
}
