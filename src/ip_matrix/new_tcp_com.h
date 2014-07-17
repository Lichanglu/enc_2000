#ifndef __NEW_TCP_COM__H
#define __NEW_TCP_COM__H

#define SUPPORT_XML_PROTOCOL
#define SUPPORT_IP_MATRIX


#ifdef SUPPORT_XML_PROTOCOL

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
#define ENC2000_INPUT_MAX				2

#define LOGIN_USER				0
#define LOGIN_ADMIN				1
#define INVALID_SOCKET 			-1
#define INVALID_FD					-1
#define MAX_CLIENT_NUM 			6

#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~(B(bit))))
#define	TST(bit)	(A(bit) & B(bit))

#ifdef SUPPORT_XML_PROTOCOL
#define TRUE              1
#define FALSE             0
#define INVALID_SOCKET 			-1
#endif

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


NewDSPCliParam g_client_para[ENC2000_INPUT_MAX];
/*if use client*/
#define ISUSED_NEW(input,cli)				(g_client_para[input].cliDATA[cli].bUsed == TRUE)
/*set client used*/
#define SETCLIUSED_NEW(input,cli,val)		(g_client_para[input].cliDATA[cli].bUsed=val)
/*if login client*/
#define ISLOGIN_NEW(input,cli)			(g_client_para[input].cliDATA[cli].bLogIn == TRUE)
/*set client login succeed or failed*/
#define SETCLILOGIN_NEW(input,cli,val)	(g_client_para[input].cliDATA[cli].bLogIn=val)
/*get socket fd*/
#define GETSOCK_NEW(input,cli)			(g_client_para[input].cliDATA[cli].sSocket)
/*set socket fd*/
#define SETSOCK_NEW(input,cli,val)		(g_client_para[input].cliDATA[cli].sSocket=val)
/*current socket if valid*/
#define ISSOCK_NEW(input,cli)				(g_client_para[input].cliDATA[cli].sSocket != INVALID_SOCKET)
/*set send audio data flag*/
#define SET_SEND_AUDIO_NEW(input,cli,val)  	(g_client_para[input].cliDATA[cli].sendAudioFlag=val)
/*get send audio data flag */
#define GET_SEND_AUDIO_NEW(input,cli)  		(g_client_para[input].cliDATA[cli].sendAudioFlag)
/*set send video data flag*/
#define SET_SEND_VIDEO_NEW(input,cli,val)  	(g_client_para[input].cliDATA[cli].sendVideoFlag=val)
/*get send video data flag */
#define GET_SEND_VIDEO_NEW(input,cli)  		(g_client_para[input].cliDATA[cli].sendVideoFlag)

/*set resize flag*/
#define SETLOWRATEFLAG_NEW(input,cli,val)  	(g_client_para[input].cliDATA[cli].LowRateflag=val)
/*get resize flag*/
#define GETLOWRATEFLAG_NEW(input,cli)  		(g_client_para[input].cliDATA[cli].LowRateflag)

/*get connect time*/
#define GETCONNECTTIME_NEW(input,cli)  		(g_client_para[input].cliDATA[cli].connect_time)
#define SETCONNECTTIME_NEW(input,cli,val)		(g_client_para[input].cliDATA[cli].connect_time = val)

#define SET_PARSE_LOW_RATE_FLAG(input,cli,val) (g_client_para[input].cliDATA[cli].parseLowRateFlag = val)
#define GET_PARSE_LOW_RATE_FLAG(input,cli) 	 (g_client_para[input].cliDATA[cli].parseLowRateFlag)

#define SET_FIX_RESOLUTION_FLAG(input,cli,val)  (g_client_para[input].cliDATA[cli].fixresolutionFlag = val)
#define GET_FIX_RESOLUTION_FLAG(input,cli)	 (g_client_para[input].cliDATA[cli].fixresolutionFlag)


#ifdef SUPPORT_IP_MATRIX
#define SET_PASSKEY_FLAG(input,cli,val)  (g_client_para[input].cliDATA[cli].passkey = val)
#define GET_PASSKEY_FLAG(input,cli)	 (g_client_para[input].cliDATA[cli].passkey)
#endif



typedef struct MsgHeader //消息头
{
	    unsigned short sLen;		// 
	    unsigned short sVer;		//1
	    unsigned short sMsgCode;	//    1
	    unsigned short sData;	//保留
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

/*
typedef struct REMOTE_INFO
{
	int type;
	int speed;
	int caddr;
	int prezition;
	int reservation;
}REMOTE_INFO;
*/
typedef struct __RATE_IFNO {
	int rateInfoNum;
	int rateType;	//码率标志位，0-----高/ 1------低。
	int nWidth;		//宽
	int nHeight;		//高
	int nFrame;		//帧率
	int nBitrate;   //带宽
	int nTemp;		//保留
} RATE_INFO;


typedef struct __AUDIO_PARAM
{
	unsigned short  InputMode;                       ////1------ MIC input 0------Line Input
	unsigned int SampleRate; 						//采样率
	unsigned int BitRate;  						//码率
	unsigned char  LVolume;							//左声音     0 --------31
	unsigned char  RVolume;							//右声音     0---------31
}AUDIO_PARAM;

typedef struct __PICTURE_ADJUST
{
	short hporch;     // 左：负值，右：正值
	short vporch;     // 上：负值，下：正值
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


void init_new_tcp_module(void);
extern void open_new_tcp_task(void * param);
extern void set_fix_resolution(int value);
extern int tcp_send_data(int sockfd, char *send_buf);
extern int get_fix_resolution(void);
extern int add_heart_count(int input,int cli);
extern int set_heart_count(int intut,int cli, int value);
extern int get_g_lock_count(int pos);

#endif

#endif //__TCPCOM__H
