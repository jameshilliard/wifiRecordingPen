#ifndef __UDP_SERVER_H_
#define __UDP_SERVER_H_

#include "driver/component/component_def.h"

#define UDP_SERVER_INFO 1
#define UDP_SERVER_WARN 1



#define UDP_SERVER(flags, fmt, arg...)	\
	do {								\
		if (flags) 						\
			printf(fmt, ##arg);		\
	} while (0)

#define UDP_SERVER_TRACK_INFO(fmt, arg...)	\
			UDP_SERVER(UDP_SERVER_INFO, "[UDP SERVER I] %20s():%04d "fmt, \
															__func__, __LINE__, ##arg)
#define UDP_SERVER_TRACK_WARN(fmt, arg...)	\
			UDP_SERVER(UDP_SERVER_WARN, "[UDP SERVER W] %20s():%04d "fmt, \
															__func__, __LINE__, ##arg)

#define UDP_SERVER_THREAD_STACK_SIZE	1024*2


enum UDP_SERVER_RUN_STATUS {
    CMD_UDP_SERVER_WAIT            = 0, 
    CMD_UDP_SERVER_START           = 1, 
    CMD_UDP_SERVER_SENDMESS        = 2, 
    CMD_UDP_SERVER_RECVMESS        = 3, 
    CMD_UDP_SERVER_OVER            = 4, 
    CMD_UDP_SERVER_NULL            = 0xFF,
};


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    Component_Status udp_server_task_init();
    void udp_server_task(void *arg);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif



