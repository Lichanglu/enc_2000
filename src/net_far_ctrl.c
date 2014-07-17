#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "net_far_ctrl.h"
//#include "nslog.h"
#if EDUKIT_FAR_CTRL
#include "reach_enc2000.h"
#include "sysparams.h"
#else
typedef int SOCKET;
typedef int DSPFD;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  BYTE;
/*message header*/
typedef struct __HDB_MSGHEAD {
	WORD	nLen;
	WORD	nVer;				//version
	BYTE	nMsg;				//message type
	BYTE	szTemp[3];			//reserve
} MSGHEAD;
#endif

typedef struct EncoderIndex_ {
	int32_t eindex;
	int32_t pos;
} EncoderIndex_t;

/*client infomation*/
typedef struct _ClientData {
	int bUsed;
	int sSocket;
	int bLogIn;
	int nLogInType;
	unsigned char PPTenable;
	unsigned char LowStream;
	unsigned char LowStreamFlag;
	unsigned char PPTquery;
} ClientData;

/*DSP client param*/
typedef struct __DSPCliParam {
	int dspFD;     	//DSP handle
	ClientData cliDATA[6]; //client param infomation
} DSPCliParam;


typedef struct _FarCtrlListenParam {
	unsigned int eindex;
	unsigned int dwAddr;
	unsigned int  port;
} FarCtrlListenParam;

#define 	MAX_CLIENT						6
#define  	MSG_MAXCLIENT_ERR  			5
#define	INVALID_SOCKET 				-1
#define 	TRUE  1
#define 	FALSE  0
#define 	MSG_FARCTRL					42
#define 	HEAD_LEN			sizeof(MSGHEAD)
#define  	PORT_ONE 						0
#define 	LOGIN_USER						0
#define 			DSP_NUM				    4

static DSPCliParam		gFarCtrlCliPara[DSP_NUM];

/*if use client*/
#define ISUSED(dsp,cli)							(gFarCtrlCliPara[dsp].cliDATA[cli].bUsed == TRUE)
/*set client used*/
#define SETCLIUSED(dsp,cli,val)					(gFarCtrlCliPara[dsp].cliDATA[cli].bUsed=val)
/*if login client*/
#define ISLOGIN(dsp,cli)							(gFarCtrlCliPara[dsp].cliDATA[cli].bLogIn == TRUE)
/*set client login succeed or failed*/
#define SETCLILOGIN(dsp,cli,val)					(gFarCtrlCliPara[dsp].cliDATA[cli].bLogIn=val)
/*get socket fd*/
#define GETSOCK(dsp,cli)							(gFarCtrlCliPara[dsp].cliDATA[cli].sSocket)
/*set socket fd*/
#define SETSOCK(dsp,cli,val)						(gFarCtrlCliPara[dsp].cliDATA[cli].sSocket=val)
/*current socket if valid*/
#define ISSOCK(dsp,cli)							(gFarCtrlCliPara[dsp].cliDATA[cli].sSocket != INVALID_SOCKET)
#define SETLOWSTREAM(dsp,cli,val)					(gFarCtrlCliPara[dsp].cliDATA[cli].LowStream = val)
#define GETLOWSTREAM(dsp,cli)					(gFarCtrlCliPara[dsp].cliDATA[cli].LowStream)
#define SETLOWSTREAMFLAG(dsp,cli,val)			(gFarCtrlCliPara[dsp].cliDATA[cli].LowStreamFlag = val)
#define GETLOWSTREAMFLAG(dsp,cli)				(gFarCtrlCliPara[dsp].cliDATA[cli].LowStreamFlag)
#define SETPPTINDEX(dsp,cli,val)					(gFarCtrlCliPara[dsp].cliDATA[cli].PPTenable = val)
#define GETPPTINDEX(dsp,cli)						(gFarCtrlCliPara[dsp].cliDATA[cli].PPTenable)
#define SETPPTQUERY(dsp,cli,val)					(gFarCtrlCliPara[dsp].cliDATA[cli].PPTquery = val)
#define GETPPTQUERY(dsp,cli)						(gFarCtrlCliPara[dsp].cliDATA[cli].PPTquery)
/*get dsp handle*/
#define GETDSPFD(dsp)							(gFarCtrlCliPara[dsp].dspFD)
/*set dsp handle*/
#define SETDSPFD(dsp,val)							(gFarCtrlCliPara[dsp].dspFD=val)
/*if handle valid*/
#define ISDSPFD(dsp)								(gFarCtrlCliPara[dsp].dspFD != INVALID_FD)
/*client login type*/
#define SETLOGINTYPE(dsp,cli,val)					(gFarCtrlCliPara[dsp].cliDATA[cli].nLogInType=val)
/*get login type*/
#define GETLOGINTYPE(dsp,cli)						(gFarCtrlCliPara[dsp].cliDATA[cli].nLogInType)


