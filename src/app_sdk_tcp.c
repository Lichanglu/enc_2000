#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <linux/videodev2.h>

#include "middle_control.h"
#include "app_sdk_tcp.h"
#include "log_common.h"



#define PORT_SDK			1

#define SDK_LOGIN_USER 	"admin"
#define SDK_LOGIN_PASSWD 	"szreach"
#define MAX_CLIENT_NUM 8


#define THREAD_SUCCESS	0
#define THREAD_FAILURE	(void *)(-1)



static int g_client_num = 0;


extern int midParseString(int identifier, int fd, char *data, int len);


extern void msgPacket(int identifier, unsigned char *data, webparsetype type,
                      int len, int cmd, int ret);




int app_sdk_login_check(char *in, char *out)
{
	char temp[256] = {0};
	snprintf(temp, sizeof(temp), "%s/%s", SDK_LOGIN_USER, SDK_LOGIN_PASSWD);

	if(strcmp(temp, in) == 0) {
		//PRINTF("login check ok\n");
		return SERVER_RET_OK;
	} else {
		//PRINTF("login check failed\n");
		return SERVER_RET_USER_INVALIED;
	}
}







static int sdk_add_client_num()
{
	g_client_num ++;

	return 0;
}
static int sdk_del_client_num()
{
	g_client_num--;

	return 0;
}

static int sdk_get_client_num()
{
	return g_client_num;

	return 0;
}

/*WEB server Thead Function*/
void *sdk_clinet_thr(int *arg)
{
	PRINTF("get pid= %d\n", getpid());
	void                   *status              = THREAD_SUCCESS;
	//	int nPos;
	int len, client_sock;
	webMsgInfo		webinfo;
	int			recvlen;
	char  sdk_data[2048] = {0};
	int sdk_ret = SERVER_RET_OK;
	int out = 0;
	int totallen = MSGINFOHEAD + sizeof(int) + sizeof(sdk_ret) + sizeof(int);
	char sendBuf[1024] = {0};
	PRINTF("enter sdk_clinet_thr() function!!\n");

	client_sock = (int)(*arg);
	PRINTF("client_sock = %d\n", client_sock);

	while(1) {  //////////////////////////////////////
		if(client_sock <= 0) {
			goto ExitClientMsg;
		}

		memset(&webinfo, 0, sizeof(webinfo));
		recvlen = recv(client_sock, &webinfo, sizeof(webinfo), 0);
		PRINTF("recvlen = %d webinfo.len =%d webinfo.type = %d,id=%x\n",
		       recvlen, webinfo.len, webinfo.type, webinfo.identifier);

		if(recvlen < 1)	{
			PRINTF("recv failed,errno = %d,error message:%s,client_sock = %d\n", errno, strerror(errno), client_sock);
			status  = THREAD_FAILURE;
			sdk_ret = SERVER_RET_RECV_FAILED;
			msgPacket(WEB_IDENTIFIER, (unsigned char *)sendBuf, INT_TYPE, totallen, 0xffff, sdk_ret);
			memcpy(sendBuf + (totallen - sizeof(out)), &out, sizeof(out));
			PRINTF("up to max client num : %x\n", sdk_ret);
			send(client_sock, sendBuf, totallen, 0);

			goto ExitClientMsg;

		}

		if(webinfo.identifier != WEB_IDENTIFIER) {
			PRINTF("id  error,client_sock = %d\n", client_sock);
			status  = THREAD_FAILURE;
			sdk_ret = SERVER_RET_ID_ERROR;
			msgPacket(WEB_IDENTIFIER, (unsigned char *)sendBuf, INT_TYPE, totallen, 0xffff, sdk_ret);
			memcpy(sendBuf + (totallen - sizeof(out)), &out, sizeof(out));
			PRINTF("up to max client num : %x\n", sdk_ret);
			send(client_sock, sendBuf, totallen, 0);
			goto ExitClientMsg;
		}

		len = webinfo.len - sizeof(webinfo);

		PRINTF("web deal begin =%d\n", webinfo.type);

		switch(webinfo.type) {
			case INT_TYPE:
				//		midParseInt(WEB_IDENTIFIER,client_sock, sdk_data, len);
				//	sdkParseInt(client_sock, sdk_data, len);
				break;

			case STRING_TYPE:
				midParseString(WEB_IDENTIFIER, client_sock, sdk_data, len);
				//sdkParseString(client_sock, sdk_data, len, nPos);
				break;

			case STRUCT_TYPE:
				//		midParseStruct(WEB_IDENTIFIER,client_sock,sdk_data,len);
				//sdkParseStruct(client_sock, sdk_data, len);
				break;

			default:
				break;
		}

		PRINTF("web deal end =%d\n", webinfo.type);
	}


	PRINTF("Web listen Thread Function Exit!!\n");
ExitClientMsg:

	close(client_sock);
	sdk_del_client_num();

	pthread_detach(pthread_self());
	pthread_exit(NULL);
	PRINTF("i exit thread\n");

	return status;
}

