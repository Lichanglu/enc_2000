#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include "webtcpcom.h"
#include "webmiddle.h"

static int connectToDecodeServer()
{
	struct sockaddr_in serv_addr;
	const char* pAddr  = "127.0.0.1";
	int socketFd;
	
	socketFd= socket(PF_INET, SOCK_STREAM, 0);
	if(socketFd < 1)
		return -1;
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(WEBSERVER);
	inet_aton(pAddr,(struct in_addr *)&serv_addr.sin_addr);
	bzero(&(serv_addr.sin_zero),8);
	
	if(connect(socketFd, (struct sockaddr *)&serv_addr,sizeof(struct sockaddr)) == -1)	{
		close(socketFd);
		return -1;
	}

	return socketFd;

	
}

FILE *create_file_fd(char *file)
{
	FILE *fp_low = NULL;

	if(NULL == fp_low) {
		fp_low = fopen(file, "a+");
	}

	return fp_low;
	
}

unsigned int mid_clock(void)
{
	unsigned int msec;
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);

	msec = tp.tv_sec;
	msec = msec * 1000 + tp.tv_nsec / 1000000;
	
	return msec;
}

void write_time_buf(char *buf)
{
	int n, m, s,us;
	us = mid_clock( ) ;
	s = us/ 1000;
	n = s / 3600;
	s = s % 3600;
	m = s / 60;
	s = s % 60;
	us = us %1000;
	
	sprintf(buf, "[%04d:%02d:%02d:%03d] : ", n, m, s,us);
}
static void write_local_file(FILE *file, char *data)
{
	char tmpbuf[512] = {0};
	int len = 0;
	
	write_time_buf(tmpbuf);
	strcat(tmpbuf,data);
	len = strlen(tmpbuf);
	fwrite(tmpbuf, len, 1, file);
}

/*网络发送数据*/
static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf+nWriteLen, nSize-nWriteLen, 0);
		if(nRet < 0 ) {
			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr,"network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}
			return nRet;
		}
		else if(nRet == 0) {
			fprintf(stderr,"Send Net Data Error nRet= %d\n",nRet);
			continue;
		}
		nWriteLen += nRet;
		nCount = 0;
	}
	return nWriteLen;
}



/*网络接收数据*/
static int ReadData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;

	while(nWriteLen < nSize) {
		nRet = recv(s, (char *)pBuf+nWriteLen, nSize-nWriteLen, 0);
		nWriteLen += nRet;
	}
	
	return nWriteLen;
}

int appCmdIntParse(int cmd,int invalue,int inlen,int *outvalue,int *outlen)
{
	webMsgInfo  msg,outmsg;
	
	int Sockfd = 0;
	int ret = 0;
	int retVal = 0;
	int valueLen = 0;
	int cmdLen = 0;
	int retcmd = 0;
	
	char intemp[4096] = {0};
	char outtmep[4096] = {0};
	char retValue[4096] = {0};
	
	
	if(Sockfd == 0)	{
		Sockfd= connectToDecodeServer();
	}
	
	if(Sockfd <= 0)	{
		return CLIENT_ERR_TCPCONNECT;
	}

	msg.identifier = WEB_IDENTIFIER;
	valueLen = inlen;
	cmdLen	= sizeof(webparsetype);
	msg.type = INT_TYPE;
	msg.len = sizeof(webMsgInfo)+valueLen + cmdLen;
	memcpy(intemp,&msg,sizeof(webMsgInfo));
	memcpy(intemp+sizeof(webMsgInfo),&cmd,cmdLen);
	memcpy(intemp+(sizeof(webMsgInfo) + cmdLen),&invalue,valueLen);
	
	if(WriteData(Sockfd, (void*)intemp,msg.len) < 1)	{
		close(Sockfd);
		return CLIENT_ERR_TCPSEND;
	}

	if(ReadData(Sockfd,(void *)&outmsg,sizeof(webMsgInfo)) < sizeof(webMsgInfo)){
		close(Sockfd);
		return CLIENT_ERR_TCPRECV;
	}

	if(ReadData(Sockfd, (void *)retValue,(outmsg.len - sizeof(webMsgInfo))) < 0){
		close(Sockfd);	
		return CLIENT_ERR_TCPRECV;
	}
	
	close(Sockfd);
	memcpy(&retcmd,&retValue,sizeof(webparsetype));
	memcpy(&retVal,retValue + 4, sizeof(int));
	memcpy(outvalue,retValue + 8,sizeof(int));

	return retVal;
	
}


