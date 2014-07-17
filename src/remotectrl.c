/*********************************************************************

Co.Name	:	   Reach Software
FileName:		   remotectrl2.c
Creater：     yangshh                    Date: 2010-08-18
Function：	  Remote Control Module
Other Description:	Control Camera	for com port

**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>
#include <mcfw/src_linux/osa/inc/osa_thr.h>

#include "common.h"


#include "remotectrl.h"
#include "log_common.h"
#include "rwini.h"
#include "new_tcp_com.h"
/*remote config File */
int gRemoteFD[REMOTE_INDEX_SUM] = { -1, -1, -1, -1};
static RemoteConfig gRemote[REMOTE_INDEX_SUM];
static int compipe[REMOTE_INDEX_SUM][2];
static int protoleindex[REMOTE_INDEX_SUM] = {4, 4, 4, 4};
REMOTE_INFO   g_remote_info[2];
char FarCtrlList[PROTNUM][20] = {{"EVI-D70"},	//protocol for remote
	{"EVI-D100"},
	{"PELCO-D"},
	{"PELCO-P"},
	{"VCC-H80PI"},
	{"AW-HE50"},
	{""}
};
const char const serial_ports[4][20] = {
	{"/dev/ttyO1"},
	{"/dev/ttyO0"},
	{"/dev/ttyO2"},
	{"/dev/ttyO3"}
};


void InitPipe(int index)
{
	if(pipe(compipe[index])) {
		PRINTF("Create gRemote pipe error/n");
	};

	int flag = fcntl(compipe[index][0], F_GETFL, 0);

	flag |= O_NONBLOCK;

	if(fcntl(compipe[index][0], F_SETFL, flag) < 0) {
		PRINTF("Set  O_NONBLOCK pipe error/n");

	}

	return;
}
int ReadFromPipe(unsigned char *buf, int count, int index)
{
	return read(compipe[index][0], buf, count);
}
int WriteToPipe(unsigned char *data, int count, int index)
{
	return write(compipe[index][1], data, count);
}
void SetSpeed(int fd, int speed)
{
	int i;
	int status;
	struct termios Opt;
	int speed_arr[] = { B115200, B38400, B19200, B9600, B4800, B2400, B1200, B300,
	                    B38400, B19200, B9600, B4800, B2400, B1200, B300,
	                  };
	int name_arr[] = {115200, 38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,
	                  19200,  9600, 4800, 2400, 1200,  300,
	                 };

	tcgetattr(fd, &Opt);

	for(i = 0;  i < sizeof(speed_arr) / sizeof(int);  i++) {
		if(speed == name_arr[i]) {
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);

			if(status != 0) {
				ERR_PRN("tcsetattr fd\n");
				return;
			}

			tcflush(fd, TCIOFLUSH);
		}
	}
}

int SetParity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if(tcgetattr(fd, &options)  !=  0) {
		ERR_PRN("SetupSerial 1");
		return(FALSE);
	}

	options.c_cflag &= ~CSIZE;
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	options.c_oflag  &= ~OPOST;   			/*Output*/
	options.c_iflag   &= ~IXON;  			//0x11
	options.c_iflag   &= ~ICRNL;  			//0x0d

	switch(databits) {
		case 7:
			options.c_cflag |= CS7;
			break;

		case 8:
			options.c_cflag |= CS8;
			break;

		default:
			ERR_PRN("Unsupported data size\n");
			return (FALSE);
	}

	switch(parity) {
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;  	/* Clear parity enable */
			options.c_iflag &= ~INPCK;	 	/* Enable parity checking */
			break;

		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;    	/* Disnable parity checking */
			break;

		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     	/* Enable parity */
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;

		case 'S':
		case 's':  							/*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;

		default:
			ERR_PRN("Unsupported parity\n");
			return (FALSE);
	}

	switch(stopbits) {
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;

		case 2:
			options.c_cflag |= CSTOPB;
			break;

		default:
			ERR_PRN("Unsupported stop bits\n");
			return (FALSE);
	}

	/* Set input parity option */
	if(parity != 'n') {
		options.c_iflag |= INPCK;
	}

	tcflush(fd, TCIFLUSH);
	options.c_cc[VTIME] = 0;
	options.c_cc[VMIN] = 1; 				/* define the minimum bytes data to be readed*/

	if(tcsetattr(fd, TCSANOW, &options) != 0) {
		ERR_PRN("SetupSerial 3");
		return (FALSE);
	}

	return (TRUE);
}

int OpenPort(int port_num)
{
	int fd;

	if(port_num > 3 || port_num < 0) {
		ERR_PRN("serial_ports num error:%d\n", port_num);
		return -1;
	}

	if((fd = open(serial_ports[port_num], O_RDWR)) < 0) {
		ERR_PRN("ERROR: failed to open %s, errno=%d\n", serial_ports[port_num], errno);
		return -1;
	} else {
		PRINTF("Open %s success!\n", serial_ports[port_num]);
	}

	return fd;
}

void ClosePort(int fd)
{
	if(fd != -1) {
		close(fd);
	}
}

