#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include <mcfw/src_linux/osa/inc/osa_thr.h>

#include "common.h"
#include "serial.h"




int set_com_parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if(tcgetattr(fd, &options)  !=  0) {
		DEBUG(DL_ERROR, "setup serial 1");
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
			DEBUG(DL_ERROR, "unsupported data size\n");
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
			DEBUG(DL_ERROR, "unsupported parity\n");
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
			DEBUG(DL_ERROR, "unsupported stop bits\n");
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
		DEBUG(DL_ERROR, "setup serial 3");
		return (FALSE);
	}

	return (TRUE);
}

void close_com_port(int fd)
{
	if(fd != -1) {
		close(fd);
	}
}


void set_com_speed(int fd, int speed)
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
				DEBUG(DL_ERROR, "tcsetattr fd\n");
				return;
			}

			tcflush(fd, TCIOFLUSH);
		}
	}
}


int open_com_port(int port_num)
{
	char port[4][20] = {{"/dev/ttyS0"}, {"/dev/ttyO1"}, {"/dev/ttyS2"}, {"/dev/ttyS3"}};
	int fd;

	if(port_num > 3 || port_num < 0) {
		DEBUG(DL_ERROR, "port num error:%d\n", port_num);
		return -1;
	}

	if((fd = open(port[port_num], O_RDWR)) < 0) {
		DEBUG(DL_ERROR, "error: failed to open %s, errno=%d\n", port[port_num], errno);
		return -1;
	} else {
		DEBUG(DL_ERROR, "open %s success!\n", port[port_num]);
	}

	return fd;
}


int init_com_port(int port_num, int baudrate, int databits, int stopbits, int parity)
{
	int fd;

	fd = open_com_port(port_num);

	if(fd == -1) {
		return -1;
	}

	set_com_speed(fd, baudrate);

	if(set_com_parity(fd, databits, stopbits, parity) == FALSE) {
		DEBUG(DL_ERROR, "set_com_parity error!\n");
		close_com_port(fd);
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

static int send_data_to_com(int fd, unsigned char *data, int len)
{
	unsigned long real_len = 0 ;

	if((real_len = writen(fd, data, len)) != len) {
		DEBUG(DL_ERROR, "send_data_to_com() write tty error\n");
		return -1;
	}

	usleep(20000);
	return (real_len);
}


int send_order(int fd, int nFlag, int nParam)
{
	unsigned char data[4];
	int length = 0, ret = -1;

	memset(data, 0, sizeof(data));

	data[0] = 0xF8;
	data[1] = nFlag;
	length = 2;

	if(nFlag == FLAG_SETCURSOR) {
		data[2] = nParam;
		length++;
	}

	ret = send_data_to_com(fd, data, length);

	if(ret == -1) {
		DEBUG(DL_ERROR, "send_order()::send_data_to_com() send error !!!\n");
		return ret;
	}

	return (ret);

}

static int __send_data(int fd, unsigned char *pdata, int nLen, int nFlag)
{
	int nLength = nLen + 4;
	unsigned char send[300];
	int ret;

	if((nFlag & FLAG_CENTER) == FLAG_CENTER) {
		if(MAX_DISPLAY_CHAR > nLen) {
			send_order(fd, FLAG_SETCURSOR, (MAX_DISPLAY_CHAR - nLen) / 2 + (((nFlag & FLAG_NEWLINE) == FLAG_NEWLINE) ? TWO_LINE_POS : ONE_LINE_POS));
		} else {
			send_order(fd, FLAG_SETCURSOR, (((nFlag & FLAG_NEWLINE) == FLAG_NEWLINE) ? TWO_LINE_POS : ONE_LINE_POS));
		}
	} else if((nFlag & FLAG_LEFT) == FLAG_LEFT) {
		send_order(fd, FLAG_SETCURSOR, (((nFlag & FLAG_NEWLINE) == FLAG_NEWLINE) ? TWO_LINE_POS : ONE_LINE_POS));
	} else if((nFlag & FLAG_RIGHT) == FLAG_RIGHT) {
		if(16 > nLen) {
			send_order(fd, FLAG_SETCURSOR, MAX_DISPLAY_CHAR - nLen + (((nFlag & FLAG_NEWLINE) == FLAG_NEWLINE) ? TWO_LINE_POS : ONE_LINE_POS));
		} else {
			send_order(fd, FLAG_SETCURSOR, (((nFlag & FLAG_NEWLINE) == FLAG_NEWLINE) ? TWO_LINE_POS : ONE_LINE_POS));
		}
	}

	memset(send, 0, nLength);
	send[0] = 0xF8;
	send[1] = 0x03;
	memcpy(send + 2, pdata, nLen);
	send[nLen + 2] = 0xA0;

	ret = send_data_to_com(fd, send, nLength);

	if(ret == -1) {
		DEBUG(DL_ERROR, "__send_data()::send_data_to_com() send error!!!\n");
		return (ret);
	}

	return (ret);
}


int send_data(int fd, char *pdata, int uflag)
{
	return (__send_data(fd, (unsigned char *)pdata, strlen(pdata), uflag));
}

