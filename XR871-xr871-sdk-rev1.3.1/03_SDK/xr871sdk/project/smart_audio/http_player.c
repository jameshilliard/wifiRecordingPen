#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/cmd/cmd.h"
#include "common/cmd/cmd_defs.h"
#include "common/framework/sysinfo.h"
#include "command.h"
#include "cjson/cJSON.h"
#include "string.h"
#include "pthread.h"
#include "md5.h"
#include "dev_config.h"
#include "http_player.h"
#include "tcp_client.h"
#include "sys/interrupt.h"
#include "unistd.h"
#include "console/console.h"
#include "ad_button_task.h"
#include "wifi_manage.h"
#include "tcp_client.h"
#include "common/framework/sys_ctrl/sys_ctrl.h"
#include "flash_media_info.h"

#define MAX_HTTPAUDIO_NUM   20

struct httpUrl_
{
    uint8_t totalHttpUrl;
    uint8_t lastHttpUrlFlag;
    char    *httpUrl[MAX_HTTPAUDIO_NUM];
}gHttpUrl;



uint8_t             http_player_task_run = 0;
OS_Thread_t         http_player_task_thread;
uint8_t             cmdState = 0;
AD_Button_Cmd_Info  AD_Button_Cmd = {AD_BUTTON_CMD_NULL, AD_BUTTON_NUM};

void initHttpAudioArray(void)
{
    int i=0;
    for(i=0;i<MAX_HTTPAUDIO_NUM;i++)
    {
        if(gHttpUrl.httpUrl[i]){
            free(gHttpUrl.httpUrl[i]);
            gHttpUrl.httpUrl[i]=NULL;
        }
    }
    memset(&gHttpUrl,0,sizeof(gHttpUrl));
}

int addHttpAudio(char *url)
{
    if(url==NULL)
        return -1;
    int i=0;
    for(i=0;i<MAX_HTTPAUDIO_NUM;i++)
    {
        if(gHttpUrl.httpUrl[i]==NULL){
            gHttpUrl.httpUrl[i]=malloc(strlen(url)+0x10);
            if(gHttpUrl.httpUrl[i])
            {
                strcpy(gHttpUrl.httpUrl[i],url);
                break;
            }
        }
    }
    gHttpUrl.totalHttpUrl++; 
    return gHttpUrl.totalHttpUrl;
}

static int  playStartHttpAudio(char *httpStr)
{
    int iRet=CMD_STATUS_FAIL;
    iRet=console_cmd("cedarx stop");
    if(iRet!=CMD_STATUS_ACKED && iRet!=CMD_STATUS_OK)
        return iRet;
    int length=strlen(httpStr)+0x40;
    char *cmdStr=malloc(length);
    if(cmdStr==NULL)
        return CMD_STATUS_FAIL;
    memset(cmdStr,0,length);
    //sprintf(cmdStr,"cedarx seturl %s",httpStr);
    iRet=cmd_cedarx_seturl_exec(httpStr);
    if(cmdStr) 
        free(cmdStr); 
    if(iRet!=CMD_STATUS_ACKED && iRet!=CMD_STATUS_OK)
    {
        HTTP_PLAYER_TRACK_INFO("failure cmd:cedarx seturl %s\n",httpStr);
        return iRet;
    } 
    iRet=console_cmd("cedarx play");
    return iRet;
}

static int  playHttpAudio(char *httpStr)
{
    if(httpStr==NULL || strlen(httpStr)==0)
        return -1;
    int iRet=playStartHttpAudio(httpStr);
    if(iRet!=CMD_STATUS_ACKED && iRet!=CMD_STATUS_OK)
    {
        HTTP_PLAYER_TRACK_INFO("cedarx play failure,so cedarx destroy and create\n");
        console_cmd("cedarx destroy");
        console_cmd("cedarx create");
        iRet=playStartHttpAudio(httpStr); 
    }
    return iRet; 
}

int  setVolume(uint8_t volume)
{
    int iRet=CMD_STATUS_FAIL;
    char cmdStr[40]={0};
    sprintf(cmdStr,"cedarx setvol %d",(volume+1)*6);//max:32
    iRet=console_cmd(cmdStr);
    if(iRet!=CMD_STATUS_ACKED && iRet!=CMD_STATUS_OK)
    {
        HTTP_PLAYER_TRACK_INFO("failure cmd:cedarx setvol %d\n",volume);
        return iRet;
    }
    return iRet; 
}

