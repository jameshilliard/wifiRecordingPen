/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef __PRJ_CONFIG_XPLAYER

#include "cmd_util.h"

//#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include "pthread.h"
#include <cdx_log.h>
#include "xplayer.h"
#include "cmd_cedarx.h"
#include "fs/fatfs/ff.h"
#include "common/framework/fs_ctrl.h"
#include "tcp_client.h"

#define DEBUG_MNT_SDCARD 0

extern SoundCtrl* SoundDeviceCreate();

typedef struct DemoPlayerContext
{
    XPlayer*        mAwPlayer;
    int             mSeekable;
    int             mError;
    int             mPreStatus;
    int             mStatus;
    OS_Semaphore_t  mSem;
}DemoPlayerContext;


static DemoPlayerContext *demoPlayer = NULL;
volatile static uint8_t   set_source_exit = 0;


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

uint8_t sys_get_status_exec(void)
{
	if( demoPlayer == NULL )
	{
        return STATUS_STOPPED;
	}

	return demoPlayer->mStatus;
}

void HAL_WDG_Feed(void);
static int set_source(DemoPlayerContext *demoPlayer, char* pUrl)
{
    uint32_t cnt = 0;
    demoPlayer->mSeekable = 1;
    int res = -1;
    //* set url to the AwPlayer.
    if(XPlayerSetDataSourceUrl(demoPlayer->mAwPlayer,
                 (const char*)pUrl, NULL, NULL) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::setDataSource() return fail.\n");
        return res;
    }
    printf("setDataSource end\n");

    if ((!strncmp(pUrl, "http://", 7)) || (!strncmp(pUrl, "https://", 8))) {
        if(XPlayerPrepareAsync(demoPlayer->mAwPlayer) != 0)
        {
            printf("error:\n");
            printf("    AwPlayer::prepareAsync() return fail.\n");
            return res;
        }
        while(!sys_set_source_exit_get())
        {
        	HAL_WDG_Feed();
			if (OS_SemaphoreWait(&demoPlayer->mSem, 100) != OS_OK)
			{
				cnt++;
				if(cnt > 150)
					break;
				continue;
			}
			else
			{
				res = 0;
				break;
			}
        }
        //sem_wait(&demoPlayer->mPrepared);
    }
    OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = STATUS_STOPPED;
    demoPlayer->mStatus    = STATUS_PREPARING;
    OS_ThreadResumeScheduler();
    printf("preparing...cnt=%d\n",cnt);
    return res;
}

static int play(DemoPlayerContext *demoPlayer)
{
    if(XPlayerStart(demoPlayer->mAwPlayer) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::start() return fail.\n");
        return -1;
    }
    OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = demoPlayer->mStatus;
    demoPlayer->mStatus    = STATUS_PLAYING;
    OS_ThreadResumeScheduler();
    printf("playing.\n");
    return 0;
}

static void pause(DemoPlayerContext *demoPlayer)
{
    if(XPlayerPause(demoPlayer->mAwPlayer) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::pause() return fail.\n");
        return;
    }
    OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = demoPlayer->mStatus;
    demoPlayer->mStatus    = STATUS_PAUSED;
    OS_ThreadResumeScheduler();
    printf("paused.\n");
}

static void stop(DemoPlayerContext *demoPlayer)
{
    printf("stop start.\n");
    if(XPlayerReset(demoPlayer->mAwPlayer) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::reset() return fail.\n");
        return;
    }
    OS_ThreadSuspendScheduler();
    demoPlayer->mPreStatus = demoPlayer->mStatus;
    demoPlayer->mStatus    = STATUS_STOPPED;
    OS_ThreadResumeScheduler();
    printf("stopped.\n");
}


