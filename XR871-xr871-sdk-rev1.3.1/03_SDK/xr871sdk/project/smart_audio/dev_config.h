#ifndef __DEV_CONFIG_H_
#define __DEV_CONFIG_H_


#define     DEV_HW_VERSION              "2.1.03"
#define     DEV_SW_VERSION              "v2.3.2000" 
#define     DEV_TYPE                    7

#define     DEV_DEBUG_ID                "FMVCI8DMYWEKU1L1FVP1"
                                        //"FMVB1WP4LNLLL3BJWPNU" 
#define     DEV_DEBUG_PWD               "123456"

//#define     DEV_DE_ROUNTER_SERVER       "ipc.100memory.com"
#define     DEV_DE_ROUNTER_SERVER       "thzn_td_dev.100memory.com"

//登录指令
#define 	DEV_LOGIN_CONSERVER  	                        "post http://%s/cmd.php?act=login"
//登出指令
#define 	DEV_LOGOUT_CONSERVER		                    "post http://%s/cmd.php?act=logout" 
//上报坐姿提醒信息记录
#define 	STRING_REPORTPOSEREMIND_CONSERVER		        "post http://%s/cmd.php?act=poseRemind"
//上报休息提醒信息记录
#define 	STRING_REPORTRESETREMIND_CONSERVER		        "post http://%s/cmd.php?act=restRemind"
//持续学习时长 的记录
#define 	STRING_REPORTCONTINUESTUDY_CONSERVER		    "post http://%s/cmd.php?act=continualStudy"
//上报坐姿达标率改变的记录
#define 	STRING_REPORTGOODPOSERATEREMIND_CONSERVER		"post http://%s/cmd.php?act=poseDataStatus"

#endif



