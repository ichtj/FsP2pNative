package com.p2p.sample;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.library.natives.BaseData;
import com.library.natives.BaseXLink;
import com.library.natives.ConnParams;
import com.library.natives.Device;
import com.library.natives.IMqttCallback;
import com.library.natives.Infomation;
import com.library.natives.Payload;
import com.library.natives.PutType;
import com.library.natives.Response;
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
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class MainActivity extends AppCompatActivity {
    private static final String TAG = MainActivity.class.getSimpleName();
    private static final String JSON_PROTOCOL = "{\"service\":[{\"data\":[{\"unit\":\"0\",\"pName\":\"net_type\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"网络类型\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"local_Ip\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"本机ip\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"eth_mode\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"以太网ip模式\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"dbm\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"4G信号值\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"baseband\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":4,\"functionType\":0,\"valueDefinition\":\"4G基带版本\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"reset_4g\",\"authority\":\"write\",\"flagName\":null,\"format\":\"string\",\"pid\":5,\"functionType\":0,\"valueDefinition\":\"4G复位 无返回值\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"wifi_status\",\"authority\":\"read,write\",\"flagName\":null,\"format\":\"string\",\"pid\":6,\"functionType\":0,\"valueDefinition\":\"WIFI 开启0/关闭1\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"iccid\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":7,\"functionType\":0,\"valueDefinition\":\"4g sim卡iccid\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"lteoperator\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":8,\"functionType\":0,\"valueDefinition\":\"4G卡运营商\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"base_station_port\",\"authority\":\"read,write,upload\",\"flagName\":null,\"format\":\"int32\",\"pid\":9,\"functionType\":0,\"valueDefinition\":\"4G基站端口\",\"required\":1,\"status\":0}],\"name\":\"network\",\"remark\":\"网络相关\",\"functionType\":0,\"required\":0,\"sid\":15,\"status\":0},{\"data\":[{\"unit\":\"0\",\"pName\":\"backlight\",\"authority\":\"read,write\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"背光值[0~255]\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"android_version\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"安卓版本\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"time\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"系统时间\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"imei\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"设备imei\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"usbList\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":4,\"functionType\":0,\"valueDefinition\":\"获取接入的usb列表\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"top_app\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":5,\"functionType\":0,\"valueDefinition\":\"获取顶层app\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"fwversion\",\"authority\":\"read,upload\",\"flagName\":null,\"format\":\"string\",\"pid\":6,\"functionType\":0,\"valueDefinition\":\"固件版本\",\"required\":1,\"status\":0},{\"unit\":\"M\",\"pName\":\"tf_space\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":7,\"functionType\":0,\"valueDefinition\":\"TF卡空间(M)\",\"required\":1,\"status\":0},{\"unit\":\"0\",\"pName\":\"fw_sys_version\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":8,\"functionType\":0,\"valueDefinition\":\"显示固件版本具体信息\",\"required\":0,\"status\":0},{\"unit\":\"0\",\"pName\":\"Iotcloud_version\",\"authority\":\"read,upload\",\"flagName\":null,\"format\":\"string\",\"pid\":9,\"functionType\":0,\"valueDefinition\":\"IotCloud APK版本\",\"required\":0,\"status\":0},{\"unit\":\"day;HH:HH;boolean\",\"pName\":\"system_restart\",\"authority\":\"read,write,upload\",\"flagName\":null,\"format\":\"string\",\"pid\":10,\"functionType\":0,\"valueDefinition\":\"重启机制 [频次;时间1:时间2;开关]\",\"required\":0,\"status\":0},{\"unit\":\"时分秒\",\"pName\":\"running_time\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":11,\"functionType\":0,\"valueDefinition\":\"设备运行时间\",\"required\":0,\"status\":0},{\"unit\":\"MB\",\"pName\":\"sdcard_space\",\"authority\":\"read\",\"flagName\":null,\"format\":\"string\",\"pid\":12,\"functionType\":0,\"valueDefinition\":\"sdcard空间\",\"required\":0,\"status\":0},{\"unit\":\"bool\",\"pName\":\"net_lost_reset\",\"authority\":\"read,write,upload\",\"flagName\":null,\"format\":\"bool\",\"pid\":13,\"functionType\":0,\"valueDefinition\":\"网络丢失模块重置\",\"required\":1,\"status\":0},{\"unit\":\"bool\",\"pName\":\"net_lost_restart\",\"authority\":\"read,write,upload\",\"flagName\":null,\"format\":\"bool\",\"pid\":15,\"functionType\":0,\"valueDefinition\":\"网络丢失一小时重启\",\"required\":1,\"status\":0}],\"name\":\"device\",\"remark\":\"设备属性\",\"functionType\":0,\"required\":0,\"sid\":16,\"status\":0}],\"action\":[{\"data\":[{\"paramType\":0,\"pName\":\"is_system_apk\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"是否是系统应用\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"apk_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"version\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"apk版本\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"package_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"url\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":4,\"functionType\":0,\"valueDefinition\":\"下载地址\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"is_required_restart\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":7,\"functionType\":0,\"valueDefinition\":\"是否需要重启true|false\",\"required\":1,\"status\":0}],\"name\":\"install_apk\",\"remark\":\"安装结果以 apk_install_result事件为准\",\"functionType\":0,\"required\":0,\"sid\":208,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"is_system_apk\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"是否是系统应用\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"apk_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"package_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"apk包名\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"is_required_restart\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":6,\"functionType\":0,\"valueDefinition\":\"是否需要重启true|false\",\"required\":1,\"status\":0}],\"name\":\"uninstall_apk\",\"remark\":\"卸载结果以 apk_uninstall_result事件为准\",\"functionType\":0,\"required\":0,\"sid\":209,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"package_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"执行结果true|false\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0}],\"name\":\"clean_app_cache\",\"remark\":\"-\",\"functionType\":0,\"required\":0,\"sid\":210,\"status\":0},{\"data\":[{\"paramType\":1,\"pName\":\"app_list\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"app列表\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0}],\"name\":\"get_app_list\",\"remark\":\"-\",\"functionType\":0,\"required\":0,\"sid\":211,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"cmd\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"命令\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"data\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"返回数据\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0}],\"name\":\"remote_cmd\",\"remark\":\"-\",\"functionType\":0,\"required\":0,\"sid\":212,\"status\":0},{\"data\":[{\"paramType\":1,\"pName\":\"file_list\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"返回指定路径的文件及目录\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"directory\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"路径\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":4,\"functionType\":0,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0}],\"name\":\"file_get_list\",\"remark\":\"-\",\"functionType\":0,\"required\":0,\"sid\":213,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"url\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"下载地址\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"file_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"文件名称\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"file_type\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"文件格式\",\"required\":1,\"status\":0}],\"name\":\"file_download\",\"remark\":\"下载结果以 file_download_percent事件为准\",\"functionType\":0,\"required\":0,\"sid\":214,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"file_path\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"文件路径\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0}],\"name\":\"file_del\",\"remark\":\"文件,或文件夹删除\",\"functionType\":0,\"required\":0,\"sid\":215,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"src_path\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"原始路径\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"des_path\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"目标路径\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0}],\"name\":\"file_move\",\"remark\":\"文件或者文件夹移动\",\"functionType\":0,\"required\":0,\"sid\":216,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"toolbar_status\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"0为显示 1为隐藏\",\"required\":1,\"status\":0}],\"name\":\"show_hide_toolbar\",\"remark\":\"显示隐藏系统状态栏\",\"functionType\":0,\"required\":0,\"sid\":217,\"status\":0},{\"data\":[{\"paramType\":1,\"pName\":\"base64\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"图片base64\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"img_url\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"文件路径\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"img_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"uint32\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"截屏文件大小\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":4,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0}],\"name\":\"screenshot\",\"remark\":\"-\",\"functionType\":0,\"required\":0,\"sid\":218,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"firmware_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"文件名称\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"firmware_type\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":4,\"functionType\":0,\"valueDefinition\":\"文件格式\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"url\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":5,\"functionType\":0,\"valueDefinition\":\"从设备固件地址\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"firmware_version\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":8,\"functionType\":0,\"valueDefinition\":\"从设备固件版本\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"node_addr\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":9,\"functionType\":0,\"valueDefinition\":\"从设备地址\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"node_baudrate\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":10,\"functionType\":0,\"valueDefinition\":\"串口波特率\",\"required\":1,\"status\":0}],\"name\":\"com_upgrade\",\"remark\":\"更新锁控板固件\",\"functionType\":0,\"required\":0,\"sid\":219,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"filePath\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":1,\"valueDefinition\":\"在设备上的路径 例如：/sdcard/LOGSAVE/\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"fileName\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":1,\"valueDefinition\":\"文件名 例如：20210810.log\",\"required\":1,\"status\":0}],\"name\":\"log_upload\",\"remark\":\"日志上传\",\"functionType\":1,\"required\":0,\"sid\":247,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"directory\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"文件所处的文件夹路径\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"file_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"文件名称\",\"required\":0,\"status\":0},{\"paramType\":1,\"pName\":\"result\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"执行结果\",\"required\":1,\"status\":0},{\"paramType\":1,\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":0,\"status\":0}],\"name\":\"file_upload\",\"remark\":null,\"functionType\":0,\"required\":0,\"sid\":206,\"status\":0},{\"data\":[{\"paramType\":0,\"pName\":\"userName\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":0,\"functionType\":1,\"valueDefinition\":\"用户名\",\"required\":1,\"status\":0},{\"paramType\":0,\"pName\":\"password\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":1,\"valueDefinition\":\"密码\",\"required\":1,\"status\":0}],\"name\":\"sign\",\"remark\":null,\"functionType\":1,\"required\":0,\"sid\":385,\"status\":0}],\"event\":[{\"data\":[{\"pName\":\"cpu_rate\",\"authority\":\"\",\"flagName\":null,\"format\":\"float\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"CPU占用率\",\"required\":1,\"status\":0},{\"pName\":\"ram_available_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"ram可用空间\",\"required\":1,\"status\":0},{\"pName\":\"ram_total_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"ram总空间\",\"required\":1,\"status\":0},{\"pName\":\"ram_use_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":4,\"functionType\":0,\"valueDefinition\":\"ram已使用空间\",\"required\":1,\"status\":0},{\"pName\":\"tf_available_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":6,\"functionType\":0,\"valueDefinition\":\"tf卡可用空间\",\"required\":1,\"status\":0},{\"pName\":\"tf_total_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":7,\"functionType\":0,\"valueDefinition\":\"tf卡总空间\",\"required\":1,\"status\":0},{\"pName\":\"tf_use_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":8,\"functionType\":0,\"valueDefinition\":\"tf卡已使用空间\",\"required\":1,\"status\":0},{\"pName\":\"sd_available_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":9,\"functionType\":0,\"valueDefinition\":\"sdcard卡可用空间\",\"required\":1,\"status\":0},{\"pName\":\"sd_total_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":10,\"functionType\":0,\"valueDefinition\":\"sdcard卡总空间\",\"required\":1,\"status\":0},{\"pName\":\"sd_use_size\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":11,\"functionType\":0,\"valueDefinition\":\"sdcard已使用空间\",\"required\":1,\"status\":0}],\"name\":\"hdev_rsrc_monitor\",\"remark\":\"-\",\"functionType\":0,\"required\":0,\"sid\":203,\"status\":0},{\"data\":[],\"name\":\"heartbeat\",\"remark\":\"\",\"functionType\":0,\"required\":0,\"sid\":204,\"status\":0},{\"data\":[{\"pName\":\"app_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0},{\"pName\":\"package_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0},{\"pName\":\"is_system_apk\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"是否是系统应用\",\"required\":1,\"status\":0},{\"pName\":\"install_status\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"安装是否完成\",\"required\":1,\"status\":0}],\"name\":\"apk_install_result\",\"remark\":\"\",\"functionType\":0,\"required\":0,\"sid\":205,\"status\":0},{\"data\":[{\"pName\":\"app_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"apk名称\",\"required\":1,\"status\":0},{\"pName\":\"package_name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"包名\",\"required\":1,\"status\":0},{\"pName\":\"is_system_apk\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"是否为系统应用\",\"required\":1,\"status\":0},{\"pName\":\"uninstall_status\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":3,\"functionType\":0,\"valueDefinition\":\"卸载状态\",\"required\":1,\"status\":0}],\"name\":\"apk_uninstall_result\",\"remark\":\"\",\"functionType\":0,\"required\":0,\"sid\":206,\"status\":0},{\"data\":[{\"pName\":\"longitude\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"经度\",\"required\":1,\"status\":0},{\"pName\":\"latitude\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"纬度\",\"required\":1,\"status\":0},{\"pName\":\"address\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"具体地址\",\"required\":1,\"status\":0}],\"name\":\"online\",\"remark\":\"\",\"functionType\":0,\"required\":0,\"sid\":207,\"status\":0},{\"data\":[{\"pName\":\"percent\",\"authority\":\"\",\"flagName\":null,\"format\":\"int32\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"文件下载进度\",\"required\":1,\"status\":0},{\"pName\":\"name\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"文件名称\",\"required\":1,\"status\":0}],\"name\":\"file_download_percent\",\"remark\":null,\"functionType\":0,\"required\":0,\"sid\":209,\"status\":0},{\"data\":[{\"pName\":\"install_version\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":0,\"functionType\":0,\"valueDefinition\":\"安装的版本\",\"required\":0,\"status\":0},{\"pName\":\"install_status\",\"authority\":\"\",\"flagName\":null,\"format\":\"bool\",\"pid\":1,\"functionType\":0,\"valueDefinition\":\"安装结果true|false\",\"required\":1,\"status\":0},{\"pName\":\"description\",\"authority\":\"\",\"flagName\":null,\"format\":\"string\",\"pid\":2,\"functionType\":0,\"valueDefinition\":\"描述\",\"required\":1,\"status\":0}],\"name\":\"firmware_install_result\",\"remark\":null,\"functionType\":0,\"required\":0,\"sid\":208,\"status\":0},{\"data\":[],\"name\":\"file_upload_result\",\"remark\":null,\"functionType\":0,\"required\":0,\"sid\":337,\"status\":0},{\"data\":[],\"name\":\"hdev_file_occupancy\",\"remark\":\"\",\"functionType\":0,\"required\":0,\"sid\":338,\"status\":0},{\"data\":[],\"name\":\"file_upload_percent\",\"remark\":null,\"functionType\":0,\"required\":0,\"sid\":403,\"status\":0},{\"data\":[],\"name\":\"door_record\",\"remark\":null,\"functionType\":1,\"required\":0,\"sid\":424,\"status\":0}]}";
    private TextView tvResult;
    private Button btnPrintLog;
    private EditText etClientId;
    private boolean isEnable = true;

    Handler handler = new Handler() {
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            FormatViewUtils.formatData(tvResult, msg.obj.toString() ,"yyyy-MM-dd HH:mm:ss:SSS");
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        btnPrintLog = findViewById(R.id.btnPrintLog);
        btnPrintLog.setText(BaseXLink.isLogEnable() ? "日志:开" : "日志:关");
        tvResult = findViewById(R.id.tvResult);
        etClientId = findViewById(R.id.etClientId);
        String readSn= Fsp2pTools.registerClientId();
        if (!TextUtils.isEmpty (readSn)) {
            etClientId.setText(readSn);
        }
        FormatViewUtils.setMovementMethod(tvResult);
    }

    private void toConnectMq(String readSn) {
        Infomation infomation=new Infomation (readSn,"22122a20-5ea8-40ef-b7d2-329ee4207474","xx","iotCloud",
                Type.Unknown,-1,"192.168.1.127",1883,"fswl","123456",JSON_PROTOCOL );
        BaseXLink.connect (infomation, new IMqttCallback ( ) {
            @Override
            public void connState(boolean connected, String description) {
                handler.sendMessage(handler.obtainMessage(0x00, "connState : "+ connected+",description>>"+description ));
            }

            @Override
            public void msgArrives(BaseData baseData) {
                handler.sendMessage(handler.obtainMessage(0x00, "msgArrives : "+baseData.toString ()));
                Log.d (TAG, "msgArrives: "+baseData.toString ()+",putType>>"+baseData.iPutType);
                Map<String, Object> maps=new HashMap<> (  );
                if (baseData.iPutType== PutType.METHOD){
                    maps.put ("data",System.currentTimeMillis ());
                }else if (baseData.iPutType==PutType.GETPERTIES){
                    maps=baseData.maps;;
                    Iterator <String> iterator=maps.keySet ().iterator ();
                    while (iterator.hasNext ()){
                        String key=iterator.next ();
                        Object value=maps.get (key);
                        Log.d (TAG, "msgArrives: key>>"+key+",value>>"+value );
                        maps.put (key,System.currentTimeMillis ());
                    }
                }else if (baseData.iPutType==PutType.SETPERTIES){
                    maps=baseData.maps;;
                    Iterator <String> iterator=maps.keySet ().iterator ();
                    while (iterator.hasNext ()){
                        String key=iterator.next ();
                        Object value=maps.get (key);
                        Log.d (TAG, "msgArrives: key>>"+key+",value>>"+value );
                        maps.put (key,value);
                    }
                }
                BaseXLink.putReply (baseData.iPutType,baseData.iid,baseData.operation, maps);
            }

            @Override
            public void pushed(BaseData data) {
                handler.sendMessage(handler.obtainMessage(0x00, "pushed : "+data.toString ()));
            }

            @Override
            public void iotReplyed(String act, String iid) {
                Log.d (TAG, "iotReplyed: ");
            }

            @Override
            public void pushFail(BaseData baseData, String description) {
                handler.sendMessage(handler.obtainMessage(0x00, "pushFailData: "+baseData.toString ()+",description>>"+description));
            }

            @Override
            public void subscribed(String topic) {
                handler.sendMessage(handler.obtainMessage(0x00, "subscribed: topic>>"+topic));
            }

            @Override
            public void subscribeFail(String topic, String description) {
                handler.sendMessage(handler.obtainMessage(0x00, "subscribeFail: topic>>"+topic+",description>>"+description)) ;;
            }
        });
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
        connParams.subDev.product_id = "19";
        connParams.subDev.model = "iotcloud";
        connParams.subDev.type = Type.Unknown;
        connParams.subDev.version = -1;
        connParams.json_protocol = Fsp2pTools.convertToJsonToBase64(JSON_PROTOCOL);
        connParams.userName = "fswl";
        connParams.passWord = "123456";
        connParams.host = "192.168.1.127";
        connParams.port = 1883;
        return connParams;
    }


    int count = 0;
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
        public void request(Request request) {
            Map<String, Object> out = new HashMap<>();
            switch (request.action) {
                case Action_Read:
                    Payload payload = request.payload;
                    Map<String, Device> devices = payload.devices;
                    Set<String> set = devices.keySet();
                    for (String key : set) {
                        Device device = devices.get(key);
                        List<Service> services = device.services;
                        for (Service s : services) {
                            Map<String, Object> in = s.propertys;
                            Set<String> inSet = in.keySet();
                            for (String k : inSet) {
                                Log.d(TAG, "receive1: key>>" + key + ",device>>" + device.toString());
                                out.put(k, (count++) + "");
                                FsPipelineJNI.replyService(request, out);
                            }
                        }

                    }
                    break;
                case Action_Method:
                    out.put("params0", false);
                    out.put("params1", "1");
                    out.put("params2", "?");
                    out.put("params3", null);
                    out.put("params4", "{\"name\":\"ichtj\",\"age\":18,\"sex\":\"男\"}");
                    out.put("params5", "[ \"dfgsdg\" ] The path entered does not exist！");
                    out.put("params6", 2);
                    out.put("params6", 1.02);
                    int result = FsPipelineJNI.replyMethod(request, out);
                    Log.d(TAG, "receive1: result>>" + result);
                    break;
            }
            handler.sendMessage(handler.obtainMessage(0x00, "receive1: request>>" + request));
        }

        @Override
        public void response(Response response) {
            handler.sendMessage(handler.obtainMessage(0x00, "response1: response>>" + response));
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
        public void response(Response response) {
            Log.d(TAG, "response2: "+response);
            handler.sendMessage(handler.obtainMessage(0x00, "response2: response>>" + response));
        }

        @Override
        public void request(Request request) {
            try {
                Thread.sleep(5000);
            } catch (Throwable throwable) {
                throwable.printStackTrace();
            }
            Log.d(TAG, "receive2: threadName>>" + Thread.currentThread().getName());
            Map<String, Object> params = new HashMap<>();
            params.put("params1", "1");
            params.put("params2", "2");
            int result = FsPipelineJNI.replyMethod(request, params);
            Log.d(TAG, "receive2: result>>" + result);
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
        toConnectMq (etClientId.getText ().toString ().trim ());
    }

    public void getConnectStateClick(View view) {
        boolean connState=BaseXLink.getConnectStatus();
        handler.sendMessage(handler.obtainMessage(0x00, "getConnectState："+connState));
    }

    public void disConnectClick(View view) {
        BaseXLink.disConnect ();
    }

    public void getDeviceModelClick(View view) {
        List<SubDev> devModels = FsPipelineJNI.getDevModelList();
        String jsonFormat = JsonFormatUtils.formatJson(GsonTools.toJsonWtihNullField(devModels));
        Log.d(TAG, "getDeviceModelClick: jsonFormat>>" + jsonFormat);
        handler.sendMessage(handler.obtainMessage(0x00, jsonFormat));
    }

    public void clearTvClick(View view) {
        tvResult.setText("");
        tvResult.scrollTo(0, 0);
    }

    public void printLogClick(View view) {
        isEnable = !isEnable;
        BaseXLink.logEnable(isEnable);
        btnPrintLog.setText(isEnable ? "日志:开" : "日志:关");
    }

    public void postEventClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("fileName", "update.zip");
        out.put("percent", "1%");
        boolean isComplete=BaseXLink.postMsg (PutType.EVENT,"FSM-0ba5f8-000069","b3f08e21-d8af-4c34-adb8-f07de0edde79","snapshot",out);
        handler.sendMessage(handler.obtainMessage(0x00, "postEventClick：isComplete>"+isComplete));
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
        String iid=FsPipelineJNI.pushEvents(events);
        handler.sendMessage(handler.obtainMessage(0x00, "postEventsClick：iid>"+iid));
    }

    public void postReadClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("iotcloud_version", "1.00.1");
        Service service = new Service("device", out, 0, "");
        String iid=FsPipelineJNI.pushRead(Fsp2pTools.getTargetSn(), service);
        handler.sendMessage(handler.obtainMessage(0x00, "postReadClick：iid>"+iid));
    }

    public void postReadListClick(View view) {
        List<Service> serviceList = new ArrayList<>();
        Map<String, Object> out = new HashMap<>();
        out.put("nettype", "unknown");
        Service service1 = new Service("network", out, 0, "");
        serviceList.add(service1);
        Map<String, String> out2 = new HashMap<>();
        out2.put("androidversion", "7.1.0");
        Service service2 = new Service("device", out, 0, "");
        serviceList.add(service2);
        String iid=FsPipelineJNI.pushReadList(Fsp2pTools.getTargetSn(), serviceList);
        handler.sendMessage(handler.obtainMessage(0x00, "postReadListClick：iid>"+iid));
    }

    public void postWriteClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("dbm", "-1");
        Service service = new Service("network", out, 0, "");
        String iid=FsPipelineJNI.pushWrite(Fsp2pTools.getTargetSn(), service);
        handler.sendMessage(handler.obtainMessage(0x00, "postWriteClick：iid>"+iid));
    }

    public void postWriteListClick(View view) {
        List<Service> serviceList = new ArrayList<>();
        Map<String, Object> out1 = new HashMap<>();
        out1.put("topapp", "com.test.ichtj");
        Service service1 = new Service("device", out1, 0, "");
        serviceList.add(service1);
        Map<String, Object> out2 = new HashMap<>();
        out2.put("lteoperator", "zgyd");
        Service service2 = new Service("network", out2, 0, "");
        serviceList.add(service2);
        String iid=FsPipelineJNI.pushWriteList(Fsp2pTools.getTargetSn(), serviceList);
        handler.sendMessage(handler.obtainMessage(0x00, "postWriteListClick：iid>"+iid));
    }

    public void postNotifyClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("backlight", "248");
        Service service = new Service("device", out, 0, "");
        String iid=FsPipelineJNI.pushNotify(service);
        handler.sendMessage(handler.obtainMessage(0x00, "postNotifyClick：iid>"+iid));
    }

    public void postNotifyListClick(View view) {
        List<Service> serviceList = new ArrayList<>();
        Map<String, Object> out1 = new HashMap<>();
        out1.put("iccid", "123456");
        out1.put("lteoperator", "中国移动");
        Service service1 = new Service("network", out1, 0, "");
        serviceList.add(service1);
        String iid=FsPipelineJNI.pushNotifyList(serviceList);
        handler.sendMessage(handler.obtainMessage(0x00, "postNotifyListClick：iid>"+iid));
    }

    public void postMethodClick(View view) {
        Map<String, Object> out1 = new HashMap<>();
        out1.put("srcPath", "/sdcard/DCIM/test.log");
        out1.put("desPath", "/sdcard/");
        Method out = new Method("file_move", out1, 0, "");
        String iid=FsPipelineJNI.pushMethod(Fsp2pTools.getTargetSn(), out);
        handler.sendMessage(handler.obtainMessage(0x00, "postMethodClick：iid>"+iid));
    }

    public void postMethodsClick(View view) {
        List<Method> methodList = new ArrayList<>();
        Map<String, Object> out1 = new HashMap<>();
        out1.put("filePath", "/sdcard/DCIM/");
        Method method1 = new Method("file_del", out1, 0, "");
        Map<String, Object> out2 = new HashMap<>();
        out2.put("url", "www.baidu.com");
        out2.put("name", "update");
        out2.put("extension", "zip");
        Method method2 = new Method("file_download", out2, 0, "");
        methodList.add(method2);
        String iid=FsPipelineJNI.pushMethods(Fsp2pTools.getTargetSn(), methodList);
        handler.sendMessage(handler.obtainMessage(0x00, "postMethodClick：iid>"+iid));
    }
}