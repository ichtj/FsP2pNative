package com.library.natives;

import android.os.Build;
import android.util.Base64;

import java.util.UUID;

public class Fsp2pTools {

    public static String convertToJsonToBase64(String jsonString) {
        // 将 JSON 字符串转换为字节数组
        byte[] jsonData = jsonString.getBytes();

        // 使用 Base64 编码字节数组
        byte[] base64Data = Base64.encode(jsonData, Base64.DEFAULT);

        // 将编码后的字节数组转换为字符串
        return new String(base64Data);
    }


    /**
     * 获取设备SN号 请使用此唯一入口
     */
    public static String getSn() {
        try {
            return Build.VERSION.SDK_INT >= 30 ? Build.getSerial() : Build.SERIAL;
        } catch (Throwable throwable) {
            return "";
        }
    }

    /**
     * 获取设备SN号 请使用此唯一入口
     */
    public static String getTargetSn() {
        /*try {
            return "FSM-1DBD81";
        } catch (Throwable throwable) {
            return "";
        }*/
        try {
            return Build.VERSION.SDK_INT >= 30 ? Build.getSerial() : Build.SERIAL;
        } catch (Throwable throwable) {
            return "";
        }
    }
    /**
     * 获取当前系统毫秒的时间戳
     * @return 1502697135
     */
    public static String getTimestamp() {
        return System.currentTimeMillis()+"";
    }

    /**
     * 创建消息数据唯一编号
     */
    public static String createIID() {
        return UUID.randomUUID().toString().replace("-", "");
    }
}
