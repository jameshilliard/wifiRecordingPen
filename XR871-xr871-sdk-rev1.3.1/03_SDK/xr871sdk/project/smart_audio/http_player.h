#ifndef __HTTP_PLAYER_H_
#define __HTTP_PLAYER_H_

#include "driver/component/component_def.h"

#define HTTP_PLAYER_INFO 1
#define HTTP_PLAYER_WARN 1

#define HTTP_PLAYER(flags, fmt, arg...)	\
	do {								\
		if (flags) 						\
			printf(fmt, ##arg);		\
	} while (0)

#define HTTP_PLAYER_TRACK_INFO(fmt, arg...)	\
			HTTP_PLAYER(HTTP_PLAYER_INFO, "[HTP PLAYER I] %20s():%04d "fmt, \
															__func__, __LINE__, ##arg)
#define HTTP_PLAYER_TRACK_WARN(fmt, arg...)	\
			HTTP_PLAYER(HTTP_PLAYER_WARN, "[HTP PLAYER W] %20s():%04d "fmt, \
															__func__, __LINE__, ##arg)

#define HTTP_PLAYER_THREAD_STACK_SIZE	1024*2

#if 0
#define FIRST_RESET 	"/usr/share/warinningSound/first/sit1.mp3"
#define SECOND_RESET 	"/usr/share/warinningSound/second/sit2.mp3"
#define THIRD_RESET 	"/usr/share/warinningSound/third/sit4.mp3"
#define RESET 	        "/usr/share/warinningSound/reset/sit5.mp3"
#define CLOSELEGWARN 	"/usr/share/closeLegWarn.mp3"
#define RSET45 	        "/usr/share/rset45.mp3"
#endif
#define FIRST_RESET 	0
#define SECOND_RESET 	1
#define THIRD_RESET 	2
#define RESET 	        3
#define CLOSELEGWARN 	4
#define RSET45 	        5
#define AFRESH_NET      6
#define CONN_SUCCESS    7
#define PLEASE_CONN     8
#define RECOVER_DEV     9
#define BEGIN_STUDY     10

#define MF_TEST110      11
#define MF_TEST111      12
#define MF_TEST11       13
#define MF_TEST12       14
#define MF_TEST13       15
#define MF_TEST14       16
#define MF_TEST15       17
#define MF_TEST16       18
#define MF_TEST17       19
#define MF_TEST18       20
#define MF_TEST19       21

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
    Component_Status http_player_task_init();
    void http_player_task(void *arg);
    int  addHttpStr(char *httpStr);
    int  analysisHttpStr(char *httpStr);
    int  stopHttpAudioPlay(int stopCode);
    int  voice_tips_add_music(int type,uint8_t nowFlag);
    int  setVolume(uint8_t volume);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