static sem_t	g_far_sem_ex[3];
static int 	gFarRunStatus_ex[3] = {1, 1, 1};

static int EdukitFarCtrlServerThread(void *arg);
static void EdukitFarCtrlProcess(void *pParams);
static void InitClient(unsigned char dsp);
static int GetNullClientData(unsigned char dsp);
static void ClearLostClient(unsigned char dsp);
static int SetSendTimeOut(int sSocket, unsigned long time);


static int SetSendTimeOut(int sSocket, unsigned long time)
{
	struct timeval timeout ;
	int ret = 0;

	timeout.tv_sec = time ; //3
	timeout.tv_usec = 0;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	if(ret == -1) {
		fprintf(stderr, "setsockopt() Set Send Time Failed\n");
	}

	return ret;
}

static int GetNullClientData(unsigned char dsp)
{
	int cli ;

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		//		fprintf(stderr, "\n\n");
		//		fprintf(stderr, "cnt: %d		used: %d\n", cli, ISUSED(dsp, cli));
		//		fprintf(stderr, "cnt: %d		login: %d\n", cli, ISLOGIN(dsp, cli));
		//		fprintf(stderr, "\n\n\n");
		if((!ISUSED(dsp, cli)) && (!ISLOGIN(dsp, cli))) {
			return cli;
		}
	}

	return -1;
}

static void InitClient(unsigned char dsp)
{
	int cli ;

	for(cli = 0; cli < MAX_CLIENT; cli++) {

		SETSOCK(dsp, cli, INVALID_SOCKET);

	}
}

static void ClearLostClient(unsigned char dsp)
{
	int cli ;

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		if(!ISUSED(dsp, cli) && ISSOCK(dsp, cli)) {
			close(GETSOCK(dsp, cli));
			SETSOCK(dsp, cli, INVALID_SOCKET);
			SETCLILOGIN(dsp, cli, FALSE);
			SETPPTINDEX(dsp, cli, FALSE);
			SETPPTQUERY(dsp, cli, FALSE);
		}
	}
}

