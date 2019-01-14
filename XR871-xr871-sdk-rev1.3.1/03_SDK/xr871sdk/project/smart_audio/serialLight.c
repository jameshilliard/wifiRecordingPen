#include "serial_debug.h"
#include "serialLight.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kernel/os/os_time.h"
#include "http_player.h"


typedef struct _studyRecord
{
	uint32_t starTime;
	uint32_t endTime;
	int correctTimes;
	int errorTimes;
	int legFlage;
}STUDYRECORD;

//记录坐姿
typedef struct _record_pose
{
	int goodPos;
	int detectHuman;
}RECORDPOSE;
//记录一个周期的坐姿数据
typedef struct _pose_data_status
{
	RECORDPOSE  recordPose[256];
	int len;
	int detectUnit;
	int manExistNum;
	int manNotExistNum;
	int goodPoseNum;
	int badPoseNum;
	char sendBuf[1024];
}POSEDATASTATUS;

extern int              daVol_def; //默认大拿音量，用户可以自定更改0-5
static uint32_t         g_isStudyMode;//getTickCountMs();
static uint32_t         g_isStudyTime;
static uint32_t         g_startStudyTime;//getTickCountMs();
static uint32_t         g_currentStudyTime;//getTickCountMs();

static POSEDATASTATUS  *g_poseData=NULL;//记录一个周期的坐姿状态;
static STUDYRECORD      oneMinData;
static STUDYRECORD      min45Data;

extern uint32_t getTickSecond();
extern int      voice_tips_add_music(int type,uint8_t nowFlag);
extern int      serial_write(uint8_t *buf, int32_t len);
extern int      reportPoseRemind();
extern int      reportRestRemind();
extern int      poseDataStatus(int detectUnit, int manExistNum, int manNotExistNum, int goodPoseNum, int badPoseNum, char *detectInfo);

static void closeLeg(int sig)
{
	unsigned char ack_ok_buffer[5];
	SERIAL_DBG("closeLeg----\n");
	memset(ack_ok_buffer,'\0',5);
	ack_ok_buffer[0]=0x55;
	ack_ok_buffer[1]=0xaa;
	ack_ok_buffer[2]=0x10;
	ack_ok_buffer[3]=0x00;
	ack_ok_buffer[4]=0x00;
	serial_write(ack_ok_buffer, 5);	

}

static void coldLeg(int sig)
{
	unsigned char ack_ok_buffer[5];
	SERIAL_DBG("coldLeg----\n");
	memset(ack_ok_buffer,'\0',5);
	ack_ok_buffer[0]=0x55;
	ack_ok_buffer[1]=0xaa;
	ack_ok_buffer[2]=0x11;
	ack_ok_buffer[3]=0x01;
	ack_ok_buffer[4]=0x00;
	serial_write(ack_ok_buffer, 5);	

}

static void warnningLeg(int sig)
{
	unsigned char ack_ok_buffer[5];
	SERIAL_DBG("warnningLeg----\n");
	memset(ack_ok_buffer,'\0',5);
	ack_ok_buffer[0]=0x55;
	ack_ok_buffer[1]=0xaa;
	ack_ok_buffer[2]=0x11;
	ack_ok_buffer[3]=0x02;
	ack_ok_buffer[4]=0x00;
	serial_write(ack_ok_buffer, 5);	

}

static void coldwarnningLeg(int sig)
{
	unsigned char ack_ok_buffer[5];
	SERIAL_DBG("coldwarnningLeg----\n");
	memset(ack_ok_buffer,'\0',5);
	ack_ok_buffer[0]=0x55;
	ack_ok_buffer[1]=0xaa;
	ack_ok_buffer[2]=0x11;
	ack_ok_buffer[3]=0x03;
	ack_ok_buffer[4]=0x00;
	serial_write(ack_ok_buffer, 5);	

}

