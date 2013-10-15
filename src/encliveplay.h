/*********************************************************************
公司名称:	Reach Software
文件名:	
创建人:	Huang Haihong				日期:	2011-06-20
修改人:	Huang Haihong				日期:	2011-06-20

说明:	
**********************************************************************/


#ifndef	_ENCLIVEPLAY_H_
#define	_ENCLIVEPLAY_H_







#define LOGIN_USER				0
#define LOGIN_ADMIN				1
#define INVALID_SOCKET 			-1
#define INVALID_FD					-1
#define	A(bit)		rcvd_tbl[(bit)>>3]	/* identify byte in array */
#define	B(bit)		(1 << ((bit) & 0x07))	/* identify bit in byte */
#define	SET(bit)	(A(bit) |= B(bit))
#define	CLR(bit)	(A(bit) &= (~(B(bit))))
#define	TST(bit)	(A(bit) & B(bit))

/*if use client*/
#define ISUSED(dsp,cli)				(gDSPCliPara[dsp].cliDATA[cli].bUsed == TRUE)
/*set client used*/
#define SETCLIUSED(dsp,cli,val)		(gDSPCliPara[dsp].cliDATA[cli].bUsed=val)
/*if login client*/
#define ISLOGIN(dsp,cli)			(gDSPCliPara[dsp].cliDATA[cli].bLogIn == TRUE)
/*set client login succeed or failed*/
#define SETCLILOGIN(dsp,cli,val)	(gDSPCliPara[dsp].cliDATA[cli].bLogIn=val)
/*get socket fd*/
#define GETSOCK(dsp,cli)			(gDSPCliPara[dsp].cliDATA[cli].sSocket)
/*set socket fd*/
#define SETSOCK(dsp,cli,val)		(gDSPCliPara[dsp].cliDATA[cli].sSocket=val)
/*current socket if valid*/
#define ISSOCK(dsp,cli)				(gDSPCliPara[dsp].cliDATA[cli].sSocket != INVALID_SOCKET)
/*get dsp handle*/
#define GETDSPFD(dsp)				(gDSPCliPara[dsp].dspFD)
/*set dsp handle*/
#define SETDSPFD(dsp,val)			(gDSPCliPara[dsp].dspFD=val)
/*if handle valid*/
#define ISDSPFD(dsp)				(gDSPCliPara[dsp].dspFD != INVALID_FD)
/*client login type*/
#define SETLOGINTYPE(dsp,cli,val)  	(gDSPCliPara[dsp].cliDATA[cli].nLogInType=val)
/*get login type*/
#define GETLOGINTYPE(dsp,cli)  		(gDSPCliPara[dsp].cliDATA[cli].nLogInType)
/*set resize flag*/
#define SETLOWRATEFLAG(dsp,cli,val)  	(gDSPCliPara[dsp].cliDATA[cli].LowRateflag=val)
/*get resize flag*/
#define GETLOWRATEFLAG(dsp,cli)  		(gDSPCliPara[dsp].cliDATA[cli].LowRateflag)
/*get connect time*/
#define GETCONNECTTIME(dsp,cli)  		(gDSPCliPara[dsp].cliDATA[cli].connect_time)



/*client infomation*/
typedef struct _ClientData {
	int bUsed;
	SOCKET sSocket;
	int bLogIn;
	int nLogInType;
	int LowRateflag;
	unsigned long connect_time;
} ClientData;

/*DSP client param*/
typedef struct __DSPCliParam
{
	int dspFD;     	//DSP handle
	ClientData cliDATA[MAX_CLIENT]; //client param infomation
}DSPCliParam;

#if 1
typedef struct _CTime {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;	
}CTime;
#endif





int StartEncoderMangerServer1();

void SendAudioToClient1(int nLen, unsigned char *pData, int nFlag, unsigned char index, unsigned int samplerate);

void SendDataToClient1(int nLen, unsigned char *pData,int nFlag, unsigned char index, int width, int height);

int get_MMP1_client_num();

int MMP_config_update1(int high, int type ,unsigned char *data,int len);

int get_MMP1_connent(void);

void LowSendAudioToClient1(int nLen, unsigned char *pData,
                           int nFlag, unsigned char index, unsigned int samplerate);


#endif

