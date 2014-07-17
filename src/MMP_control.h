/*******************************************************************************
*  设置EncodeMange协议，用于通信的MSG以及结构体
*											
*
*            add by zm 2012-10-30
*
*
*
********************************************************************************/
#ifndef ENCODEMANGE_CONTROL_H__
#define ENCODEMANGE_CONTROL_H__

#include "common.h"

#define PORT_LISTEN_DSP1						3100
#define LOWRATE_MAX_WIDTH			(704)
#define LOWRATE_MAX_HEIGHT			(576)

#define LOWRATE_MIN_WIDTH           (352)
#define LOWRATE_MIN_HEIGHT          (288)

#define MAX_PACKET							14600
#define AVIIF_KEYFRAME						0x00000010

#define DSP1								0
#define DSP2								1









/*****************************cmd******begin****************************************************************/


#define  MSG_ADDCLIENT      	1
#define  MSG_DELETECLIENT   	2
#define  MSG_CONNECTSUCC    	3
#define  MSG_PASSWORD_ERR   	4
#define  MSG_MAXCLIENT_ERR  	5
#define  MSG_AUDIODATA			6
#define  MSG_SCREENDATA     	7
#define  MSG_HEARTBEAT      	8
#define  MSG_PASSWORD       	9
#define  MSG_DATAINFO       	10
#define  MSG_REQ_I          	11
#define  MSG_SET_FRAMERATE  	12
#define  MSG_PPT_INDEX  		15

#define MSG_SYSPARAMS			16
#define MSG_SETPARAMS			17
#define MSG_RESTARTSYS			18

#define MSG_UpdataFile			19
#define MSG_SAVEPARAMS			20
#define MSG_UpdataFile_FAILS		21
#define MSG_UpdataFile_SUCC			22

#define MSG_DECODE_STATUS			23
#define MSG_DECODE_DISCONNECT		24
#define MSG_DECODE_CONNECT			25

#define MSG_UPDATEFILE_DECODE 		26
#define MSG_UPDATEFILE_ROOT 		27
#define MSG_UPDATEFILE_WEB 			28

#define MSG_MODE_CODE				29
#define MSG_MODE_DECODE				30

#define MSG_ADD_TEXT				33

#define MSG_MOUT          			40
#define MSG_SENDFLAG    			41
#define MSG_FARCTRL      			42
#define MSG_VGA_ADJUST				43

#define MSG_QUALITYVALUE			46 //设置高清编码器码流模式(0:低 1:中 2:高) 
#define TCP_MSG_UPLOADIMG 			49//上传logo图片
#define MMP_MSG_GET_LOGOINFO 		50//获取logo图片信息
#define MMP_MSG_SET_LOGOINFO 		51//设置logo图片信息
#define MSG_GET_VIDEOPARAM			0x70
#define MSG_SET_VIDEOPARAM			0x71
#define MSG_GET_AUDIOPARAM			0x72
#define MSG_SET_AUDIOPARAM			0x73
#define MSG_REQ_AUDIO				0x74
#define MSG_CHG_PRODUCT				0x75

#define MSG_SET_SYSTIME				0x77
#define MSG_SET_DEFPARAM			0x79

#define MSG_SET_PICPARAM			0x90
#define MSG_GET_PICPARAM			0x91


#define MSG_CHANGE_INPUT			0x92


#define MSG_SEND_INPUT				0x93


#define MSG_PIC_DATA                0x94        //自动ppt索引
#define MSG_LOW_SCREENDATA          0x95        //低码率数据
#define MSG_LOW_BITRATE             0x96        //请求低码流
#define MSG_MUTE                    0x97        //静音（1：静音，0：不静音，不带参数表示查询静音状态）
#define MSG_PHOTO                   0x98        //照相功能
#define MSG_LOCK_SCREEN    0x99//锁定屏幕
#define MSG_GSCREEN_CHECK           0x9a   //绿屏校正，无参数
#define MSG_SETVGAADJUST            43 //边框调整,消息头+pic_revise
#define MSG_CAMERACTRL_PROTOCOL     0x9b   //摄像头控制协议修改 消息头后跟一个字节，

#define MSG_TRACKAUTO				0x9f

