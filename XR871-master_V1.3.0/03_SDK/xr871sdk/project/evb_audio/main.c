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

#include "common/framework/platform_init.h"
#include "audio_player.h"
#include "command.h"
#include "wifi_manage.h"
#include "rtp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_player.h"

void initGpio()
{
    //PB13:audio¿ØÖÆÒý½Å
    //mem w32 0x40050028 0x00100000
    //mem w32 0x40050034 0x00002000
    //AD:PA11 CH3
    //mem r32 0x40050004 4
    //mem w32 0x40050004 0x44440000
    char cmdStr[512]={0};
    sprintf(cmdStr,"mem w32 0x40050028 0x00100000");
    main_cmd_exec(cmdStr);
    sprintf(cmdStr,"mem w32 0x40050034 0x00002000");
    main_cmd_exec(cmdStr);
    sprintf(cmdStr,"mem w32 0x40050004 0x44440000");
    main_cmd_exec(cmdStr);
}

int main(void)
{
	platform_init();
	//gpio_button_ctrl_init();
    //player_task_init();
    //main_cmd_exec("net sta enable");
    //player_task_init();
    initGpio();
	ad_button_init();
	wifi_task_init();
	rtp_server_task_init();
	http_player_task_init();
	
	return 0;
}
