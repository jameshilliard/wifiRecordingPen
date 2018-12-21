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
#include "lwip/tcp.h"
#include "lwip/pbuf.h"

#include "dev_config.h"
#include "tcp_client.h"
#include "http_player.h"
struct client
{
    uint8_t state;
    uint8_t loginState;
    uint32_t sendLastAliveTime;
    uint32_t recvLastAliveTime;
    struct tcp_pcb *pcb;
    struct pbuf *p_tx;
}tcpClient;

static int  lastSumLength=0;
static char *maxPacket=NULL;

static void dumpHex(const char* data,int len)
{
	char *msgBuffer=malloc(1024);
	if(msgBuffer==NULL)
	    return;
	char msgSinglebuffer[16]={0};
	int i=0;
	memset(msgBuffer,0,1024);
	for(i = 0; i< len;i++){
		sprintf(msgSinglebuffer,"0x%02x ",*data++);
		strcat(msgBuffer,msgSinglebuffer);
		if(strlen(msgBuffer)>(1024-32))
			break;
	}
	TCP_CLIENT_TRACK_INFO("%s\n",msgBuffer);
	TCP_CLIENT_TRACK_INFO("message len=%d\n",len);
	if(msgBuffer)
	    free(msgBuffer);
}

static int isValidPacket(const char *ptr,uint32_t size)
{ 
    struct  TMSG_HEADER *pMsgHeader = (struct TMSG_HEADER *)ptr; 
    if(ptr==NULL){
		TCP_CLIENT_TRACK_WARN("error:ptr==NULL\n");
		return -1;
    }
    if(pMsgHeader->m_iXmlLen > size || (char)(*((char *)(ptr+pMsgHeader->m_iXmlLen+4-1))) != 0){
		TCP_CLIENT_TRACK_WARN("error:pMsgHeader->m_iXmlLen:%d size:%d\n", pMsgHeader->m_iXmlLen, size);
		return -2;
    }
    if(memcmp(pMsgHeader->session,CMDSESSION,strlen(CMDSESSION)) !=0){
		return -3;
	}
    else{
    	if(pMsgHeader->cMsgID < MSG_CAPLOGIN || pMsgHeader->cMsgID >MSG_CMDMAX){
			TCP_CLIENT_TRACK_WARN("error:pMsgHeader->cMsgID=%d\n",pMsgHeader->cMsgID);
			return -4;
        }
    }
    return 0;
}

static int procLoginReturnMsg(struct TMSG_HEADER *pMsgHeader)
{
	struct TMSG_LOGINRET *pMsgRet = (struct TMSG_LOGINRET*)pMsgHeader;
	tcpClient.loginState=0;
	if(pMsgRet->m_nStatus == MSG_CAPLOGINSUCCESS)
	{
		tcpClient.loginState=1;
	}
	TCP_CLIENT_TRACK_INFO("rtpSend::MSG_CAPLOGIN state=%d\n",pMsgRet->m_nStatus);
	return pMsgRet->m_nStatus;
}

