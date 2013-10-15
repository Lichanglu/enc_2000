/*******************************************************************************
*     封装LOG日志后台的服务器端代码
*                                      add by zyb
*																										2011-02-06
*
*
********************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include<stdarg.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <strings.h>
#include "log_server.h"


static int g_audio_num = 0;
static CLIENTPARAM g_client_param;
int (*g_func)(char * , int, char * , int *);
//static callfunc *g_func = NULL;

#if 0
#define LPRINTF(X...)\
	do {														\
		PRINTF("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);			\
		PRINTF(X);											\
	} while(0)
#else
#define LPRINTF(X...)
#endif

#define LOG_HEAD_LEN			sizeof(LOGMSGHEAD)


static void *tcp_thread_recv(int *fd);
static void *tcp_thread_listen();
static void PackHeaderMSG(char *data, char type, unsigned short len);


static unsigned long mid_clock(void)
{
	//return mid_time_get_current_time();
	unsigned long msec;
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;

	return msec;
}


//通信添加 find the free socket _zhengyb
static int find_sock_f()
{
	int i_loop = 0;

	for(i_loop = 0 ; i_loop < SOCK_NUM ; i_loop++) {
		LPRINTF("i_loop = %d\n", i_loop);

		if(g_client_param.clidata[i_loop].login == 0 && g_client_param.clidata[i_loop].socket == 0) {
			return i_loop;
		}
	}

	if(i_loop == SOCK_NUM) {
		LPRINTF("there is no free socket!\n");
		return -1;
	}

	LPRINTF("hahah\n");

	return 0;
}



// 通信添加severce端发送函数  _zhengyb
static int WriteData(int s , void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			if((errno == ENOBUFS) && (nCount < 10)) {
				LPRINTF("network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;

			}

			return nRet;
		} else if(nRet == 0) {
			LPRINTF("Send Net Data Error nRet = %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	return 0;
}

//通信添加 server listen thread  _zhengyb
static void *tcp_thread_listen()
{
	int sock_listen = 0;
	int opt = 1;
	struct sockaddr_in serv_addr;
SERVER_RETURN:

	//	struct sockaddr_in client_addr;
	if((sock_listen = socket(AF_INET, SOCK_STREAM , 0)) < 0) {
		perror("sock create si error!\n");
		return 0;
	}

	LPRINTF("socket create is ok!\n");
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(LOG_SERVPORT);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	//设置重用
	setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));


	if(bind(sock_listen , (struct sockaddr *)&serv_addr, sizeof(struct sockaddr)) == -1) {
		perror("socket bind is error!\n");
		close(sock_listen);
		return 0;
	}

	LPRINTF("socket bind is ok!\n");


	if(listen(sock_listen, BACKLOG) < 0) {
		perror("socket listen is error!\n");
		close(sock_listen);
		return 0;
	}

	LPRINTF("socket [%d] is listening .......\n", sock_listen);

	LPRINTF("init_network_s is ok!\n");

	struct sockaddr_in client_addr;
	int sin_size = sizeof(struct sockaddr);
	int ack_sock = 0;

	while(1) {
		LPRINTF("while is benging....\n");
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(sock_listen, &rfds);
		memset(&client_addr, 0, sizeof(struct sockaddr_in));
		select(FD_SETSIZE , &rfds , NULL , NULL, NULL);
		LPRINTF("select \n");

		if(FD_ISSET(sock_listen, &rfds)) {
			if((ack_sock = accept(sock_listen , (struct sockaddr *)&client_addr, (socklen_t *)&sin_size)) < 0) {
				perror("socket accept is error!\n");

				if(errno == ECONNABORTED || errno == EAGAIN) { //软件原因中断
					//				DEBUG(DL_ERROR,"errno =%d program again start!!!\n",errno);
					usleep(100000);
					continue;
				}

				if(sock_listen > 0) {
					close(sock_listen);
				}

				goto SERVER_RETURN;
			}

			int sock_num = find_sock_f();

			if(-1 == sock_num) {
				//发送链接满的消息
				close(ack_sock);
				continue;
			}

			char client_ip[32];
			inet_ntop(AF_INET, (void *)&client_addr.sin_addr, client_ip, 32);
			LPRINTF("received a connetion from [%s]\n", client_ip);
			g_client_param.clidata[sock_num].socket =  ack_sock;

			pthread_t thread_id_recv;
			pthread_attr_t attr_recv;
			pthread_attr_init(&attr_recv);
			pthread_attr_setdetachstate(&attr_recv, PTHREAD_CREATE_DETACHED);
			pthread_create(&thread_id_recv, &attr_recv, (void *)tcp_thread_recv, (void *)&sock_num);
			pthread_attr_destroy(&attr_recv);
		}
	}

}
#define RECV_BUF_SIZE 2048*20
static unsigned int g_start_time = 0;
//通信添加 severce recv thread  _zhengyb
static void *tcp_thread_recv(int *fd)
{

	g_audio_num = 0;
	char *in = NULL;
	char out[RECV_BUF_SIZE] = {0};
	int inlen, outlen;
	int nLen = 0;
	char recv_buf[RECV_BUF_SIZE] = {0};
	int i = (int)(*fd);
	LPRINTF("i = %d\n", i);
	LOGMSGHEAD *phead;
	int csocket =  g_client_param.clidata[i].socket;

	while(1) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(csocket, &rfds);
		//接收recv_buf 复位为空!
		memset(recv_buf, 0, sizeof(recv_buf));
		select(FD_SETSIZE, &rfds , NULL , NULL , NULL);

		if(FD_ISSET(csocket, &rfds)) {
			FD_CLR(csocket, &rfds);
			phead = (LOGMSGHEAD *)recv_buf;
			int recv_len = recv(csocket, recv_buf, LOG_HEAD_LEN, 0);

			if(recv_len == -1 || recv_len < LOG_HEAD_LEN) {
				perror("communication error!\n");
				LPRINTF("the device time = %ld\n", mid_clock() - g_start_time);
				goto EXITCLIENT;
			}

			//message length
			phead->nLen = ntohs(phead->nLen);
			LPRINTF("phead->nMsg = 0x%02x=len = %d=recvlen= %d\n", phead->nMsg, phead->nLen, recv_len);

			if(phead->nLen - LOG_HEAD_LEN > 0) {
				nLen = 0;
				nLen = recv(csocket, recv_buf + LOG_HEAD_LEN, phead->nLen - LOG_HEAD_LEN, 0);
				LPRINTF("nMsgLen = %d,len:%d!\n", phead->nLen, nLen);

				if(nLen == -1 || nLen < phead->nLen - LOG_HEAD_LEN) {
					LPRINTF("nLen < nMsgLen -LOG_HEAD_LEN\n");
					goto EXITCLIENT;
				}
			}

			inlen = nLen;
			outlen = 0;
			in = recv_buf + LOG_HEAD_LEN;
			int length = 0;

			switch(phead->nMsg) {
				case MSG_PRINTF_INFO:
					if(g_func != NULL) {
						g_func(in, inlen, out + sizeof(LOGMSGHEAD), &outlen);
						LPRINTF("out=#%s#,len=%d\n", out + sizeof(LOGMSGHEAD), outlen);
						length = LOG_HEAD_LEN + outlen;

						PackHeaderMSG(out, MSG_PRINTF_INFO, length);
						send(csocket, out, length, 0);
					} else { //需要返回消息告知错误
						strcpy(out + LOG_HEAD_LEN, "WARNNING,the device is not support this cmd.\n");
						length = LOG_HEAD_LEN + strlen(out + LOG_HEAD_LEN);
						PackHeaderMSG(out, MSG_PRINTF_INFO, length);
						send(csocket, out, length, 0);
					}

					break;

				case MSG_DEBUG_OUT:
					break;

				case MSG_LOGIN:
					if(strcmp("reachlogin", in) == 0) {
						g_client_param.clidata[i].login = 1;
						LPRINTF("the client socket = %d is login\n", csocket);
					} else {
						goto EXITCLIENT;
					}

					length = LOG_HEAD_LEN ;
					PackHeaderMSG(out, MSG_LOGIN, length);
					send(csocket, out, length, 0);
					break;

				default:
					break;
			}
		}
	}

EXITCLIENT:
	close(csocket);
	g_client_param.clidata[i].socket = 0;
	g_client_param.clidata[i].login = 0;

	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

/*消息头打包*/
static void PackHeaderMSG(char *data, char type, unsigned short len)
{
	LOGMSGHEAD  *p;
	p = (LOGMSGHEAD *)data;
	memset(p, 0, sizeof(LOGMSGHEAD));
	p->nLen = htons(len);
	p->nMsg = type;
	return ;
}


