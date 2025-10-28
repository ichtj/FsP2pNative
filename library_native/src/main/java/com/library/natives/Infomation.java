package com.library.natives;

public class Infomation {
    private String sn;
    private String productId;
    private String name;
    private String model;
    private Type type;
    private int version;

    public Infomation() {
    }

    public Infomation(String sn, String productId, String name, String model, Type type, int version) {
        this.sn = sn;
        this.productId = productId;
        this.name = name;
        this.model = model;
        this.type = type;
        this.version = version;
    }

    public String getSn() { return sn; }
    public void setSn(String sn) { this.sn = sn; }

    public String getProductId() { return productId; }
    public void setProductId(String productId) { this.productId = productId; }

    public String getName() { return name; }
    public void setName(String name) { this.name = name; }

    public String getModel() { return model; }
    public void setModel(String model) { this.model = model; }

    public Type getType() { return type; }
    public void setType(Type type) { this.type = type; }

    public int getVersion() { return version; }
    public void setVersion(int version) { this.version = version; }



    @Override
    public String toString() {
        return "InfomationManifest{" +
                "sn='" + sn + '\'' +
                ", productId='" + productId + '\'' +
                ", name='" + name + '\'' +
                ", model='" + model + '\'' +
                ", type=" + type +
                ", version=" + version +
                '}';
    }

}