static int  resolvePacket(const char *ptr,uint32_t size)
{
    int iRet=-1;
    if(ptr==NULL){
		TCP_CLIENT_TRACK_WARN("error:ptr==NULL\n");
		return -1;
    }
    if(maxPacket){
        memcpy(maxPacket+lastSumLength,ptr,size);
        lastSumLength=lastSumLength+size;
        ptr=maxPacket;
        size=lastSumLength;
    }
    iRet=isValidPacket(ptr,size);
    if(iRet==-2){
        if(maxPacket==NULL){
            struct TMSG_HEADER *pMsgHeader = (struct TMSG_HEADER *)ptr;
            maxPacket=malloc(pMsgHeader->m_iXmlLen+100);
            if(maxPacket){
                memcpy(maxPacket,ptr,size);
                lastSumLength=size;
            }
            TCP_CLIENT_TRACK_INFO("pMsgHeader->m_iXmlLen=%d %d\n",pMsgHeader->m_iXmlLen,lastSumLength);
        }
        return iRet;
    }else if(iRet!=0){
    	if(maxPacket){
            free(maxPacket);
            lastSumLength=0;
        }
    } 
    struct TMSG_HEADER *pMsgHeader = (struct TMSG_HEADER *)ptr;
    struct TMSG_REQUESTPLAYURL *pPlayUrl=NULL; 
    struct TMSG_REQUESTPLAYURLLIST *pPlayUrlList=NULL;
	TCP_CLIENT_TRACK_INFO("msgId=%d\n", pMsgHeader->cMsgID);
    switch (pMsgHeader->cMsgID)
	{   
    	case MSG_INTELLIGENT:
    		break;
    	case MSG_PLAYURL:
        	{
        	    pPlayUrl = (struct TMSG_REQUESTPLAYURL*)pMsgHeader;
        	    TCP_CLIENT_TRACK_INFO("MSG_PLAYURL %d %s\n",pPlayUrl->flage,pPlayUrl->url); 
        	}
    		break;
    	case MSG_PLAYURLLIST:
    	    {
    			pPlayUrlList = (struct TMSG_REQUESTPLAYURLLIST*)pMsgHeader;
    			if(strlen(pPlayUrlList->url)>10)
    			    analysisHttpStr(pPlayUrlList->url);
    			TCP_CLIENT_TRACK_INFO("MSG_PLAYURL %d %s\n",pPlayUrlList->flage,pPlayUrlList->url); 
    	    }
    	    break;
    	case MSG_PLAYSTATUS:
    		break;
    	case MSG_NODIFYCAPSOUNDVALUE:
    		break;
    	case MSG_GETSOUNDVALUE:
    		break;
    	case MSG_SENDSWITCH:
    		break;
    	case MSG_LOGINRET:
            {
                procLoginReturnMsg(pMsgHeader); 
                TCP_CLIENT_TRACK_INFO("MSG_LOGINRET\n");    
    		} 
    		break;
    	case MSG_KEEPALIVE:
            {
                tcpClient.recvLastAliveTime = OS_JiffiesToMSecs(OS_GetJiffies());
                TCP_CLIENT_TRACK_INFO("MSG_KEEPALIVE %d\n",tcpClient.recvLastAliveTime);
    		}
    		break;
    	case MSG_PTZCOMMAND:
    		break;
    	case MSG_PARAMCONFIG:
    		break;
    	case MSG_GETPARAMCONFIG:
    		break;
    	case MSG_AUDIODATAGSM:
    		break;
    	case MSG_ViewOutTalk:
    		break;
    	case MSG_ViewTalk:
    		break;
        default:
            break;   
	}  
	if(maxPacket){
        free(maxPacket);
        lastSumLength=0;
    }
	return pMsgHeader->cMsgID;
}

static void tcp_errfun(void *arg,err_t err)
{
    if(err == ERR_OK)
        return;
    tcp_client_connection_close();
    TCP_CLIENT_TRACK_INFO("tcp_errfun be called err=%d tcpClient.state=%d\n",err,tcpClient.state);
}
 
static void tcp_err_close()
{
    tcp_recv(tcpClient.pcb, NULL);
    if(tcpClient.p_tx != NULL){
        pbuf_free(tcpClient.p_tx);
        tcpClient.p_tx=NULL;
    }
    if(tcpClient.pcb != NULL){
        tcp_close(tcpClient.pcb);
        tcpClient.pcb=NULL;
    }
    tcpClient.state = STATE_TCP_CLINET_CLOSING;
    memset(&tcpClient,0,sizeof(tcpClient));
    if(maxPacket){
        free(maxPacket);
        lastSumLength=0;
    }
    TCP_CLIENT_TRACK_INFO("tcp_err_close be called tcpClient.state=%d\n",tcpClient.state);
}
 
