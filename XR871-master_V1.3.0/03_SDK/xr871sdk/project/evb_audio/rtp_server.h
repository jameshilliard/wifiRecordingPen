#ifndef __RTP_SERVER_H_
#define __RTP_SERVER_H_

#define RTP_SERVER_INFO 1
#define RTP_SERVER_WARN 1

#define RTP_SERVER(flags, fmt, arg...)	\
	do {								\
		if (flags) 						\
			printf(fmt, ##arg);		\
	} while (0)

#define RTP_SERVER_TRACK_INFO(fmt, arg...)	\
			RTP_SERVER(RTP_SERVER_INFO, "[RTP SERVER TRACK] %s():%d "fmt, \
															__func__, __LINE__, ##arg)
#define RTP_SERVER_TRACK_WARN(fmt, arg...)	\
			RTP_SERVER(RTP_SERVER_WARN, "[RTP SERVER WARN] %s():%d "fmt, \
															__func__, __LINE__, ##arg)

#define RTP_SERVER_THREAD_STACK_SIZE	1024*2


enum RTP_SERVER_RUN_STATUS {
    CMD_RTP_SERVER_GET_ADDR        = 0, 
    CMD_RTP_SERVER_CONNECT         = 1, 
    CMD_RTP_SERVER_LOGIN           = 2, 
    CMD_RTP_SERVER_LOGIN_DETECT    = 3,
    CMD_RTP_SERVER_LOGIN_SUC       = 4, 
    CMD_RTP_SERVER_CLOSING         = 5, 
    CMD_RTP_SERVER_NULL            = 0xFF,
};

typedef struct loginReturnInfo_
{
	int result;//返回结果
	int isRequireToReboot;//重启
	int logUploadEnable;//重启
	char relayServerAddr[28];
	int relayServerPort;
	
}LoginReturnInfo;               //服务器返回信息列表

enum RETURNTYPE
{
    OK	= 1,
    NOTLOGINYET = 2,			//设备还未登陆
    UIDNOTEXIST=3,			    //UID不存在
    PWDERROR,					//密码出错
    RETNULL,                       //返回NULL
    OTHERERROR,                 //其它类型错误
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    Component_Status rtp_server_task_init();
    void rtp_server_task(void *arg);
   

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


