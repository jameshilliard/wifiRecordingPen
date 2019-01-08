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
#include "command.h"
#include "wifi_manage.h"
#include "rtp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http_player.h"
#include "console/console.h"
#include "ad_button_task.h"
#include "tcp_client.h"
#include "kernel/os/os.h"
#include "driver/chip/hal_gpio.h"
#include "serial.h"


#define SAMRT_DEBUG 1

#define LEDX_PORT           GPIO_PORT_A
#define LED0_PIN            GPIO_PIN_6
#define LED1_PIN            GPIO_PIN_19
//#define LED2_PIN          GPIO_PIN_17


#define MIC_DETECT_PORT     GPIO_PORT_B
#define MIC_DETECT_PIN      GPIO_PIN_15


void initGpio()
{
    //PB13:audio¿ØÖÆÒý½Å
    //mem w32 0x40050028 0x00100000
    //mem w32 0x40050034 0x00002000
    //AD:PA11 CH3
    //mem r32 0x40050004 4
    //mem w32 0x40050004 0x44440000
    //console_cmd("mem w32 0x40050028 0x00100000");
    //console_cmd("mem w32 0x40050034 0x00002000");
    //console_cmd("mem w32 0x40050004 0x44440000");
    /*PA0 output*/
	GPIO_InitParam param;
	/*set pin driver capability*/
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F1_OUTPUT;
	param.pull = GPIO_PULL_NONE;
	HAL_GPIO_Init(LEDX_PORT, LED0_PIN, &param);
	HAL_GPIO_Init(LEDX_PORT, LED1_PIN, &param);
	HAL_GPIO_WritePin(LEDX_PORT, LED0_PIN, GPIO_PIN_HIGH);
	HAL_GPIO_WritePin(LEDX_PORT, LED1_PIN, GPIO_PIN_HIGH);

	/*PB15 input mode*/
	param.driving = GPIO_DRIVING_LEVEL_1;
	param.mode = GPIOx_Pn_F0_INPUT;
	param.pull = GPIO_PULL_UP;
	HAL_GPIO_Init(MIC_DETECT_PORT, MIC_DETECT_PIN, &param);

	GPIO_PinState sta = HAL_GPIO_ReadPin(MIC_DETECT_PORT, MIC_DETECT_PIN);
	printf("sta=%d\n",sta);
    ad_button_init();
}

void initSerial()
{
	serial_init(SERIAL_UART_ID, 115200, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1, 0);
	serial_start();
}

int main(void)
{
	platform_init();
    //initSerial();
	#if SAMRT_DEBUG
    initGpio();
	wifi_task_init();
	rtp_server_task_init();
	http_player_task_init();
	tcp_client_task_init();
	#endif
	return 0;
}
