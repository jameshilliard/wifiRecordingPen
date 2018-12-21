#ifndef __WIFI_MANAGE_H_
#define __WIFI_MANAGE_H_

#include "net/wlan/wlan_defs.h"
#include "driver/component/component_def.h"

#define WIFI_MANAGE_INFO 1
#define WIFI_MANAGE_WARN 1

#define WIFI_MANAGE(flags, fmt, arg...)	\
	do {								\
		if (flags) 						\
			printf(fmt, ##arg);		\
	} while (0)

#define WIFI_MANAGE_TRACK_INFO(fmt, arg...)	\
			WIFI_MANAGE(WIFI_MANAGE_INFO, "[WIFI MANAGE I] %20s():%04d "fmt, \
															__func__, __LINE__, ##arg)
#define WIFI_MANAGE_TRACK_WARN(fmt, arg...)	\
			WIFI_MANAGE(WIFI_MANAGE_WARN, "[WIFI MANAGE W] %20s():%04d "fmt, \
															__func__, __LINE__, ##arg)

#define WIFI_MANAGE_THREAD_STACK_SIZE	1024*2

enum    WIFI_RUN_STATUS {
    CMD_WIFI_SET_SSID        = 0, 
    CMD_WIFI_SET_PSK         = 1, 
    CMD_WIFI_ENABLE          = 2, 
    CMD_WIFI_DETECT          = 3, 
    CMD_WIFI_SMARTLINK       = 4,
    CMD_WIFI_NULL            = 0xFF,
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    Component_Status    wifi_task_init();
    void                wifi_task(void *arg);
    wlan_sta_states_t   getWifiState(void);
    void                setWifiRunState(uint8_t runstate);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

