/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_recorder.h
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Record module API.
 */
#ifndef __DUERAPP_XPLAY_H__
#define __DUERAPP_XPLAY_H__
enum
{	
	DUERAPP_XPLAY_NO_ERR = 0,
	DUERAPP_XPLAY_ERR_CREATE_FAIL = -1,
	DUERAPP_XPLAY_ERR_SETHTTPBUFFER = -2,
	DUERAPP_XPLAY_ERR_SETURL = -3,
	DUERAPP_XPLAY_ERR_PALY = -4,
	DUERAPP_XPLAY_ERR_SEEEK = -5,
	DUERAPP_XPLAY_ERR_DESTORY = -6,
	DUERAPP_XPLAY_ERR_STOP = -7,
	DUERAPP_XPLAY_ERR_PAUSE = -8,
	DUERAPP_XPLAY_ERR_SETDATASOUCR_BTEAK = -9,
	DUERAPP_XPLAY_ERR_DEVICE = -10,
};
#endif

