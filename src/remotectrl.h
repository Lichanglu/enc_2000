/*********************************************************************
��˾����:     Reach Software
�ļ���:	   remotectrl.h
�� �� �ˣ�    yangshaohua                    ���ڣ�2009-01-14
�� �� �ˣ�    DUHAO                            ���ڣ�2012-9-26
��������������ͷԶң����ģ��ͷ�ļ�

����˵���� ͨ������ͨѶ��������ͷ����ת�Ȳ���
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

#define POSITIONPRESET			16//Ԥ��λ
#define POSITIONPRESETSET		17//����Ԥ��λ

#define CTRL_CODE_MAX_NUM		18   //struct __CTRLCODE__
#define PROTNUM 				7	//Զ��ң��Э���ܸ���

/*����*/
#define PORT_COM0		0
#define PORT_COM1		1
#define PORT_COM2		2
#define PORT_COM3		3
#define	REMOTE_INDEX_SUM	4
#define	PRINT_COM	2
/*Զҡ���ƽṹ�������ļ�*/
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
	/*ˮƽ����*/
	HVSSlid HSSlid;
	/*��ֱ����*/
	HVSSlid VSSlid;
	/*ָ���ַ*/
	ADDR Addr;
	/*��������*/
	CTRLCODE CCode[CTRL_CODE_MAX_NUM];
	/*��������*/
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


/*����ͨѶ�ܵ������ڴ������ݵĶ�д*/
extern void InitPipe(int index);
/*�ӹܵ������ݴ*/
extern int ReadFromPipe(unsigned char *buf, int count, int index);
/*�Ѵ����յ�������д��ܵ�*/
extern int WriteToPipe(unsigned char *data, int count, int index);
/*����ͷ�Ŀ���*/
extern int CameraControl();
/*��ʼ��Զҡ�������ļ�*/
extern int InitRemoteStruct(int index);

/*����ͷ��ʼ��*/
extern int CameraCtrlInit(int index);
extern int InitPort(int port_num, int baudrate, int databits, int stopbits, int parity);
/*ԶҡAPI */
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


