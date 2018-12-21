#ifndef __TCP_CLIENT_H_
#define __TCP_CLIENT_H_

#include "driver/component/component_def.h"

#define TCP_CLIENT_INFO 1
#define TCP_CLIENT_WARN 1

#define TCP_CLIENT(flags, fmt, arg...)	\
	do {								\
		if (flags) 						\
			printf(fmt, ##arg);		\
	} while (0)

#define TCP_CLIENT_TRACK_INFO(fmt, arg...)	\
			TCP_CLIENT(TCP_CLIENT_INFO, "[TCP CLIENT TRACK] %s():%d "fmt, \
															__func__, __LINE__, ##arg)
#define TCP_CLIENT_TRACK_WARN(fmt, arg...)	\
			TCP_CLIENT(TCP_CLIENT_WARN, "[TCP CLIENT WARN] %s():%d "fmt, \
															__func__, __LINE__, ##arg)

#define TCP_CLIENT_THREAD_STACK_SIZE	1024*2
#define TCP_SEND_DATA_MAX_LEN    (1344)
enum    TCP_CLIENT_STATE{
    STATE_TCP_CLINET_CLOSING       = 0, 
    STATE_TCP_CLINET_CONNECT       = 1, 
    STATE_TCP_CLINET_CONNECTED     = 2, 
    STATE_TCP_CLINET_NULL          = 0xFF,
};

#define AUDIOSAMPLERATE     8000
enum    TCMDTYPE
{
	INVALID_MSG	= -1,
	MSG_CAPLOGIN = 0,						//客户端登陆
	MSG_VIEWLOGIN=1,						    //观看端登陆
	MSG_LOGINRET,							//服务器返回
	MSG_SENDSWITCH,							//客户端与服务器发送开关
	MSG_KEEPALIVE,							//心跳保活检测
	MSG_PTZCOMMAND,						    //云台控制
	MSG_SENDLOGFILE,						//发送日志文件
	MSG_PARAMCONFIG,						//参数配置
	MSG_GETPARAMCONFIG =8,				    //获取参数
	MSG_AUDIODATA,							//音频数据
	MSG_AUDIODREQ,							//音频请求
	MSG_PLAYBACKREQ,						//录像回放请求
	MSG_PLAYBACKASK,						//录像回放应答
	MSG_PLAYBACKCTL,						//录像回放控制
	MSG_AUDIODATAGSM,					    //音频数据GSM
	MSG_AUDIOREJECT,						//音频请求拒绝
	MSG_MIRRORINC,							//镜像连接++
	MSG_MIRRORDEC,							//镜像连接--
	MSG_MIRRORREQ,							//镜像服务器登录请求视频
	MSG_ViewOutTalk = 26,					//浏览端退出讲话状态
	MSG_ViewTalk = 32,						//通知采集端，浏览端正在讲话
	MSG_UPLOADCAPSTATUE = 35,               //采集端异常状态上传（35）
	MSG_NODIFYCAPSOUNDVALUE = 39,           //浏览端命令：通知采集端修改音量
	MSG_GETSOUNDVALUE = 40,                 //浏览端命令：通知采集端修改音量
	MSG_INTELLIGENT = 121,
	MSG_PLAYURL = 122,
	MSG_PLAYSTATUS = 123,
	MSG_PLAYURLLIST = 124,
	MSG_CMDMAX,	
};

enum	 TMSGRETTYPE						//返回内容
{
	INVALID_RETMSG	= -1,
	MSG_CAPLOGINSUCCESS = 0,	            //客户端登陆成功
	MSG_VIEWLOGINSUCCESS = 1,	            //观看端登陆成功
	MSG_CAPLOGINPSWRONG ,				    //客户端登陆密码错误
	MSG_CAPLOGINHASONE ,				    //客户端登陆已经有连接
	MSG_VIEWLOGINPSWRONG,				    //观看端登陆失败
	MSG_VIEWLOGINOVER ,					    //观看端超过人数
	MSG_AUDIOCONNECTED ,					//音频接入
	MSG_AUDIOREFUSED ,					    //音频拒绝
	MSG_AUDIOQUIT ,					        //音频退出
	MSG_AUDIOKICK,					        //音频踢出
	MSG_RETMAX,
};

#define	CMDSESSION			        "DMCT" 	        //命令标识
#define BWJYINTELLIGENT			    "NIWB"
#define TIME_ALIVE_TIME             20*1000
#define TIME_ALIVE_TIMEOUT          60*1000
#define MAX_PACKET_LENGTH           03*1024

/**************************************************************
此模块只完成录音，目前以AMR或WAV文件格式为输出，但是编码可以支持AMR-NB,MP3,WAV这几种格式，
如果需要其它格式，请向安凯申请相应的音频库

***************************************************************/
typedef unsigned int        T_U32;
typedef unsigned char       T_U8;
typedef unsigned short int  WORD;
typedef unsigned int        DWORD;

typedef T_U32	FOURCC;    /* a four character code */


/* MMIO macros */
#define mmioFOURCC(ch0, ch1, ch2, ch3) \
	((T_U32)(T_U8)(ch0) | ((T_U32)(T_U8)(ch1) << 8) | \
	((T_U32)(T_U8)(ch2) << 16) | ((T_U32)(T_U8)(ch3) << 24))

typedef struct CHUNKHDR {	
	FOURCC ckid;        /* chunk ID */	
	DWORD dwSize;       /* chunk size */
} CHUNKHDR;

#define FOURCC_RIFF		mmioFOURCC ('R', 'I', 'F', 'F')		//RIFF
#define FOURCC_LIST		mmioFOURCC ('L', 'I', 'S', 'T')		//LIST
#define FOURCC_WAVE		mmioFOURCC ('W', 'A', 'V', 'E')		//WAVE
#define FOURCC_FMT		mmioFOURCC ('f', 'm', 't', ' ')		//fmt
#define FOURCC_DATA		mmioFOURCC ('d', 'a', 't', 'a')		//data

//#define cpu_to_le32(x) (x)
//#define cpu_to_le16(x) (x)
//#define le32_to_cpu(x) (x)
//#define le16_to_cpu(x) (x)

/*  simplified Header for standard WAV files */
typedef struct WAVEHDR {	
	CHUNKHDR chkRiff;
	FOURCC fccWave;
	CHUNKHDR chkFmt;
    /*format type*/
	WORD wFormatTag; 
	WORD nChannels; 
	DWORD nSamplesPerSec;  
    /* for buffer estimation */
	DWORD nAvgBytesPerSec; 
    /* block size of data*/
	WORD nBlockAlign;       
	WORD wBitsPerSample;
	CHUNKHDR chkData;
} WAVEHDR;

#pragma pack(1)								// 使结构体的数据按照1字节来对齐,省空间
#ifndef UINT
typedef uint32_t		UINT;
#endif
#ifndef BOOL
#define BOOL  char
#endif
struct TMSG_HEADER
{          
 	int     m_iXmlLen;					    // 数据长度（xml类型+xml数据内容长度+结束符长度）
	char    m_bytXmlType;				    // xml类型   
	char 	session[4];					    //协议标识，在此为’TCMD’
	char    cMsgID;						    // 消息标识
};

//账号密码登陆认证 
//客户端登陆
struct TMSG_CAPLOGIN
{
    struct TMSG_HEADER header;
	char password[20];
	char SxtID[30];	
	char endChar;				
};

//客户端与服务器开关发送数据
//开关数据发送
struct TMSG_SENDSWITCH
{
    struct TMSG_HEADER header;
	BOOL		m_nSwitch;	
};

//设置音量
struct TMSG_SETAUDIOVALUE
{
    struct TMSG_HEADER header;
	int	setAudioValue;	
};
//获取音量
struct TMSG_GETAUDIOVALUE
{
    struct TMSG_HEADER header;
	int	getAudioValue;	
};


struct INTELLIGENTDATATOIPC
{
	struct TMSG_HEADER header;
	int				tipe;                       //11, 1:开始发送数据，2正在发送数据,3结束发送数据，4，本地文件播放
	int      		fileNun;                         //文件  序号     
	int 			isCalling;             		//通道号,从0开始计数
	unsigned int	CallingNum;     		//采样率
	int				wBitPerSample;        		//位数
	int				wFormat;					//压缩格式，目前只支持PCM裸音频输入，此参数暂忽略
	int 			len;
	char 			uid[50];
	char 			fileName[50];
    //unsigned char  *pdata;						//音频数据
};;
//
struct TMSG_REQUESTPLAYURL
		
{
	struct TMSG_HEADER header;
	int  flage;  //播放文件类型
	char url[128];
};

struct TMSG_REQUESTPLAYURLLIST
{
	struct TMSG_HEADER header;
	int  flage;  //播放文件类型
	char url[128*20];
};


struct TMSG_PLAYSTATUS
		
{
	struct TMSG_HEADER header;
	int  playStatus;  //播放文件类型
};

//心跳保活检测
struct TMSG_KEEPALIVE
{
     struct TMSG_HEADER header;
     char endChar;// xml类型  
};

//服务器返回
struct TMSG_LOGINRET
{
    struct TMSG_HEADER header;
	int		m_nStatus;	
};

//云台控制命令
struct TMSG_PTZCOMMAND
{
    struct TMSG_HEADER header;
	char	m_nFunc;				//功能
	char	m_nCtl;					//动作
	char	m_nSpeed;			//转速	
};
//参数配置
struct TMSG_PARAMCONFIG
{
	struct TMSG_HEADER header;
	UINT videoParam;
};

typedef struct
{
	char 	        session[4];					    //协议标识，在此为’TCMD’
	unsigned int    ts;
	int				tipe;                       //11, 1:开始发送数据，2结束发送数据
	int 			nChannel;             		//通道号,从0开始计数
	unsigned int	nSamplePerSec;     		//采样率
	int				wBitPerSample;        		//位数
	int				wFormat;					//压缩格式，目前只支持PCM裸音频输入，此参数暂忽略
	int 			len;//数据长度
	int 			humanSize;					//音频数据
}INTELLIGENT_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    int     tcp_client_connect(const char *destipStr, uint16_t port);
    int     tcp_send_message(void *msg, uint16_t len);
    void    tcp_client_connection_close();
    int     sendLoginData(const char *m_szCameraID);
    int     getTcpClientState(void);
    int     getTcpClientLoginState(void);
    int     sendAliveDataTask(const char *m_szCameraID);
    int     sendAliveData(const char *m_szCameraID);
    int     getTcpTimeout(void);
	int 	pushPcmAudioData(const char *audioBuffer,int length,int flag,int type);
    Component_Status tcp_client_task_init();
    int     sendTcpClientStatus(uint8_t status);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


