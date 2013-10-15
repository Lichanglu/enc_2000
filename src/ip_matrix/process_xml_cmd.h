#ifndef __PROCESS_XML_CMD__H
#define __PROCESS_XML_CMD__H
#include "MMP_control.h"
#include "middle_control.h"
#include "remotectrl.h"

//#define SUPPORT_XML_PROTOCOL
///#define SUPPORT_IP_MATRIX

#ifdef SUPPORT_XML_PROTOCOL

typedef struct __SYSINFO
{
	char deviceType[16];					//设备类型
	char strName[16];						//产品序列号
	unsigned char szMacAddr[8];				//MAC 地址
	unsigned int dwAddr;					//IP 地址
	unsigned int dwGateWay;					//网关
	unsigned int dwNetMark;					//子网掩码
	char strVer[10];						//编码器版本号
	unsigned short channelNum;				//通道数
	unsigned short pptIndex;				//PPT索引标识
	unsigned short lowRate;
	unsigned short encodeType;
}SYSTEMINFO;

 
#define NEW_MSG_TCP_LOGIN 				31001
#define NEW_MSG_REQUEST_RATE			30011
#define NEW_MSG_REMOTE					30013
#define NEW_MSG_SYS_RESTART				31002
#define NEW_MSG_SET_AUDIO				30006
#define NEW_MSG_GET_ROOM_INFO			30003
#define NEW_MSG_SET_QUALITY_INFO		30005
#define NEW_MSG_GET_QUALITY_INFO		31021
#define NEW_MSG_SYS_UPDATE				31004
#define NEW_MSG_UPLOAD_LOGO				30016
#define NEW_MSG_ADD_TITLE				30018
#define NEW_MSG_GET_VOLUME				30019
#define NEW_MSG_MUTE					30020
#define NEW_MSG_PIC_REVISE				30021
#define NEW_MSG_WARN_REPORT				10009
#define NEW_MSG_LOG_REPORT				10010
#define NEW_MSG_STATUS_REPORT			30032
#define NEW_REQUEST_IFRAME				30014
#define NEW_FIX_RESOLUTION				30010
#ifdef SUPPORT_IP_MATRIX
#define NEW_ENCODE_ENABLE 				30044
#define NEW_STOP_RATE					30012
#define NEW_LOCK_AV_PARAM				31045
#define NEW_GET_SYS_PARAM				31046
#endif
#define     START          1
#define     STOP           0
 
#define MAX_CLIENT_NUM 			6
 
 /*client infomation*/
 typedef struct __NewClientData {
	 int bUsed;
	 int sSocket;
	 int bLogIn;
	 int sendAudioFlag;
	 int LowRateflag;
	 int sendVideoFlag;
	 int parseLowRateFlag;
	 int fixresolutionFlag;
	 int heartCount;
	#ifdef SUPPORT_IP_MATRIX
	 int passkey;
	#endif
	 pthread_mutex_t heart_count_mutex;
	 unsigned long connect_time;
 } NewClientData;
 
 /*DSP client param*/
 typedef struct __NewDSPCliParam {
	 NewClientData cliDATA[MAX_CLIENT_NUM]; //client param infomation
 } NewDSPCliParam;
 
 
 NewDSPCliParam g_client_para[2];
 /*if use client*/
#define ISUSED_NEW(dsp, cli)				(g_client_para[dsp].cliDATA[cli].bUsed == TRUE)
 /*set client used*/
#define SETCLIUSED_NEW(dsp, cli,val)		(g_client_para[dsp].cliDATA[cli].bUsed=val)
 /*if login client*/
#define ISLOGIN_NEW(dsp, cli)			(g_client_para[dsp].cliDATA[cli].bLogIn == TRUE)
 /*set client login succeed or failed*/
#define SETCLILOGIN_NEW(dsp, cli,val)	(g_client_para[dsp].cliDATA[cli].bLogIn=val)
 /*get socket fd*/
#define GETSOCK_NEW(dsp, cli)			(g_client_para[dsp].cliDATA[cli].sSocket)
 /*set socket fd*/
#define SETSOCK_NEW(dsp, cli,val)		(g_client_para[dsp].cliDATA[cli].sSocket=val)
 /*current socket if valid*/
#define ISSOCK_NEW(dsp, cli)				(g_client_para[dsp].cliDATA[cli].sSocket != INVALID_SOCKET)
 /*set send audio data flag*/
#define SET_SEND_AUDIO_NEW(dsp, cli,val)  	(g_client_para[dsp].cliDATA[cli].sendAudioFlag=val)
 /*get send audio data flag */
#define GET_SEND_AUDIO_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].sendAudioFlag)
 /*set send video data flag*/
#define SET_SEND_VIDEO_NEW(dsp, cli,val)  	(g_client_para[dsp].cliDATA[cli].sendVideoFlag=val)
 /*get send video data flag */