static void openLegOne(int sig)
{
	unsigned char ack_ok_buffer[5];
	SERIAL_DBG("openLegOne----\n");
	memset(ack_ok_buffer,'\0',5);
	ack_ok_buffer[0]=0x55;
	ack_ok_buffer[1]=0xaa;
	ack_ok_buffer[2]=0x10;
	ack_ok_buffer[3]=0x01;
	ack_ok_buffer[4]=0x00;
	serial_write(ack_ok_buffer, 5);	

}

static void openLegTwo(int sig)
{
	unsigned char ack_ok_buffer[5];
	SERIAL_DBG("openLegTwo----\n");
	memset(ack_ok_buffer,'\0',5);
	ack_ok_buffer[0]=0x55;
	ack_ok_buffer[1]=0xaa;
	ack_ok_buffer[2]=0x10;
	ack_ok_buffer[3]=0x02;
	ack_ok_buffer[4]=0x00;
	serial_write(ack_ok_buffer, 5);	

}

static void openLegThree(int sig)
{
	unsigned char ack_ok_buffer[5];
	SERIAL_DBG("openLegThree----\n");
	memset(ack_ok_buffer,'\0',5);
	ack_ok_buffer[0]=0x55;
	ack_ok_buffer[1]=0xaa;
	ack_ok_buffer[2]=0x10;
	ack_ok_buffer[3]=0x03;
	ack_ok_buffer[4]=0x00;
	serial_write(ack_ok_buffer, 5);	

}

void oneMinWarnning(STUDYRECORD *data)
{
	if(data->starTime == 0)
	{
		data->starTime = getTickSecond();
	}
	
	data->endTime = getTickSecond();
	if( data->endTime - data->starTime > 60)
	{
		data->starTime = 0;
		if(data->errorTimes > 10)
		{
			
			closeLeg(1);//close leg
		    OS_Sleep(1);
			voice_tips_add_music(CLOSELEGWARN,0);//warnning sit correct
			OS_Sleep(1);
			closeLeg(1);//close leg
			data->legFlage = 0;//灯关闭状态
			data->errorTimes = 0;
			data->starTime = getTickSecond();
		}
	}
}

void study45MinWarnning(STUDYRECORD *data)
{
	SERIAL_DBG("study45MinWarnning:%d--------------------0\n", data->correctTimes);
	if(data->correctTimes * 2 > 45*60)//45分钟提醒一次
	{
		SERIAL_DBG("study45MinWarnning:%d--------------------2\n", data->correctTimes);
		voice_tips_add_music(RSET45,0); //提醒休息时间到了
		data->correctTimes = 0;
		reportRestRemind();
	}
}

//一分钟一次提醒
void oneTimeWarnning()
{
	SERIAL_DBG("oneTimeWarnning-------------------0\n");
	static uint32_t start = 0;
	//char path[128] = {0};
	if(start == 0)
	{
		start = getTickSecond();
		//voice_tips_stop();
		int fileNum = OS_Rand32()%3;
		//fileNum += 1;
		//sprintf(path, "/usr/share/warinningSound/first/sit%d.mp3", fileNum);
		voice_tips_add_music(fileNum,0);//坐姿错误提醒
	}
	else
	{
		if(getTickSecond() - start > 30)
		{
			//warnning 
			//voice_tips_stop();
			int fileNum = OS_Rand32()%3;
			//fileNum += 1;
			//sprintf(path, "/usr/share/warinningSound/first/sit%d.mp3", fileNum);
			voice_tips_add_music(fileNum,0);//坐姿错误提醒
			start = 0;
			start = getTickSecond();
		}
		
	}
}