/*TCP task mode*/
void listenSdkTask()
{
	PRINTF("get pid= %d\n", getpid());
	int sClientSocket;
	struct sockaddr_in SrvAddr, ClientAddr;
	//	pthread_t 		drawtimethread;
	int			ServSock = 0;
	int nLen;
	char newipconnect[20] = {0};
	//	int clientsocket = 0;
	//	void *ret = 0;
	//	int ipos = 0;
	int fileflags;
	int opt = 1;
	pthread_t sdk_client_thread_id[APP_SDK_MAX_CLIENT] = {0};
DSP1STARTRUN:

	PRINTF("listenSdkTask Task start...\n");
	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	//	SrvAddr.sin_port = htons(SDK_LISTEN_PORT);
	SrvAddr.sin_port = htons(ENCODESERVER_PORT);
	SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);

	ServSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(ServSock < 0)  {
		PRINTF("ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		return;
	}

	setsockopt(ServSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(ServSock, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0)  {
		PRINTF("bind terror:%d,error msg: = %s\n", errno, strerror(errno));
		return;
	}

	if(listen(ServSock, 10) < 0) {
		PRINTF("listen error:%d,error msg: = %s", errno, strerror(errno));
		return ;
	}

	if((fileflags = fcntl(ServSock, F_GETFL, 0)) == -1) {
		PRINTF("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		return;
	}

	if(fcntl(ServSock, F_SETFL, fileflags | O_NONBLOCK) == -1) {
		PRINTF("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		return;
	}

	nLen = sizeof(struct sockaddr_in);

	while(1)  {
		memset(&ClientAddr, 0, sizeof(struct sockaddr_in));
		nLen = sizeof(struct sockaddr_in);
		sClientSocket = accept(ServSock, (void *)&ClientAddr, (unsigned int *)&nLen);

		if(sClientSocket > 0) {
			//打印客户端ip//
			inet_ntop(AF_INET, (void *) & (ClientAddr.sin_addr), newipconnect, 16);
			PRINTF("<><><><><>new client: socket = %d, ip = %s<><><><><>\n", sClientSocket, newipconnect);
			int nPos = 0;
			char chTemp[16], *pchTemp;

			pchTemp = &chTemp[0];
			//	ClearLostClient(PORT_SDK);
			//	nPos = GetNullClientData(PORT_SDK);

			if(sdk_get_client_num() >=  MAX_CLIENT_NUM) {
				unsigned char chData[512];
				int totallen = 0;
				int cmd = 0;
				int web_ret = SERVER_RET_SOCK_MAX_NUM;
				int out = 0;
				totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + sizeof(out);
				msgPacket(WEB_IDENTIFIER, chData, INT_TYPE, totallen, cmd, web_ret);
				memcpy(chData + (totallen - sizeof(out)), &out, sizeof(out));
				PRINTF("up to max client num : %x\n", web_ret);
				send(sClientSocket, chData, totallen, 0);
			} else {
				int nSize = 0;
				int result;
				//int *pnPos = malloc(sizeof(int));

				//inet_ntop(AF_INET, (void *) & (ClientAddr.sin_addr), IPdotdec[nPos], 16);
				/* set client used */
				//SETCLIUSED(PORT_SDK, nPos, TRUE);
				//	SETSOCK(PORT_SDK, nPos, sClientSocket);
				nSize = 1;

				if((setsockopt(sClientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
				               sizeof(nSize))) == -1) {
					perror("setsockopt failed");
				}

				nSize = 0;
				nLen = sizeof(nLen);
				result = getsockopt(sClientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (unsigned int *)&nLen);

				if(result) {
					PRINTF("getsockopt() errno:%d socket:%d  result:%d\n", errno, sClientSocket, result);
				}

				nSize = 1;

				if(setsockopt(sClientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
					PRINTF("Setsockopt error%d\n", errno);
				}

				memset(chTemp, 0, 16);
				pchTemp = inet_ntoa(ClientAddr.sin_addr);
				PRINTF("Clent:%s connected,nPos:%d socket:%d!\n", chTemp, nPos, sClientSocket);

				//Create ClientMsg task!
				sdk_add_client_num();
				result = pthread_create(&sdk_client_thread_id[nPos], NULL, (void *)sdk_clinet_thr, &sClientSocket);
				usleep(1500000);

				if(result) {
					close(sClientSocket);   //
					PRINTF("creat pthread ClientMsg error  = %d!\n" , errno);
					continue;
				}

			}
		} else {
			if(errno == ECONNABORTED || errno == EAGAIN) { //软件原因中断
				//				DEBUG(DL_ERROR,"errno =%d program again start!!!\n",errno);
				usleep(100000);
				continue;
			}

			if(ServSock > 0)	{
				close(ServSock);
			}

			goto DSP1STARTRUN;
		}

	}

	if(ServSock > 0)	{
		close(ServSock);
	}

	ServSock = 0;
	return;
}