static int g_printf_num = 0;
// 通信添加 server send测试用例    zhengyb
void test_log_send_to_client(char *buf, int buf_len)
{
	int i ;
	int length = 0;
	char temp[RECV_BUF_SIZE] = {0};

	if(g_printf_num == 62325) {
		g_printf_num = 0;
	} else {
		g_printf_num ++;
	}

	snprintf(temp + LOG_HEAD_LEN, sizeof(temp) - LOG_HEAD_LEN, "%05d:%s", g_printf_num, buf);
	length = LOG_HEAD_LEN + strlen(temp + LOG_HEAD_LEN);
	PackHeaderMSG(temp, MSG_DEBUG_OUT, length);

	for(i = 0 ; i < SOCK_NUM ; i++) {
		if(g_client_param.clidata[i].login == 1 && 	g_client_param.clidata[i].socket != 0) {
			WriteData(g_client_param.clidata[i].socket, temp, length);
		}
	}

	return ;
}


void test_server_send_audio_test(char *buf, int buf_len)
{
	int i ;
	int length = 0;
	char temp[RECV_BUF_SIZE] = {0};

	if(g_printf_num == 62325) {
		g_printf_num = 0;
	} else {
		g_printf_num ++;
	}

	//	snprintf(temp+LOG_HEAD_LEN,sizeof(temp)- LOG_HEAD_LEN,"%05d:%s",g_printf_num,buf);
	memcpy(temp + LOG_HEAD_LEN, buf, buf_len);
	length = LOG_HEAD_LEN + buf_len;

	PackHeaderMSG(temp, MSG_DEBUG_OUT, length);

	for(i = 0 ; i < SOCK_NUM ; i++) {
		if(g_client_param.clidata[i].login == 1 && 	g_client_param.clidata[i].socket != 0) {
			WriteData(g_client_param.clidata[i].socket, temp, length);
			g_audio_num++;

			if(g_audio_num % 100 == 0) {
				LPRINTF("i will send the audio buf len==%ld= %d= %d\n", (mid_clock() - g_start_time), g_audio_num, buf_len);
			}

			if(g_start_time == 0) {
				g_start_time = mid_clock();
				LPRINTF("i will send the audio buf len===%d= %d= %d\n", g_start_time, g_audio_num++, buf_len);
			}
		}
	}

	return ;
}



//extern int	app_log_porting_parse(char *in,int inlen,char *out,int *outlen);

int test_log_server_init()
{
	//添加 开启server端监听线程 start_thread_listen  _zhengyb
	pthread_t thread_id;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &attr, tcp_thread_listen, NULL);
	pthread_attr_destroy(&attr);
	g_func = NULL;
	return 0;
}

int test_log_porting_init(void *func)
{
	g_func = func;
	return 0;
}