void tcp_client_connection_close()
{
    tcp_err_close();
}
       
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{ 
    struct client *tcpClient;
    err_t ret_err;
    tcpClient = (struct client *)arg;
    if(p == NULL){
        /* remote host closed connection */
        tcpClient->state = STATE_TCP_CLINET_CLOSING;
        if(tcpClient->p_tx == NULL){
           /* we're done sending, close connection */
           tcp_client_connection_close(tpcb, tcpClient);
        }
        ret_err = ERR_OK;
    }   
    else if(err != ERR_OK)
    {
        /* free received pbuf*/
        if(p != NULL)
            pbuf_free(p);
        ret_err = err;
    }
    else if(tcpClient->state == STATE_TCP_CLINET_CONNECTED)
    {  
        /* Acknowledge data reception */
        tcp_recved(tpcb, p->tot_len);
        //dumpHex((char*)p->payload,p->len);
        resolvePacket((char*)p->payload,p->len);
        pbuf_free(p);                   
        ret_err = ERR_OK;   
    }
    else /* data received when connection already closed */
    {
        /* Acknowledge data reception */
        tcp_recved(tpcb, p->tot_len);
        /* free pbuf and do nothing */
        pbuf_free(p);
        ret_err = ERR_OK;
    }
    TCP_CLIENT_TRACK_INFO("tcp_client_recv be called err=%d tcpClient.state=%d\n",err,tcpClient->state);
    return ret_err;
}

static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    switch(err){
        case ERR_OK:  
          tcpClient.pcb = tpcb;                                 
          tcpClient.p_tx = NULL;
          tcpClient.state = STATE_TCP_CLINET_CONNECTED;  
          tcp_arg(tpcb, (void *)&tcpClient); 
          tcp_recv(tpcb, tcp_client_recv);      
          break;
       case ERR_MEM :
           tcp_client_connection_close();                
          break ;
       default :
           break ;
   }
   TCP_CLIENT_TRACK_INFO("tcp_client_connected be called err=%d tcpClient.state=%d\n",err,tcpClient.state);
   return err;
}

err_t tcp_client_connect(const char *destipStr, uint16_t port)
{
    struct tcp_pcb *client_pcb;
    struct ip_addr DestIPaddr;
    uint8_t destip[4]={0};
    err_t err = ERR_OK;
    memset(&tcpClient,0,sizeof(tcpClient));
    sscanf(destipStr,"%d.%d.%d.%d",(int *)destip, (int *)(destip+1),(int *)(destip+2), (int *)(destip+3));
    client_pcb = tcp_new();
    client_pcb->so_options |= SOF_KEEPALIVE;
    client_pcb->keep_idle = 50000;	   // ms
    client_pcb->keep_intvl = 20000;	   // ms
    client_pcb->keep_cnt = 5;  
    if(client_pcb != NULL){
        IP4_ADDR( &DestIPaddr, *destip, *(destip+1),*(destip+2), *(destip+3) );
        err = tcp_connect(client_pcb,&DestIPaddr,port,tcp_client_connected);
        TCP_CLIENT_TRACK_INFO("connect ip=%d.%d.%d.%d,port %d",*destip, *(destip+1),*(destip+2), *(destip+3),port);
        tcp_err(client_pcb,tcp_errfun);
        tcpClient.sendLastAliveTime = OS_JiffiesToMSecs(OS_GetJiffies());
        tcpClient.recvLastAliveTime = tcpClient.sendLastAliveTime;
    }
    TCP_CLIENT_TRACK_INFO("tcp_client_connected be called err=%d tcpClient.state=%d\n",err,tcpClient.state);
    return err;
}

err_t tcp_client_send(struct tcp_pcb *tpcb, struct client *tcpClient)
{
    struct pbuf *ptr;
    err_t wr_err = ERR_OK; 
    while ((wr_err == ERR_OK) && (tcpClient->p_tx != NULL) && (tcpClient->p_tx->len <= tcp_sndbuf(tpcb)))
    {
        /* get pointer on pbuf from tcpClient structure */
        ptr = tcpClient->p_tx;
        wr_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);
        TCP_CLIENT_TRACK_INFO("tcp_write ptr->len=%d wr_err=%d\n", ptr->len,wr_err);
        //dumpHex((char*)ptr->payload,ptr->len);
        if (wr_err == ERR_OK){ 
            /* continue with next pbuf in chain (if any) */
            tcpClient->p_tx = ptr->next;
            if(tcpClient->p_tx != NULL){
                /* increment reference count for tcpClient.p */
                pbuf_ref(tcpClient->p_tx);
            }
            pbuf_free(ptr);
        }
        else if(wr_err == ERR_MEM){
             /* we are low on memory, try later, defer to poll */
            tcpClient->p_tx = ptr;                                                    
        }
        else{
            tcpClient->p_tx = ptr;    
            /* other problem ?? */
        }
    }
    return wr_err;
}

