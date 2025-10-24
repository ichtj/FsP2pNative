package com.library.natives;

public class Infomation {
    private String sn;
    private String productId;
    private String name;
    private String model;
    private Type type;
    private int version;
    private String host;
    private int port;
    private String username;
    private String password;
    private String protocol;

    public Infomation() {
    }

    public Infomation(String sn, String productId, String name, String model, Type type, int version, String host, int port, String username, String password, String protocol) {
        this.sn = sn;
        this.productId = productId;
        this.name = name;
        this.model = model;
        this.type = type;
        this.version = version;
        this.host = host;
        this.port = port;
        this.username = username;
        this.password = password;
        this.protocol = protocol;
    }

    public String getProtocol() {
        return protocol;
    }

    public void setProtocol(String protocol) {
        this.protocol = protocol;
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

    public String getHost() { return host; }
    public void setHost(String host) { this.host = host; }

    public int getPort() { return port; }
    public void setPort(int port) { this.port = port; }

    public String getUsername() { return username; }
    public void setUsername(String username) { this.username = username; }

    public String getPassword() { return password; }
    public void setPassword(String password) { this.password = password; }

    @Override
    public String toString() {
        return "InfomationManifest{" +
                "sn='" + sn + '\'' +
                ", productId='" + productId + '\'' +
                ", name='" + name + '\'' +
                ", model='" + model + '\'' +
                ", type=" + type +
                ", version=" + version +
                ", host='" + host + '\'' +
                ", port=" + port +
                ", username='" + username + '\'' +
                ", password='[redacted]'" +
                '}';
    }

}
