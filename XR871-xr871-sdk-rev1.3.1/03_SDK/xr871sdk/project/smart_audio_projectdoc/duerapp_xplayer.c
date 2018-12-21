
//#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pthread.h"
//#include <ctype.h>
//#include "errno.h"
//#include <sys/select.h>

#include "iniparserapi.h"

#include "duerapp_config.h"
//#include "cdx_config.h"
#include <cdx_log.h>
#include "xplayer.h"
#include "CdxTypes.h"
//#include "memoryAdapter.h"
//#include "deinterlace.h"
//typedef unsigned long uintptr_t ;

#include "driver/chip/hal_codec.h"
#include "audio/manager/audio_manager.h"
#include "duerapp_xplayer.h"
extern SoundCtrl* SoundDeviceCreate();
extern void AwParserInit(void);
extern void AwStreamInit(void);
#include "api_includes.h"



typedef struct DemoPlayerContext
{
    XPlayer*       mAwPlayer;
    int             mPreStatus;
    int             mStatus;
    int             mSeekable;
    int             mError;
//    pthread_mutex_t mMutex;
    OS_Semaphore_t  mSem;
  	int  father_taskid;
   
//    int             mVideoFrameNum;
}DemoPlayerContext;

static DemoPlayerContext *demoPlayer = NULL;

volatile static uint8_t set_source_exit = 0;
uint8_t sys_set_source_exit(uint8_t set)
{
	taskENTER_CRITICAL();
	set_source_exit = set;
	taskEXIT_CRITICAL();
	return 0;
}

uint8_t sys_set_source_exit_get(void)
{
	return set_source_exit;
}
void HAL_WDG_Feed(void);

static int set_source(DemoPlayerContext *demoPlayer, char* pUrl)
{
	u32 cnt = 0;
    demoPlayer->mSeekable = 1;
	int res = -1;
    //* set url to the AwPlayer.
    if(XPlayerSetDataSourceUrl(demoPlayer->mAwPlayer,
                 (const char*)pUrl, NULL, NULL) != 0)
    {
    	DUER_LOGI("[cedar] error:AwPlayer::setDataSource() return fail.\n");
        return DUERAPP_XPLAY_ERR_SETURL;
    }
    DUER_LOGI("[cedar] setDataSource end\n");

    if (!strncmp(pUrl, "http://", 7)) 
    {
        if(XPlayerPrepareAsync(demoPlayer->mAwPlayer) != 0)
        {
        	DUER_LOGI("[cedar] error:AwPlayer::prepareAsync() return fail.\n");
            return DUERAPP_XPLAY_ERR_SETURL;
        }

        while(!sys_set_source_exit_get())
        {
        	HAL_WDG_Feed();
			if (OS_SemaphoreWait(&demoPlayer->mSem, 5) != OS_OK)
			{
				cnt++;
				if(cnt > 200*15)
					break;
				continue;
			}
			else
			{
				res = 0;
				break;
			}
        }
        if(res == -1)
       	 	return DUERAPP_XPLAY_ERR_SETDATASOUCR_BTEAK;
    }

	//* start preparing.
//    pthread_mutex_lock(&demoPlayer->mMutex);
	OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = STATUS_STOPPED;
    demoPlayer->mStatus    = STATUS_PREPARING;
    OS_ThreadResumeScheduler();
//    pthread_mutex_unlock(&demoPlayer->mMutex);
    DUER_LOGI("[cedar] preparing...\n");
    return DUERAPP_XPLAY_NO_ERR;
}

static int play(DemoPlayerContext *demoPlayer)
{
    if(XPlayerStart(demoPlayer->mAwPlayer) != 0)
    {
    	DUER_LOGI("[cedar] error:AwPlayer::start() return fail.\n");
        return DUERAPP_XPLAY_ERR_PALY;
    }
    
	OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = demoPlayer->mStatus;
    demoPlayer->mStatus    = STATUS_PLAYING;
    OS_ThreadResumeScheduler();
    DUER_LOGI("[cedar] playing.\n");
    return DUERAPP_XPLAY_NO_ERR;
}