int tcp_send_message(void *msg, uint16_t len)
{
    if(tcpClient.state != STATE_TCP_CLINET_CONNECTED)  
        return -1;
    if(tcpClient.p_tx == NULL){
        tcpClient.p_tx  = pbuf_alloc(PBUF_TRANSPORT,len,PBUF_RAM);          
        pbuf_take(tcpClient.p_tx , (char*)msg, len);
    }
    return tcp_client_send(tcpClient.pcb,&tcpClient);
}

int sendLoginData(const char *m_szCameraID)
{ 
    struct TMSG_CAPLOGIN msgLogin;
    int iRet;
    if(m_szCameraID == NULL)
        return -1;
    memset(&msgLogin,0,sizeof(struct TMSG_CAPLOGIN));    
    msgLogin.header.m_iXmlLen = sizeof(struct TMSG_CAPLOGIN)-4;
    msgLogin.header.m_bytXmlType = 0;
    memcpy(&msgLogin.header.session,CMDSESSION,strlen(CMDSESSION));
    msgLogin.header.cMsgID = MSG_CAPLOGIN;
    strcpy(msgLogin.password, "9999jecky");
    strcpy(msgLogin.SxtID, m_szCameraID);
    msgLogin.endChar = 0;
    iRet = tcp_send_message((char*)&msgLogin, sizeof(struct TMSG_CAPLOGIN));
    if (iRet < 0){
        TCP_CLIENT_TRACK_WARN("msg login fail %s!\n", m_szCameraID);
        return -1;
    } 
    else
    {
       TCP_CLIENT_TRACK_INFO("msg: login success %s,send %d bytes\n",m_szCameraID, iRet);
       return 0;
    }     
}

int sendAliveData(const char *m_szCameraID)
{ 
    struct TMSG_KEEPALIVE msKeepAlive;
    int iRet;
    memset(&msKeepAlive,0,sizeof(struct TMSG_KEEPALIVE));
    msKeepAlive.header.m_iXmlLen = sizeof(struct TMSG_KEEPALIVE)-4;
    msKeepAlive.header.m_bytXmlType = 0;
    memcpy(&msKeepAlive.header.session,CMDSESSION,strlen(CMDSESSION));
    msKeepAlive.header.cMsgID = MSG_KEEPALIVE;
    msKeepAlive.endChar = 0;
    iRet = tcp_send_message((char*)&msKeepAlive, sizeof(struct TMSG_KEEPALIVE));
    if (iRet < 0){
        TCP_CLIENT_TRACK_WARN("msg alive fail %s!\n", m_szCameraID);
        return -1;
    } 
    else
    {
       TCP_CLIENT_TRACK_INFO("msg: alive success %s,send %d bytes\n",m_szCameraID, iRet);
       return 0;
    }         
}

int sendAliveDataTask(const char *m_szCameraID)
{
    uint32_t now = OS_JiffiesToMSecs(OS_GetJiffies());
    int iRet=0;
    if((now-tcpClient.sendLastAliveTime)>TIME_ALIVE_TIME)
    {
       tcpClient.sendLastAliveTime=now;
       iRet=sendAliveData(m_szCameraID);
    }
    return iRet;
}

int getTcpClientState(void)
{
    return tcpClient.state;
}

int getTcpClientLoginState(void)
{
    return tcpClient.loginState;
}

int getTcpTimeout(void)
{
    uint32_t now = OS_JiffiesToMSecs(OS_GetJiffies()); 
    if((now-tcpClient.recvLastAliveTime) > TIME_ALIVE_TIMEOUT)
        return 1; 
    else
        return 0;
}

