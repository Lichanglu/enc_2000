#ifndef __PROCESS_XML_CMD__H
#define __PROCESS_XML_CMD__H
#include "MMP_control.h"
#include "middle_control.h"
#include "remotectrl.h"


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
