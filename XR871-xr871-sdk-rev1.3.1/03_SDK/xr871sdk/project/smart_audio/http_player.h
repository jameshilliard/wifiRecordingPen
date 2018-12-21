#ifndef __HTTP_PLAYER_H_
#define __HTTP_PLAYER_H_

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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
    Component_Status http_player_task_init();
    void http_player_task(void *arg);
    int  analysisHttpStr(char *httpStr);
    int  stopHttpAudioPlay(int stopCode);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