static int pause(DemoPlayerContext *demoPlayer)
{
    if(XPlayerPause(demoPlayer->mAwPlayer) != 0)
    {
    	DUER_LOGI("[cedar] error:AwPlayer::pause() return fail.\n");
        return DUERAPP_XPLAY_ERR_PAUSE;
    }
    
	OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = demoPlayer->mStatus;
    demoPlayer->mStatus    = STATUS_PAUSED;
    OS_ThreadResumeScheduler();
    DUER_LOGI("[cedar] paused.\n");
    return DUERAPP_XPLAY_NO_ERR;
}
static int stop(DemoPlayerContext *demoPlayer)
{
    if(XPlayerReset(demoPlayer->mAwPlayer) != 0)
    {
    	DUER_LOGI("[cedar] error:AwPlayer::reset() return fail.\n");
        return DUERAPP_XPLAY_ERR_STOP;
    }
    
	OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = demoPlayer->mStatus;
    demoPlayer->mStatus    = STATUS_STOPPED;
    OS_ThreadResumeScheduler();
    DUER_LOGI("[cedar] stopped.\n");

    return DUERAPP_XPLAY_NO_ERR;
}
#include "api_includes.h"
//* a callback for awplayer.
static int CallbackForAwPlayer(void* pUserData, int msg, int ext1, void* param)
{
    DemoPlayerContext* pDemoPlayer = (DemoPlayerContext*)pUserData;
	YUHENG_DBG("CallbackForAwPlayer:%x\n",msg);
    switch(msg)
    {
        case AWPLAYER_MEDIA_INFO:
        {
            switch(ext1)
            {
                case AW_MEDIA_INFO_NOT_SEEKABLE:
                {
					OS_ThreadSuspendScheduler();
                    pDemoPlayer->mSeekable = 0;
					OS_ThreadResumeScheduler();
                    DUER_LOGI("[cedar] info: media source is unseekable.\n");
                    break;
                }
                case AW_MEDIA_INFO_RENDERING_START:
                {
                	DUER_LOGI("[cedar] info: start to show pictures.\n");
                    break;
                }
                default:
                {
                	DUER_LOGI("[cedar] info: unknown cedar ext1 state %d.\n", ext1);
                    break;
                }
            }
            break;
        }

        case AWPLAYER_MEDIA_ERROR:
        {
        	DUER_LOGI("[cedar] error: open media source fail.\n");
            DUER_LOGI("[cedar] error : how to deal with it\n");
//            pthread_mutex_lock(&pDemoPlayer->mMutex);
			OS_ThreadSuspendScheduler();
            pDemoPlayer->mStatus = STATUS_STOPPED;
            pDemoPlayer->mPreStatus = STATUS_STOPPED;
            pDemoPlayer->mError = 1;
			OS_ThreadResumeScheduler();
//            pthread_mutex_unlock(&pDemoPlayer->mMutex);
            OS_SemaphoreRelease(&pDemoPlayer->mSem);
            if(pDemoPlayer->father_taskid < TASK_ID_MAX)
            	os_taskq_post_msg(pDemoPlayer->father_taskid,1,SYS_EVENT_DEC_DEVICE_ERR);
            break;
        }

        case AWPLAYER_MEDIA_PREPARED:
        {
        	DUER_LOGI("[cedar] info : preared\n");
        	
			OS_ThreadSuspendScheduler();
            pDemoPlayer->mPreStatus = pDemoPlayer->mStatus;
            pDemoPlayer->mStatus = STATUS_PREPARED;
			OS_ThreadResumeScheduler();
            DUER_LOGI("[cedar] info: prepare ok.\n");
            OS_SemaphoreRelease(&pDemoPlayer->mSem);
            break;
        }

        case AWPLAYER_MEDIA_PLAYBACK_COMPLETE:
        {
        	DUER_LOGI("[cedar] info : complete\n");
        	
			OS_ThreadSuspendScheduler();
            pDemoPlayer->mStatus = STATUS_STOPPED;
            pDemoPlayer->mPreStatus = STATUS_STOPPED;
			OS_ThreadResumeScheduler();
            OS_SemaphoreRelease(&pDemoPlayer->mSem);//* stop the player.
            if(pDemoPlayer->father_taskid < TASK_ID_MAX)
            	os_taskq_post_msg(pDemoPlayer->father_taskid,1,SYS_EVENT_DEC_END);
        
            //* TODO
            break;
        }

        case AWPLAYER_MEDIA_SEEK_COMPLETE:
        {
        	YUHENG_DBG(" pDemoPlayer->mStatus:%d, pDemoPlayer->mStatus:%d\n", pDemoPlayer->mStatus, pDemoPlayer->mPreStatus);
            YUHENG_DBG("[cedar] info: seek complete.\n");
            break;
        }
		case AWPLAYER_MEDIA_STOPPED:
			YUHENG_DBG("[cedar] AWPLAYER_MEDIA_STOPPED %d.\n", msg);
			break;
        default:
        {
        	YUHENG_ERR("[cedar] info: unknown cedar state %d.\n", msg);
            break;
        }
    }

    return 0;
}
static uint8_t xplayer_inited = 0;