//* a callback for awplayer.
static char *url = NULL;
static int CallbackForAwPlayer(void* pUserData, int msg, int ext1, void* param)
{
    DemoPlayerContext* pDemoPlayer = (DemoPlayerContext*)pUserData;

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
                    printf("info: media source is unseekable.\n");
                    break;
                }
                case AW_MEDIA_INFO_RENDERING_START:
                {
                    printf("info: start to show pictures.\n");
                    break;
                }
            }
            break;
        }

        case AWPLAYER_MEDIA_ERROR:
        {
            printf("error: open media source fail.\n");
            //sem_post(&pDemoPlayer->mPrepared);
            pDemoPlayer->mError = 1;
            OS_ThreadSuspendScheduler();
            pDemoPlayer->mStatus = STATUS_STOPPED;
            pDemoPlayer->mPreStatus = STATUS_STOPPED;
            pDemoPlayer->mError = 1;
			OS_ThreadResumeScheduler();
            //pthread_mutex_unlock(&pDemoPlayer->mMutex);
            OS_SemaphoreRelease(&pDemoPlayer->mSem);
            loge(" error : how to deal with it");
            break;
        }

        case AWPLAYER_MEDIA_PREPARED:
        {
            logd("info : preared");
            //sem_post(&pDemoPlayer->mPrepared);
            printf("info: prepare ok.\n");
            OS_ThreadSuspendScheduler();
            pDemoPlayer->mPreStatus = pDemoPlayer->mStatus;
            pDemoPlayer->mStatus = STATUS_PREPARED;
			OS_ThreadResumeScheduler();
            OS_SemaphoreRelease(&pDemoPlayer->mSem);
            break;
        }

        case AWPLAYER_MEDIA_PLAYBACK_COMPLETE:
        {
            //* TODO
            OS_ThreadSuspendScheduler();
            pDemoPlayer->mStatus = STATUS_STOPPED;
            pDemoPlayer->mPreStatus = STATUS_STOPPED;
			OS_ThreadResumeScheduler();
            OS_SemaphoreRelease(&pDemoPlayer->mSem);//* stop the player.
            break;
        }

        case AWPLAYER_MEDIA_SEEK_COMPLETE:
        {
            printf("info: seek ok.\n");
            break;
        }

        case AWPLAYER_MEDIA_CHANGE_URL:
        {
            url = strdup(param);
            if (!url)
                loge("malloc url error\n");
            break;
        }
		case AWPLAYER_MEDIA_STOPPED:
			printf("[cedar] AWPLAYER_MEDIA_STOPPED %d.\n", msg);
			break;
        default:
        {
            printf("warning: unknown callback from AwPlayer.\n");
            break;
        }
    }

    return 0;
}

#if DEBUG_MNT_SDCARD 
static uint8_t cedarx_inited = 0;
#endif

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


