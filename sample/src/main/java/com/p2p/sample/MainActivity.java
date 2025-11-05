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
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.ToggleButton;

import com.library.natives.BaseData;
import com.library.natives.BaseFsP2pTools;
import com.library.natives.BlackBean;
import com.library.natives.IBlackCallback;
import com.library.natives.IInfomationsCallback;
import com.library.natives.IPipelineCallback;
import com.library.natives.Infomation;
import com.library.natives.PutType;
import com.library.natives.SubDev;
import com.library.natives.FsPipelineJNI;
import com.library.natives.Fsp2pTools;
import com.library.natives.Type;
import com.library.natives.XCoreBean;

import org.json.JSONArray;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class MainActivity extends AppCompatActivity{
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
        btnPrintLog.setText(BaseFsP2pTools.isLogEnable() ? "日志:开" : "日志:关");
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
                Type.Unknown,-1);
        XCoreBean xCoreBean=new XCoreBean ("192.168.1.177",1883,"fswl","123456" );
        BaseFsP2pTools.connect (infomation,xCoreBean,JSON_PROTOCOL, new IPipelineCallback ( ) {
            @Override
            public void p2pConnState(boolean connected, String description) {
                handler.sendMessage(handler.obtainMessage(0x00, "p2pConnState : "+ connected+",description>>"+description ));
            }


            @Override
            public void iotConnState(boolean connected, String description) {
                handler.sendMessage(handler.obtainMessage(0x00, "mqttConnState : "+ connected+",description>>"+description ));
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
                BaseFsP2pTools.putIotReply (baseData.iPutType,baseData.iid,baseData.operation, maps);
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

    public void connectClick(View view) {
        toConnectMq (etClientId.getText ().toString ().trim ());
    }

    public void getConnectStateClick(View view) {
        boolean connState= BaseFsP2pTools.getConnectStatus();
        handler.sendMessage(handler.obtainMessage(0x00, "getIotConnectState："+connState));
    }

    public void disConnectClick(View view) {
        BaseFsP2pTools.disConnect ();
    }

    public void subscribeClick(View view) {
        Infomation infomation=new Infomation ("FSM-0ba5f8-000070","b3f08e21-d8af-4c34-adb8-f07de0edde79","xx","edge_sub_dev",
                Type.Unknown,-1);
        boolean isComplete= BaseFsP2pTools.subscribe (infomation);
        handler.sendMessage(handler.obtainMessage(0x00, "subscribeClick>>infomation : "+infomation+",isComplete : "+isComplete));
    }


    public void unSubscribeClick(View view) {
        Infomation infomation=new Infomation ("FSM-0ba5f8-000070","b3f08e21-d8af-4c34-adb8-f07de0edde79","xx","edge_sub_dev",
                Type.Unknown,-1);
        boolean isComplete= BaseFsP2pTools.unSubscribe (infomation);
        handler.sendMessage(handler.obtainMessage(0x00, "unSubscribeClick>>infomation : "+infomation+",isComplete : "+isComplete));
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
        BaseFsP2pTools.logEnable(isEnable);
        btnPrintLog.setText(isEnable ? "日志:开" : "日志:关");
    }

    public void getInfomationsClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("fileName", "update.zip");
        out.put("percent", "1%");
        BaseFsP2pTools.getInfomationList (new IInfomationsCallback ( ) {
            @Override
            public void deivces(List<Infomation> infomations) {
                handler.sendMessage(handler.obtainMessage(0x00, "getInfomationsClick：infomations>"+infomations));
            }
        });
    }

    public void postEventClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("fileName", "update.zip");
        out.put("percent", "1%");
        boolean isComplete= BaseFsP2pTools.postMsg (PutType.EVENT,"WN1241222","22122a20-5ea8-40ef-b7d2-329ee4207474","file_download_percent",out);
        handler.sendMessage(handler.obtainMessage(0x00, "postEventClick：isComplete>"+isComplete));
    }

    public void postReadClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("android_version", "");
        boolean isComplete= BaseFsP2pTools.postMsg (PutType.GETPERTIES,"WN1241222","22122a20-5ea8-40ef-b7d2-329ee4207474","device",out);
        handler.sendMessage(handler.obtainMessage(0x00, "postReadClick：isComplete>"+isComplete));
    }


    public void postWriteClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("backlight", "150");
        boolean isComplete= BaseFsP2pTools.postMsg (PutType.SETPERTIES,"WN1241222","22122a20-5ea8-40ef-b7d2-329ee4207474","device",out);
        handler.sendMessage(handler.obtainMessage(0x00, "postWriteClick：isComplete>"+isComplete));
    }

    public void postNotifyClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("base_station_port", "192.158.145.100");
        boolean isComplete= BaseFsP2pTools.postMsg (PutType.UPLOAD,"WN1241222","22122a20-5ea8-40ef-b7d2-329ee4207474","network",out);
        handler.sendMessage(handler.obtainMessage(0x00, "postNotifyClick：isComplete>"+isComplete));
    }

    public void postMethodClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("sn", "");
        out.put("product_id", "");
        boolean isComplete= BaseFsP2pTools.postMsg (PutType.METHOD,"FSCOREaedff2dc03dc-FSM-ff0981","173206b4-7235-454f-ad51-0604dd9ef8a8","gw_node_list",out);
        handler.sendMessage(handler.obtainMessage(0x00, "postMethodClick：isComplete>"+isComplete));
    }


    public void setIotBlacklistClick(View view) {
        Map<String, Object> out = new HashMap<>();
        out.put("devices_array",GsonTools.toJsonWtihNullField (new String[]{"001","002","003"}));
        out.put("model_array",  GsonTools.toJsonWtihNullField (new String[]{"x01","x02","x03"}));
        out.put("desc", "这是一个描述");
        boolean isComplete=BaseFsP2pTools.setBlackList (out);
        handler.sendMessage(handler.obtainMessage(0x00, "setIotBlacklistClick：isComplete>"+isComplete));
    }


    public void getIotBlacklistClick(View view) {
        BaseFsP2pTools.getBlackList (new IBlackCallback ( ) {
            @Override
            public void onBlack(List<BlackBean> beanList) {
                handler.sendMessage(handler.obtainMessage(0x00, "getIotBlacklistClick：beanList>"+beanList));
            }
        });
    }
}