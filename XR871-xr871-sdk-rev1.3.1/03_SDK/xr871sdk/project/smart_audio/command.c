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

#include "common/cmd/cmd_util.h"
#include "common/cmd/cmd.h"

#define COMMAND_IPERF	0
#define COMMAND_PING	1
#define COMMAND_HTTPC   1
#define COMMAND_CLOSED	1
/*
 * net commands
 */
static const struct cmd_data g_net_cmds[] = {
#if COMMAND_CLOSED	
	{ "ifconfig",	cmd_ifconfig_exec },
#endif
	{ "sta",		cmd_wlan_sta_exec },
	{ "smartconfig",cmd_smart_config_exec },
	{ "airkiss",	cmd_airkiss_exec },
	{ "voiceprint",	cmd_voice_print_exec },
	{ "smartlink",	cmd_smartlink_exec },
#if COMMAND_HTTPC
	{ "httpc",		cmd_httpc_exec },
#endif	
#if COMMAND_IPERF
	{ "iperf",		cmd_iperf_exec },
#endif
#if COMMAND_PING
	{ "ping",		cmd_ping_exec },
#endif
};

static enum cmd_status cmd_net_exec(char *cmd)
{
	return cmd_exec(cmd, g_net_cmds, cmd_nitems(g_net_cmds));
}

/*
 * driver commands
 */
static const struct cmd_data g_drv_cmds[] = {
#if COMMAND_CLOSED
	{ "sd",		cmd_sd_exec },
#endif
};

static enum cmd_status cmd_drv_exec(char *cmd)
{
	return cmd_exec(cmd, g_drv_cmds, cmd_nitems(g_drv_cmds));
}

/*
 * main commands
 */
static const struct cmd_data g_main_cmds[] = {
	#if COMMAND_CLOSED
	{ "drv",    cmd_drv_exec },
	{ "echo",   cmd_echo_exec },
	{ "auddbg", cmd_auddbg_exec },
	#endif
	{ "fs", 	cmd_fs_exec },
	{ "heap",	cmd_heap_exec },
	{ "net",    cmd_net_exec },
	{ "mem",    cmd_mem_exec },
	{ "upgrade",cmd_upgrade_exec },
	{ "reboot", cmd_reboot_exec },
	{ "pm",		cmd_pm_exec },
	{ "audio", 	cmd_audio_exec },
	{ "cedarx",	cmd_cedarx_exec},
	{ "sysinfo",cmd_sysinfo_exec },
};

int main_cmd_exec(char *cmd)
{
	enum cmd_status status;

	if (cmd[0] == '\0') { /* empty command */
		CMD_LOG(1, "$\n");
		return -1;
	}

	CMD_LOG(CMD_DBG_ON, "$ %s\n", cmd);

	status = cmd_exec(cmd, g_main_cmds, cmd_nitems(g_main_cmds));
	if (status == CMD_STATUS_ACKED) {
		return status; /* already acked, just return */
	}

	cmd_write_respond(status, cmd_get_status_desc(status));
	return status;
}