static enum cmd_status cmd_cedarx_create_exec(char *cmd)
{
    demoPlayer = malloc(sizeof(*demoPlayer));
    #if DEBUG_MNT_SDCARD 
	if (cedarx_inited++ == 0) {
		if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
			printf("mount fail\n");
			return -1;
		} else {
			printf("mount success\n");
		}
	}
    #endif
    //* create a player.
    memset(demoPlayer, 0, sizeof(DemoPlayerContext));
    //sem_init(&demoPlayer->mPrepared, 0, 0);
    OS_SemaphoreCreateBinary(&demoPlayer->mSem);
    
    demoPlayer->mAwPlayer = XPlayerCreate();
    if(demoPlayer->mAwPlayer == NULL)
    {
        printf("can not create AwPlayer, quit.\n");
        return -1;
    } else {
        printf("create AwPlayer success.\n");
    }

    //* set callback to player.
    XPlayerSetNotifyCallback(demoPlayer->mAwPlayer, CallbackForAwPlayer, (void*)demoPlayer);

    //* check if the player work.
    if(XPlayerInitCheck(demoPlayer->mAwPlayer) != 0)
    {
        printf("initCheck of the player fail, quit.\n");
        XPlayerDestroy(demoPlayer->mAwPlayer);
        demoPlayer->mAwPlayer = NULL;
        return -1;
    } else
        printf("AwPlayer check success.\n");
	//set buffer size
	//if(compare_task_id(TASK_ID_NETMUSIC_TASK) || compare_task_id(TASK_ID_NETVOICE_TASK) )
	//	XPlayerSetBuffer(demoPlayer->mAwPlayer, &bufcfg_LOW);
	//else
    //	XPlayerSetBuffer(demoPlayer->mAwPlayer, &bufcfg_HITH);

    SoundCtrl* sound = SoundDeviceCreate();
    XPlayerSetAudioSink(demoPlayer->mAwPlayer, (void*)sound);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_destroy_exec(char *cmd)
{ 
    if(demoPlayer==NULL)
        return CMD_STATUS_OK;
        
    if(demoPlayer->mAwPlayer != NULL)
    {
        XPlayerDestroy(demoPlayer->mAwPlayer);
        demoPlayer->mAwPlayer = NULL;
    }
    printf("destroy AwPlayer.\n");

    //sem_destroy(&demoPlayer->mPrepared);
    #if DEBUG_MNT_SDCARD 
	if (--cedarx_inited == 0) {
		if (fs_ctrl_unmount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
			printf("unmount fail\n");
		}
	}
	#endif
	if(demoPlayer)
	    free(demoPlayer);

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_play_exec(char *cmd)
{
    if(demoPlayer==NULL)
        return CMD_STATUS_FAIL;
    int iRet=play(demoPlayer);
	return (iRet==0)?CMD_STATUS_OK:CMD_STATUS_FAIL;
}

static enum cmd_status cmd_cedarx_stop_exec(char *cmd)
{
    if(demoPlayer==NULL)
        return CMD_STATUS_FAIL;
    stop(demoPlayer);
    msleep(5);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_pause_exec(char *cmd)
{
    if(demoPlayer==NULL)
        return CMD_STATUS_FAIL;
    pause(demoPlayer);
	return CMD_STATUS_OK;
}

enum cmd_status cmd_cedarx_seturl_exec(char *cmd)
{
    if(demoPlayer==NULL)
        return CMD_STATUS_FAIL;
    int iRet=set_source(demoPlayer, cmd);
    if (url) {
        stop(demoPlayer);
        iRet=set_source(demoPlayer, url);
        free(url);
        url = NULL;
    }
    return (iRet==0)?CMD_STATUS_OK:CMD_STATUS_FAIL;
}

static enum cmd_status cmd_cedarx_getpos_exec(char *cmd)
{
	int msec;
    if(demoPlayer==NULL)
        return CMD_STATUS_FAIL;
	XPlayerGetCurrentPosition(demoPlayer->mAwPlayer, &msec);
	cmd_write_respond(CMD_STATUS_OK, "played time: %d ms", msec);
	return CMD_STATUS_ACKED;
}

static enum cmd_status cmd_cedarx_seek_exec(char *cmd)
{
	int nSeekTimeMs = atoi(cmd);
    if(demoPlayer==NULL)
        return CMD_STATUS_FAIL;
	XPlayerSeekTo(demoPlayer->mAwPlayer, nSeekTimeMs);
	return CMD_STATUS_OK;
}

#include "driver/chip/hal_codec.h"
#include "audio/manager/audio_manager.h"

static enum cmd_status cmd_cedarx_setvol_exec(char *cmd)
{
	int vol = atoi(cmd);
	if (vol > 31)
		vol = 31;

	aud_mgr_handler(AUDIO_DEVICE_MANAGER_VOLUME, AUDIO_OUT_DEV_SPEAKER, vol);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_playdic_exec(char *cmd)
{
	return CMD_STATUS_OK;
}


//#include <CdxQueue.h>
//#include <AwPool.h>
//#include <CdxMuxer.h>
//#include "awencoder.h"
//#include "RecoderWriter.h"
#include "xrecord.h"
#include "CaptureControl.h"

static XRecord *xrecord;

static enum cmd_status cmd_cedarx_rec_exec(char *cmd)
{
	XRECODER_AUDIO_ENCODE_TYPE type;
	if (strstr(cmd, ".amr"))
		type = XRECODER_AUDIO_ENCODE_AMR_TYPE;
	else if (strstr(cmd, ".pcm"))
		type = XRECODER_AUDIO_ENCODE_PCM_TYPE;
	else {
		printf("do not support this encode type\n");
		return CMD_STATUS_INVALID_ARG;
	}
    #if DEBUG_MNT_SDCARD 
	if (cedarx_inited++ == 0) {
		if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
			printf("mount fail\n");
			return -1;
		}
	}
    #endif
	xrecord = XRecordCreate();
	if (xrecord == NULL)
		printf("create success\n");

	CaptureCtrl* cap = CaptureDeviceCreate();
	if (!cap)
		printf("cap device create failed\n");
	XRecordSetAudioCap(xrecord, cap);

	XRecordConfig audioConfig;
	audioConfig.nChan = 1;
	audioConfig.nSamplerate = 8000;//ÐÞ¸Ä³É16000
	audioConfig.nSamplerBits = 16;
	audioConfig.nBitrate = 12200;

	XRecordSetDataDstUrl(xrecord, cmd, NULL, NULL);
	XRecordSetAudioEncodeType(xrecord, type, &audioConfig);

	XRecordPrepare(xrecord);
	XRecordStart(xrecord);
	printf("record start\n");


	return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_end_exec(char *cmd)
{
    XRecordStop(xrecord);
	printf("record stop\n");
	XRecordDestroy(xrecord);
	printf("record destroy\n");
    #if DEBUG_MNT_SDCARD 
	if (--cedarx_inited == 0) {
		if (fs_ctrl_unmount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
			printf("unmount fail\n");
		}
	}
    #endif
	return CMD_STATUS_OK;
}


int echocloud_cedarx_rec_create_exec(char *cmd, void *arg)
{
    XRECODER_AUDIO_ENCODE_TYPE type;
    if (strstr(cmd, ".amr"))
        type = XRECODER_AUDIO_ENCODE_AMR_TYPE;
    else if (strstr(cmd, ".pcm"))
        type = XRECODER_AUDIO_ENCODE_PCM_TYPE;
    else
    {
        type = XRECODER_AUDIO_ENCODE_PCM_TYPE;
        printf("do not support this encode type,default pcm\n");
    }

    if(xrecord != NULL)
    {
        XRecordStop(xrecord);
        printf("record stop\n");
        XRecordDestroy(xrecord);
        printf("record destroy\n");
        xrecord = NULL;
    }

    xrecord = XRecordCreate();
    if (xrecord == NULL)
    {
        printf("create fail\n");
        return -1;
    }

    CaptureCtrl *cap = CaptureDeviceCreate();
    if (!cap)
    {
        printf("cap device create failed\n");
    }
    
    XRecordSetAudioCap(xrecord, cap);
	XRecordConfig audioConfig;
    audioConfig.nBitrate = 12200;
    audioConfig.nChan = 1;
    audioConfig.nSamplerate = AUDIOSAMPLERATE;
    audioConfig.nSamplerBits = 16;
    XRecordSetDataDstUrl(xrecord, cmd, arg, NULL);
    XRecordSetAudioEncodeType(xrecord, type, &audioConfig);

    XRecordPrepare(xrecord);
    XRecordStart(xrecord);
    printf("record start\n");

    return 0;
}

int echocloud_cedarx_rec_destory_exec(void)
{
    if(xrecord)
    {
        XRecordStop(xrecord);
        printf("record stop\n");
        XRecordDestroy(xrecord);
        printf("record destroy\n");
        xrecord = NULL;
    }
    return 0;
}

static uint32_t httpc_record_callback(void*buf,uint32_t size)
{
    pushPcmAudioData((char *)buf,size,2,1);
	return size;
}

static int httpc_record_stat = 0;
static enum cmd_status httpc_record_start_exec(char *cmd)
{
	if(echocloud_cedarx_rec_create_exec("callback://",httpc_record_callback) == 0)//
	{
		printf("REC START succ\n");
		httpc_record_stat = 1;
		return CMD_STATUS_OK;
	}
	else
	{
		printf("REC START fail\n");
		return CMD_STATUS_FAIL;
	}
}

static enum cmd_status httpc_record_stop_exec(char *cmd)
{
	echocloud_cedarx_rec_destory_exec();
	OS_MSleep(5);
	httpc_record_stat = 0;
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_log_exec(char *cmd)
{
	int level = atoi(cmd);
	if (level > 6)
		level = 6;

	void log_set_level(unsigned level);
	log_set_level(level);
	return CMD_STATUS_OK;

}

static enum cmd_status cmd_cedarx_version_exec(char *cmd)
{
    XPlayerShowVersion();
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_showbuf_exec(char *cmd)
{
    XPlayerShowBuffer();
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_setbuf_exec(char *cmd)
{
    int argc;
    char *argv[4];
    XPlayerBufferConfig cfg;
    int maxAudioFrameSize;
    argc = cmd_parse_argv(cmd, argv, 4);
    if (argc < 4) {
        CMD_ERR("invalid cedarx set buf cmd, argc %d\n", argc);
        goto exit;
    }

    cfg.maxStreamBufferSize = cmd_atoi(argv[0]);
    cfg.maxBitStreamBufferSize = cmd_atoi(argv[1]);
    cfg.maxPcmBufferSize = cmd_atoi(argv[2]);
    cfg.maxStreamFrameCount = 0;
    cfg.maxBitStreamFrameCount = 0;
    maxAudioFrameSize = cmd_atoi(argv[3]);

    XPlayerSetBuffer(demoPlayer->mAwPlayer, &cfg);
    XPlayerSetPcmFrameSize(demoPlayer->mAwPlayer, maxAudioFrameSize);
    XPlayerShowBuffer();
exit:
    return CMD_STATUS_OK;
}

extern void CdxBufStatStart(void);
static enum cmd_status cmd_cedarx_bufinfo_exec(char *cmd)
{
    CdxBufStatStart();
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_aacsbr_exec(char *cmd)
{
    int use_sbr = atoi(cmd);

    if (demoPlayer->mAwPlayer)
    {
        XPlayerSetAacSbr(demoPlayer->mAwPlayer, use_sbr);
        printf("set aac sbr success\n");
    }
    else
    {
        printf("set aac sbr fail\n");
    }
    return CMD_STATUS_OK;
}

/*
 * brief cedarx Test Command
 *          cedarx play
 *          cedarx stop
 *          cedarx create
 *          cedarx seturl file://wechat.pcm
 *          cedarx getpos
 *          cedarx seek 6000
 *          cedarx setvol 8
 *          cedarx playdic file://music
 *          cedarx rec file://wechat.pcm
 *          cedarx end
            cedarx showbuf
 */
static const struct cmd_data g_cedarx_cmds[] = {
    { "create",     cmd_cedarx_create_exec      },
    { "destroy",    cmd_cedarx_destroy_exec     },
    { "play",       cmd_cedarx_play_exec        },
    { "stop",       cmd_cedarx_stop_exec        },
    { "pause",      cmd_cedarx_pause_exec       },
    { "seturl",     cmd_cedarx_seturl_exec      },
    { "getpos",     cmd_cedarx_getpos_exec      },
    { "seek",      	cmd_cedarx_seek_exec        },
    { "setvol",     cmd_cedarx_setvol_exec      },
    { "playdic",    cmd_cedarx_playdic_exec     },
    { "log",        cmd_cedarx_log_exec         },
    { "version",    cmd_cedarx_version_exec     },
    { "showbuf",    cmd_cedarx_showbuf_exec     },
    { "setbuf",     cmd_cedarx_setbuf_exec      },
    { "bufinfo",    cmd_cedarx_bufinfo_exec     },
    { "aacsbr",     cmd_cedarx_aacsbr_exec      },
    //{ "rec",        cmd_cedarx_rec_exec         },
    //{ "end",        cmd_cedarx_end_exec         },
    { "rec",        httpc_record_start_exec     },
    { "end",        httpc_record_stop_exec      },
};

enum cmd_status cmd_cedarx_exec(char *cmd)
{
    return cmd_exec(cmd, g_cedarx_cmds, cmd_nitems(g_cedarx_cmds));
}

#endif /* __PRJ_CONFIG_XPLAYER */