int appCmdStringParse(int cmd,char *invalue,int inlen,char  *outvalue,int *outlen)
{
	webMsgInfo  msg,outmsg;

	int sockfd = 0;
	int retcmd = 0;
	int retval = 0;
	int valuelen = 0;
	int cmdlen = 0;
	
	char intemp[4096] = {0};
	char outtmep[4096] = {0};
	char retvalue[4096] = {0};
	
	if(sockfd == 0){
		sockfd= connectToDecodeServer();
	}
	if(sockfd <= 0){
		return CLIENT_ERR_TCPCONNECT;
	}

	msg.identifier = WEB_IDENTIFIER;
	valuelen = inlen;
	cmdlen	= sizeof(webparsetype);
	msg.type = STRING_TYPE;
		
	msg.len = sizeof(webMsgInfo) + cmdlen + valuelen;
	memcpy(intemp,&msg,sizeof(webMsgInfo));
	memcpy(intemp+sizeof(webMsgInfo),&cmd,cmdlen);
	
	if(invalue != NULL){
		memcpy(intemp+(sizeof(webMsgInfo) + cmdlen),invalue,valuelen);
	}
	
	if(WriteData(sockfd, (void*)intemp,msg.len) < 1)	{
		close(sockfd);
		return CLIENT_ERR_TCPSEND;
	}

	if(ReadData(sockfd,(void *)&outmsg,sizeof(webMsgInfo)) < sizeof(webMsgInfo)){
		close(sockfd);
		return CLIENT_ERR_TCPRECV;
	}

	if(ReadData(sockfd, (void *)retvalue,(outmsg.len - sizeof(webMsgInfo))) < 0){
		close(sockfd);	
		return CLIENT_ERR_TCPRECV;
	}
	
	close(sockfd);
	memcpy(&retcmd,retvalue,cmdlen);
	memcpy(&retval,retvalue + 4,sizeof(int));
	memcpy(outvalue,retvalue+cmdlen+sizeof(int),(outmsg.len - sizeof(webMsgInfo) - sizeof(webparsetype)));

	return retval;
	
}

int appCmdStructParse(int cmd,void  *invalue,int inlen,void *outvalue,int *outlen)
{
	webMsgInfo inmsg;
	webMsgInfo outmsg;
	
	int Sockfd = 0;
	int retCmd = 0;
	int retval = 0;
	int valueLen = 0;
	int cmdLen = 0;
	int msgHeadLen = sizeof(webMsgInfo);
	
	char intemp[4096] = {0};
	char outtmep[4096] = {0};
	char retValue[4096] = {0};
	
	
	if(Sockfd == 0){
		Sockfd= connectToDecodeServer();
	}
	
	if(Sockfd <= 0){
		return CLIENT_ERR_TCPCONNECT;
	}

	inmsg.identifier = WEB_IDENTIFIER;
	valueLen = inlen;
	cmdLen	= sizeof(int);
	inmsg.type = STRUCT_TYPE;
	inmsg.len = msgHeadLen+valueLen + cmdLen;
	memcpy(intemp,&inmsg,msgHeadLen);
	memcpy(intemp+msgHeadLen,&cmd,cmdLen);
	
	if(invalue !=NULL){
		memcpy(intemp+(msgHeadLen + cmdLen),invalue,valueLen);
	}
	
	if(WriteData(Sockfd, (void*)intemp,inmsg.len) < 1)	{
		close(Sockfd);
		return CLIENT_ERR_TCPSEND;
	}
	
	if(ReadData(Sockfd,(void *)&outmsg,msgHeadLen) < msgHeadLen){
		close(Sockfd);
		return CLIENT_ERR_TCPRECV;
	}


	if(ReadData(Sockfd, (void *)retValue,(outmsg.len - msgHeadLen)) < 0){
		close(Sockfd);	
		return CLIENT_ERR_TCPRECV;
	}
	
	close(Sockfd);
	memcpy(&retCmd,retValue,cmdLen);
//	printf("<-----MsgType[%d]\n",retCmd);
	memcpy(&retval,retValue+cmdLen,sizeof(int));
	memcpy(outvalue,retValue+cmdLen+sizeof(int),outmsg.len-msgHeadLen-cmdLen-sizeof(int));

	return retval;
	
}

