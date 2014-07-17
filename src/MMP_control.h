/*******************************************************************************
*  ����EncodeMangeЭ�飬����ͨ�ŵ�MSG�Լ��ṹ��
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

#define MSG_QUALITYVALUE			46 //���ø������������ģʽ(0:�� 1:�� 2:��) 
#define TCP_MSG_UPLOADIMG 			49//�ϴ�logoͼƬ
#define MMP_MSG_GET_LOGOINFO 		50//��ȡlogoͼƬ��Ϣ
#define MMP_MSG_SET_LOGOINFO 		51//����logoͼƬ��Ϣ
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


#define MSG_PIC_DATA                0x94        //�Զ�ppt����
#define MSG_LOW_SCREENDATA          0x95        //����������
#define MSG_LOW_BITRATE             0x96        //���������
#define MSG_MUTE                    0x97        //������1��������0��������������������ʾ��ѯ����״̬��
#define MSG_PHOTO                   0x98        //���๦��
#define MSG_LOCK_SCREEN    0x99//������Ļ
#define MSG_GSCREEN_CHECK           0x9a   //����У�����޲���
#define MSG_SETVGAADJUST            43 //�߿����,��Ϣͷ+pic_revise
#define MSG_CAMERACTRL_PROTOCOL     0x9b   //����ͷ����Э���޸� ��Ϣͷ���һ���ֽڣ�

#define MSG_TRACKAUTO				0x9f

/*****************************cmd*******end***************************************************************/

/*��Ϣͷ�ṹ*/
typedef struct __MSGHEAD__ {
	/*
	## nLen:
	## ͨ��htonsת����ֵ
	## �����ṹ�屾�������
	##
	*/
	WORD	nLen;
	WORD	nVer;							//�汾��(�ݲ���)
	BYTE	nMsg;							//��Ϣ����
	BYTE	szTemp[3];						//�����ֽ�
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
	int nTemp[7];							//reserve  nTemp[0] ��ʾDHCP FLAG
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
	unsigned short  InputMode;             // ��Ҫ��������֧�֣�����ƽ�����ƽ��MAC
} MMPAudioParam;


/*�����ʲ���*/
typedef struct strLOW_BITRATE {
	int nWidth;		//��
	int nHeight;		//��
	int nFrame;		//֡��
	int nBitrate;   //����
	int nTemp;		//����
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


/*��Ƶ����Ƶ���緢������ͷ*/
typedef struct __HDB_FRAME_HEAD {
	DWORD ID;								//=mmioFOURCC('4','D','S','P');
	DWORD nTimeTick;    					//ʱ���
	DWORD nFrameLength; 					//֡����
	DWORD nDataCodec;   					//��������
	//���뷽ʽ
	//��Ƶ :mmioFOURCC('H','2','6','4');
	//��Ƶ :mmioFOURCC('A','D','T','S');
	DWORD nFrameRate;   					//��Ƶ  :֡��
	//��Ƶ :������ (default:44100)
	DWORD nWidth;       					//��Ƶ  :��
	//��Ƶ :ͨ���� (default:2)
	DWORD nHight;       					//��Ƶ  :��
	//��Ƶ :����λ (default:16)
	DWORD nColors;      					//��Ƶ  :��ɫ��  (default: 24)
	//��Ƶ :��Ƶ���� (default:64000)
	DWORD dwSegment;						//�ְ���־λ
	DWORD dwFlags;							//��Ƶ:  I ֡��־
	//��Ƶ:  ����
	DWORD dwPacketNumber; 					//�����
	DWORD nOthers;      					//silverlight ��� aac-he=0 aac-lc=1
} FRAMEHEAD;

//#########################encodemange end######################################



#endif