int  addHttpStr(char *httpStr)
{
    char *result = NULL;
    int iRet=0,i=0;
    result = strtok(httpStr,";");
    for(i=0;i<MAX_HTTPAUDIO_NUM;i++)
    {
        if(result != NULL){
           HTTP_PLAYER_TRACK_INFO("url[%02d] is %s\n",i,result);
           iRet=addHttpAudio(result);
           result = strtok(NULL, ";");
        }
        else{
            break;
        }
    }
    if(iRet>0)
    {
        AD_Button_Cmd.id = AD_BUTTON_5;
        AD_Button_Cmd.cmd = AD_BUTTON_CMD_SHORT_PRESS;
    }
    HTTP_PLAYER_TRACK_INFO("totalHttpUrl=%d\n",iRet);
    return 0;
}

int  analysisHttpStr(char *httpStr)
{
    char *result = NULL;
    int iRet=0,i=0;
    result = strtok(httpStr,";");
    initHttpAudioArray();
    for(i=0;i<MAX_HTTPAUDIO_NUM;i++)
    {
        if(result != NULL){
           HTTP_PLAYER_TRACK_INFO("url[%02d] is %s\n",i,result);
           iRet=addHttpAudio(result);
           result = strtok(NULL, ";");
        }
        else{
            break;
        }
    }
    if(iRet>0)
    {
        AD_Button_Cmd.id = AD_BUTTON_5;
        AD_Button_Cmd.cmd = AD_BUTTON_CMD_SHORT_PRESS;
    }
    HTTP_PLAYER_TRACK_INFO("totalHttpUrl=%d\n",iRet);
    return 0;
}

int voice_tips_add_music(int type,uint8_t nowFlag)
{
    char mp3Path[128]={0};
    memset(mp3Path,0,sizeof(mp3Path));
    switch(type)
    {
        case FIRST_RESET:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_SIT1_FLASHADDR,IS_SIT1_LENGTH);
            break;
        case SECOND_RESET:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_SIT2_FLASHADDR,IS_SIT2_LENGTH);
            break;
        case THIRD_RESET:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_SIT3_FLASHADDR,IS_SIT3_LENGTH);
            break;
        case RESET:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_SIT5_FLASHADDR,IS_SIT5_LENGTH);
            break; 
        case CLOSELEGWARN:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_CLOSELEGWARN_FLASHADDR,IS_CLOSELEGWARN_LENGTH);
            break;
        case RSET45:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_RSET45_FLASHADDR,IS_RSET45_LENGTH);
            break;
        case AFRESH_NET:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_ANYKA_AFRESH_NET_CONFIG_FLASHADDR,IS_ANYKA_AFRESH_NET_CONFIG_LENGTH);
            break;  
        case CONN_SUCCESS: 
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_ANYKA_CONNECTED_SUCCESS_FLASHADDR,IS_ANYKA_CONNECTED_SUCCESS_LENGTH);
            break;
        case PLEASE_CONN : 
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_ANYKA_PLEASE_CONFIG_NET_FLASHADDR,IS_ANYKA_PLEASE_CONFIG_NET_LENGTH);
            break;
        case RECOVER_DEV : 
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_ANYKA_RECOVER_DEVICE_FLASHADDR,IS_ANYKA_RECOVER_DEVICE_LENGTH);
            break;
        case BEGIN_STUDY : 
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",IS_BEGINSTUDYMODE_FLASHADDR,IS_BEGINSTUDYMODE_LENGTH);
            break;
        case SENDFINISH:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",V00_IS_SENDFINISH_FLASHADDR,V00_IS_SENDFINISH_LENGTH);
            break;
        case SENDSTART:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",V00_IS_SENDSTART_FLASHADDR,V00_IS_SENDSTART_LENGTH);
            break;
        case MF_TEST110:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST110_FLASHADDR,MF_TEST110_LENGTH);
            break;
        case MF_TEST111:
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST111_FLASHADDR,MF_TEST111_LENGTH);
            break;
        case MF_TEST11 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST11_FLASHADDR,MF_TEST11_LENGTH);
            break;       
        case MF_TEST12 :
             sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST12_FLASHADDR,MF_TEST12_LENGTH);
            break;        
        case MF_TEST13 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST13_FLASHADDR,MF_TEST13_LENGTH);
            break; 
        case MF_TEST14 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST14_FLASHADDR,MF_TEST14_LENGTH);
            break; 
        case MF_TEST15 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST15_FLASHADDR,MF_TEST15_LENGTH);
            break; 
        case MF_TEST16 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST16_FLASHADDR,MF_TEST16_LENGTH);
            break; 
        case MF_TEST17 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST17_FLASHADDR,MF_TEST17_LENGTH);
            break; 
        case MF_TEST18 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST18_FLASHADDR,MF_TEST18_LENGTH);
            break; 
        case MF_TEST19 :
            sprintf(mp3Path,"flash://0?addr=%d&length=%d",MF_TEST19_FLASHADDR,MF_TEST19_LENGTH);
            break; 
        default:
            break;
    }
    if(strlen(mp3Path)<=0)
    {
        return -1; 
    }

    if(nowFlag==0)
        return addHttpStr(mp3Path);
    else
        return playHttpAudio(mp3Path);
}

