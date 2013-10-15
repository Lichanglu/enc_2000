#ifndef _PARAMS_H_
#define _PARAMS_H_

#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#define CONFIG_TABLE_FILE		("config.xml")

#define MAX_VIDEO_NUM		(2)
#define MAX_AUDIO_NUM		(2)

#define ADTS                    0x53544441
#define AAC                     0x43414130
#define PCM                     0x4d435030

#define MIC_INPUT               1
#define LINE_INPUT              0

#define REMOTE_NAME				".config/remote.ini"

#if 0
#ifndef ENC_HD_PTO
#define ENC_HD_PTO

/*system param table*/
typedef struct __SYSPARAMS {
	unsigned char szMacAddr[8];				//MAC address
	unsigned int dwAddr;					//IP address
	unsigned int dwGateWay;					//getway
	unsigned int dwNetMark;					//sub network mask
	char strName[16];						//encode box name
	char strVer[10];						//Encode box version
	unsigned short unChannel;							//channel numbers
	char bType; 							//flag  0 -------VGABOX     3-------200 4-------110 5-------120 6--------1200
	char bTemp[3]; 							//bTemp[0] ------1260
	int nTemp[7];							//reserve  nTemp[0] ±íÊ¾DHCP FLAG
} SYSPARAMS;

/*video param*/
typedef struct __VIDEOPARAM {
	unsigned int nDataCodec;   					//Encode Mode
	//video: mmioFOURCC('H','2','6','4');
	unsigned int nFrameRate;   					//vdieo: FrameRate
	unsigned int nWidth;       					//video: width
	unsigned int nHight;       					//video: height
	unsigned int nColors;      					//video: colors
	unsigned int nQuality;							//video: qaulity
	unsigned short sCbr;								//video: Qaulity/BitRate
	unsigned short sBitrate;							//video: bitrate
} VideoParam;

/*audio param*/
typedef struct __AUDIOPARAM {
	unsigned int Codec;  							//Encode Type
	unsigned int SampleRate; 						//SampleRate
	unsigned int BitRate;  						//BitRate
	unsigned int Channel;  						//channel numbers
	unsigned int SampleBit;  						//SampleBit
	unsigned char  LVolume;					//left volume       0 --------31
	unsigned char  RVolume;					//right volume      0---------31
	unsigned short  InputMode;               //1------ MIC input 0------Line Input
} AudioParam;

#endif


typedef struct _params_table_ {
	pthread_mutex_t	lock;
	SYSPARAMS sysPara;
	VideoParam videoPara[MAX_VIDEO_NUM];
	AudioParam audioPara[MAX_AUDIO_NUM];
} params_table;




int create_params_table_file(char *xml_file, params_table *ptable);
int read_params_table_file(char *xml_file, params_table *ptable);
int modify_params_table_file(char *xml_file, params_table *pold_table, params_table *pnew_table);

int init_all_params();

int get_system_param(params_table *ptable, SYSPARAMS *psys);
int get_video_param(params_table *ptable, int chid, VideoParam *pvideo);
int get_audio_param(params_table *ptable, int chid, AudioParam *paudio);
int get_realtime_param(params_table *p_rt_table, params_table *p_out_table);

int msg_get_video_param(int socket, int chid, unsigned char *data, params_table *ptable);
int msg_get_audio_param(int socket, int chid, unsigned char *data, params_table *ptable);
int msg_get_system_param(int socket, unsigned char *data, params_table *ptable);
int msg_set_video_param(int chid, unsigned char *data, params_table *ptable);
int msg_set_audio_param(int chid, unsigned char *data, params_table *ptable);
int msg_save_param_table(params_table *pold_table, params_table *pnew_table);

void printf_params();

int audio_setCapParamInput(int ch,int mode);
int audio_setCapvol(int ch,int mode);
#endif
#endif
