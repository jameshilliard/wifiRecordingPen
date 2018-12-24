#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/cmd/cmd.h"
#include "common/cmd/cmd_defs.h"
#include "common/framework/sysinfo.h"
#include "driver/component/component_def.h"
#include "cjson/cJSON.h"
#include "string.h"
#include "pthread.h"
#include "md5.h"
#include "dev_config.h"
#include "rtp_server.h"
#include "wifi_manage.h"
#include "tcp_client.h"

static uint8_t      rtp_server_task_run = 0;
static OS_Thread_t  rtp_server_task_thread;

static int analyzeReturnInfo(const char *strResponse)
{
	if(NULL == strResponse)
		return RETNULL;
	if(strcmp(strResponse, "ok")==0)
		return OK;
	else if(strcmp(strResponse, "uidNotExist")==0)
		return UIDNOTEXIST;
	else if(strcmp(strResponse, "pwdError")==0)
		return PWDERROR;
	else if(strcmp(strResponse, "notLoginYet")==0)
		return NOTLOGINYET;
	else
		return OTHERERROR;
}

static int analyzeLoginInfo(const char *strResponse, LoginReturnInfo *returnInfo)
{
	if(NULL == strResponse || NULL == returnInfo){
		RTP_SERVER_TRACK_WARN("param null\n");
		return -1;
	}
	cJSON * pJson = cJSON_Parse(strResponse);
	if(NULL == pJson){
		RTP_SERVER_TRACK_WARN("pJson null\n");
		return -2;
	}
	cJSON * pSub = cJSON_GetObjectItem(pJson, "result");
	if(NULL == pSub){
	   RTP_SERVER_TRACK_WARN("result null\n");
	   cJSON_Delete(pJson);
	   return -3;
	}
	else{
		returnInfo->result = analyzeReturnInfo(pSub->valuestring);
		if(returnInfo->result != 1){
			RTP_SERVER_TRACK_INFO("result:%s, pSub->valueint:%d\n", pSub->valuestring, returnInfo->result);
			cJSON_Delete(pJson);
			return returnInfo->result;
		}
	}
	pSub = cJSON_GetObjectItem(pJson, "ServerInfo_relayServerAddr");
	if(NULL == pSub){
		 RTP_SERVER_TRACK_INFO("ServerInfo_relayServerAddr null\n");
		 cJSON_Delete(pJson);
		 return -4;
	}
	else{
		sprintf(returnInfo->relayServerAddr, "%s", pSub->valuestring);
		RTP_SERVER_TRACK_INFO("ServerInfo_relayServerAddr : %s\n", pSub->valuestring);
	}
	pSub = cJSON_GetObjectItem(pJson, "ServerInfo_relayServerPort");
	if(NULL == pSub){
		 RTP_SERVER_TRACK_INFO("ServerInfo_relayServerPort null\n");
		 cJSON_Delete(pJson);
		 return -5;
	}
	else{
	    returnInfo->relayServerPort=atoi(pSub->valuestring);
		RTP_SERVER_TRACK_INFO("ServerInfo_relayServerPort : %s=%d\n", pSub->valuestring,returnInfo->relayServerPort);
	}
	cJSON_Delete(pJson);
	return 0;
}

static int calSecret(char uid[80],char passwd[80])
{
	int length=strlen(uid);
	char newUid[80]={0,};
	int i=0;
	memset(newUid,0,sizeof(newUid));
	for(i=0;i<length;i++)
	{
		newUid[i]=uid[i]%10+0x30;
	}
	char *outString=MDString(newUid);
	memcpy(passwd,outString,strlen(outString));
	printf("uid=%s,newuid=%s,passwd=%s\n",uid,newUid,passwd);
	return 0;
}

static int loginRounterServer(LoginReturnInfo *returnInfo,char *cmdResponse)
{
    char httpCmdStr[256]={0};
    char httpServerAddr[512]={0};
    if(returnInfo==NULL || cmdResponse==NULL){
        RTP_SERVER_TRACK_WARN("parm error: %p %p\n",returnInfo,cmdResponse);
        return -1;
    }
    char secret[80]={0};
    int iRet=calSecret(DEV_DEBUG_ID,secret);
	sprintf(httpServerAddr,DEV_LOGIN_CONSERVER, DEV_DE_ROUNTER_SERVER);
	sprintf(httpCmdStr,"%s id=%s&pwd=%s&HWVersion=%s&SWVersion=%s&devType=7",httpServerAddr,DEV_DEBUG_ID,secret,DEV_HW_VERSION,DEV_SW_VERSION);
    iRet=httpc_cmd_self(httpCmdStr,cmdResponse); 
    RTP_SERVER_TRACK_INFO("loginRounterServer: iRet=%d StrcmdResponse=%s end\n",iRet,cmdResponse);
    if(iRet==0){
        iRet=analyzeLoginInfo(cmdResponse,returnInfo);
    }
    return iRet;
}