int  stopHttpAudioPlay(int stopCode)
{
    AD_Button_Cmd.id = AD_BUTTON_0;
    AD_Button_Cmd.cmd = AD_BUTTON_CMD_SHORT_PRESS;
    return 0;
}
	
PLAYER_CMD read_payer_ctrl_cmd()
{
	PLAYER_CMD cmd = CMD_PLAYER_NULL;
    if (AD_Button_Cmd.cmd != AD_BUTTON_CMD_NULL) {
		switch (AD_Button_Cmd.id) {
			case AD_BUTTON_1:
				if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_LONG_PRESS)
					cmd = CMD_PLAYER_SET_WIFI;
				break;
		    case AD_BUTTON_2:
				if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_SHORT_PRESS)
					cmd = CMD_PLAYER_VOLUME_DOWN;
				else if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_REPEAT)
					cmd = CMD_PLAYER_VOLUME_DOWN;
				break;
			case AD_BUTTON_3:
				if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_SHORT_PRESS)
					cmd = CMD_PLAYER_VOLUME_UP;
				else if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_REPEAT)
					cmd = CMD_PLAYER_VOLUME_UP;
				break;
			case AD_BUTTON_5:
				if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_SHORT_PRESS)
					cmd = CMD_PLAYER_SMART_PLAY;
				break;
		    case AD_BUTTON_4:
				if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_LONG_PRESS)
					cmd = CMD_PLAYER_SMART_VOICE_START;
				else if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_RELEASE)
					cmd = CMD_PLAYER_SMART_VOICE_STOP;
				break;
			case AD_BUTTON_0:
				if (AD_Button_Cmd.cmd == AD_BUTTON_CMD_SHORT_PRESS)
					cmd = CMD_PLAYER_SMART_PAUSE;
				break;
			default:
				break;
		}
		AD_Button_Cmd.cmd = AD_BUTTON_CMD_NULL;
		AD_Button_Cmd.id = AD_BUTTON_NUM;
	}
	return cmd;
}

