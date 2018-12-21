#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/cmd/cmd.h"
#include "common/cmd/cmd_defs.h"
#include "common/framework/sysinfo.h"
#include "driver/component/component_def.h"
#include "command.h"
#include "cjson/cJSON.h"
#include "string.h"
#include "pthread.h"
#include "md5.h"
#include "dev_config.h"
#include "http_player.h"
#include "tcp_client.h"

static uint8_t      http_player_task_run = 0;
static OS_Thread_t  http_player_task_thread;
static uint8_t      cmdState = 0;

struct httpUrl_
{
    uint8_t totalHttpUrl;
    uint8_t lastHttpUrl;
    char    *httpUrl[10];
}gHttpUrl;


void deinitHttpUrl(void)
{
    int i=0;
    for(i=0;i<10;i++)
    {
        if(gHttpUrl.httpUrl[i]){
            free(gHttpUrl.httpUrl[i]);
            gHttpUrl.httpUrl[i]=NULL;
        }
    }
    memset(&gHttpUrl,0,sizeof(gHttpUrl));
}

int addHttpUrl(char *url)
{
    if(url==NULL)
        return -1;
    int i=0;
    for(i=0;i<10;i++)
    {
        if(gHttpUrl.httpUrl[i]==NULL){
            gHttpUrl.httpUrl[i]=malloc(strlen(url)+10);
            if(gHttpUrl.httpUrl[i])
                strcpy(gHttpUrl.httpUrl[i],url);
        }
    }
    gHttpUrl.totalHttpUrl++; 
    return gHttpUrl.totalHttpUrl;
}

static int cmd_exe(const char *cmd)
{
    int iRet=CMD_STATUS_FAIL;
    char *cmdStr=malloc(512);
    if(cmdStr==NULL)
        return iRet;
    memset(cmdStr,0,512);
    sprintf(cmdStr,cmd);
    iRet=main_cmd_exec(cmdStr);
    if(iRet!=CMD_STATUS_ACKED && iRet!=CMD_STATUS_OK)
    {
        memset(cmdStr,0,512);
        sprintf(cmdStr,cmd);
        OS_Sleep(1);
        iRet=main_cmd_exec(cmdStr);
    }  
    if(cmdStr)
        free(cmdStr);
    return iRet;
}

static int  playHttpStr(char *httpStr)
{
    int iRet=CMD_STATUS_FAIL;
    cmd_exe("cedarx destroy");
    cmd_exe("cedarx create");
    HTTP_PLAYER_TRACK_INFO("cedarx stop\n");
    //iRet=cmd_exe("cedarx stop");
    //if(iRet!=CMD_STATUS_ACKED && iRet!=CMD_STATUS_OK)
   //     return iRet;
    OS_Sleep(1);
    char *cmdStr=malloc(512);
    if(cmdStr==NULL)
        return CMD_STATUS_FAIL;
    memset(cmdStr,0,512);
    sprintf(cmdStr,"cedarx seturl %s",httpStr);
    HTTP_PLAYER_TRACK_INFO("%s\n",cmdStr);
    iRet=cmd_exe(cmdStr);
    if(cmdStr)
        free(cmdStr); 
    OS_Sleep(1);
    if(iRet!=CMD_STATUS_ACKED && iRet!=CMD_STATUS_OK)
        return iRet;
    HTTP_PLAYER_TRACK_INFO("cedarx play\n");
    iRet=cmd_exe("cedarx play");
    return iRet; 
}


int  analysisHttpStr(char *httpStr)
{
    char *result = NULL;
    int iRet=0;
    result = strtok(httpStr,";");
    if(result)
        deinitHttpUrl();
    while(result != NULL)
    {
       HTTP_PLAYER_TRACK_INFO("result is %s\n", result);
       iRet=addHttpUrl(result);
       result = strtok(NULL, ";");
    }
    if(iRet>0)
        cmdState = 1;
    HTTP_PLAYER_TRACK_INFO("cmdState=%d totalHttpUrl=%d\n",cmdState,iRet);
    return 0;
}

void http_player_task(void *arg)
{
   
	while (http_player_task_run) {
        if(cmdState==1)
        {
            cmdState=0;
            if(gHttpUrl.httpUrl[0]!=NULL){
                playHttpStr(gHttpUrl.httpUrl[0]);
            }    
        }
	    OS_MSleep(100);
	}
	HTTP_PLAYER_TRACK_INFO("rtp server task end\n");
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

	return COMP_OK;
}