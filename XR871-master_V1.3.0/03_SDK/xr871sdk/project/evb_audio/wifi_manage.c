#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/cmd/cmd.h"
#include "common/cmd/cmd_defs.h"
#include "common/framework/sysinfo.h"
#include "driver/component/component_def.h"
#include "string.h"
#include "pthread.h"
#include "wifi_manage.h"


static uint8_t              wifi_task_run = 0;
static OS_Thread_t          wifi_manage_task_thread;
static wlan_sta_states_t     wifiState = WLAN_STA_STATE_DISCONNECTED;

wlan_sta_states_t getWifiState(void)
{
    return wifiState;
}

void wifi_task(void *arg)
{
    int disConStateCount=0;
    uint8_t runState=CMD_WIFI_SET_SSID;
    char cmd[256]={0};
    int cmdStatus=CMD_STATUS_FAIL;
    struct sysinfo *sysinfo = sysinfo_get();

    WIFI_MANAGE_TRACK_INFO("wifi task start\n");
	while (wifi_task_run) 
	{
	    switch(runState){
        case CMD_WIFI_SET_SSID:
            {
                memset(cmd,0,sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"set ssid %s",sysinfo->wlan_sta_param.ssid);
                cmdStatus=cmd_wlan_sta_exec(cmd);
                if(cmdStatus==CMD_STATUS_OK || cmdStatus==CMD_STATUS_ACKED){
                   runState=CMD_WIFI_SET_PSK; 
                }
                WIFI_MANAGE_TRACK_INFO("wifi run %s,return %d\n",cmd,cmdStatus);
            }
            break;
        case CMD_WIFI_SET_PSK:
            {
                memset(cmd,0,sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"set psk %s",sysinfo->wlan_sta_param.psk);
                cmdStatus=cmd_wlan_sta_exec(cmd);
                if(cmdStatus==CMD_STATUS_OK || cmdStatus==CMD_STATUS_ACKED){
                   runState=CMD_WIFI_ENABLE; 
                }
                WIFI_MANAGE_TRACK_INFO("wifi run %s,return %d\n",cmd,cmdStatus);
            }
            break;
        case CMD_WIFI_ENABLE:
            {
                memset(cmd,0,sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"enable");
                cmdStatus=cmd_wlan_sta_exec(cmd);
                if(cmdStatus==CMD_STATUS_OK || cmdStatus==CMD_STATUS_ACKED){
                   runState=CMD_WIFI_DETECT; 
                }
                WIFI_MANAGE_TRACK_INFO("wifi run %s,return %d\n",cmd,cmdStatus);
            }
            break;
        case CMD_WIFI_DETECT:
            {
                wlan_sta_state(&wifiState);
                if(wifiState==WLAN_STA_STATE_DISCONNECTED)
                    disConStateCount++;
                if(disConStateCount>10){
                    disConStateCount=0;
                    wlan_sta_connect();
                    WIFI_MANAGE_TRACK_INFO("wifi run wlan_sta_connect\n");
                }
            }
            break;
        default:
            break;
        }
		OS_MSleep(500);
	}
	WIFI_MANAGE_TRACK_INFO("wifi task end\n");
}

Component_Status wifi_task_init()
{
	wifi_task_run = 1;
	if (OS_ThreadCreate(&wifi_manage_task_thread,
		                "",
		                wifi_task,
		               	NULL,
		                OS_THREAD_PRIO_APP,
		                WIFI_MANAGE_THREAD_STACK_SIZE) != OS_OK) {
		WIFI_MANAGE_TRACK_WARN("wifi thread create error\n");
		return COMP_ERROR;
	}

	return COMP_OK;
}

