#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/cmd/cmd.h"
#include "common/cmd/cmd_defs.h"
#include "common/framework/sysinfo.h"
#include "driver/component/component_def.h"
#include "cjson/cJSON.h"
#include "string.h"
#include "pthread.h"
#include "command.h"
#include "lwip/opt.h"
#include "lwip/ip_addr.h"
#include "lwip/inet.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include "dev_config.h"
#include "wifi_set_broadcast.h"
#include "wifi_manage.h"


static uint8_t      udp_server_task_run = 0;
static OS_Thread_t  udp_server_task_thread;
static struct udp_pcb *    g_pcbRecv=NULL;
static struct udp_pcb *    g_pcbSend=NULL;
static uint8_t cmdStatus   = CMD_UDP_SERVER_WAIT;
static char *message       = NULL;  


#define UDP_LOCAL_PORT  8124 
#define UDP_REMOTE_PORT 8123 

int set_udp_server_status(int status)
{
    cmdStatus=status;
    return 0;
}

void udp_server_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,struct ip_addr *addr, unsigned short int port)
{
    //struct ip_addr destAddr = *addr;
    if(p != NULL)
    {
        UDP_SERVER_TRACK_INFO("udp_server_recv %s %d\n",(char *)p->payload,p->len);
        //udp_sendto(g_pcbSend,p,&destAddr,UDP_REMOTE_PORT);
        //sendCount++;
        pbuf_free(p);
        if(p->payload!=NULL && p->len>0)
        {
            if(message!=NULL)
            {
                if(memcmp(message,p->payload,strlen(message))==0)
                {

                    set_udp_server_status(CMD_UDP_SERVER_OVER);
                }
    
            }

        }
        pbuf_free(p);
    }
}


void UDP_server_init(void)
{
    int iRet=-1;
    g_pcbRecv = udp_new();	
    if(g_pcbRecv==NULL)
    {
        UDP_SERVER_TRACK_WARN("UDP_server_init failure\n");
    }
    iRet=udp_bind(g_pcbRecv,IP_ADDR_ANY,UDP_LOCAL_PORT);
    g_pcbRecv->so_options |= SOF_BROADCAST;
    UDP_SERVER_TRACK_WARN("udp_bind %d\n",iRet);
    udp_recv(g_pcbRecv,udp_server_recv,NULL); 

    //g_pcbSend = udp_new();	
    //udp_connect(g_pcbSend,IP_ADDR_BROADCAST,UDP_REMOTE_PORT);
    //udp_recv(g_pcbRecv,udp_server_recv,NULL); 
}


void UDP_server_deinit(void)
{
    if(g_pcbRecv)
    {
        udp_recv(g_pcbRecv,NULL,NULL); 
        udp_remove(g_pcbRecv);
    }

    if(g_pcbSend)
    {
        udp_remove(g_pcbSend);
    }
}


int udp_send_message(void *msg, uint16_t len)
{
    int iRet=-1;
    struct pbuf *p_tx = pbuf_alloc(PBUF_TRANSPORT,len,PBUF_RAM);          
    if(p_tx)
    {
        pbuf_take(p_tx,(char*)msg, len);
        iRet=udp_sendto(g_pcbRecv,p_tx,IP_ADDR_BROADCAST,UDP_REMOTE_PORT);
        UDP_SERVER_TRACK_INFO("udp_send_message %s %d\n",(char *)msg,iRet);
        OS_MSleep(100);
        pbuf_free(p_tx);
    }
    return iRet;
}


void udp_server_task(void *arg)
{
    uint32_t startTime = 0;
    uint32_t nowTime=0;
    char  guid[64]={0};
    struct sysinfo *sysinfo = sysinfo_get();
    if(strlen(sysinfo->udid)>0)
        strncpy(guid,sysinfo->udid,sizeof(guid));
    else
        strncpy(guid,DEV_DEBUG_ID,sizeof(guid));
    message=malloc(128);
    if(message!=NULL)
    {
        memset(message,0,128);
        sprintf(message,"ROBOT:%s",guid);
    }
    UDP_SERVER_TRACK_INFO("udp server task start\n"); 
   
	while (udp_server_task_run) {
	    if(cmdStatus != CMD_UDP_SERVER_WAIT)
	        UDP_SERVER_TRACK_INFO("udp server task run %d\n",cmdStatus);
        switch(cmdStatus){
        case CMD_UDP_SERVER_WAIT:
            break;
        case CMD_UDP_SERVER_START:
            {
                if(getWifiState() == WLAN_STA_STATE_DISCONNECTED){
                    break;
                } 
                startTime=OS_JiffiesToMSecs(OS_GetJiffies());
                UDP_server_init();
                cmdStatus=CMD_UDP_SERVER_SENDMESS;
            }
            break;
        case CMD_UDP_SERVER_SENDMESS:
            if(message!=NULL)
                udp_send_message(message,strlen(message)+1);
            break; 
        case CMD_UDP_SERVER_OVER:
            UDP_server_deinit();
            cmdStatus=CMD_UDP_SERVER_WAIT;
            startTime=0;
            break;
        default:
            break;
        }
	    OS_Sleep(1);
        if(startTime!=0)
        {
           nowTime=OS_JiffiesToMSecs(OS_GetJiffies());
           if((nowTime-startTime)>6*1000)
           {
                UDP_SERVER_TRACK_INFO("rtp server task time over\n");
                set_udp_server_status(CMD_UDP_SERVER_OVER);
                startTime=0;
           }
        }
	}  
	UDP_SERVER_TRACK_INFO("rtp server task end\n");
	OS_ThreadDelete(&udp_server_task_thread);
    if(message!=NULL)
    {
        memset(message,0,128);
        free(message);
    }
}

Component_Status udp_server_task_init()
{
	udp_server_task_run = 1;
	if (OS_ThreadCreate(&udp_server_task_thread,
		                "",
		                udp_server_task,
		               	NULL,
		                OS_THREAD_PRIO_APP,
		                UDP_SERVER_THREAD_STACK_SIZE) != OS_OK) {
		UDP_SERVER_TRACK_WARN("rtp server thread create error\n");
		return COMP_ERROR;
	}

	return COMP_OK;
}