int InitPort(int port_num, int baudrate,
             int databits, int stopbits, int parity)
{
	int fd;

	fd = OpenPort(port_num);

	if(fd == -1) {
		return -1;
	}

	SetSpeed(fd, baudrate);

	if(SetParity(fd, databits, stopbits, parity) == FALSE) {
		ERR_PRN("Set Parity Error!\n");
		ClosePort(fd);
		return -1;
	}

	return fd;
}
static unsigned long writen(int fd, const void *vptr, size_t n)
{
	unsigned long nleft;
	unsigned long nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;

	while(nleft > 0) {
		if((nwritten = write(fd, ptr, nleft)) <= 0) {
			if(nwritten < 0 && errno == EINTR) {
				nwritten = 0;    /* and call write() again */
			} else {
				return(-1);    /* error */
			}
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}

	return(n);
}
/* end writen */
/*send data to tty com*/
int SendDataToCom(int fd, unsigned char *data, int len)
{
	unsigned long real_len = 0 ;

	if((real_len = writen(fd, data, len)) != len) {
		ERR_PRN("SendDataToCom() write tty error\n");
		return -1;
	}

	usleep(20000);
	return (real_len);
}

/*initial camera*/
int InitCamera(int fd)
{
	unsigned char comm[16];
	int length = 0;
	int ret = -1;

	if(fd == -1) {
		goto INITCEXIT;
	}

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x88;
	comm[1] = 0x01;
	comm[2] = 0x00;
	comm[3] = 0x01;
	comm[4] = 0xFF;
	length = 5;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);
	memset(comm, 0, sizeof(comm));
	comm[0] = 0x88;
	comm[1] = 0x30;
	comm[2] = 0x01;
	comm[3] = 0xff;
	length = 4;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	memset(comm, 0, sizeof(comm));
	comm[0] = 0x81;
	comm[1] = 0x09;
	comm[2] = 0x00;
	comm[3] = 0x02;
	comm[4] = 0xFF;
	length = 5;
	ret = SendDataToCom(fd, comm, length);

	if(ret == -1) {
		goto INITCEXIT;
	}

	usleep(200);

	PRINTF("InitCamera() Succeed\n");
	return 0;
INITCEXIT:
	ERR_PRN("InitCamera() Error\n");
	return -1;
}
int CCtrlCodeAnalysis1(BYTE *pCode, char *strCode, BYTE Address, BYTE hSpeed, BYTE vSpeed, short nHRange, short nWRange)
{
	char pstr[16];

	int nIndex = 0;
	char *ptmp = 0;
	char *ptmp1 = 0;
	int nHIndex = 3;
	int nWIndex = 3;

	ptmp1 = strCode;
	ptmp = strstr(strCode, ",");
	memset(pstr, 0, 16);

	while(ptmp != NULL) {
		strncpy(pstr, ptmp1, ptmp - ptmp1);

		if(pstr[0] == 0x5e) {
			pCode[nIndex] = Address;
		} else if(pstr[0] == 'H' && pstr[1] == '(') {
			pCode[nIndex] = hSpeed;
		} else if(pstr[0] == 'V' && pstr[1] == '(') {
			pCode[nIndex] = vSpeed;
		} else if(pstr[0] == '0' && pstr[1] == 'Y') {
			pCode[nIndex] = (nWRange >> (4 * nWIndex)) & 0xf;
			nWIndex--;
		} else if(pstr[0] == '0' && pstr[1] == 'Z') {
			pCode[nIndex] = (nHRange >> (4 * nHIndex)) & 0xf;
			nHIndex--;
		} else {
			pCode[nIndex] = strtol(pstr, 0, 16);
		}

		nIndex++;
		ptmp1 = ptmp + 1;
		ptmp = strstr(ptmp + 1, ",");
		memset(pstr, 0, 16);
	}

	strcpy(pstr, ptmp1);

	if(pstr[0] == '=') {
		if(pstr[1] == '7') {
			pCode[nIndex] = pCode[1] ^ pCode[2] ^ pCode[3] ^ pCode[4] ^ pCode[5] ^ pCode[6];
		}
	} else if(pstr[0] == '+') {
		if(pstr[1] == '5') {
			pCode[nIndex] = (pCode[1] + pCode[2] + pCode[3] + pCode[4] + pCode[5]) % 256;
		}
	} else {
		pCode[nIndex] = strtol(pstr, 0, 16);
	}

	return nIndex + 1;
}
/*control command Analysis*/
int CCtrlCodeAnalysis(unsigned char *pCode, char *strCode,
                      unsigned char Address, unsigned char hSpeed,
                      unsigned char vSpeed)
{
	char pstr[16];
	int nIndex = 0;
	char *ptmp = 0;
	char *ptmp1 = 0;

	memset(pstr, 0, 16);

	ptmp1 = strCode;
	ptmp = strstr(strCode, ",");

	while(ptmp != NULL) {
		strncpy(pstr, ptmp1, ptmp - ptmp1);

		if(pstr[0] == 0x5e) {
			pCode[nIndex] = Address;
		} else if(pstr[0] == 'H' && pstr[1] == '(') {
			pCode[nIndex] = hSpeed;
		} else if(pstr[0] == 'V' && pstr[1] == '(') {
			pCode[nIndex] = vSpeed;
		} else {
			pCode[nIndex] = strtol(pstr, 0, 16);
		}

		nIndex++;
		ptmp1 = ptmp + 1;
		ptmp = strstr(ptmp + 1, ",");
		memset(pstr, 0, 16);
	}

	strcpy(pstr, ptmp1);

	if(pstr[0] == '=') {
		if(pstr[1] == '7') {
			pCode[nIndex] = pCode[0] ^ pCode[1] ^ pCode[2] ^ pCode[3] ^ pCode[4] ^ pCode[5] ^ pCode[6];
		}
	} else if(pstr[0] == '+') {
		if(pstr[1] == '5') {
			pCode[nIndex] = (pCode[1] + pCode[2] + pCode[3] + pCode[4] + pCode[5]) % 256;
		}
	} else {
		pCode[nIndex] = strtol(pstr, 0, 16);
	}

	return nIndex + 1;
}