void uart_rec_ack(void *rec_buffer, unsigned int rec_length)
{
	static int goodPose = 0;
	static int detectHuman = 0;
	//static int badPose = 0;
	static int timeValue = 0;
	//static int first = 0;
	static int legMode = 3;
	static int legClass = 3;
	
	int flage = 0;
	unsigned char tmp_buffer[33];
	unsigned char ack_ok_buffer[6];
	//char path[128] = {0};
	if(g_isStudyMode == 0)
	{
		daVol_def = 1;
	}
	g_isStudyMode = 1;
    g_isStudyTime = getTickSecond();
	memset(tmp_buffer,'\0',32);
	memcpy(tmp_buffer,rec_buffer,rec_length);
	

	if(g_startStudyTime == 0)
	{
		timeValue = 0;
		memset(&g_poseData, 0, sizeof(g_poseData));
		g_poseData->detectUnit = 5;
		g_poseData->len = 0;
	}
	SERIAL_DBG("timeValue:%d--------------\n", timeValue);
	if(tmp_buffer[0]== 0x55 && tmp_buffer[1]== 0xaa)
	{
		flage = 1;
		if(g_startStudyTime == 0 )
		{
			g_startStudyTime = getTickSecond();
		}
		
		g_currentStudyTime = getTickSecond();
		SERIAL_DBG("aistudy head\n");
		if(tmp_buffer[2]==0x01)
		{
			
			if(tmp_buffer[4]==0x01)
			{
				SERIAL_DBG("detect human \n");
				detectHuman = 1;
				g_poseData->recordPose[timeValue].detectHuman = 1;
				g_poseData->manExistNum++;
				strcat(g_poseData->sendBuf, "1,");
				//voice_tips_add_music("/usr/share/hasHuman.mp3");
			}
			else
			{
				SERIAL_DBG("detect nothing \n");
				detectHuman = 0;
				g_poseData->recordPose[timeValue].detectHuman = 1;
				g_poseData->manNotExistNum++;
				strcat(g_poseData->sendBuf, "0,");
				//voice_tips_add_music("/usr/share/noHuman.mp3");
			}

			if(tmp_buffer[3]==0x01)
			{
				SERIAL_DBG("sit correct \n");
				goodPose = 1;
				g_poseData->recordPose[timeValue].goodPos = 1;
				g_poseData->goodPoseNum++;
				strcat(g_poseData->sendBuf, "1#");
				//voice_tips_add_music("/usr/share/sitCorrect.mp3");
			}
			else
			{
				SERIAL_DBG("sit error \n");
				goodPose = 0;
				g_poseData->recordPose[timeValue].goodPos = 0;
				g_poseData->badPoseNum++;
				strcat(g_poseData->sendBuf, "0#");
				//voice_tips_add_music("/usr/share/errorSit.mp3");
			}
			//
			timeValue++;
			g_poseData->len = timeValue;
			
			
			if(detectHuman == 1)//检测到有人
			{
			    if(goodPose == 1)
				{
					if(oneMinData.legFlage == 0)
					{
						OS_Sleep(1);
						coldwarnningLeg(1);
						OS_Sleep(2);
						coldwarnningLeg(1);
						OS_Sleep(3);
						coldwarnningLeg(1);
						oneMinData.legFlage = 1;
					}
					oneMinData.correctTimes ++;
					min45Data.correctTimes ++;
				}
				else //坐姿错误
				{			
					oneTimeWarnning();
					oneMinData.errorTimes ++;
					min45Data.errorTimes ++;	
					//SERIAL_DBG("oneTimeWarnning--------------------1\n");
					reportPoseRemind();
					OS_Sleep(2);
				}
				oneMinWarnning(&oneMinData);//一分钟坐姿提醒一次
				SERIAL_DBG("study45MinWarnning-------min45Data correct:%d-------------1\n", min45Data.correctTimes);
				study45MinWarnning(&min45Data);//45分钟提醒一次
			}

			SERIAL_DBG("sendBuf=%s\n", g_poseData->sendBuf);
		}
		else if(tmp_buffer[2]==0x02)
		{

			if(tmp_buffer[3]==0x00)
			{
				SERIAL_DBG("close, 灯关闭状态 \n");
				//voice_tips_add_music("/usr/share/closeLeg.mp3");
				g_startStudyTime = 0;
				//zss++stopPlay(3);
				//zss++stopPlay(4);
			}
			else if(tmp_buffer[3]==0x01)
			{
				SERIAL_DBG("one, 一级亮度 \n");
				legClass = 1;
				//voice_tips_add_music("/usr/share/oneLeg.mp3");
			}
			else if(tmp_buffer[3]==0x02)
			{
				SERIAL_DBG("two，二级亮度 \n");
				legClass = 2;
				//voice_tips_add_music("/usr/share/twoLeg.mp3");
			}
			else if(tmp_buffer[3]==0x03)
			{
				SERIAL_DBG("three，三级亮度 \n");
				legClass = 3;
				//voice_tips_add_music("/usr/share/threeLeg.mp3");
			}

			if(tmp_buffer[4]==0x01)
			{
				SERIAL_DBG("one, 冷光 \n");
				legMode = 1;
				//voice_tips_add_music("/usr/share/coldLeg.mp3");
			}
			else if(tmp_buffer[4]==0x02)
			{
				SERIAL_DBG("two,  暖光 \n");
				legMode = 2;
				//voice_tips_add_music("/usr/share/sunLeg.mp3");
			}
			else if(tmp_buffer[4]==0x03)
			{
				SERIAL_DBG("three, 冷暖光 \n");
				legMode = 3;
				//voice_tips_add_music("/usr/share/coldSunLeg.mp3");
			}
			SERIAL_DBG("legMode = %d legClass = %d\n",legMode,legClass);
			//reportRestRemind(g_stConfigCfg.m_unMasterServerCfg.m_objMasterServerCfg.m_szMasterIP, g_szServerNO);
		}
		/*else if(tmp_buffer[2]==0x03)
		{
			if(tmp_buffer[3]== 0x00)//第一次坐姿提醒
			{
				voice_tips_add_music(FIRST_RESET);
			}
			else if(tmp_buffer[3]== 0x01)//第二次坐姿提醒
			{
				voice_tips_add_music(SECOND_RESET);
			}
			else if(tmp_buffer[3]== 0x02)//第三次坐姿提醒
			{
				voice_tips_add_music(THIRD_RESET);
			}
			else if(tmp_buffer[3]== 0x03)//第四次坐姿提醒
			{
				voice_tips_add_music(RESET);				
				reportRestRemind(g_stConfigCfg.m_unMasterServerCfg.m_objMasterServerCfg.m_szMasterIP, g_szServerNO);
			}
			//reportPoseRemind(g_stConfigCfg.m_unMasterServerCfg.m_objMasterServerCfg.m_szMasterIP, g_szServerNO);
			//reportRestRemind(g_stConfigCfg.m_unMasterServerCfg.m_objMasterServerCfg.m_szMasterIP, g_szServerNO);
		}*/
		else if(tmp_buffer[2]==0x04)
		{
			if(tmp_buffer[3]==0x00)
			{
				SERIAL_DBG("volaue 音量减少 \n");
				daVol_def--;
				if(daVol_def < 1)
				{
					daVol_def = 1;
					//strcpy(path, "/usr/share/soundVolume1.mp3");
					//zss++stopPlay(3);
					//zss++stopPlay(4);
					//voice_tips_add_music("/usr/share/soundVolume1.mp3");
				}
				else if(daVol_def == 5)
				{
					//strcpy(path, "/usr/share/soundVolume5.mp3");
					//voice_tips_add_music("/usr/share/soundVolume5.mp3");
				}
				else if(daVol_def == 4)
				{
					
					//strcpy(path, "/usr/share/soundVolume4.mp3");
					//voice_tips_add_music("/usr/share/soundVolume4.mp3");
				}
				else if(daVol_def == 3)
				{
					
					//strcpy(path, "/usr/share/soundVolume3.mp3");
					//voice_tips_add_music("/usr/share/soundVolume3.mp3");
				}
				else if(daVol_def == 2)
				{
					
					//strcpy(path, "/usr/share/soundVolume2.mp3");
					//voice_tips_add_music("/usr/share/soundVolume2.mp3");
				}
				else if(daVol_def == 1)
				{
					
					//strcpy(path, "/usr/share/soundVolume1.mp3");
					//voice_tips_add_music("/usr/share/soundVolume1.mp3");
				}
				/*else if(daVol_def < 1)
				{
					voice_tips_add_music("/tmp/soundVolume0.mp3");
				}*/
			
				/*if(play_http_tipe.once_play_flag != 1)
				{
					//voice_tips_add_music(path);
				}*/
				setVolume(daVol_def);
			}
			else if(tmp_buffer[3]==0x01)
			{
				SERIAL_DBG("volaue 音量增加 \n");
				daVol_def++;
				if(daVol_def >4)
				{
					daVol_def = 5;
					//strcpy(path, "/usr/share/soundVolume5.mp3");
					//voice_tips_add_music("/usr/share/soundVolume1.mp3");
				}
				else if(daVol_def == 4)
				{
					
					//strcpy(path, "/usr/share/soundVolume4.mp3");
					//voice_tips_add_music("/usr/share/soundVolume4.mp3");
				}
				else if(daVol_def == 3)
				{
					
					//strcpy(path, "/usr/share/soundVolume3.mp3");
					//voice_tips_add_music("/usr/share/soundVolume3.mp3");
				}
				else if(daVol_def == 2)
				{
					
					//strcpy(path, "/usr/share/soundVolume2.mp3");
					//voice_tips_add_music("/usr/share/soundVolume2.mp3");
				}
				else if(daVol_def == 1)
				{
					
					//strcpy(path, "/usr/share/soundVolume1.mp3");
					//voice_tips_add_music("/usr/share/soundVolume1.mp3");
				}
				/*else if(daVol_def < 1)
				{
					voice_tips_add_music("/tmp/soundVolume0.mp3");
				}*/
			
				/*if(play_http_tipe.once_play_flag != 1)
				{
					//voice_tips_add_music(path);
				}*/
				setVolume(daVol_def);
			}
			//setVolume(daVol_def);
		}
		else if(tmp_buffer[2]==0x00)
		{
			if(tmp_buffer[2]==0x01)
			{
				SERIAL_DBG("recv uart data correct \n");
			}
			else
			{
				SERIAL_DBG("recv uart data wrong \n");
			}
		}
	}
	else
	{
		flage = 0;
		SERIAL_DBG("wrong head\n");
	}
	
	if(flage == 1)
	{
		SERIAL_DBG("return right h\n");
		memset(ack_ok_buffer,'\0',5);
		ack_ok_buffer[0]=0x55;
		ack_ok_buffer[1]=0xaa;
		ack_ok_buffer[2]=0x00;
		ack_ok_buffer[3]=0x01;
		ack_ok_buffer[4]=0x00;
		//serial_write(ack_ok_buffer, 5);
	}
	else
	{
		SERIAL_DBG("return error h\n");
		memset(ack_ok_buffer,'\0',5);
		ack_ok_buffer[0]=0x55;
		ack_ok_buffer[1]=0xaa;
		ack_ok_buffer[2]=0x00;
		ack_ok_buffer[3]=0x00;
		ack_ok_buffer[4]=0x00;
		//serial_write(ack_ok_buffer, 5);
	}
	
	
}

void  ctrlLegs(void)
{
	uint32_t  currentTime = 0;
	if(g_isStudyMode == 1)
	{
		if(getTickSecond() - g_isStudyTime > 6)
		{
			g_isStudyMode = 0;
			g_startStudyTime = 0;
			//zss++stopPlay(3);
			//zss++stopPlay(4);
		}
	}
	if(g_startStudyTime > 0)
	{
		currentTime = getTickSecond();
		if((currentTime - g_startStudyTime) > 60*5)//5分钟上传一次报告
		{		
			SERIAL_DBG("poseDataStatus---------%s-----------1\n", g_poseData->sendBuf);
			poseDataStatus(g_poseData->detectUnit, g_poseData->manExistNum, g_poseData->manNotExistNum, g_poseData->goodPoseNum, g_poseData->badPoseNum, g_poseData->sendBuf);

			g_startStudyTime = 0;
			g_currentStudyTime = 0;
			g_poseData->len = 0;
		}
	}
}

int initSerialLight(void)
{
    if(g_poseData==NULL)
    {
        g_poseData=(POSEDATASTATUS *)malloc(sizeof(POSEDATASTATUS));
    }
    return 0;
}