int duer_cedarx_sethttp_buffer_size(void)
{
	if(demoPlayer == NULL)
	{
		return DUERAPP_XPLAY_ERR_SETHTTPBUFFER;
	}
	XPlayerSetHttpBuffer(demoPlayer->mAwPlayer,HTTP_CEDARX_BUFFER_SIZE,HTTP_CEDARX_BUFFER_THRESHOLD);
	return DUERAPP_XPLAY_NO_ERR;
}

const XPlayerBufferConfig bufcfg_HITH __xip_rodata = 
{
	.maxBitStreamBufferSize = 6 * 1024,
	.maxBitStreamFrameCount = 6,
	.maxPcmBufferSize = 24 * 1024,
	.maxStreamBufferSize = 14 * 1024,
	.maxStreamFrameCount = 18,
};

const XPlayerBufferConfig bufcfg_LOW __xip_rodata = 
{
	.maxStreamBufferSize = 6*1024,
	.maxStreamFrameCount = 12,
	.maxBitStreamBufferSize = 1024 * 6,
	.maxBitStreamFrameCount = 6,
	.maxPcmBufferSize = 16 * 1024,
};

int duer_cedarx_create_exec(int id)
{
	if( demoPlayer != NULL )
	{
        return DUERAPP_XPLAY_NO_ERR;
	}

    demoPlayer = malloc(sizeof(*demoPlayer));
    if(demoPlayer == NULL)
    	return DUERAPP_XPLAY_ERR_CREATE_FAIL;

	if(!xplayer_inited)
	{
		AwParserInit();
		AwStreamInit();
		xplayer_inited = 1;
	}
  
    //* create a player.
    memset(demoPlayer, 0, sizeof(DemoPlayerContext));
    demoPlayer->father_taskid = id;
//    pthread_mutex_init(&demoPlayer->mMutex, NULL);
    OS_SemaphoreCreateBinary(&demoPlayer->mSem);

    demoPlayer->mAwPlayer = XPlayerCreate();
    if(demoPlayer->mAwPlayer == NULL)
    {
    	DUER_LOGI("[cedar] can not create AwPlayer, quit.\n");
        return DUERAPP_XPLAY_ERR_CREATE_FAIL;
    } else {
    	DUER_LOGI("[cedar] create AwPlayer success.\n");
    }

    //* set callback to player.
    XPlayerSetNotifyCallback(demoPlayer->mAwPlayer, CallbackForAwPlayer, (void*)demoPlayer);

    //* check if the player work.
    if(XPlayerInitCheck(demoPlayer->mAwPlayer) != 0)
    {
    	DUER_LOGI("[cedar] initCheck of the player fail, quit.\n");
        XPlayerDestroy(demoPlayer->mAwPlayer);
        demoPlayer->mAwPlayer = NULL;
        return DUERAPP_XPLAY_ERR_CREATE_FAIL;
    } else
    	DUER_LOGI("[cedar] AwPlayer check success.\n");
	//set buffer size
	//if(compare_task_id(TASK_ID_NETMUSIC_TASK) || compare_task_id(TASK_ID_NETVOICE_TASK) )
	//	XPlayerSetBuffer(demoPlayer->mAwPlayer, &bufcfg_LOW);
	//else
		XPlayerSetBuffer(demoPlayer->mAwPlayer, &bufcfg_HITH);
    SoundCtrl* sound = SoundDeviceCreate();

    XPlayerSetAudioSink(demoPlayer->mAwPlayer, (void*)sound);

	return DUERAPP_XPLAY_NO_ERR;
}