static int EdukitFarCtrlServerThread(void *arg)
{
	struct sockaddr_in		SrvAddr, ClientAddr;
	struct in_addr addr1;
	int					sClientSocket = -1;
	int					ServSock = -1;

	int testaddr = 0;
	pthread_t				client_threadid[MAX_CLIENT] = {0};

	short				sPort					= 3700;
	void					*ret 					= 0;

	int 					clientsocket				= 0;
	int 					nLen					= 0;;
	int 					ipos						= 0;
	int 					fileflags					= 0;
	int 					opt 					= 1;

	FarCtrlListenParam *param	= (FarCtrlListenParam *)arg;
	int eindex = param->eindex;
	memcpy(&addr1, &(param->dwAddr), 4);
	testaddr = param->dwAddr;
	sPort = param->port;
	free(arg);
	arg = NULL;

	InitClient(eindex);
	sem_init(&g_far_sem_ex[eindex], 0, 0);

SERVERSTARTRUN:
	//	nslog(NS_WARN, "Start...\n");
	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = htons(sPort);

	if(testaddr == 0) {
		printf("the ip is 127.0.0.1\n");
		SrvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	} else {
		SrvAddr.sin_addr  = addr1;
	}



	ServSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(ServSock < 0) {
		ERR_PRN("ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		gFarRunStatus_ex[eindex] = 0;
		return -1;
	}

	setsockopt(ServSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(ServSock, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
		ERR_PRN( "bind terror:%d,error msg: = %s\n", errno, strerror(errno));
		gFarRunStatus_ex[eindex] = 0;
		return -1;
	}

	if(listen(ServSock, 10) < 0) {
		ERR_PRN("listen error:%d,error msg: = %s", errno, strerror(errno));
		gFarRunStatus_ex[eindex] = 0;
		return -1;
	}

	if((fileflags = fcntl(ServSock, F_GETFL, 0)) == -1) {
		ERR_PRN("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		gFarRunStatus_ex[eindex] = 0;
		return -1;
	}


	nLen = sizeof(struct sockaddr_in);

	while(gFarRunStatus_ex[eindex]) {
		memset(&ClientAddr, 0, sizeof(struct sockaddr_in));

		nLen = sizeof(struct sockaddr_in);
		sClientSocket = accept(ServSock, (void *)&ClientAddr, (DWORD *)&nLen);

		PRINTF("naccept a valid connect!!!!-----------------, socket = %d\n\n\n\n", sClientSocket);

		if(sClientSocket > 0) {

			int nPos = 0;
			ClearLostClient(eindex);
			nPos = GetNullClientData(eindex);

			if(-1 == nPos) {
				char chData[20];
				chData[0] = 0;
				chData[1] = 3;
				chData[2] = MSG_MAXCLIENT_ERR;
				send(sClientSocket, chData, 3, 0);
				close(sClientSocket);
				ERR_PRN("MAXCLIENT ERR!\n");
			} else {
				int nSize = 0;
				int result;
				EncoderIndex_t *pEindex = malloc(sizeof(EncoderIndex_t));
				/* set client used */
				SETCLIUSED(eindex, nPos, TRUE);
				SETSOCK(eindex, nPos, sClientSocket) ;

				nSize = 1;

				if((setsockopt(sClientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
				               sizeof(nSize))) == -1) {
					ERR_PRN("setsockopt failed");
				}

				nSize = 0;
				nLen = sizeof(nLen);
				result = getsockopt(sClientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (DWORD *)&nLen);

				if(result) {
					PRINTF("getsockopt() errno:%d socket:%d  result:%d\n", errno, sClientSocket, result);
				}

				nSize = 1;

				if(setsockopt(sClientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
					ERR_PRN( "Setsockopt error%d\n", errno);
				}

				PRINTF("Clent:%s connected,nPos:%d socket:%d!\n", inet_ntoa(ClientAddr.sin_addr), nPos, sClientSocket);
				pEindex->pos = nPos;
				pEindex->eindex = eindex;
				result = pthread_create(&client_threadid[nPos], NULL, (void *)EdukitFarCtrlProcess, (void *)pEindex);

				if(result) {
					close(sClientSocket);	//
					ERR_PRN( "creat pthread ClientMsg error  = %d!\n" , errno);
					continue;
				}

				sem_wait(&g_far_sem_ex[eindex]);
				PRINTF("sem_wait() semphone inval!!!  result = %d\n", result);
			}
		} else {

			if(sClientSocket > 0) {
				close(sClientSocket);
				//				nslog(NS_WARN, "close sClientSocket socket!!! %d\n", sClientSocket);
				sClientSocket = -1;
			}

			if(errno == ECONNABORTED || errno == EAGAIN)
				//软件原因中断
			{
				usleep(100000);
				continue;
			}

			if(ServSock > 0) {
				//				nslog(NS_WARN, "close enclive socket!!! %d\n", ServSock);
				close(ServSock);
				ServSock = -1;
				sleep(1);
			}

			goto SERVERSTARTRUN;
		}

	}

	//	nslog(NS_WARN, "begin exit the DSP1TCPTask \n");

	//	nslog(NS_WARN,  "exit the drawtimethread \n");

	for(ipos = 0; ipos < MAX_CLIENT; ipos++) {
		if(client_threadid[ipos]) {
			clientsocket = GETSOCK(eindex, ipos);

			if(clientsocket != INVALID_SOCKET) {
				close(clientsocket);
				SETSOCK(eindex, ipos, INVALID_SOCKET);
			}

			if(pthread_join(client_threadid[ipos], &ret) == 0) {
			}
		}
	}

	//	nslog(NS_WARN,  "close the encoder server socket \n");

	if(ServSock > 0) {
		//		nslog(NS_WARN,  "close gserv socket \n");
		close(ServSock);
	}

	ServSock = 0;
	//	nslog(NS_WARN, "End.\n");
	return 0;
}

static void EdukitFarCtrlProcess(void *pParams)
{
	int					sSocket;
	//WORD					length;

	char			szData[256] = {0};

	int 					nPos					= 0;
	int32_t index = 0;
	int 					nMsgLen 				= 0;
	EncoderIndex_t	*pEindex					= (EncoderIndex_t *)pParams;
	int 					nLen					= 0;
	MSGHEAD header, *phead;
	PRINTF("Enter DSP1TcpComMsg() function!!\n");
	nPos = pEindex->pos;
	index = pEindex->eindex;
	free(pParams);
	sSocket = GETSOCK(index, nPos);
	phead = &header;
	memset(&header, 0, sizeof(MSGHEAD));
	/*设置TCP发送超时 3s*/
	SetSendTimeOut(sSocket, 3);
	memset(szData, 0, 256);
	sem_post(&g_far_sem_ex[index]);

	while(gFarRunStatus_ex[index]) {
		memset(szData, 0, sizeof(szData));

		if(sSocket <= 0) {
			fprintf(stderr, "sSocket<0 !\n");
			goto ExitClientMsg;
		}

		nLen = recv(sSocket, szData, HEAD_LEN, 0);
		PRINTF( "receive length:%d,errno=%d\n", nLen, errno);
		memcpy(phead, szData, HEAD_LEN);

		if(nLen < HEAD_LEN || nLen == -1) {
			fprintf(stderr, "recv nLen < 2 || nLen == -1  goto ExitClientMsg\n ");
			goto ExitClientMsg;
		}

		nMsgLen = ntohs(*((short *)szData));
		phead->nLen = nMsgLen;
		nLen = recv(sSocket, szData + HEAD_LEN, nMsgLen - HEAD_LEN, 0);
		PRINTF("nMsgLen = %d,szData[2]=%d!\n", nMsgLen, szData[2]);

		if(nLen < nMsgLen - HEAD_LEN) {
			fprintf(stderr, "nLen < nMsgLen -2  goto ExitClientMsg\n ");
			goto ExitClientMsg;
		}

		PRINTF( "------- socket = %d, type = %d, nLen = %d, nMsgLen = %d -------\n", sSocket, phead->nMsg, nLen, nMsgLen);

		/*信令解析处理*/
		switch(phead->nMsg) {
#if 0

			case MSG_REQ_AUDIO:
				fprintf(stderr, "DSP1 request Audio Data \n");
				break;

			case MSG_GET_AUDIOPARAM:
				fprintf(stderr, "DSP1 Get AudioParam \n");

				GetAudioParam(sSocket, PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
				break;

			case MSG_SET_AUDIOPARAM:
				fprintf(stderr, "DSP1 Set AudioParam \n");
				//			SetAudioParam(DSP1,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_GET_VIDEOPARAM:

				GetVideoParam(sSocket, PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);

				fprintf(stderr, "DSP1 Get VideoParam \n");
				break;

			case MSG_SET_VIDEOPARAM:

				SetVideoParam(PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
				fprintf(stderr, "DSP1 Set VideoParam \n");
				break;

			case MSG_SETVGAADJUST:
				fprintf(stderr, "DSP1 ReviseVGAPic vga picture!\n");
				//			ReviseVGAPic(DSP1,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_GSCREEN_CHECK:
				//			GreenScreenAjust();
				break;

			case MSG_SET_DEFPARAM:
#if 0
				fprintf(stderr, "MSG_SET_DEFPARAM\n");
				unsigned char ParamBuf[200];
				SysParamsV2 *Newp, *Oldp;
				int ret = 0;

				memcpy(ParamBuf, &szData[3], nMsgLen - 3);
				gSysParaT.sysPara.sCodeType = 0x0F;
				Oldp = &gSysParaT.sysPara;
				Newp = (SysParamsV2 *)&ParamBuf[0];

				ret = SetSysParams(Oldp, Newp);
				SetEncodeVideoParams(&gVAenc_p, Newp);
#endif
				break;

			case MSG_ADD_TEXT:	//字幕
				printf("haaaaaaa\n");
				fprintf(stderr, "enter into MSG_ADD_TEXT \n");

				//			AddOsdText(PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
				break;



			case MSG_SET_SYSTIME:
				fprintf(stderr, "DSP1 Set system Time\n");
				break;



			case MSG_SYSPARAMS: { //获取系统参数

				length = HEAD_LEN + sizeof(gSysParsm);
				PackHeaderMSG((BYTE *)szData, MSG_SYSPARAMS, length);
				memcpy(szData + HEAD_LEN, &gSysParsm, sizeof(gSysParsm));
				send(sSocket, szData, length, 0);

				fprintf(stderr, "Get Sys Params ..........................\n");
			}
			break;

			case MSG_SETPARAMS: {	//设置系统参数

				fprintf(stderr, "Set Params! gSysParams %d Bytes\n", sizeof(gSysParsm));
				unsigned char ParamBuf[200];
				SYSPARAMS *Newp;

				memcpy(ParamBuf, &szData[HEAD_LEN], nMsgLen - HEAD_LEN);

				Newp = (SYSPARAMS *)&ParamBuf[0];


				fprintf(stderr, "==================================================\n");
				fprintf(stderr, "new:	unChannel = %d\n", Newp->unChannel);
				fprintf(stderr, "new:	dwAddr    = %d\n", Newp->dwAddr);
				fprintf(stderr, "new:	strName   = %s\n", Newp->strName);
				fprintf(stderr, "new:	bType     = %d\n", Newp->bType);
				fprintf(stderr, "==================================================\n");

				//enc_set_fps(gEduKit.encLink.link_id, 0, (Newp->nFrame)*1000, (Newp->sBitrate)*1000);

			}
			break;

			case MSG_SAVEPARAMS:		//保存参数到flash
				fprintf(stderr, "Save Params!\n");
				//		SaveIPinfo(IPINFO_NAME, &gSysParaT);
				//		SaveSysParams(DSP1, CONFIG_NAME, &gSysParaT);
				break;

			case MSG_RESTARTSYS:
				fprintf(stderr, "Restart sys\n");
				system("reboot -f");
				break;

			case MSG_UpdataFile: { //升级过程
				int ret = 0 ;

				//		ret = DoUpdateProgram(sSocket,szData,nMsgLen);
				if(ret <= 1) {
					goto ExitClientMsg;
				}
			}
			break;

			case MSG_UPDATEFILE_ROOT:
				fprintf(stderr, "Updata Root file\n");
				break;

			case MSG_REQ_I:
				//				gReqIframe1 = 1;
				//				gReqIframe2 = 1;
				fprintf(stderr, "Request I Frame!\n");
				break;

			case MSG_PIC_DATA:
				//				SetPPTIndex(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				fprintf(stderr, "SetPPTIndex\n");
				break;

			case MSG_MUTE:
				//				SetNosound(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				fprintf(stderr, "NoSound!\n");
				break;

			case MSG_LOCK_SCREEN:
				//				LockScreen(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				break;

			case MSG_LOW_BITRATE:
				//				SetLowStream(sSocket,nPos,(unsigned char *)&szData[3],nMsgLen-3);
				//				SETLOWSTREAMFLAG(DSP1, nPos, 1);
				//				gLowStreamFlag = 1;
				break;

			case MSG_FIX_RESOLUTION:
				//				FixResolution(sSocket,nPos,(unsigned char *)(&szData[3]),nMsgLen-3);
				break;

			case MSG_SEND_INPUT:
				break;


			case MSG_PASSWORD: { //登陆请求
				if(!(strcmp("sawyer", szData + HEAD_LEN))) {
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);

				} else if(szData[HEAD_LEN] == 'A' && !strcmp(szPass, szData + HEAD_LEN + 1)) {
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);
					fprintf(stderr, "logo Admin!\n");
				} else if(szData[HEAD_LEN] == 'U' && !strcmp(szPass, szData + HEAD_LEN + 1)) {

					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_USER);
					fprintf(stderr, "logo User!\n");
				} else {
					PackHeaderMSG((BYTE *)szData, MSG_PASSWORD_ERR, HEAD_LEN);
					send(sSocket, szData, HEAD_LEN, 0);
					fprintf(stderr, "logo error!\n");
					SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);
					goto ExitClientMsg;   //
				}


				PackHeaderMSG((BYTE *)szData, MSG_CONNECTSUCC, HEAD_LEN);
				send(sSocket, szData, HEAD_LEN, 0);
				fprintf(stderr, "send MSG_CONNECTSUCC!\n");
				/*length*/
				length = HEAD_LEN + sizeof(gSysParsm);
				PackHeaderMSG((BYTE *)szData, MSG_SYSPARAMS, length);

				memcpy(szData + HEAD_LEN, &gSysParsm, sizeof(gSysParsm));
				send(sSocket, szData, length, 0);
				/*set client login succeed*/
				SETCLIUSED(PORT_ONE, nPos, TRUE);
				SETCLILOGIN(PORT_ONE, nPos, TRUE);
				break;
			}

#endif

			case MSG_FARCTRL:
				//				nslog(NS_WARN, "DSP[%d] Far Control\n", index);
				FarCtrlCamera(index, (unsigned char *)&szData[HEAD_LEN], nMsgLen - HEAD_LEN);

				break;

			default
					:
				fprintf(stderr, "Error Cmd=%d!\n", phead->nMsg);
				break;
		}

	}


ExitClientMsg:
	fprintf(stderr, "\n|||||||| go down  |||||||||\n\n");
	fprintf(stderr, " socket = %d %d\n", GETSOCK(index, nPos), sSocket);

	SETCLIUSED(index, nPos, FALSE);
	SETSOCK(index, nPos, INVALID_SOCKET);
	SETLOGINTYPE(index, nPos, LOGIN_USER);
	SETCLILOGIN(index, nPos, FALSE);
	SETPPTINDEX(index, nPos, FALSE);
	SETPPTQUERY(index, nPos, FALSE);
	SETLOWSTREAM(index, nPos, 0);

	if(GETLOWSTREAMFLAG(index, nPos)) {
		SETLOWSTREAMFLAG(index, nPos, 0);
	}

	close(sSocket);

	pthread_detach(pthread_self());
	pthread_exit(0);
}



//传入index 以及dwaddr ,port
int StartEdukitFarCtrlServerEx(int32_t eindex, unsigned int dwAddr , unsigned short port)
{
	int result;
	pthread_t	serverThid;

	FarCtrlListenParam *param = NULL;
	param = (FarCtrlListenParam *)malloc(sizeof(FarCtrlListenParam));

	if(param == NULL) {
		printf("ERROR,StartEdukitFarCtrlServerEx\n");
		return -1;
	}

	memset(param, 0, sizeof(FarCtrlListenParam));

	param->eindex = eindex;
	param->dwAddr = dwAddr;
	param->port = port;

	result = pthread_create(&serverThid, NULL, (void *)EdukitFarCtrlServerThread, (void *)param);

	if(result < 0) {
		fprintf(stderr, "create EncoderServerThread() failed\n");
		return -1;
	}

	return 0;
}


#if    EDUKIT_FAR_CTRL
#define EDUKIT_FAR_CTRL_PORT		3000
int init_edukit_far_ctrl(void)
{
	unsigned int dwAddr0 = 0, gateway0 = 0, netmask0 = 0;
	unsigned int dwAddr1 = 0, gateway1 = 0, netmask1 = 0;
	int ret1=0;

	int  ret0 = sys_get_network(ETH0, &dwAddr0, &gateway0, &netmask0);

	if(ret0 < 0) {
		ERR_PRN("sys_get_network ETH0 IP failed!\n");
		return ret1;
	}

	ret1 = sys_get_network(ETH0_1, &dwAddr1, &gateway1, &netmask1);

	if(ret1 < 0) {
		ERR_PRN("sys_get_network ETH1 IP failed!\n");
		return ret1;
	}
	PRINTF("init  ----->\n");

	StartEdukitFarCtrlServerEx(0, dwAddr0, EDUKIT_FAR_CTRL_PORT);
	StartEdukitFarCtrlServerEx(1, dwAddr1, EDUKIT_FAR_CTRL_PORT);
}
#endif