/*HE50 摄像头速度索引表*/
int HE50CameraIndex(int speed, int way)
{
	int index = 0;

	switch(way) {
		case UPSTART:
			switch(speed) {
				case 1:
					index = 6;
					break;

				case 5:
					index = 7;
					break;

				case 10:
					index = 9;
					break;

				default:
					index = 7;
					break;
			}

			break;

		case DOWNSTART:
			switch(speed) {
				case 1:
					index = 4;
					break;

				case 5:
					index = 2;
					break;

				case 10:
					index = 1;
					break;

				default:
					index = 2;
					break;
			}

			break;

		case LEFTSTART:
			switch(speed) {
				case 1:
					index = 4;
					break;

				case 5:
					index = 2;
					break;

				case 10:
					index = 1;
					break;

				default:
					index = 2;
					break;
			}

			break;

		case RIGHTSTART:
			switch(speed) {
				case 1:
					index = 6;
					break;

				case 5:
					index = 7;
					break;

				case 10:
					index = 9;
					break;

				default:
					index = 7;
					break;
			}

			break;

		case DDECSTART:
			switch(speed) {
				case 1:
					index = 4;
					break;

				case 5:
					index = 2;
					break;

				case 10:
					index = 1;
					break;

				default:
					index = 2;
					break;
			}

			break;

		case DADDSTART:
			switch(speed) {
				case 1:
					index = 6;
					break;

				case 5:
					index = 7;
					break;

				case 10:
					index = 9;
					break;

				default:
					index = 7;
					break;
			}

			break;

		default:
			index = 7;
			PRINTF("[HE50CameraIndex] Error way = %d \n", way);
			break;
	}

	return (index);
}

/*
##	Camera control up start
##  fd ------handle operation
##  addr-----Camera operation address
##	speed----Camera operation speed
##
*/
int CCtrlUpStart(int fd, int addr, int speed, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, vSpeed, hSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[UPSTART].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		speed = HE50CameraIndex(speed, UPSTART);
		/*vSpeed=5    5=8*/
		vSpeed = gRemote[index].VSSlid.speed[speed];
		/*vSpeed=5    5=8*/
		hSpeed = gRemote[index].HSSlid.speed[speed];
		/*Address1='#'*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, vSpeed);
		PRINTF("speed=%d Vspeed=%x  hSpeed = %x addr = %d nAddr = %x\n", speed,
		       vSpeed, hSpeed, addr, nAddr);
	} else {
		/*vSpeed=5    5=8*/
		vSpeed = gRemote[index].VSSlid.speed[speed];
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, vSpeed, vSpeed);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlUpStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlUpStart()->SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlUpStart() End  size = %d\n\n", size);
	return 0;

}

/*
##	Camera control UP stop
##  fd ------handle operation
##  addr-----Camera operation address
##
*/
int CCtrlUpStop(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[UPSTOP].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		/*Address1=23*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0x35, 0x30);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlUpStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlUpStop() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlUpStop() End\n");
	return 0;

}


/*
##	Remote control down start
##  fd ------handle operation
##  addr-----Camera address operation
##	speed----Camera trans speed
##
*/
int CCtrlDownStart(int fd, int addr, int speed, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, vSpeed, hSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[DOWNSTART].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		speed = HE50CameraIndex(speed, DOWNSTART);
		/*vSpeed=5    5=8*/
		vSpeed = gRemote[index].VSSlid.speed[speed];

		PRINTF("speed:[%d] vSpeed:[%d]\n", speed, vSpeed);
		/*vSpeed=5    5=8*/
		hSpeed = gRemote[index].HSSlid.speed[speed];

		PRINTF("speed:[%d] vSpeed:[%d]\n", speed, vSpeed);
		/*Address1='#'*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, vSpeed);
	} else {
		/*vSpeed=5*/
		vSpeed = gRemote[index].VSSlid.speed[speed];
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, vSpeed);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlDownStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlDownStart()->SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlDownStart() End\n");
	return 0;

}

/*
##	Remote control down Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlDownStop(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[DOWNSTOP].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		/*Address1=23*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0x35, 0x30);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlUpStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlUpStop() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlDownStop() End\n");
	return 0;

}


/*
##
##	Remote control Left Start
##  fd ------handle operation
##  addr-----Camera address operation
##	speed----Camera trans speed
##
##
*/
int CCtrlLeftStart(int fd, int addr, int speed, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, hSpeed, vSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[LEFTSTART].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		speed = HE50CameraIndex(speed, LEFTSTART);
		/*vSpeed=5    5=8*/
		vSpeed = gRemote[index].VSSlid.speed[speed];
		/*vSpeed=5    5=8*/
		hSpeed = gRemote[index].HSSlid.speed[speed];
		/*Address1='#'*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, vSpeed);
	} else {
		/*hSpeed=5*/
		hSpeed = gRemote[index].HSSlid.speed[speed];
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlLeftStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlLeftStart()->SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlLeftStart() End\n");
	return 0;

}

