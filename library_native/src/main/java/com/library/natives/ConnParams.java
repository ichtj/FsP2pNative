package com.library.natives;

public class ConnParams {
    public String sn;
    public String name;
    public String product_id;
    public String model;
    public Type type;
    public int version;
    public String json_protocol;//产品协议体 json
    public String userName;
    public String passWord;
    public String host;
    public int port;

    public ConnParams(String sn, String name, String product_id, String model, Type type, int version, String json_protocol, String userName, String passWord, String host, int port) {
        this.sn = sn;
        this.name = name;
        this.product_id = product_id;
        this.model = model;
        this.type = type;
        this.version = version;
        this.json_protocol = json_protocol;
        this.userName = userName;
        this.passWord = passWord;
        this.host = host;
        this.port = port;
    }

    public ConnParams() {
    }

    @Override
    public String toString() {
        return "ConnParams{" +
                "sn='" + sn + '\'' +
                ", name='" + name + '\'' +
                ", product_id='" + product_id + '\'' +
                ", model='" + model + '\'' +
                ", type=" + type +
                ", version=" + version +
                ", json_protocol='" + json_protocol + '\'' +
                ", userName='" + userName + '\'' +
                ", passWord='" + passWord + '\'' +
                ", host='" + host + '\'' +
                ", port=" + port +
                '}';
    }
}
