#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/cmd/cmd.h"
#include "common/cmd/cmd_defs.h"
#include "common/framework/sysinfo.h"
#include "string.h"
#include "pthread.h"
#include "wifi_manage.h"
#include "http_player.h"

static uint8_t              wifi_task_run = 0;
static OS_Thread_t          wifi_manage_task_thread;
static wlan_sta_states_t    wifiStatus = WLAN_STA_STATE_DISCONNECTED;
static uint8_t              runStatus = CMD_WIFI_SET_SSID;

wlan_sta_states_t getWifiState(void)
{
    return wifiStatus;
}

void setWifiRunState(uint8_t runstate)
{
    wifiStatus=WLAN_STA_STATE_DISCONNECTED;
    runStatus=runstate;
}

void wifi_task(void *arg)
{
    int  disConStateCount=0;
    char cmd[256]={0};
    char ssid[32]={0};
    char psk[32]={0};
    int  cmdStatus=CMD_STATUS_FAIL;
    uint8_t i=0;
    struct sysinfo *sysinfo = sysinfo_get();
    uint8_t wifiSuccess=0;
    uint8_t wifiSuccessSetCount=0;
    uint8_t saveWifiFlag=sysinfo->wlanStaFlag;
    memcpy(ssid,sysinfo->wlan_sta_param.ssid,sizeof(ssid));
    memcpy(psk,sysinfo->wlan_sta_param.psk,sizeof(psk));
    WIFI_MANAGE_TRACK_INFO("wifi task start %s %s\n",ssid,psk);
    for(i=0;i<5;i++)
    {
        if(strlen((char *)sysinfo->wlan_sta_params[i].ssid)>0)
        {
            WIFI_MANAGE_TRACK_INFO("wifi save %d %s %s\n",i,sysinfo->wlan_sta_params[i].ssid,sysinfo->wlan_sta_params[i].psk);  
        } 
    }
	while (wifi_task_run) 
	{
	    switch(runStatus){
        case CMD_WIFI_SET_SSID:
            {
                if(strlen((char *)ssid)<=0)
                {
                    runStatus=CMD_WIFI_SET_SSID;
                    break;
                }    
                memset(cmd,0,sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"set ssid %s",ssid);
                WIFI_MANAGE_TRACK_INFO("wifi run: %s\n",cmd);
                cmdStatus=cmd_wlan_sta_exec(cmd);
                if(cmdStatus==CMD_STATUS_OK || cmdStatus==CMD_STATUS_ACKED){
                   runStatus=CMD_WIFI_SET_PSK; 
                }
                WIFI_MANAGE_TRACK_INFO("wifi run: return %d\n",cmdStatus);
            }
            break;
        case CMD_WIFI_SET_PSK:
            {
                if(strlen((char *)psk)<=0)
                {
                    runStatus=CMD_WIFI_SET_PSK;
                    break;
                } 
                memset(cmd,0,sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"set psk %s",psk);
                WIFI_MANAGE_TRACK_INFO("wifi run: %s\n",cmd);
                cmdStatus=cmd_wlan_sta_exec(cmd);
                if(cmdStatus==CMD_STATUS_OK || cmdStatus==CMD_STATUS_ACKED){
                   runStatus=CMD_WIFI_ENABLE; 
                }
                WIFI_MANAGE_TRACK_INFO("wifi run: return %d\n",cmdStatus);
            }
            break;
        case CMD_WIFI_ENABLE:
            {
                memset(cmd,0,sizeof(cmd));
                snprintf(cmd,sizeof(cmd),"enable");
                WIFI_MANAGE_TRACK_INFO("wifi run: %s\n",cmd);
                cmdStatus=cmd_wlan_sta_exec(cmd);
                if(cmdStatus==CMD_STATUS_OK || cmdStatus==CMD_STATUS_ACKED){
                   runStatus=CMD_WIFI_DETECT; 
                }
                WIFI_MANAGE_TRACK_INFO("wifi run: return %d\n",cmdStatus);
            }
            break;
        case CMD_WIFI_DETECT:
            {
                wlan_sta_state(&wifiStatus);
                if(wifiStatus==WLAN_STA_STATE_DISCONNECTED)
                {
                    disConStateCount++;
                }
                else
                {
                    if(wifiSuccess==0)
                    {
                        wifiSuccess=1;
                        if(strncmp(ssid,(char *)sysinfo->wlan_sta_param.ssid,strlen(ssid))!=0)
                        {
                            sprintf(cmd,"sysinfo set sta ssid %s",(char *)ssid);
                            console_cmd(cmd);
                            sprintf(cmd,"sysinfo set sta psk %s",(char *)psk);
                            console_cmd(cmd);
                            sprintf(cmd,"sysinfo save");
                            console_cmd(cmd);
                        }
                    }
                }
                if(disConStateCount>12)
                {
                    disConStateCount=0;
                    if(wifiSuccess==1)
                    {
                        wifiSuccessSetCount++;
                        wlan_sta_connect();
                        if(wifiSuccessSetCount>3)
                        {
                            wifiSuccess=0;
                        }
                    }
                    else
                    {
                        if(saveWifiFlag==0)
                            saveWifiFlag=5;
                        saveWifiFlag--;
                        if(strlen((char *)sysinfo->wlan_sta_params[saveWifiFlag].ssid)>0)
                        {
                            memcpy(ssid,sysinfo->wlan_sta_params[saveWifiFlag].ssid,sizeof(ssid));
                            memcpy(psk,sysinfo->wlan_sta_params[saveWifiFlag].psk,sizeof(psk));                            
                            runStatus=CMD_WIFI_SET_SSID;
                        }    
                    }
                    WIFI_MANAGE_TRACK_INFO("wifi run wlan_sta_connect %s %s\n",ssid,psk);
                }
            }
            break;
        case CMD_WIFI_SMARTLINK:
            wlan_sta_state(&wifiStatus);
            if(wifiStatus==WLAN_STA_STATE_CONNECTED)
            {
                runStatus=CMD_WIFI_DETECT; 
            }   
            WIFI_MANAGE_TRACK_INFO("wifi run smartlink,wifiStatus=%d\n",wifiStatus);
            break;
        default:
            break;
        }
		OS_MSleep(500);
	}
	WIFI_MANAGE_TRACK_INFO("wifi task end\n");
	OS_ThreadDelete(&wifi_manage_task_thread);
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

