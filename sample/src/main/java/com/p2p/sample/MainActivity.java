package com.p2p.sample;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.EditText;
import android.widget.TextView;

import com.library.natives.ConnParams;
import com.library.natives.SubDev;
import com.library.natives.Event;
import com.library.natives.FsPipelineJNI;
import com.library.natives.Fsp2pTools;
import com.library.natives.Method;
import com.library.natives.PipelineCallback;
import com.library.natives.Request;
import com.library.natives.Service;
import com.library.natives.Type;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = MainActivity.class.getSimpleName();
    private static final String JSON_PROTOCOL = "{\"service\":[{\"data\":[{\"unit\":\"bool\",\"pName\":\"net_lost_restart\",\"authority\":\"read,write,upload\",\"format\":\"bool\",\"pid\":15,\"valueDefinition\":\"网络丢失一小时重启\",\"required\":1,\"status\":0},{\"unit\":\"bool\",\"pName\":\"net_lost_reset\",\"authority\":\"read,write,upload\",\"format\":\"bool\",\"pid\":13,\"valueDefinition\":\"网络丢失模块重置\",\"required\":1,\"status\":0},{\"unit\":\"MB\",\"pName\":\"sdcard_space\",\"authority\":\"read\",\"format\":\"string\",\"pid\":12,\"valueDefinition\":\"sdcard空间\",\"required\":0,\"status\":0},{\"unit\":\"时分秒\",\"pName\":\"running_time\",\"authority\":\"read\",\"format\":\"string\",\"pid\":11,\"valueDefinition\":\"设备运行时间\",\"required\":0,\"status\":0},{\"unit\":\"day;HH:HH;boolean\",\"pName\":\"system_restart\",\"authority\":\"read,write,upload\",\"format\":\"string\",\"pid\":10,\"valueDefinition\":\"重启机制 [频次;时间1:时间2;开关]\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"Iotcloud_version\",\"authority\":\"read,upload\",\"format\":\"string\",\"pid\":9,\"valueDefinition\":\"IotCloud APK版本\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"fw_sys_version\",\"authority\":\"read\",\"format\":\"string\",\"pid\":8,\"valueDefinition\":\"显示固件版本具体信息\",\"required\":0,\"status\":0},{\"unit\":\"M\",\"pName\":\"tf_space\",\"authority\":\"read\",\"format\":\"string\",\"pid\":7,\"valueDefinition\":\"TF卡空间(M)\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"fwversion\",\"authority\":\"read,upload\",\"format\":\"string\",\"pid\":6,\"valueDefinition\":\"固件版本\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"top_app\",\"authority\":\"read\",\"format\":\"string\",\"pid\":5,\"valueDefinition\":\"获取顶层app\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"usbList\",\"authority\":\"read\",\"format\":\"string\",\"pid\":4,\"valueDefinition\":\"获取接入的usb列表\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"imei\",\"authority\":\"read\",\"format\":\"string\",\"pid\":3,\"valueDefinition\":\"设备imei\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"time\",\"authority\":\"read\",\"format\":\"string\",\"pid\":2,\"valueDefinition\":\"系统时间\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"android_version\",\"authority\":\"read\",\"format\":\"string\",\"pid\":1,\"valueDefinition\":\"安卓版本\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"backlight\",\"authority\":\"read,write\",\"format\":\"string\",\"pid\":0,\"valueDefinition\":\"背光值[0~255]\",\"required\":1,\"status\":0}],\"name\":\"device\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":16},{\"data\":[{\"unit\":\"0\",\"pName\":\"base_station_port\",\"authority\":\"read,write,upload\",\"format\":\"int32\",\"pid\":9,\"valueDefinition\":\"4G基站端口\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"lteoperator\",\"authority\":\"read\",\"format\":\"string\",\"pid\":8,\"valueDefinition\":\"4G卡运营商\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"iccid\",\"authority\":\"read\",\"format\":\"string\",\"pid\":7,\"valueDefinition\":\"4g sim卡iccid\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"wifi_status\",\"authority\":\"read,write\",\"format\":\"string\",\"pid\":6,\"valueDefinition\":\"WIFI 开启0/关闭1\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"reset_4g\",\"authority\":\"write\",\"format\":\"string\",\"pid\":5,\"valueDefinition\":\"4G复位 无返回值\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"baseband\",\"authority\":\"read\",\"format\":\"string\",\"pid\":4,\"valueDefinition\":\"4G基带版本\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"dbm\",\"authority\":\"read\",\"format\":\"string\",\"pid\":3,\"valueDefinition\":\"4G信号值\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"eth_mode\",\"authority\":\"read\",\"format\":\"string\",\"pid\":2,\"valueDefinition\":\"以太网ip模式\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"local_Ip\",\"authority\":\"read\",\"format\":\"string\",\"pid\":1,\"valueDefinition\":\"本机ip\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"net_type\",\"authority\":\"read\",\"format\":\"string\",\"pid\":0,\"valueDefinition\":\"网络类型\",\"required\":1,\"status\":0}],\"name\":\"network\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":15}],\"action\":[{\"data\":[{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"bool\",\"pId\":1,\"valueDefinition\":\"执行结果true|false\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"package_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0}],\"name\":\"clean_app_cache\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":9},{\"data\":[{\"paramType\":0,\"pName\":\"node_baudrate\",\"authority\":\"\",\"format\":\"string\",\"pId\":10,\"valueDefinition\":\"串口波特率\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"node_addr\",\"authority\":\"\",\"format\":\"string\",\"pId\":9,\"valueDefinition\":\"从设备地址\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"firmware_version\",\"authority\":\"\",\"format\":\"string\",\"pId\":8,\"valueDefinition\":\"从设备固件版本\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"url\",\"authority\":\"\",\"format\":\"string\",\"pId\":5,\"valueDefinition\":\"从设备固件地址\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"firmware_type\",\"authority\":\"\",\"format\":\"string\",\"pId\":4,\"valueDefinition\":\"文件格式\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"firmware_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"文件名称\",\"required\":1,\"status\":0}],\"name\":\"com_upgrade\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":32},{\"data\":[{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"bool\",\"pId\":1,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"file_path\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"文件路径\",\"required\":1,\"status\":0}],\"name\":\"file_del\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":18},{\"data\":[{\"paramType\":0,\"pName\":\"file_type\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"文件格式\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"file_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"文件名称\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"url\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"下载地址\",\"required\":1,\"status\":0}],\"name\":\"file_download\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":13},{\"data\":[{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"bool\",\"pId\":4,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":0,\"pName\":\"directory\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"路径\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"file_list\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"返回指定路径的文件及目录\",\"required\":1,\"status\":0}],\"name\":\"file_get_list\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":12},{\"data\":[{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"bool\",\"pId\":2,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"des_path\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"目标路径\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"src_path\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"原始路径\",\"required\":1,\"status\":0}],\"name\":\"file_move\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":19},{\"data\":[{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"file_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"文件名称\",\"required\":0,\"status\":0},{\"paramType\":0,\"pName\":\"directory\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"文件所处的文件夹路径\",\"required\":1,\"status\":0}],\"name\":\"file_upload\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":35},{\"data\":[{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"bool\",\"pId\":1,\"valueDefinition\":\"结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"app_list\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"app列表\",\"required\":1,\"status\":0}],\"name\":\"get_app_list\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":10},{\"data\":[{\"paramType\":0,\"pName\":\"is_required_restart\",\"authority\":\"\",\"format\":\"bool\",\"pId\":7,\"valueDefinition\":\"是否需要重启true|false\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"url\",\"authority\":\"\",\"format\":\"string\",\"pId\":4,\"valueDefinition\":\"下载地址\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"package_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"version\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"apk版本\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"apk_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"is_system_apk\",\"authority\":\"\",\"format\":\"bool\",\"pId\":0,\"valueDefinition\":\"是否是系统应用\",\"required\":1,\"status\":0}],\"name\":\"install_apk\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":7},{\"data\":[{\"paramType\":0,\"pName\":\"fileName\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"文件名 例如：20210810.log\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"filePath\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"在设备上的路径 例如：/sdcard/LOGSAVE/\",\"required\":1,\"status\":0}],\"name\":\"log_upload\",\"functionType\":1,\"required\":0,\"status\":0,\"sid\":33},{\"data\":[{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"bool\",\"pId\":2,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"data\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"返回数据\",\"required\":0,\"status\":0},{\"paramType\":0,\"pName\":\"cmd\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"命令\",\"required\":1,\"status\":0}],\"name\":\"remote_cmd\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":11},{\"data\":[{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":4,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"format\":\"bool\",\"pId\":3,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"img_size\",\"authority\":\"\",\"format\":\"uint32\",\"pId\":2,\"valueDefinition\":\"截屏文件大小\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"img_url\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"文件路径\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"base64\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"图片base64\",\"required\":1,\"status\":0}],\"name\":\"screenshot\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":31},{\"data\":[{\"paramType\":0,\"pName\":\"toolbar_status\",\"authority\":\"\",\"format\":\"int32\",\"pId\":0,\"valueDefinition\":\"0为显示 1为隐藏\",\"required\":1,\"status\":0}],\"name\":\"show_hide_toolbar\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":28},{\"data\":[{\"paramType\":0,\"pName\":\"password\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"密码\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"userName\",\"authority\":\"\",\"format\":\"bool\",\"pId\":0,\"valueDefinition\":\"用户名\",\"required\":1,\"status\":0}],\"name\":\"sign\",\"functionType\":1,\"required\":0,\"status\":0,\"sid\":39},{\"data\":[{\"paramType\":0,\"pName\":\"is_required_restart\",\"authority\":\"\",\"format\":\"bool\",\"pId\":6,\"valueDefinition\":\"是否需要重启true|false\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"package_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"apk包名\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"apk_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"is_system_apk\",\"authority\":\"\",\"format\":\"bool\",\"pId\":0,\"valueDefinition\":\"是否是系统应用\",\"required\":1,\"status\":0}],\"name\":\"uninstall_apk\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":8}],\"event\":[{\"data\":[{\"pName\":\"install_status\",\"authority\":\"\",\"format\":\"bool\",\"pId\":3,\"valueDefinition\":\"安装是否完成\",\"required\":1,\"status\":0},{\"pName\":\"is_system_apk\",\"authority\":\"\",\"format\":\"bool\",\"pId\":2,\"valueDefinition\":\"是否是系统应用\",\"required\":1,\"status\":0},{\"pName\":\"package_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0},{\"pName\":\"app_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0}],\"name\":\"apk_install_result\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":33},{\"data\":[{\"pName\":\"uninstall_status\",\"authority\":\"\",\"format\":\"bool\",\"pId\":3,\"valueDefinition\":\"卸载状态\",\"required\":1,\"status\":0},{\"pName\":\"is_system_apk\",\"authority\":\"\",\"format\":\"bool\",\"pId\":2,\"valueDefinition\":\"是否为系统应用\",\"required\":1,\"status\":0},{\"pName\":\"package_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0},{\"pName\":\"app_name\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0}],\"name\":\"apk_uninstall_result\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":34},{\"data\":[],\"name\":\"door_record\",\"functionType\":1,\"required\":0,\"status\":0,\"sid\":47},{\"data\":[{\"pName\":\"name\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"文件名称\",\"required\":1,\"status\":0},{\"pName\":\"percent\",\"authority\":\"\",\"format\":\"int32\",\"pId\":0,\"valueDefinition\":\"文件下载进度\",\"required\":1,\"status\":0}],\"name\":\"file_download_percent\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":36},{\"data\":[],\"name\":\"file_upload_percent\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":46},{\"data\":[],\"name\":\"file_upload_result\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":39},{\"data\":[{\"pName\":\"description\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"描述\",\"required\":1,\"status\":0},{\"pName\":\"install_status\",\"authority\":\"\",\"format\":\"bool\",\"pId\":1,\"valueDefinition\":\"安装结果true|false\",\"required\":1,\"status\":0},{\"pName\":\"install_version\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"安装的版本\",\"required\":0,\"status\":0}],\"name\":\"firmware_install_result\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":38},{\"data\":[],\"name\":\"hdev_file_occupancy\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":45},{\"data\":[{\"pName\":\"sd_use_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":11,\"valueDefinition\":\"sdcard已使用空间\",\"required\":1,\"status\":0},{\"pName\":\"sd_total_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":10,\"valueDefinition\":\"sdcard卡总空间\",\"required\":1,\"status\":0},{\"pName\":\"sd_available_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":9,\"valueDefinition\":\"sdcard卡可用空间\",\"required\":1,\"status\":0},{\"pName\":\"tf_use_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":8,\"valueDefinition\":\"tf卡已使用空间\",\"required\":1,\"status\":0},{\"pName\":\"tf_total_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":7,\"valueDefinition\":\"tf卡总空间\",\"required\":1,\"status\":0},{\"pName\":\"tf_available_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":6,\"valueDefinition\":\"tf卡可用空间\",\"required\":1,\"status\":0},{\"pName\":\"ram_use_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":4,\"valueDefinition\":\"ram已使用空间\",\"required\":1,\"status\":0},{\"pName\":\"ram_total_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":3,\"valueDefinition\":\"ram总空间\",\"required\":1,\"status\":0},{\"pName\":\"ram_available_size\",\"authority\":\"\",\"format\":\"int32\",\"pId\":2,\"valueDefinition\":\"ram可用空间\",\"required\":1,\"status\":0},{\"pName\":\"cpu_rate\",\"authority\":\"\",\"format\":\"float\",\"pId\":0,\"valueDefinition\":\"CPU占用率\",\"required\":1,\"status\":0}],\"name\":\"hdev_rsrc_monitor\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":4},{\"data\":[],\"name\":\"heartbeat\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":30},{\"data\":[{\"pName\":\"net_delay\",\"authority\":\"\",\"format\":\"string\",\"pId\":4,\"valueDefinition\":\"网络延迟\",\"required\":1,\"status\":0},{\"pName\":\"operator\",\"authority\":\"\",\"format\":\"string\",\"pId\":3,\"valueDefinition\":\"运营商\",\"required\":1,\"status\":0},{\"pName\":\"net_ip\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"ip地址\",\"required\":1,\"status\":0},{\"pName\":\"signal_strength\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"信号强度\",\"required\":1,\"status\":0},{\"pName\":\"net_type\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"网络类型\",\"required\":1,\"status\":0}],\"name\":\"network_monitor\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":49},{\"data\":[{\"pName\":\"address\",\"authority\":\"\",\"format\":\"string\",\"pId\":2,\"valueDefinition\":\"具体地址\",\"required\":1,\"status\":0},{\"pName\":\"latitude\",\"authority\":\"\",\"format\":\"string\",\"pId\":1,\"valueDefinition\":\"纬度\",\"required\":1,\"status\":0},{\"pName\":\"longitude\",\"authority\":\"\",\"format\":\"string\",\"pId\":0,\"valueDefinition\":\"经度\",\"required\":1,\"status\":0}],\"name\":\"online\",\"functionType\":0,\"required\":0,\"status\":0,\"sid\":35}]}";
    private TextView tvResult;
    private EditText etClientId;

    Handler handler = new Handler() {
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            FormatViewUtils.formatData(tvResult, msg.obj.toString() + "\n\r");
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tvResult = findViewById(R.id.tvResult);
        etClientId = findViewById(R.id.etClientId);
        etClientId.setText(Fsp2pTools.registerClientId());
        FormatViewUtils.setMovementMethod(tvResult);
    }

    public static void hearbeat() {
        new Thread() {
            @Override
            public void run() {
                super.run();
                while (true) {
                    FsPipelineJNI.postHeartbeat();
                    try {
                        Thread.sleep(1 * 60 * 1000);
                    } catch (Throwable throwable) {
                        throwable.printStackTrace();
                    }
                }
            }
        }.start();
    }

    public ConnParams getConnParams() {
        ConnParams connParams = new ConnParams();
        connParams.subDev = new SubDev();
        connParams.subDev.sn = etClientId.getText().toString().trim();
        connParams.subDev.name = "xxx";
        connParams.subDev.product_id = "139";
        connParams.subDev.model = "iotcloud";
        connParams.subDev.type = Type.Unknown;
        connParams.subDev.version = -1;
        connParams.json_protocol = Fsp2pTools.convertToJsonToBase64(JSON_PROTOCOL);
        connParams.userName = "fswl";
        connParams.passWord = "123456";
        connParams.host = "192.168.1.245";
        connParams.port = 1883;
        return connParams;
    }

    /**
     * 初始化
     *
     * @param view
     */
    public void initClick(View view) {
        FsPipelineJNI.init(getConnParams());

    }

    PipelineCallback pipelineCallback1 = new PipelineCallback() {
        @Override
        public void connectStatus(boolean status) {
            handler.sendMessage(handler.obtainMessage(0x00, "connectStatus1>>" + status));
            if (status) {
                FsPipelineJNI.postOnLine();
                hearbeat();
            }
        }

        @Override
        public void errCallback(int errCode, String description) {
            handler.sendMessage(handler.obtainMessage(0x00, "errCallback1: errCode>>" + errCode + ",description>>" + description));
        }

        @Override
        public void pipelineLog(int level, String str) {
            handler.sendMessage(handler.obtainMessage(0x00, "pipelineLog1: level>>" + level + ",str>>" + str));
        }

        @Override
        public void callback(Request request) {
            Map<String, String> params = new HashMap<>();
            params.put("params1", "1");
            params.put("params2", "");
            params.put("params3", null);
            params.put("params4", "{\"name\":\"ichtj\",\"age\":18,\"sex\":\"男\"}");
            int result = FsPipelineJNI.replyMethod(request, params);
            Log.d(TAG, "callback1: result>>" + result);
            handler.sendMessage(handler.obtainMessage(0x00, "request1: request>>" + request));
        }
    };

    PipelineCallback pipelineCallback2 = new PipelineCallback() {
        @Override
        public void connectStatus(boolean status) {
            handler.sendMessage(handler.obtainMessage(0x00, "connectStatus2>>" + status));
            if (status) {
                FsPipelineJNI.postOnLine();
                hearbeat();
            }
        }

        @Override
        public void errCallback(int errCode, String description) {
            handler.sendMessage(handler.obtainMessage(0x00, "errCallback2: errCode>>" + errCode + ",description>>" + description));
        }

        @Override
        public void pipelineLog(int level, String str) {
            handler.sendMessage(handler.obtainMessage(0x00, "pipelineLog2: level>>" + level + ",str>>" + str));
        }

        @Override
        public void callback(Request request) {
            Map<String, String> params = new HashMap<>();
            params.put("params1", "1");
            params.put("params2", "2");
            int result = FsPipelineJNI.replyMethod(request, params);
            Log.d(TAG, "callback2: result>>" + result);
            handler.sendMessage(handler.obtainMessage(0x00, "request2: request>>" + request));
        }
    };

    public void unRegisterCallback1(View view) {
        FsPipelineJNI.unRegisterCallback(pipelineCallback1);
    }

    public void unRegisterCallback2(View view) {
        FsPipelineJNI.unRegisterCallback(pipelineCallback2);
    }

    public void registerCallback1(View view) {
        FsPipelineJNI.addPipelineCallback(pipelineCallback1);
    }

    public void registerCallback2(View view) {
        FsPipelineJNI.addPipelineCallback(pipelineCallback2);
    }

    public void connectClick(View view) {
        FsPipelineJNI.connect();
    }

    public void disConnectClick(View view) {
        FsPipelineJNI.close();
    }

    public void getDeviceModelClick(View view) {
        List<SubDev> devModels = FsPipelineJNI.getDevModelList();
        String jsonFormat = JsonFormatUtils.formatJson(GsonTools.toJsonWtihNullField(devModels));
        Log.d(TAG, "getDeviceModelClick: jsonFormat>>"+jsonFormat);
        handler.sendMessage(handler.obtainMessage(0x00, jsonFormat));
    }

    public void clearTvClick(View view) {
        tvResult.setText("");
        tvResult.scrollTo(0, 0);
    }

    public void postEventClick(View view) {
        Map<String, String> out = new HashMap<>();
        out.put("fileName", "update.zip");
        out.put("percent", "1%");
        Event event = new Event("file_download_percent", out);
        FsPipelineJNI.pushEvent(event);
    }

    public void postEventsClick(View view) {
        List<Event> events = new ArrayList<>();
        Map<String, String> out1 = new HashMap<>();
        out1.put("apkName", "test");
        out1.put("packageName", "com.face.test");
        out1.put("isSys", "true");
        out1.put("unInstallStatus", "true");
        Event event1 = new Event("apk_uninstall_result", out1);
        events.add(event1);
        Map<String, String> out2 = new HashMap<>();
        out2.put("fileName", "update.zip");
        out2.put("percent", "1%");
        Event event2 = new Event("file_download_percent", out2);
        events.add(event2);
        FsPipelineJNI.pushEvents(events);
    }

    public void postReadClick(View view) {
        Map<String, String> out = new HashMap<>();
        out.put("iotcloud_version", "1.00.1");
        Service service = new Service("device", out, 0, "");
        FsPipelineJNI.pushRead(Fsp2pTools.getTargetSn(), service);
    }

    public void postReadListClick(View view) {
        List<Service> serviceList = new ArrayList<>();
        Map<String, String> out = new HashMap<>();
        out.put("nettype", "unknown");
        Service service1 = new Service("network", out, 0, "");
        serviceList.add(service1);
        Map<String, String> out2 = new HashMap<>();
        out2.put("androidversion", "7.1.0");
        Service service2 = new Service("device", out, 0, "");
        serviceList.add(service2);
        FsPipelineJNI.pushReadList(Fsp2pTools.getTargetSn(), serviceList);
    }

    public void postWriteClick(View view) {
        Map<String, String> out = new HashMap<>();
        out.put("dbm", "-1");
        Service service = new Service("network", out, 0, "");
        FsPipelineJNI.pushWrite(Fsp2pTools.getTargetSn(), service);
    }

    public void postWriteListClick(View view) {
        List<Service> serviceList = new ArrayList<>();
        Map<String, String> out1 = new HashMap<>();
        out1.put("topapp", "com.test.ichtj");
        Service service1 = new Service("device", out1, 0, "");
        serviceList.add(service1);
        Map<String, String> out2 = new HashMap<>();
        out2.put("lteoperator", "zgyd");
        Service service2 = new Service("network", out2, 0, "");
        serviceList.add(service2);
        FsPipelineJNI.pushWriteList(Fsp2pTools.getTargetSn(), serviceList);
    }

    public void postNotifyClick(View view) {
        Map<String, String> out = new HashMap<>();
        out.put("backlight", "248");
        Service service = new Service("device", out, 0, "");
        FsPipelineJNI.pushNotify(service);
    }

    public void postNotifyListClick(View view) {
        List<Service> serviceList = new ArrayList<>();
        Map<String, String> out1 = new HashMap<>();
        out1.put("iccid", "123456");
        out1.put("lteoperator", "中国移动");
        Service service1 = new Service("network", out1, 0, "");
        serviceList.add(service1);
        FsPipelineJNI.pushNotifyList(serviceList);
    }

    public void postMethodClick(View view) {
        Map<String, String> out1 = new HashMap<>();
        out1.put("srcPath", "/sdcard/DCIM/test.log");
        out1.put("desPath", "/sdcard/");
        Method out = new Method("file_move", out1, 0, "");
        FsPipelineJNI.pushMethod(Fsp2pTools.getTargetSn(), out);
    }

    public void postMethodsClick(View view) {
        List<Method> methodList = new ArrayList<>();
        Map<String, String> out1 = new HashMap<>();
        out1.put("filePath", "/sdcard/DCIM/");
        Method method1 = new Method("file_del", out1, 0, "");
        Map<String, String> out2 = new HashMap<>();
        out2.put("url", "www.baidu.com");
        out2.put("name", "update");
        out2.put("extension", "zip");
        Method method2 = new Method("file_download", out2, 0, "");
        methodList.add(method2);
        FsPipelineJNI.pushMethods(Fsp2pTools.getTargetSn(), methodList);
    }
}