void rtp_server_task(void *arg)
{
    uint8_t cmdStatus=CMD_RTP_SERVER_GET_ADDR;
    int     iRet=-1;
    char    *cmdResponse=NULL;
    char    idStr[32]={0};
    LoginReturnInfo  mLoginReturnInfo;
    strncpy(idStr,DEV_DEBUG_ID,sizeof(idStr));
    
    RTP_SERVER_TRACK_INFO("rtp server task start\n"); 
	while (rtp_server_task_run) {
	    if(cmdStatus != CMD_RTP_SERVER_LOGIN_SUC)
	        RTP_SERVER_TRACK_INFO("rtp server task run %d\n",cmdStatus);
        switch(cmdStatus){
        case CMD_RTP_SERVER_GET_ADDR:
            {
                if(getWifiState() == WLAN_STA_STATE_DISCONNECTED)
                    break;
                if(cmdResponse==NULL)
                    cmdResponse=malloc(4096);
                memset(cmdResponse,0,4096);
                memset(&mLoginReturnInfo,0,sizeof(mLoginReturnInfo));
                iRet=loginRounterServer(&mLoginReturnInfo,cmdResponse);
                if(iRet==0 || iRet==4)
                {
                    if(iRet==4)
                    {
                        sprintf(mLoginReturnInfo.relayServerAddr, "%s", DEBUG_RTP_SERVER_IP);
                        mLoginReturnInfo.relayServerPort=DEBUG_RTP_SERVER_PORT;
                    }
                    cmdStatus=CMD_RTP_SERVER_CONNECT;
                }   
                if(cmdResponse){
                    free(cmdResponse);
                    cmdResponse=NULL;
                }
            }
            break;
        case CMD_RTP_SERVER_CONNECT:
            {
                if(getWifiState() == WLAN_STA_STATE_DISCONNECTED){
                    cmdStatus=CMD_RTP_SERVER_CONNECT;
                    break;
                } 
                iRet=tcp_client_connect(mLoginReturnInfo.relayServerAddr,mLoginReturnInfo.relayServerPort);
                if(iRet==ERR_OK){
                    cmdStatus=CMD_RTP_SERVER_LOGIN;
                }
            }
            break;
        case CMD_RTP_SERVER_LOGIN:
            {
                if(getWifiState() == WLAN_STA_STATE_DISCONNECTED || 
                   getTcpClientState() != STATE_TCP_CLINET_CONNECTED){
                    cmdStatus=CMD_RTP_SERVER_CLOSING;
                    break;
                } 
                iRet=sendLoginData(idStr);
                if(iRet==ERR_OK){
                    cmdStatus=CMD_RTP_SERVER_LOGIN_DETECT;
                } 
            }
            break;
         case CMD_RTP_SERVER_LOGIN_DETECT:
            if(getWifiState() == WLAN_STA_STATE_DISCONNECTED || 
               getTcpClientState() != STATE_TCP_CLINET_CONNECTED ||
               getTcpClientLoginState() != 1){
                cmdStatus=CMD_RTP_SERVER_CLOSING;
            }
            if(getTcpClientLoginState() == 1){
                cmdStatus=CMD_RTP_SERVER_LOGIN_SUC;
            }
            break; 
        case CMD_RTP_SERVER_LOGIN_SUC:
            if(getWifiState() == WLAN_STA_STATE_DISCONNECTED || 
               getTcpTimeout()==1){
                cmdStatus=CMD_RTP_SERVER_CLOSING;
            }
            sendAliveDataTask(idStr);
            break;
        case CMD_RTP_SERVER_CLOSING:
            tcp_client_connection_close();
            cmdStatus=CMD_RTP_SERVER_GET_ADDR;
            break;
        case CMD_RTP_SERVER_SMARTLINK:
            break;
        default:
            break;
        }
        if(cmdStatus==CMD_RTP_SERVER_GET_ADDR || cmdStatus==CMD_RTP_SERVER_CONNECT)
            OS_MSleep(2000);
        else
		    OS_MSleep(500);
	}  
	RTP_SERVER_TRACK_INFO("rtp server task end\n");
	OS_ThreadDelete(&rtp_server_task_thread);
}

Component_Status rtp_server_task_init()
{
	rtp_server_task_run = 1;
	if (OS_ThreadCreate(&rtp_server_task_thread,
		                "",
		                rtp_server_task,
		               	NULL,
		                OS_THREAD_PRIO_APP,
		                RTP_SERVER_THREAD_STACK_SIZE) != OS_OK) {
		RTP_SERVER_TRACK_WARN("rtp server thread create error\n");
		return COMP_ERROR;
	}

	return COMP_OK;
}