/*
##
##	Remote control Left Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlLeftStop(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[LEFTSTOP].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		/*Address1=23*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0x35, 0x30);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlLeftStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlLeftStop() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlLeftStop() End\n");
	return 0;

}

/*
##
##	Remote control right Start
##  fd ------handle operation
##  addr-----Camera address operation
##	speed----Camera trans speed
##
*/
int CCtrlRightStart(int fd, int addr, int speed, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, hSpeed, vSpeed, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[RIGHTSTART].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		speed = HE50CameraIndex(speed, RIGHTSTART);
		/*vSpeed=5    5=8*/
		vSpeed = gRemote[index].VSSlid.speed[speed];
		/*vSpeed=5    5=8*/
		hSpeed = gRemote[index].HSSlid.speed[speed];
		/*Address1='#'*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, vSpeed);
	} else {
		/*hSpeed=5*/
		hSpeed = gRemote[index].HSSlid.speed[speed];
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlRightStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlRightStart()->SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlRightStart() End\n");
	return 0;

}

/*
##
##	Remote control Right Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlRightStop(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[RIGHTSTOP].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		/*Address1=23*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0x35, 0x30);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlRightStop() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlRightStop() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlRightStop() End\n");
	return 0;

}

/*
##
##	Remote control Zoom out  start
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomAddStart(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;
	int speed = 5, vSpeed, hSpeed;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[DADDSTART].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		speed = HE50CameraIndex(speed, DADDSTART);
		/*vSpeed=5    5=8*/
		vSpeed = gRemote[index].VSSlid.speed[speed];
		/*vSpeed=5    5=8*/
		hSpeed = gRemote[index].HSSlid.speed[speed];
		/*Address1='#'*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, vSpeed);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlZoomAddStart() End\n");
	return 0;

}


/*
##	Remote control Zoom out Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomAddStop(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[DADDSTOP].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		/*Address1=23*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0x35, 0x30);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlZoomAddStart() End\n");
	return 0;

}

/*
##
##	Remote control Zoom In start
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomDecStart(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;
	int vSpeed, hSpeed, speed = 5;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[DDECSTART].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		speed = HE50CameraIndex(speed, DDECSTART);
		/*vSpeed=5    5=8*/
		vSpeed = gRemote[index].VSSlid.speed[speed];
		/*vSpeed=5    5=8*/
		hSpeed = gRemote[index].HSSlid.speed[speed];
		/*Address1='#'*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, hSpeed, vSpeed);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlZoomAddStart() End\n");
	return 0;

}


/*
##
##	Remote control Zoom In Stop
##  fd ------handle operation
##  addr-----Camera address operation
##
*/
int CCtrlZoomDecStop(int fd, int addr, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[DDECSTOP].comm);

	if(strcmp((char *)gRemote[index].ptzName, "/etc/camera/AW-HE50.ini") == 0) {
		/*Address1=23*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0x35, 0x30);
	} else {
		/*Address1=81*/
		nAddr  = gRemote[index].Addr.addr[addr];
		size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);
	}

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlZoomAddStart() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlZoomAddStart() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlZoomAddStart() End\n");
	return 0;

}

