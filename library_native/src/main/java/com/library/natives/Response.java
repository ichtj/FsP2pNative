package com.library.natives;

public class Response {
    public String iid;
    public Action action;
    public String time;
    public Payload payload;

    public Response() {
    }

    public Response(String iid, Action action, String time, Payload payload) {
        this.iid = iid;
        this.action = action;
        this.time = time;
        this.payload = payload;
    }

    @Override
    public String toString() {
        return "Response{" +
                "iid='" + iid + '\'' +
                ", action=" + action +
                ", time='" + time + '\'' +
                ", payload=" + payload +
                '}';
    }
}
