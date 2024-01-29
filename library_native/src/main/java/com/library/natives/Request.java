package com.library.natives;

public class Request {
    public String iid;
    public int action;
    public String ack;
    public String time;
    public Payload payload;

    public Request() {
    }

    public Request(String iid, int action, String ack, String time, Payload payload) {
        this.iid = iid;
        this.action = action;
        this.ack = ack;
        this.time = time;
        this.payload = payload;
    }

    @Override
    public String toString() {
        return "Request{" +
                "iid='" + iid + '\'' +
                ", action=" + action +
                ", ack='" + ack + '\'' +
                ", time='" + time + '\'' +
                ", payload=" + payload +
                '}';
    }
}
