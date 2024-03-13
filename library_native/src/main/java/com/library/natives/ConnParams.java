package com.library.natives;

public class ConnParams {
    public SubDev subDev;
    public String json_protocol;//产品协议体 json
    public String userName;
    public String passWord;
    public String host;
    public int port;

    public ConnParams(SubDev subDev, String json_protocol, String userName, String passWord, String host, int port) {
        this.subDev = subDev;
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
                "subDev=" + subDev +
                ", json_protocol='" + json_protocol + '\'' +
                ", userName='" + userName + '\'' +
                ", passWord='" + passWord + '\'' +
                ", host='" + host + '\'' +
                ", port=" + port +
                '}';
    }
}