#define GET_SEND_VIDEO_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].sendVideoFlag)
 
 /*set resize flag*/
#define SETLOWRATEFLAG_NEW(dsp, cli,val)  	(g_client_para[dsp].cliDATA[cli].LowRateflag=val)
 /*get resize flag*/
#define GETLOWRATEFLAG_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].LowRateflag)
 
 /*get connect time*/
#define GETCONNECTTIME_NEW(dsp, cli)  		(g_client_para[dsp].cliDATA[cli].connect_time)
#define SETCONNECTTIME_NEW(dsp, cli,val)		(g_client_para[dsp].cliDATA[cli].connect_time = val)
 
#define SET_PARSE_LOW_RATE_FLAG(dsp, cli,val) (g_client_para[dsp].cliDATA[cli].parseLowRateFlag = val)
#define GET_PARSE_LOW_RATE_FLAG(dsp, cli) 	 (g_client_para[dsp].cliDATA[cli].parseLowRateFlag)
 
#define SET_FIX_RESOLUTION_FLAG(dsp, cli,val)  (g_client_para[dsp].cliDATA[cli].fixresolutionFlag = val)
#define GET_FIX_RESOLUTION_FLAG(dsp, cli)	 (g_client_para[dsp].cliDATA[cli].fixresolutionFlag)
 
 
#ifdef SUPPORT_IP_MATRIX
#define SET_PASSKEY_FLAG(dsp, cli,val)  (g_client_para[dsp].cliDATA[cli].passkey = val)
#define GET_PASSKEY_FLAG(dsp, cli)	 (g_client_para[dsp].cliDATA[cli].passkey)
#endif
 
 
 
 typedef struct MsgHeader //消息头
 {
		 unsigned short sLen;		 // 
		 unsigned short sVer;		 //1
		 unsigned short sMsgCode;	 //    1
		 unsigned short sData;	 //保留
 }MsgHead;
 
 typedef struct __xml_msghead{
	 char msgcode[8];
	 char passkey[64];
 }xml_msghead;
 
 typedef struct user_login_info
 {
	 char username[32];
	 char password[8];
 }user_login_info;
 
 
 typedef struct __RATE_IFNO {
	 int rateInfoNum;
	 int rateType;	 //码率标志位，0-----高/ 1------低。
	 int nWidth;	 //宽
	 int nHeight;		 //高
	 int nFrame;	 //帧率
	 int nBitrate;	 //带宽
	 int nTemp; 	 //保留
 } RATE_INFO;
 
 
 typedef struct __AUDIO_PARAM
 {
	 unsigned short  InputMode; 					  ////1------ MIC input 0------Line Input
	 unsigned int SampleRate;						 //采样率
	 unsigned int BitRate;						 //码率
	 unsigned char	LVolume;						 //左声音	  0 --------31
	 unsigned char	RVolume;						 //右声音	  0---------31
 }AUDIO_PARAM;
 
 typedef struct __PICTURE_ADJUST
 {
	 short hporch;	   // 左：负值，右：正值
	 short vporch;	   // 上：负值，下：正值
	 short color_trans;
	 int   temp[4];
 }PICTURE_ADJUST;
 
#ifdef SUPPORT_IP_MATRIX
 typedef struct __LOCK_PARAM
 {
	 int video_lock;
	 int audio_lock;
	 int resulotion_lock;
 }LOCK_PARAM;
#endif
#if 0
 void *report_pthread_fxn(void *arg);

int process_xml_login_msg(int index, int pos,char *send_buf,user_login_info *login_info, int msgCode, char *passkey,char *user_id);
int process_xml_request_rate_msg(int index, int pos,char *send_buf,int msgCode,char *passkey,char *user_id,int *roomid);
int process_xml_remote_ctrl_msg(int index, int pos,char *send_buf,REMOTE_INFO *far_ctrl,int msgCode,char *passkey,char *user_id,int *roomid);
int process_set_audio_cmd(int index, int pos,char *send_buf,AUDIO_PARAM *audio_info,int msgCode,char *passkey,char *user_id,int *roomid);
int process_get_room_info_cmd(int index, int pos,char *send_buf,int msgCode,char *passkey,char *user_id,int *roomid);
int process_xml_get_volume_msg(int index, int pos,char *send_buf,int msgCode,char *passkey,char *user_id,int *roomid);
int process_mute_cmd(int index, int pos,char *send_buf,int msgCode,char *passkey,char *user_id,int *roomid);
int process_pic_adjust_cmd(int index, int pos,char *send_buf,PICTURE_ADJUST *info,int msgCode,char *passkey,char *user_id,int *roomid);
int process_xml_request_IFrame_msg(int index, int pos,char *send_buf,int msgCode,char*passkey,char *user_id,int *roomid);
int  SendVideoDataToIpMatrixClient(int index, void *pData, int nLen);
int  SendAudioDataToIpMatrixClient(int index, void *pData, int nLen);
#endif
#endif



#endif