void http_player_task(void *arg)
{
    uint8_t cedarxControlStatus=0;
    PLAYER_CMD buttonCmd=CMD_PLAYER_NULL;
    struct sysinfo *sysInfo=sysinfo_get();
    HTTP_PLAYER_TRACK_INFO("cedarx destroy and create\n");
    console_cmd("cedarx destroy");
    console_cmd("cedarx create");
    setVolume(sysInfo->volume);
	while (http_player_task_run) {
	    buttonCmd=read_payer_ctrl_cmd();
	    switch(buttonCmd)
	    {
	    case CMD_PLAYER_SET_WIFI:
            voice_tips_add_music(PLEASE_CONN,1);
	        OS_Sleep(5);
	        HTTP_PLAYER_TRACK_INFO("buttonCmd CMD_PLAYER_SET_WIFI\n");
	        setWifiRunState(CMD_WIFI_SMARTLINK);
	        console_cmd("cedarx stop");
	        console_cmd("net sta disable");
	        console_cmd("net smartlink stop");
	        console_cmd("net smartlink set_airkiss_key e1c361cc29e43fb8");
	        console_cmd("net smartlink start");
	        break;
	    case CMD_PLAYER_VOLUME_DOWN:
	        HTTP_PLAYER_TRACK_INFO("buttonCmd CMD_PLAYER_VOLUME_DOWN\n");
	        sysInfo->volume--;
	        if(sysInfo->volume < 0)
	    	    sysInfo->volume=0;
            else if(sysInfo->volume>=4)
                sysInfo->volume=4;
	        sysinfo_save();
	        setVolume(sysInfo->volume);
	        break;  
	    case CMD_PLAYER_VOLUME_UP:
	        HTTP_PLAYER_TRACK_INFO("buttonCmd CMD_PLAYER_VOLUME_UP\n");
	    	sysInfo->volume++;
            if(sysInfo->volume < 0)
                sysInfo->volume=0;
            else if(sysInfo->volume>=4)
                sysInfo->volume=4;
	        sysinfo_save();
	        setVolume(sysInfo->volume);
	        break;
	   	case CMD_PLAYER_SMART_VOICE_START:
	   	    if(getWifiState() == WLAN_STA_STATE_DISCONNECTED)
	   	    {
                if(strlen((char *)sysInfo->wlan_sta_param.ssid)>0)
                    voice_tips_add_music(AFRESH_NET,1);
                else
                    voice_tips_add_music(PLEASE_CONN,1);
	   	    } 
            if(getWifiState() == WLAN_STA_STATE_DISCONNECTED || 
               getTcpClientState() != STATE_TCP_CLINET_CONNECTED ||
               getTcpClientLoginState() != 1)
                  break;
            voice_tips_add_music(SENDSTART,1);
            OS_MSleep(225);
	   	    HTTP_PLAYER_TRACK_INFO("buttonCmd CMD_PLAYER_SMART_VOICE_START\n");
	   	    cedarxControlStatus=0;
	   	    initHttpAudioArray();
	   	    if(sys_get_status_exec()!= STATUS_STOPPED)
	   	        console_cmd("cedarx stop");
	   	    //console_cmd("cedarx rec callback://");
	   	    //console_cmd("cedarx rec file://wechat.pcm");
	   	    console_cmd("audio httpcap 16000 1 record.pcm");
	   	    sendTcpClientStatus(1);
	        break; 
		case CMD_PLAYER_SMART_VOICE_STOP:
	   	    HTTP_PLAYER_TRACK_INFO("buttonCmd CMD_PLAYER_SMART_VOICE_STOP\n");
	   	    //console_cmd("cedarx end"); 
	   	    console_cmd("audio end");
	   	    sendTcpClientStatus(2);
            if(getWifiState() == WLAN_STA_STATE_CONNECTED)
            {
                voice_tips_add_music(SENDFINISH,0);
                //OS_MSleep(500);
            }
            break;          
	    case CMD_PLAYER_SMART_PLAY:
	        if(gHttpUrl.lastHttpUrlFlag < MAX_HTTPAUDIO_NUM && 
	           gHttpUrl.lastHttpUrlFlag < gHttpUrl.totalHttpUrl &&
	           gHttpUrl.httpUrl[gHttpUrl.lastHttpUrlFlag]!=NULL)
	        {
                playHttpAudio(gHttpUrl.httpUrl[gHttpUrl.lastHttpUrlFlag]);
                cedarxControlStatus=1;
            }
	        break;
	    case CMD_PLAYER_SMART_PAUSE:
            voice_tips_add_music(RECOVER_DEV,1);
            sysInfo->volume=1;
            memset(&(sysInfo->wlan_sta_param),0,sizeof(sysInfo->wlan_sta_param));
            memset(sysInfo->wlan_sta_params,0,sizeof(sysInfo->wlan_sta_param)*5);
	        //cedarxControlStatus=0;
	        console_cmd("sysinfo save");
            HTTP_PLAYER_TRACK_INFO("buttonCmd CMD_PLAYER_SMART_RESET\n");
	        break;
	    default:
	        break;
	    }
	    if(gHttpUrl.lastHttpUrlFlag < MAX_HTTPAUDIO_NUM && 
           gHttpUrl.lastHttpUrlFlag < gHttpUrl.totalHttpUrl &&
           gHttpUrl.httpUrl[gHttpUrl.lastHttpUrlFlag] != NULL &&
           cedarxControlStatus==1 && sys_get_status_exec() == STATUS_STOPPED)
        {
            gHttpUrl.lastHttpUrlFlag=gHttpUrl.lastHttpUrlFlag+1;
            playHttpAudio(gHttpUrl.httpUrl[gHttpUrl.lastHttpUrlFlag]);
            free(gHttpUrl.httpUrl[gHttpUrl.lastHttpUrlFlag]);
            gHttpUrl.httpUrl[gHttpUrl.lastHttpUrlFlag]=NULL;
        }  
        OS_MSleep(50);
	}
	OS_ThreadDelete(&http_player_task_thread);
	HTTP_PLAYER_TRACK_INFO("rtp server task end\n");
}

void player_set_ad_button_cmd(AD_Button_Cmd_Info *cmd)
{
	AD_Button_Cmd = *cmd;
}

static void vkey_ctrl(uint32_t event, uint32_t data, void *arg)
{
	if (EVENT_SUBTYPE(event) == CTRL_MSG_SUB_TYPE_AD_BUTTON)
		player_set_ad_button_cmd((AD_Button_Cmd_Info *)data);
}

Component_Status http_player_task_init()
{
	http_player_task_run = 1;
	if (OS_ThreadCreate(&http_player_task_thread,
		                "",
		                http_player_task,
		               	NULL,
		                OS_THREAD_PRIO_APP,
		                HTTP_PLAYER_THREAD_STACK_SIZE) != OS_OK) {
		HTTP_PLAYER_TRACK_WARN("rtp server thread create error\n");
		return COMP_ERROR;
	}
	observer_base *obs = sys_callback_observer_create(CTRL_MSG_TYPE_VKEY,
	                                                  CTRL_MSG_SUB_TYPE_ALL,
	                                                  vkey_ctrl,
	                                                  NULL);
	sys_ctrl_attach(obs);                                            
	return COMP_OK;
}
