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
#include "rtp_server.h"
#include "http_player.h"
#include "fs/fatfs/ff.h"

#define WAVE_FORMAT_PCM 	1
struct client
{
    uint8_t  state;
    uint8_t  loginState;
    uint32_t sendLastAliveTime;
    uint32_t recvLastAliveTime;
    struct tcp_pcb *pcb;
    struct pbuf *p_tx;
}tcpClient;


static uint8_t      tcp_client_task_run = 0;
static OS_Thread_t  tcp_client_task_thread;
static int          lastSumLength=0;
static int          amrSumLength=0;
static char *       msgPacket         = NULL;
static char *       speexBuffer       = NULL;
static char *       bytSendAudioBuf   = NULL;
static char *       amrAudioBuf       = NULL;
uint8_t             tcpClientStatus   = 0; // 1 run 2 sendBuf and end 0 stop

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
    if(ptr==NULL){
		TCP_CLIENT_TRACK_WARN("error:ptr==NULL\n");
		return -1;
    }
    struct  TMSG_HEADER *pMsgHeader = (struct TMSG_HEADER *)ptr; 
    int packetSum=(pMsgHeader->m_iXmlLen+4);
    if(packetSum > size){
		TCP_CLIENT_TRACK_WARN("error:packetSize:%d size:%d\n", pMsgHeader->m_iXmlLen, size);
		return -2;
    }
    if((char)(*((char *)(ptr+packetSum-1))) != 0)
    {
    	TCP_CLIENT_TRACK_WARN("error:last char is 0x%x\n",(char)(*((char *)(ptr+packetSum-1))));
		return -3;
    }
    if(pMsgHeader->m_iXmlLen > MAX_PACKET_LENGTH)
    {
        TCP_CLIENT_TRACK_WARN("error:packet is to big 0x%x\n",pMsgHeader->m_iXmlLen);
        return -4;
    }
    if(memcmp(pMsgHeader->session,CMDSESSION,strlen(CMDSESSION)) !=0)
    {
		return -5;
	}
	if(pMsgHeader->cMsgID < MSG_CAPLOGIN || pMsgHeader->cMsgID >MSG_CMDMAX){
		TCP_CLIENT_TRACK_WARN("error:MsgID=%d\n",pMsgHeader->cMsgID);
		return -6;
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

static int  realResolvePacket(const char *ptr,uint32_t size)
{
    if(ptr==NULL){
		TCP_CLIENT_TRACK_WARN("error:ptr==NULL\n");
		return -1;
    }
    struct TMSG_HEADER *pMsgHeader = (struct TMSG_HEADER *)ptr;
    struct TMSG_REQUESTPLAYURL *pPlayUrl=NULL; 
    struct TMSG_REQUESTPLAYURLLIST *pPlayUrlList=NULL;
    struct TMSG_PLAYSTATUS *pPlayStatus=NULL;
	TCP_CLIENT_TRACK_INFO("msgId=%d\n", pMsgHeader->cMsgID);
    switch (pMsgHeader->cMsgID)
	{   
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
		case MSG_PLAYSTATUS:
		    {
		        pPlayStatus = (struct TMSG_PLAYSTATUS*)pMsgHeader;
		        stopHttpAudioPlay(pPlayStatus->playStatus);
		        TCP_CLIENT_TRACK_INFO("MSG_PLAYSTATUS %d\n",pPlayStatus->playStatus);
		    }
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
		case MSG_INTELLIGENT:
    	case MSG_NODIFYCAPSOUNDVALUE:
    	case MSG_GETSOUNDVALUE:
    	case MSG_SENDSWITCH:
    	case MSG_PTZCOMMAND:
    	case MSG_PARAMCONFIG:
    	case MSG_GETPARAMCONFIG:
    	case MSG_AUDIODATAGSM:
    	case MSG_ViewOutTalk:
    	case MSG_ViewTalk:
        default:
            break;   
	}
	return pMsgHeader->cMsgID;
}

static int  resolvePacket(const char *ptr,uint32_t size)
{
    int iRet=-1;
    int i=0;
    int packetSum=0;
    struct TMSG_HEADER *pMsgHeader=NULL;
    if(ptr==NULL){
		TCP_CLIENT_TRACK_WARN("error:ptr==NULL\n");
		return -1;
    }
    //dumpHex(ptr,size);
    if(lastSumLength!=0)
    {
        if((lastSumLength+size)<=MAX_PACKET_LENGTH)
        {
             int packetFlag=0;
             memcpy(msgPacket+lastSumLength,ptr,size);
             lastSumLength=lastSumLength+size;
             for(i=0;i<10;i++)
             {
                 iRet=isValidPacket(msgPacket+packetFlag,lastSumLength);
                 if(iRet==0)
                 {
                     pMsgHeader=(struct TMSG_HEADER *)msgPacket+packetFlag; 
                     packetSum=(pMsgHeader->m_iXmlLen+4);
                     iRet=realResolvePacket(msgPacket+packetFlag,packetSum);
                     packetFlag=packetFlag+packetSum;
                     lastSumLength=lastSumLength-packetFlag;
                     TCP_CLIENT_TRACK_WARN("packet success msgid=%d,length=%d,sumLess=%d\n",pMsgHeader->cMsgID,packetSum,lastSumLength);
                     if(lastSumLength==0)
                     {
                        return 0;
                     }    
                 }  
                 else if(iRet==-2)
                 {
                     if(i>0)
                     {
                         memcpy(msgPacket,ptr+(size-lastSumLength),lastSumLength);
                     }
                     TCP_CLIENT_TRACK_WARN("packet lastSumLength=%d\n",lastSumLength);
                     return -2;
                             
                 }
                 else
                 {
                     lastSumLength=0;
                     TCP_CLIENT_TRACK_WARN("packet error\n");
                     return -1;
                 }
             }
  
        }
        else
            lastSumLength=0; 
    }
    
    if(lastSumLength==0)
    {
        iRet=isValidPacket(ptr,size);
        if(iRet==0)
        {
           pMsgHeader = (struct TMSG_HEADER *)ptr; 
           packetSum=(pMsgHeader->m_iXmlLen+4);
           if(size>packetSum)  //一个字节长度和最后一个字节 
           {
               lastSumLength=size-packetSum;
               memcpy(msgPacket,ptr+packetSum,lastSumLength);
               TCP_CLIENT_TRACK_WARN("packet lastSumLength=%d\n",lastSumLength);
           }
           iRet=realResolvePacket(ptr,packetSum);
        }
        else if(iRet==-2)
        {
            lastSumLength=size;  
            memcpy(msgPacket,ptr,lastSumLength);
        }     
    } 
    return iRet;
    
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
    if(tcpClient.p_tx){
        pbuf_free(tcpClient.p_tx);
        tcpClient.p_tx=NULL;
    }
    if(tcpClient.pcb)
    {
        tcp_abort(tcpClient.pcb);//终止连接,删除pcb控制块
        tcp_recv(tcpClient.pcb, NULL);
        tcp_close(tcpClient.pcb);
        tcpClient.pcb=NULL;
    }
    tcpClient.state = STATE_TCP_CLINET_CLOSING;
    memset(&tcpClient,0,sizeof(tcpClient));
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
            tcp_nagle_disable(tpcb);
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

int tcp_client_connect(const char *destipStr, uint16_t port)
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
    client_pcb->keep_intvl = 10000;	   // ms
    client_pcb->keep_cnt = 5;  
    if(client_pcb != NULL){
        IP4_ADDR( &DestIPaddr, *destip, *(destip+1),*(destip+2), *(destip+3) );
        err = tcp_connect(client_pcb,&DestIPaddr,port,tcp_client_connected);
        tcp_err(client_pcb,tcp_errfun);
        tcpClient.sendLastAliveTime = OS_JiffiesToMSecs(OS_GetJiffies());
        tcpClient.recvLastAliveTime = tcpClient.sendLastAliveTime;
    }
    TCP_CLIENT_TRACK_INFO("connect ip=%d.%d.%d.%d,port %d,err=%d,state=%d\n",*destip, *(destip+1),*(destip+2), *(destip+3),port,err,tcpClient.state);
    return (int)err;
}

err_t tcp_client_send(struct tcp_pcb *tpcb, struct client *tcpClient)
{
    struct pbuf *ptr;
    err_t wr_err = ERR_OK;
    
    TCP_CLIENT_TRACK_INFO("wr_err=%d,p_tx=%p,len=%d,snd_buf=%d\n",
                           wr_err,tcpClient->p_tx,tcpClient->p_tx->len,tcp_sndbuf(tpcb));
    
    while ((wr_err == ERR_OK) && (tcpClient->p_tx != NULL) && (tcpClient->p_tx->len <= tcp_sndbuf(tpcb)))
    {
        /* get pointer on pbuf from tcpClient structure */
        ptr = tcpClient->p_tx;
        wr_err = tcp_write(tpcb, ptr->payload, ptr->len, TCP_WRITE_FLAG_COPY);
        //TCP_CLIENT_TRACK_INFO("tcp_write ptr->len=%d wr_err=%d\n", ptr->len,wr_err);
        //dumpHex((char*)ptr->payload,ptr->len);
        if (wr_err == ERR_OK){ 
            tcp_output(tpcb);
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
        TCP_CLIENT_TRACK_WARN("msg: login fail %s,iRet=%d\n",m_szCameraID,iRet);
        return -1;
    } 
    else
    {
       TCP_CLIENT_TRACK_INFO("msg: login success %s,send %d bytes\n",m_szCameraID,sizeof(struct TMSG_CAPLOGIN));
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
        TCP_CLIENT_TRACK_WARN("msg: alive fail %s,iRet=%d\n", m_szCameraID,iRet);
        return -1;
    } 
    else
    {
       TCP_CLIENT_TRACK_INFO("msg: alive success %s,send %d bytes\n",m_szCameraID, sizeof(struct TMSG_KEEPALIVE));
       return 0;
    }         
}

int saveAudioDataToMMC(const char *audioBuffer,int length,int flag)
{
	static FIL file;
	static char path[64]={0};
	int result = FR_OK;
	uint32_t writenum=0;
    if(flag==1)
    {
        sprintf(path,"0:/%d.amr",OS_JiffiesToMSecs(OS_GetJiffies()));
        result = f_open(&file, path, FA_OPEN_ALWAYS|FA_READ|FA_WRITE);
        if(result != FR_OK)
			TCP_CLIENT_TRACK_WARN("[music file]failed to open,%s\n",path); 
        else 
			TCP_CLIENT_TRACK_WARN("[music file]success to open,%s\n",path);
    }
	else if(flag==2)
	{
		result = f_write(&file, audioBuffer, length, &writenum);
		if(result != FR_OK)
			TCP_CLIENT_TRACK_WARN("write failed(%d).\n",result);
	}
    else if(flag==3)
    {
        result=f_close(&file); 
        if(result != FR_OK)
			TCP_CLIENT_TRACK_WARN("[music file]failed to close,%s\n",path); 
        else 
			TCP_CLIENT_TRACK_WARN("[music file]success to close,%s\n",path);

    }
	return result;
}

int sendAudioData(const char *audioBuffer,int length,int flag,int type)
{
    INTELLIGENT_DATA m_intelligentData;
    memset(&m_intelligentData, 0, sizeof(INTELLIGENT_DATA));
    memcpy(&m_intelligentData.session,BWJYINTELLIGENT,strlen(BWJYINTELLIGENT));//2048+36+2+4
    m_intelligentData.nChannel = 1;
    m_intelligentData.nSamplePerSec = AUDIOSAMPLERATE;
    m_intelligentData.wBitPerSample = 16;
    m_intelligentData.wFormat = 8;//1024 PCM 8:amr
    m_intelligentData.len = length;
    m_intelligentData.ts = type;
    m_intelligentData.tipe = flag;
	//saveAudioDataToMMC(audioBuffer,length,flag);
    if(bytSendAudioBuf==NULL || length>MAX_PACKET_LENGTH)
        return -1;
    int iSendAudioLen = 0;  
    int sumLength=sizeof(INTELLIGENT_DATA) + length + 2;
    memcpy(bytSendAudioBuf+iSendAudioLen, &sumLength, sizeof(sumLength));
    iSendAudioLen += sizeof(sumLength);
	bytSendAudioBuf[iSendAudioLen++] = 0x00;
    memcpy(bytSendAudioBuf+iSendAudioLen,&m_intelligentData,sizeof(INTELLIGENT_DATA));
    iSendAudioLen += sizeof(INTELLIGENT_DATA);
	if(length>0 && audioBuffer!=NULL)
		memcpy(bytSendAudioBuf+iSendAudioLen,audioBuffer,length);
    iSendAudioLen += length;
    bytSendAudioBuf[iSendAudioLen++] = 0x00;
    int iRet = tcp_send_message((char*)bytSendAudioBuf,iSendAudioLen);
    if(iRet < 0){
        TCP_CLIENT_TRACK_WARN("msg: audio fail,iRet=%d\n",iRet);
        iRet=-1;
    } 
    else
    {
		TCP_CLIENT_TRACK_INFO("msg: audio success,send %d bytes\n",iSendAudioLen);
		iRet=0;
    }  
	return iRet;
}

int pushPcmAudioData(const char *audioBuffer,int length,int flag,int type)
{
    if(tcpClientStatus==3)//start and read
    {
        if((amrSumLength+length)<=20*1024)
        {
            memcpy(amrAudioBuf+amrSumLength,audioBuffer,length);
            amrSumLength=amrSumLength+length;
        }    
    }
    return 0;
}

int sendTcpClientStatus(uint8_t status)
{
    const unsigned char amrHeader[6]={0x23,0x21,0x41,0x4d,0x52,0x0A};
    tcpClientStatus=status;
    if(tcpClientStatus==1)
    {
        memcpy(amrAudioBuf,amrHeader,sizeof(amrHeader));
        amrSumLength=sizeof(amrHeader);
    }
    return tcpClientStatus;
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

void tcp_client_speex_task(void *arg)
{
    int iRet=0;
    int i=0;
    int length=0;
	tcpClientStatus=0;
	amrSumLength=0;
    msgPacket=malloc(MAX_PACKET_LENGTH);
    bytSendAudioBuf=malloc(MAX_PACKET_LENGTH);
    amrAudioBuf=malloc(20*1024);
    speexBuffer=malloc(512);
    TCP_CLIENT_TRACK_INFO("tcp client task start\n");
	while (tcp_client_task_run) 
	{
		switch(tcpClientStatus)
		{
			case 1:
				sendAudioData(NULL,0,1,7);
				tcpClientStatus=3;
				TCP_CLIENT_TRACK_INFO("tcp_client_speex_task start\n");
				break;
			case 2:
			    {
                    length=amrSumLength;
                    for(i=0;i<20;i++)
                    {
                        if(length>TCP_SEND_DATA_MAX_LEN)
                        {
                        	iRet=sendAudioData((char *)amrAudioBuf+TCP_SEND_DATA_MAX_LEN*i,TCP_SEND_DATA_MAX_LEN,2,1);
                            length=amrSumLength-TCP_SEND_DATA_MAX_LEN*(i+1);
                        }
                        else
                        {
                            iRet=sendAudioData((char *)amrAudioBuf+TCP_SEND_DATA_MAX_LEN*i,length,2,1);
                            tcpClientStatus=4;
                            break;
                        }
                        if(iRet!=ERR_OK)
	                    {
	                        TCP_CLIENT_TRACK_WARN("msg: sendAudioData fail,iRet=%d,i=%d\n",iRet,i);
	                    }
                    }
			    }
			    break;
			case 3:
				break;
			case 4:
				sendAudioData(NULL,0,3,7);
				tcpClientStatus=0;
				TCP_CLIENT_TRACK_INFO("tcp_client_speex_task over\n");
				break;
			default:
				OS_MSleep(100);
				break;
		}
		if(tcpClientStatus!=2 && tcpClientStatus!=4)
		    OS_MSleep(100);
	}
	if(msgPacket)
	    free(msgPacket);
	if(bytSendAudioBuf) 
	    free(bytSendAudioBuf);
    if(speexBuffer)
        free(speexBuffer);
    if(amrAudioBuf)
        free(amrAudioBuf);
	OS_ThreadDelete(&tcp_client_task_thread);
	TCP_CLIENT_TRACK_INFO("tcp client task end\n");
}

Component_Status tcp_client_task_init()
{
	tcp_client_task_run = 1;
	if (OS_ThreadCreate(&tcp_client_task_thread,
		                "",
		                tcp_client_speex_task,
		               	NULL,
		                OS_THREAD_PRIO_APP,
		                RTP_SERVER_THREAD_STACK_SIZE) != OS_OK) {
		TCP_CLIENT_TRACK_WARN("tcp client thread create error\n");
		return COMP_ERROR;
	}

	return COMP_OK;
}