/*****************************cmd*******end***************************************************************/

/*消息头结构*/
typedef struct __MSGHEAD__ {
	/*
	## nLen:
	## 通过htons转换的值
	## 包括结构体本身和数据
	##
	*/
	WORD	nLen;
	WORD	nVer;							//版本号(暂不用)
	BYTE	nMsg;							//消息类型
	BYTE	szTemp[3];						//保留字节
} MSGHEAD;




//###########################encodemange beigin#################################
/*system param table*/
typedef struct __REACHSYSPARAMS {
	unsigned char szMacAddr[8];				//MAC address
	unsigned int dwAddr;					//IP address
	unsigned int dwGateWay;					//getway
	unsigned int dwNetMask;					//sub network mask
	char strName[16];						//encode box name
	char strVer[10];						//Encode box version
	unsigned short unChannel;							//channel numbers
	char bType; 							//flag  0 -------VGABOX     3-------200 4-------110 5-------120 6--------1200
	char bTemp[3]; 							//bTemp[0] ------1260
	int nTemp[7];							//reserve  nTemp[0] 表示DHCP FLAG
} MMPSysParam;

/*video param*/
typedef struct __REACHVIDEOPARAM {
	unsigned int nDataCodec;   					//Encode Mode
	//video: mmioFOURCC('H','2','6','4');
	unsigned int nFrameRate;   					//vdieo: FrameRate
	unsigned int nWidth;       					//video: width
	unsigned int nHight;       					//video: height
	unsigned int nColors;      					//video: colors
	unsigned int nQuality;							//video: qaulity
	unsigned short sCbr;								//video: Qaulity/BitRate quality = 0,bitrate = 1
	unsigned short sBitrate;							//video: bitrate
} MMPVideoParam;

/*audio param*/
typedef struct __REACHAUDIOPARAM {
	unsigned int Codec;  							//Encode Type
	unsigned int SampleRateIndex; 						//SampleRate
	unsigned int BitRate;  						//BitRate
	unsigned int Channel;  						//channel numbers
	unsigned int SampleBit;  						//SampleBit
	unsigned char  LVolume;					//left volume       0 --------31
	unsigned char  RVolume;					//right volume      0---------31
	unsigned short  InputMode;             // 需要在上面做支持，区分平衡与非平衡MAC
} MMPAudioParam;


/*低码率参数*/
typedef struct strLOW_BITRATE {
	int nWidth;		//宽
	int nHeight;		//高
	int nFrame;		//帧率
	int nBitrate;   //带宽
	int nTemp;		//保留
} MMPLowVideoParam;

//MSG_LOCK_SCREEN
typedef enum {
   MMP_LOCK_SCREEN_OUT = 0,
   MMP_LOCK_SCREEN_IN = 1
} MMP_LOCK_SCREEN_STATUS;

typedef enum{
	LOW_VIDEO_STOP = 0,
	LOW_VIDEO_START = 1
}LOW_VIDEO_STATUS;


/*音频和视频网络发送数据头*/
typedef struct __HDB_FRAME_HEAD {
	DWORD ID;								//=mmioFOURCC('4','D','S','P');
	DWORD nTimeTick;    					//时间戳
	DWORD nFrameLength; 					//帧长度
	DWORD nDataCodec;   					//编码类型
	//编码方式
	//视频 :mmioFOURCC('H','2','6','4');
	//音频 :mmioFOURCC('A','D','T','S');
	DWORD nFrameRate;   					//视频  :帧率
	//音频 :采样率 (default:44100)
	DWORD nWidth;       					//视频  :宽
	//音频 :通道数 (default:2)
	DWORD nHight;       					//视频  :高
	//音频 :采样位 (default:16)
	DWORD nColors;      					//视频  :颜色数  (default: 24)
	//音频 :音频码率 (default:64000)
	DWORD dwSegment;						//分包标志位
	DWORD dwFlags;							//视频:  I 帧标志
	//音频:  保留
	DWORD dwPacketNumber; 					//包序号
	DWORD nOthers;      					//silverlight 配合 aac-he=0 aac-lc=1
} FRAMEHEAD;

//#########################encodemange end######################################



#endif




