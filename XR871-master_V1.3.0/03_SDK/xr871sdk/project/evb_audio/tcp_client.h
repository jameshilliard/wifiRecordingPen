#ifndef __TCP_CLIENT_H_
#define __TCP_CLIENT_H_

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

enum    TCP_CLIENT_STATE{
    STATE_TCP_CLINET_CLOSING       = 0, 
    STATE_TCP_CLINET_CONNECT       = 1, 
    STATE_TCP_CLINET_CONNECTED     = 2, 
    STATE_TCP_CLINET_NULL          = 0xFF,
};


enum    TCMDTYPE
{
	INVALID_MSG	= -1,
	MSG_CAPLOGIN = 0,						//�ͻ��˵�½
	MSG_VIEWLOGIN=1,						//�ۿ��˵�½
	MSG_LOGINRET,							//����������
	MSG_SENDSWITCH,							//�ͻ�������������Ϳ���
	MSG_KEEPALIVE,							//����������
	MSG_PTZCOMMAND,						    //��̨����
	MSG_SENDLOGFILE,						//������־�ļ�
	MSG_PARAMCONFIG,						//��������
	MSG_GETPARAMCONFIG =8,				    //��ȡ����
	MSG_AUDIODATA,							//��Ƶ����
	MSG_AUDIODREQ,							//��Ƶ����
	MSG_PLAYBACKREQ,						//¼��ط�����
	MSG_PLAYBACKASK,						//¼��ط�Ӧ��
	MSG_PLAYBACKCTL,						//¼��طſ���
	MSG_AUDIODATAGSM,					    //��Ƶ����GSM
	MSG_AUDIOREJECT,						//��Ƶ����ܾ�
	MSG_MIRRORINC,							//��������++
	MSG_MIRRORDEC,							//��������--
	MSG_MIRRORREQ,							//�����������¼������Ƶ
	MSG_ViewOutTalk = 26,					//������˳�����״̬
	MSG_ViewTalk = 32,						//֪ͨ�ɼ��ˣ���������ڽ���
	MSG_UPLOADCAPSTATUE = 35,               //�ɼ����쳣״̬�ϴ���35��
	MSG_NODIFYCAPSOUNDVALUE = 39,           //��������֪ͨ�ɼ����޸�����
	MSG_GETSOUNDVALUE = 40,                 //��������֪ͨ�ɼ����޸�����
	MSG_INTELLIGENT = 121,
	MSG_PLAYURL = 122,
	MSG_PLAYSTATUS = 123,
	MSG_PLAYURLLIST = 124,
	MSG_CMDMAX,	
};

enum	 TMSGRETTYPE						//��������
{
	INVALID_RETMSG	= -1,
	MSG_CAPLOGINSUCCESS = 0,	            //�ͻ��˵�½�ɹ�
	MSG_VIEWLOGINSUCCESS = 1,	            //�ۿ��˵�½�ɹ�
	MSG_CAPLOGINPSWRONG ,				    //�ͻ��˵�½�������
	MSG_CAPLOGINHASONE ,				    //�ͻ��˵�½�Ѿ�������
	MSG_VIEWLOGINPSWRONG,				    //�ۿ��˵�½ʧ��
	MSG_VIEWLOGINOVER ,					    //�ۿ��˳�������
	MSG_AUDIOCONNECTED ,					//��Ƶ����
	MSG_AUDIOREFUSED ,					    //��Ƶ�ܾ�
	MSG_AUDIOQUIT ,					        //��Ƶ�˳�
	MSG_AUDIOKICK,					        //��Ƶ�߳�
	MSG_RETMAX,
};

#define	CMDSESSION			"DMCT" 	        //�����ʶ
#define TIME_ALIVE_TIME     20*1000
#define TIME_ALIVE_TIMEOUT  60*1000

#pragma pack(1)								// ʹ�ṹ������ݰ���1�ֽ�������,ʡ�ռ�
#ifndef UINT
typedef uint32_t		UINT;
#endif
#ifndef BOOL
#define BOOL  char
#endif
struct TMSG_HEADER
{          
 	int     m_iXmlLen;					    // ���ݳ��ȣ�xml����+xml�������ݳ���+���������ȣ�
	char    m_bytXmlType;				    // xml����   
	char 	session[4];					    //Э���ʶ���ڴ�Ϊ��TCMD��
	char    cMsgID;						    // ��Ϣ��ʶ
};

//�˺������½��֤ 
//�ͻ��˵�½
struct TMSG_CAPLOGIN
{
    struct TMSG_HEADER header;
	char password[20];
	char SxtID[30];	
	char endChar;				
};

//�ͻ�������������ط�������
//�������ݷ���
struct TMSG_SENDSWITCH
{
    struct TMSG_HEADER header;
	BOOL		m_nSwitch;	
};

//��������
struct TMSG_SETAUDIOVALUE
{
    struct TMSG_HEADER header;
	int	setAudioValue;	
};
//��ȡ����
struct TMSG_GETAUDIOVALUE
{
    struct TMSG_HEADER header;
	int	getAudioValue;	
};


struct INTELLIGENTDATATOIPC
{
	struct TMSG_HEADER header;
	int				tipe;                       //11, 1:��ʼ�������ݣ�2���ڷ�������,3�����������ݣ�4�������ļ�����
	int      		fileNun;                         //�ļ�  ���     
	int 			isCalling;             		//ͨ����,��0��ʼ����
	unsigned int	CallingNum;     		//������
	int				wBitPerSample;        		//λ��
	int				wFormat;					//ѹ����ʽ��Ŀǰֻ֧��PCM����Ƶ���룬�˲����ݺ���
	int 			len;
	char 			uid[50];
	char 			fileName[50];
    //unsigned char  *pdata;						//��Ƶ����
};;
//
struct TMSG_REQUESTPLAYURL
		
{
	struct TMSG_HEADER header;
	int  flage;  //�����ļ�����
	char url[128];
};

struct TMSG_REQUESTPLAYURLLIST
{
	struct TMSG_HEADER header;
	int  flage;  //�����ļ�����
	char url[128*20];
};


struct TMSG_PLAYSTATUS
		
{
	struct TMSG_HEADER header;
	int  playStatus;  //�����ļ�����
};

//����������
struct TMSG_KEEPALIVE
{
     struct TMSG_HEADER header;
     char endChar;// xml����  
};

//����������
struct TMSG_LOGINRET
{
    struct TMSG_HEADER header;
	int		m_nStatus;	
};

//��̨��������
struct TMSG_PTZCOMMAND
{
    struct TMSG_HEADER header;
	char	m_nFunc;				//����
	char	m_nCtl;					//����
	char	m_nSpeed;			//ת��	
};
//��������
struct TMSG_PARAMCONFIG
{
	struct TMSG_HEADER header;
	UINT videoParam;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    err_t   tcp_client_connect(const char *destipStr, uint16_t port);
    int     tcp_send_message(void *msg, uint16_t len);
    void    tcp_client_connection_close();
    int     sendLoginData(const char *m_szCameraID);
    int     getTcpClientState(void);
    int     getTcpClientLoginState(void);
    int     sendAliveDataTask(const char *m_szCameraID);
    int     sendAliveData(const char *m_szCameraID);
    int     getTcpTimeout(void);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


