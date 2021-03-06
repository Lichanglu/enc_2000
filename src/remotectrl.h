/*********************************************************************
公司名称:     Reach Software
文件名:	   remotectrl.h
创 建 人：    yangshaohua                    日期：2009-01-14
修 改 人：    DUHAO                            日期：2012-9-26
功能描述：摄像头远遥控制模块头文件

其他说明： 通过串口通讯控制摄像头的旋转等操作
**********************************************************************/
//#include "encliveplay.h"
#ifndef __REMOTECTRL__
#define __REMOTECTRL__
#include "common.h"
#include "new_tcp_com.h"
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

#define POSITIONPRESET			16//预置位
#define POSITIONPRESETSET		17//设置预置位

#define CTRL_CODE_MAX_NUM		18   //struct __CTRLCODE__
#define PROTNUM 				7	//远程遥控协议总个数

/*串口*/
#define PORT_COM0		0
#define PORT_COM1		1
#define PORT_COM2		2
#define PORT_COM3		3
#define	REMOTE_INDEX_SUM	4
#define	PRINT_COM	2
/*远摇控制结构体配置文件*/
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
	/*水平方向*/
	HVSSlid HSSlid;
	/*垂直方向*/
	HVSSlid VSSlid;
	/*指令地址*/
	ADDR Addr;
	/*控制命令*/
	CTRLCODE CCode[CTRL_CODE_MAX_NUM];
	/*串口属性*/
	COMPARAM comm;
} RemoteConfig;

typedef struct _REMOTE_INFO_
{
	int type;
	int speed;
	int caddr;
	int prezition;
	int reservation;
}REMOTE_INFO;


/*设置通讯管道，用于串口数据的读写*/
extern void InitPipe(int index);
/*从管道读数据�*/
extern int ReadFromPipe(unsigned char *buf, int count, int index);
/*把串口收到的数据写入管道*/
extern int WriteToPipe(unsigned char *data, int count, int index);
/*摄像头的控制*/
extern int CameraControl();
/*初始化远摇的配置文件*/
extern int InitRemoteStruct(int index);

/*摄像头初始化*/
extern int CameraCtrlInit(int index);
extern int InitPort(int port_num, int baudrate, int databits, int stopbits, int parity);
/*远摇API */
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