int duer_cedarx_destroy_exec(void)
{
	if( demoPlayer == NULL )
	{
        return DUERAPP_XPLAY_ERR_DESTORY;
	}

	DUER_LOGI("[cedar] destroy AwPlayer.\n");

    if(demoPlayer->mAwPlayer != NULL)
    {
        XPlayerDestroy(demoPlayer->mAwPlayer);
        demoPlayer->mAwPlayer = NULL;
    }

   
//    pthread_mutex_destroy(&demoPlayer->mMutex);

    OS_SemaphoreDelete(&demoPlayer->mSem);

	free(demoPlayer);
	demoPlayer = NULL;
	DUER_LOGI("[cedar] destroy AwPlayer 1.\n");
    return DUERAPP_XPLAY_NO_ERR;
}

int duer_cedarx_play_exec(void)
{
	if( demoPlayer == NULL )
	{
        return DUERAPP_XPLAY_ERR_PALY;
	}

	if (demoPlayer->mError == 0)
	{
		return play(demoPlayer);
	}
	else
		return DUERAPP_XPLAY_ERR_PALY;
}


int duer_cedarx_stop_exec(void)
{
	if( demoPlayer == NULL )
	{
        return DUERAPP_XPLAY_ERR_STOP;
	}
	if(stop(demoPlayer) != 0)
		return DUERAPP_XPLAY_ERR_STOP;
	OS_MSleep(5);
	return DUERAPP_XPLAY_NO_ERR;
}

int duer_cedarx_pause_exec(void)
{
	if( demoPlayer == NULL )
	{
        return DUERAPP_XPLAY_ERR_PAUSE;
	}
	return pause(demoPlayer);
}

int duer_cedarx_seturl_exec(char *url)
{
	if( demoPlayer == NULL )
	{
        return DUERAPP_XPLAY_ERR_SETURL;
	}

	return set_source(demoPlayer, url);
	
}

int duer_cedarx_getpos_exec(void)
{
	int msec = 0;

	if( demoPlayer == NULL )
	{
        return 0;
	}

	if(XPlayerGetCurrentPosition(demoPlayer->mAwPlayer, &msec) != 0)
		return 0;
	else 
		return msec;
}

int duer_cedarx_gettotalpos_exec(void)
{
	int msec = 0;

	if( demoPlayer == NULL )
	{
        return 0;
	}

	if(XPlayerGetDuration(demoPlayer->mAwPlayer, &msec) !=0)
		return 0;
	else
		return msec;
}



int duer_cedarx_getstatus_exec(void)
{
	if( demoPlayer == NULL )
	{
        return STATUS_STOPPED;
	}

	return demoPlayer->mStatus;
}

int duer_cedarx_seek_exec(int nSeekTimeMs)
{
	if( demoPlayer == NULL )
	{
        return DUERAPP_XPLAY_ERR_SEEEK;
	}

	if( demoPlayer->mSeekable == 1 )
	{
		return XPlayerSeekTo(demoPlayer->mAwPlayer, nSeekTimeMs);
	}
	else
	{
		return DUERAPP_XPLAY_ERR_SEEEK;
	}
}


int duer_cedarx_setvol_exec(int vol)
{
	if( vol < 0 ) vol = 0;
	if (vol > 31) vol = 31;

	aud_mgr_handler(AUDIO_DEVICE_MANAGER_VOLUME, vol);
	s_vol = vol;
	return 0;
}

int duer_cedarx_getvol_exec(void)
{
	return s_vol;
}




