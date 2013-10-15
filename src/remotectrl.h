/*********************************************************************
¹«Ë¾Ãû³Æ:     Reach Software
ÎÄ¼şÃû:	   remotectrl.h
´´ ½¨ ÈË£º    yangshaohua                    ÈÕÆÚ£º2009-01-14
ĞŞ ¸Ä ÈË£º    DUHAO                            ÈÕÆÚ£º2012-9-26
¹¦ÄÜÃèÊö£ºÉãÏñÍ·Ô¶Ò£¿ØÖÆÄ£¿éÍ·ÎÄ¼ş

ÆäËûËµÃ÷£º Í¨¹ı´®¿ÚÍ¨Ñ¶¿ØÖÆÉãÏñÍ·µÄĞı×ªµÈ²Ù×÷
**********************************************************************/
//#include "encliveplay.h"
#ifndef __REMOTECTRL__
#define __REMOTECTRL__
#include "common.h"
#define REMOTE_COM				0
#define SLID_MAX_NUM  			11
#define ADDR_MAX_NUM			101   //struct __ADDRESS__  default addr 1-100
#define ADDR_CMD_MAX_NUM		64   //struct __CTRLCODE__

#define UPSTART					0
#define UPSTOP					1

#define DOWNSTART				2
#define DOWNSTOP				3

#define LEFTSTART				4
#define LEFTSTOP				5

#define RIGHTSTART				6
#define RIGHTSTOP				7

#define DADDSTART				8
#define DADDSTOP				9

#define DDECSTART				10
#define DDECSTOP				11

#define REL_POSITION			12
#define DDEC_DIRECT				13

#define ZOOMPOSQUERY			14
#define PAN_TILTPOSQUERY		15

#define POSITIONPRESET			16//Ô¤ÖÃÎ»
#define POSITIONPRESETSET		17//ÉèÖÃÔ¤ÖÃÎ»

#define CTRL_CODE_MAX_NUM		18   //struct __CTRLCODE__
#define PROTNUM 				7	//Ô¶³ÌÒ£¿ØĞ­Òé×Ü¸öÊı

/*´®¿Ú*/
#define PORT_COM0		0
#define PORT_COM1		1
#define PORT_COM2		2
#define PORT_COM3		3
#define	REMOTE_INDEX_SUM	4
#define	PRINT_COM	2
/*Ô¶Ò¡¿ØÖÆ½á¹¹ÌåÅäÖÃÎÄ¼ş*/
typedef struct __HVSEEPSLID__ {
	BYTE speed[SLID_MAX_NUM];
} HVSSlid;

typedef struct __ADDRESS__ {
	BYTE addr[ADDR_MAX_NUM];
} ADDR;

typedef struct __CTRLCODE__ {
	char comm[ADDR_CMD_MAX_NUM];
} CTRLCODE;

typedef struct __COMPARAM__ {
	int stopbits;
	int databits;
	int parity;
	int baud;
} COMPARAM;
typedef struct tagRECT {
	int    left;
	int    top;
	int    right;
	int    bottom;
} RECT;


typedef struct tagPOINT {
	int  x;
	int  y;
} POINT;
typedef struct __REMOTE_CONFIG__ {
	BYTE ptzName[50];
	/*Ë®Æ½·½Ïò*/
	HVSSlid HSSlid;
	/*´¹Ö±·½Ïò*/
	HVSSlid VSSlid;
	/*Ö¸ÁîµØÖ·*/
	ADDR Addr;
	/*¿ØÖÆÃüÁî*/
	CTRLCODE CCode[CTRL_CODE_MAX_NUM];
	/*´®¿ÚÊôĞÔ*/
	COMPARAM comm;
} RemoteConfig;

typedef struct REMOTE_INFO
{
	int type;
	int speed;
	int caddr;
	int prezition;
	int reservation;
}REMOTE_INFO;


/*ÉèÖÃÍ¨Ñ¶¹ÜµÀ£¬ÓÃÓÚ´®¿ÚÊı¾İµÄ¶ÁĞ´*/
extern void InitPipe(int index);
/*´Ó¹ÜµÀ¶ÁÊı¾İ´*/
extern int ReadFromPipe(unsigned char *buf, int count, int index);
/*°Ñ´®¿ÚÊÕµ½µÄÊı¾İĞ´Èë¹ÜµÀ*/
extern int WriteToPipe(unsigned char *data, int count, int index);
/*ÉãÏñÍ·µÄ¿ØÖÆ*/
extern int CameraControl();
/*³õÊ¼»¯Ô¶Ò¡µÄÅäÖÃÎÄ¼ş*/
extern int InitRemoteStruct(int index);

/*ÉãÏñÍ·³õÊ¼»¯*/
extern int CameraCtrlInit(int index);
extern int InitPort(int port_num, int baudrate, int databits, int stopbits, int parity);
/*Ô¶Ò¡API */
extern int CCtrlUpStart(int fd, int addr, int speed, int index);
extern int CCtrlUpStop(int fd, int addr, int index);
extern int CCtrlDownStart(int fd, int addr, int speed, int index);
extern int CCtrlDownStop(int fd, int addr, int index);
extern int CCtrlLeftStart(int fd, int addr, int speed, int index);
extern int CCtrlLeftStop(int fd, int addr, int index);
extern int CCtrlRightStart(int fd, int addr, int speed, int index);
extern int CCtrlRightStop(int fd, int addr, int index);
extern int CCtrlZoomAddStart(int fd, int addr, int index);
extern int CCtrlZoomAddStop(int fd, int addr, int index);
extern int CCtrlZoomDecStart(int fd, int addr, int index);
extern int CCtrlZoomDecStop(int fd, int addr, int index);
extern void FarCtrlCamera(int dsp, unsigned char *data, int len, int index);
extern int SetTrackStatus(int fd, int val);
extern int ListenSeries(void *pParam, int index);
int GetRmtProtoleIndex(int remote);
int SetRmtProtoleIndex(int remote, int ProtoleIndex);
int remote_ctrl_camera(int index, REMOTE_INFO *far_ctrl_info);


#endif   //__REMOTECTRL__