int CCtrlPositionPreset(int fd, int addr, int position, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[POSITIONPRESET].comm);
	/*Address1=81*/
	nAddr  = gRemote[index].Addr.addr[addr];

	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlPositionPreset() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	pCode[5] = position; //第5位表示预置位
	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlPositionPreset() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlPositionPreset() End\n");
	return 0;

}
int CCtrlSetPositionPreset(int fd, int addr, int position, int index)
{
	unsigned char pCode[16];
	char strCode[200];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;

	memset(pCode, 0, 16);
	memset(strCode, 0, 200);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);

	/*Up_Start=*/
	strcpy(strCode, gRemote[index].CCode[POSITIONPRESETSET].comm);
	/*Address1=81*/
	nAddr  = gRemote[index].Addr.addr[addr];
	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		ERR_PRN("CCtrlPositionPreset() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	pCode[5] = position; //第5位表示预置位
	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		ERR_PRN("CCtrlPositionPreset() SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("CCtrlPositionPreset() End\n");
	return 0;

}
int GetZoomPos(int fd, int addr, int index)
{
	BYTE pCode[16];
	char strCode[256];
	char strKey[64];
	char strtmp[8];
	int nAddr, size, ret = -1;
	static short nPos = 0;
	memset(pCode, 0, 16);
	memset(strCode, 0, 256);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);
	strcpy(strCode, gRemote[index].CCode[ZOOMPOSQUERY].comm);
	nAddr  = gRemote[index].Addr.addr[addr];
	size = CCtrlCodeAnalysis(pCode, strCode, nAddr, 0, 0);

	if(size <= 0 || fd == -1) {
		PRINTF("GetZoomPos() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		PRINTF("GetZoomPos() SendDataToCom() Error\n");
		return -1;
	}

	usleep(2000);
	memset(pCode, 0, 16);
	ret = ReadFromPipe(pCode, 16, index);

	if(ret < 0) {
		PRINTF("GetZoomPos() read from pip Error\n");
		//return nPos;
	}

	//PRINTF("0=%2x  %2x  %2x  %2x  %2x  %2x  %2x  ret=%d\n",pCode[0],pCode[1],pCode[2],pCode[3],pCode[4],pCode[5],pCode[6],ret);

	if(ret == 7) {
		nPos = 0;
		nPos |= pCode[2];
		nPos = nPos << 4;
		nPos |= pCode[3];
		nPos = nPos << 4;
		nPos |= pCode[4];
		nPos = nPos << 4;
		nPos |= pCode[5];

		//return nPos;
	}

	return nPos;

}

int CCMovetoPos(int fd, int addr, RECT rc, POINT pt, int index)
{
	int nWidth = rc.right - rc.left;
	int nHeight = rc.bottom - rc.top;
	int nShiftH = pt.y - rc.top - nHeight / 2;
	int nShiftW = pt.x - rc.left - nWidth / 2;
	int nAddr, size, ret = -1;
	double nZoomPos1;
	BYTE pCode[16];
	char strCode[256];
	char strKey[64];
	char strtmp[8];
	double Range;
	short RangeH;
	short RangeW;
	short nZoomPos = GetZoomPos(fd, addr, index);

	if(nZoomPos < 0) {
		return 0;
	}

	if(nZoomPos <= 0x5000) {
		nZoomPos1 = 3.141 + tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	} else if(nZoomPos <= 0x6000) {
		nZoomPos1 = 3.141 + (double)(3.14 / (double)4) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	} else {
		nZoomPos1 = 3.141 + (double)(3.14 / (double)16) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	}

	Range = (double)2592 * atan(1.8 / (double)nZoomPos1) / 3.141;

	RangeH = -(double)(Range * ((double)nShiftH * 2 / (double)nHeight) * 3 / (double)4);
	RangeW = (double)(Range * ((double)nShiftW * 2 / (double)nWidth));

	memset(pCode, 0, 16);
	memset(strCode, 0, 256);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);
	strcpy(strCode, gRemote[index].CCode[REL_POSITION].comm);
	nAddr  = gRemote[index].Addr.addr[addr];
	size = CCtrlCodeAnalysis1(pCode, strCode, nAddr, 14, 14, RangeH, RangeW);

	if(size <= 0 || fd == -1) {
		PRINTF("CCMovetoPos() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		PRINTF("CCMovetoPos() SendDataToCom() Error\n");
		return -1;
	}

	return 0;
}

int CCZoomInRect(int fd, int addr, RECT rc, RECT childRc, int index)
{
	POINT pt;
	int nAddr, size, ret = -1;
	BYTE pCode[16];
	char strCode[256];
	char strKey[64];
	char strtmp[8];
	double RangeRate;
	double Range;
	short nZoomPos = 0;
	double nZoomPos1;
	nZoomPos = GetZoomPos(fd, addr, index);

	if(nZoomPos < 0) {
		return 0;
	}

	pt.x = childRc.left + (childRc.right - childRc.left) / 2;
	pt.y = childRc.top + (childRc.bottom - childRc.top) / 2;
	CCMovetoPos(fd, addr, rc, pt, index);

	if(nZoomPos <= 0x5000) {
		nZoomPos1 = 3.141 + tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	}

	else if(nZoomPos <= 0x6000) {
		nZoomPos1 = 3.141 + (double)(3.141 / (double)4) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	} else {
		nZoomPos1 = 3.141 + (double)(3.141 / (double)16) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * tan((double)nZoomPos * 3.141 / (double)32768 / 2) * 27.9/*(*nZoomPos/16374)*/;
	}

	Range = (double)360 * atan(1.8 / (double)nZoomPos1) / 3.14;

	RangeRate = (double)((double)(childRc.bottom - childRc.top) / (double)(rc.bottom - rc.top)) > (double)((double)(childRc.right - childRc.left) / (double)(rc.right - rc.left)) ? (double)(childRc.bottom - childRc.top) / (double)(rc.bottom - rc.top) : (double)(childRc.right - childRc.left) / (double)(rc.right - rc.left);

	Range = Range * RangeRate;
	nZoomPos1 = 1.8 / tan(Range * 3.141 / 360);
	nZoomPos = 2 * 32768 * atan((nZoomPos1 - 3.1) / 27.9) / 3.141;

	memset(pCode, 0, 16);
	memset(strCode, 0, 256);
	memset(strKey, 0, 64);
	memset(strtmp, 0, 8);
	strcpy(strCode, gRemote[index].CCode[DDEC_DIRECT].comm);
	nAddr  = gRemote[index].Addr.addr[addr];
	size = CCtrlCodeAnalysis1(pCode, strCode, nAddr, 14, 14, nZoomPos, 0);

	if(size <= 0 || fd == -1) {
		PRINTF("CCZoomInRect() CCtrlCodeAnalysis Error\n");
		return -1;
	}

	ret = SendDataToCom(fd, pCode, size);

	if(ret == -1) {
		PRINTF("CCZoomInRect() SendDataToCom() Error\n");
		return -1;
	}

	return 0;

}
/*Trans parity*/
int TransParity(int parity)
{
	int ch = 'N';

	switch(parity)	{
		case 0:
			ch = 'N';			//none
			break;

		case 1:
			ch = 'O';    		//odd
			break;

		case 2:
			ch = 'E';			//even
			break;

		default:
			ERR_PRN("TransParity() parity:%d  Error\n", parity);
			break;
	}

	return ch;
}


/*Initial remote config file*/
int InitRemoteStruct(int index)
{
	int cnt, ret;
	char filename[50], buf[60], buf1[20];

	if((protoleindex[index] < 0) && (protoleindex[index] > PROTNUM - 1)) {
		protoleindex[index] = 0;
	}

	memset(&gRemote[index], 0, sizeof(gRemote[index]));
	/*Camera No.*/
	strcpy(filename, "/etc/camera/");
	strcat(filename, FarCtrlList[protoleindex[index]]);
	strcat(filename, ".ini");

	strcpy((char *)gRemote[index].ptzName, filename);

	PRINTF("Config File: %s\n", filename);

	/*Speed control*/
	for(cnt = 1; cnt < SLID_MAX_NUM; cnt++) {
		memset(buf, 0, sizeof(buf));
		memset(buf1, 0, sizeof(buf1));
		sprintf(buf1, "%d", cnt);
		ret = ConfigGetKey(filename, "HSeepSlid", buf1, buf);

		if(ret == -10) {    //file no exist
			ERR_PRN("file no exist %s\n", gRemote[index].ptzName);
			return -1;
		}

		if(ret != 0) {
			continue;
		}

		gRemote[index].HSSlid.speed[cnt] = strtol(buf, 0, 16);
		memset(buf, 0, sizeof(buf));
		memset(buf1, 0, sizeof(buf1));
		sprintf(buf1, "%d", cnt);
		ret = ConfigGetKey(filename, "VSeepSlid", buf1, buf);

		if(ret != 0) {
			continue;
		}

		gRemote[index].VSSlid.speed[cnt] = strtol(buf, 0, 16);

	}

	/*operation addr*/
	for(cnt = 1; cnt < ADDR_MAX_NUM ; cnt++)  {
		memset(buf, 0, sizeof(buf));
		memset(buf1, 0, sizeof(buf1));
		sprintf(buf1, "Address%d", cnt);
		ret = ConfigGetKey(filename, "Address", buf1, buf);

		if(ret != 0) {
			continue;
		}

		gRemote[index].Addr.addr[cnt] = strtol(buf, 0, 16);;
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Up_Start", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[UPSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Up_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[UPSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Down_Start", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[DOWNSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Down_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[DOWNSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Left_Start", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[LEFTSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Left_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[LEFTSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Right_Start", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[RIGHTSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Right_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[RIGHTSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DADD_Start", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[DADDSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DADD_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[DADDSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DDEC_Start", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[DDECSTART].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DDEC_Stop", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[DDECSTOP].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Rel_position", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[REL_POSITION].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "DDEC_Direct", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[DDEC_DIRECT].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "ZoomPosQuery", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[ZOOMPOSQUERY].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "Pan_TiltPosQuery", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[PAN_TILTPOSQUERY].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "PositionPreset", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[POSITIONPRESET].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "CtrlCode", "PositionPresetSet", buf);

	if(ret == 0) {
		strcpy(gRemote[index].CCode[POSITIONPRESETSET].comm, buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "StopBit", buf);

	if(ret == 0) {
		gRemote[index].comm.stopbits = atoi(buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "DataBit", buf);

	if(ret == 0) {
		gRemote[index].comm.databits = atoi(buf);
	}

	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "Baud", buf);

	if(ret == 0) {
		gRemote[index].comm.baud = atoi(buf);
	}


	memset(buf, 0, sizeof(buf));
	ret = ConfigGetKey(filename, "Comm", "Parity", buf);

	if(ret == 0) {
		gRemote[index].comm.parity = TransParity(atoi(buf));
	}

	return 0;

}

/*Camera control initial*/
int CameraCtrlInit(int index)
{
	SetRmtProtoleIndex(index, ReadRemoteCtrlIndex(index));
	InitRemoteStruct(index);
	//	int ret = -1;
	int baudrate = 9600, databits = 8;
	int stopbits = 1, parity = 'N';

	baudrate = gRemote[index].comm.baud;
	databits = gRemote[index].comm.databits;
	parity = gRemote[index].comm.parity;
	stopbits = gRemote[index].comm.stopbits;
	InitPipe(index);
	gRemoteFD[index] = InitPort(index, baudrate, databits, stopbits, parity);

	if(gRemoteFD[index] <= 0) {
		PRINTF("[CameraCtrlInit]  ERROR gRemoteFD[%d]:[%d]\n", index, gRemoteFD[index]);
		return -1;
	}

#if 0
	ret = InitCamera(gRemoteFD[index]);

	if(ret != 0) {
		printf("[CameraCtrlInit] gRemoteFD[%d]:[%d] InitCamera failed!!!\n", index, gRemoteFD[index]);
		return -1;
	}

#endif
	PRINTF("[CameraCtrlInit] gRemoteFD[%d]:[%d]\n", index, gRemoteFD[index]);
	return gRemoteFD[index];
}


void Print(BYTE *data, int len)
{
	int i;

	PRINTF("\n");

	for(i = 0; i < len; i++) {
		PRINTF("%2x  ", data[i]);
	}

	PRINTF("\n");
}
void FarCtrlCameraEx(int dsp, unsigned char *data, int len, int index)
{
	int type, speed, addr = 1;
	int fd;
	RECT rc;
	RECT rc2;
	POINT pt;
	type = (int)(data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24);
	speed = (int)(data[4] | data[5] << 8 | data[6] << 16 | data[7] << 24);

	if(len < 12) {
		addr = 1;
	} else {
		addr = (int)(data[8] | data[9] << 8 | data[10] << 16 | data[11] << 24);
	}

	if(addr < 1) {
		addr = 1;
	}

	memcpy(&rc, data + 12, sizeof(rc));
	memcpy(&pt, data + 12 + sizeof(rc), sizeof(pt));
	memcpy(&rc2, data + 12 + sizeof(rc), sizeof(rc2));

	if(gRemoteFD[index] <= 0) {
		ERR_PRN("Remote Port Open Failed\n");
	}

	fd = gRemoteFD[index];


	//CCMovetoPos(fd,addr,rc,pt);
	if(type == 13) {
		CCMovetoPos(fd, addr, rc, pt, index);
	} else if(type == 18) {

		CCZoomInRect(fd, addr, rc, rc2, index);
	}
}

int remote_ctrl_camera(int index, REMOTE_INFO *far_ctrl_info)
{
	int type, speed;
	int fd;
	int caddr = 1;
	int preset_bit;
	//Print(data, len);
	type = far_ctrl_info->type;
	speed = far_ctrl_info->speed;
	caddr  = far_ctrl_info->caddr;
	preset_bit = far_ctrl_info->prezition;

	if(caddr < 1 || caddr > 16) {
		caddr = 1;
	}


	PRINTF("index:[%d] type:%d,caddr=%d\n", index, type, caddr);
	PRINTF("speed:%d\n", speed);
	//	PRINTF("the camera addr = %d\n",caddr);
	memcpy(&g_remote_info[index], far_ctrl_info, sizeof(REMOTE_INFO));

	if(gRemoteFD[index] <= 0) {
		ERR_PRN("Remote Port Open Failed\n");
	}

	fd = gRemoteFD[index];

	switch(type) {
		case 2:		//UP Start
			CCtrlUpStart(fd, caddr, speed, index);
			break;

		case 16:	//UP stop
			CCtrlUpStop(fd, caddr, index);
			break;

		case 3:		//DOWN start
			CCtrlDownStart(fd, caddr, speed, index);
			break;

		case 17:	//DOWN stop
			CCtrlDownStop(fd, caddr, index);
			break;

		case 0:		// LEFT Start
			CCtrlLeftStart(fd, caddr, speed, index);
			break;

		case 14:	//LEFT Stop
			CCtrlLeftStop(fd, caddr, index);
			break;

		case 1:		//RIGHT Start
			CCtrlRightStart(fd, caddr, speed, index);
			break;

		case 15:    //RIGHT Stop
			CCtrlRightStop(fd, caddr, index);
			break;

		case 8:		//FOCUS ADD Start
			CCtrlZoomAddStart(fd, caddr, index);
			break;

		case 22:	//FOCUS ADD Stop
			CCtrlZoomAddStop(fd, caddr, index);
			break;

		case 9:		//FOCUS DEC Start
			CCtrlZoomDecStart(fd, caddr, index);
			break;

		case 23:	//FOCUS DEC Stop
			CCtrlZoomDecStop(fd, caddr, index);
			break;

		case 10:	//预置位，speed表示预置位号
			CCtrlPositionPreset(fd, caddr, preset_bit, index);
			break;

		case 11:	//设置预置位，speed表示预置位号
			CCtrlSetPositionPreset(fd, caddr, preset_bit, index);
			break;

		default:
			ERR_PRN("FarCtrlCamera() command Error\n");
			break;
	}

	return 0;
}


/*
** Remote
**Send MSG_FARCTRL Message
**MSG_FARCTRL message header
**4BYTES    for  Type
**4BYTES    fpr  Speed
Type
2            UP Start
16          UP Stop
3            DOWN Start
17          DOWN Stop
0            LEFT Start
14          LEFT Stop
1            RIGHT Start
15          RIGHT Stop
8            FOCUS ADD Start
22          FOCUS ADD Stop
9            FOCUS DEC Start
23          FOCUS DEC Stop
Speed
1            LOW
5            MID
10          HIGH

 */
void FarCtrlCamera(int index, unsigned char *data, int len, int dsp)
{
	int type, speed;
	int fd;
	int caddr = 1;

	//	Print(data, len);
	type = (int)(data[0] | data[1] << 8 | data[2] << 16 | data[3] << 24);
	speed = (int)(data[4] | data[5] << 8 | data[6] << 16 | data[7] << 24);

	if(len < 12) {
		caddr = 1;
	} else {
		caddr = (int)(data[8] | data[9] << 8 | data[10] << 16 | data[11] << 24);
	}

	if(caddr < 1 || caddr > 100) {
		caddr = 1;
	}

	PRINTF("***index:[%d] type:%d  speed:%d  len:%d,caddr	=%d,fd:%d ***\n", index, type, speed, len,caddr, gRemoteFD[index]);

	if(gRemoteFD[index] <= 0) {
		ERR_PRN("Remote Port Open Failed\n");
	}

	fd = gRemoteFD[index];

	switch(type) {
		case 2:		//UP Start
			CCtrlUpStart(fd, caddr, speed, index);
			break;

		case 16:	//UP stop
			CCtrlUpStop(fd, caddr, index);
			break;

		case 3:		//DOWN start
			CCtrlDownStart(fd, caddr, speed, index);
			break;

		case 17:	//DOWN stop
			CCtrlDownStop(fd, caddr, index);
			break;

		case 0:		// LEFT Start
			CCtrlLeftStart(fd, caddr, speed, index);
			break;

		case 14:	//LEFT Stop
			CCtrlLeftStop(fd, caddr, index);
			break;

		case 1:		//RIGHT Start
			CCtrlRightStart(fd, caddr, speed, index);
			break;

		case 15:    //RIGHT Stop
			CCtrlRightStop(fd, caddr, index);
			break;

		case 8:		//FOCUS ADD Start
			CCtrlZoomAddStart(fd, caddr, index);
			break;

		case 22:	//FOCUS ADD Stop
			CCtrlZoomAddStop(fd, caddr, index);
			break;

		case 9:		//FOCUS DEC Start
			CCtrlZoomDecStart(fd, caddr, index);
			break;

		case 23:	//FOCUS DEC Stop
			CCtrlZoomDecStop(fd, caddr, index);
			break;

		case 10:	//预置位，speed表示预置位号
			CCtrlPositionPreset(fd, caddr, speed, index);
			break;

		case 11:	//设置预置位，speed表示预置位号
			CCtrlSetPositionPreset(fd, caddr, speed, index);
			break;

		case 12://设置自动跟踪模块的手动和自动模式0自动1手动
			SetTrackStatus(fd, speed);
			break;

		case 13:
		case 18:
			FarCtrlCameraEx(fd, data, len, index);
			break;

		default:
			ERR_PRN("FarCtrlCamera() command Error\n");
			break;
	}

}

int SetTrackStatus(int fd, int val)
{
	unsigned char pCode[5];
	int ret;
	memset(pCode, 0, 5);

	if(fd == -1) {
		ERR_PRN("gRemoteFD=-1\n");
		return -1;
	}

	if(0 != val) {
		val = 0x3;
	}

	pCode[0] = 0xC0;
	pCode[1] = 0x00;
	pCode[2] = val;
	pCode[3] = 0xFF;
	pCode[4] = pCode[0] ^ pCode[1] ^ pCode[2] ^ pCode[3];
	ret = SendDataToCom(fd, pCode, 5);

	if(ret == -1) {
		ERR_PRN("SendDataToCom() Error\n");
		return -1;
	}

	PRINTF("SendCommand() End\n");
	return 0;
}

int ListenSeries(void *pParam, int index)
{
	unsigned char data[256];
	//	char szData[256];
	int len, i;
	fd_set rfds;

	while(1) {

		//#ifdef DSS_ENC_1100_1200_DEBUG
		if(gRemoteFD[index] <= 0) {
			sleep(1);
			continue;
		}

		//#endif
		FD_ZERO(&rfds);
		FD_SET(gRemoteFD[index], &rfds);
		select(gRemoteFD[index] + 1, &rfds, NULL, NULL, NULL);
		len = read(gRemoteFD[index], data, 256) ; //成功返回数据量大小，失败返回－1
		//PackHeaderMSG((BYTE *)szData, 0x11, HEAD_LEN + 1);
		//szData[HEAD_LEN] = data[3];

		if(data[0] == 0x08 && data[1] == 0x09  && data[2] == 0x08) {
			for(i = 0; i < len; i++) {
				//PRINTF("ReadDataFroCom:%x\n",data[i]);
			}


		} else if(data[0] == 0x90 && data[1] == 0x50  && data[len - 1] == 0xFF) {
			WriteToPipe(data, len, index);
			PRINTF("write(compipe[1], data, len=%d);\n", len);
		}
	}

	return len;
}

/*###########################################*/
#if 0
/*摄像头的控制*/
int CameraControl()
{
	int ret = -1;
	int fd;
	int baudrate = 9600, databits = 8;
	int stopbits = 1, parity = 'N';

	baudrate = gRemote.comm.baud;
	databits = gRemote.comm.databits;
	parity = gRemote.comm.parity;
	stopbits = gRemote.comm.stopbits;


	Printf("Initial succeed\n");
	fd = InitPort(PORT_COM2, baudrate, databits, stopbits, parity);
	ret = InitCamera(fd);

	if(ret != 0) {
		return -1;
	}

	/*	sleep(1);
		CCtrlUpStart(fd,1,10);
		sleep(10);
		CCtrlUpStop(fd,1);
		usleep(100);

		CCtrlDownStart(fd,1,10);
		sleep(10);
		CCtrlDownStop(fd,1);
		usleep(100);
	*/
	CCtrlLeftStart(fd, 1, 10);
	sleep(10);
	CCtrlLeftStop(fd, 1);
	/*	usleep(100);
		CCtrlRightStart(fd,1,10);
		sleep(20);
		CCtrlRightStop(fd,1);
		usleep(100);
		CCtrlZoomAddStart(fd,1);
		sleep(10);
		CCtrlZoomAddStop(fd,1);
		usleep(100);
		CCtrlZoomDecStart(fd,1);
		sleep(20);
		CCtrlZoomDecStop(fd,1);
		usleep(100);*/
	return 0;
}

#endif
int GetRmtProtoleIndex(int input)
{
	return protoleindex[input];
}
int SetRmtProtoleIndex(int input, int ProtoleIndex)
{
	protoleindex[input] = ProtoleIndex;
	return 0;
}

