package com.library.natives;

import java.util.Map;

public class Event {
    public String name;
    public Map<String,String> params;

    public Event(String name, Map<String, String> params) {
        this.name = name;
        this.params = params;
    }

    public Event() {
    }

    @Override
    public String toString() {
        return "Event{" +
                "name='" + name + '\'' +
                ", params=" + params +
                '}';
    }
}
