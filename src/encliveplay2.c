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
#include <ctype.h>


#include "reach_enc2000.h"


#include "params.h"
#include "osd.h"
#include "select.h"

#include "common.h"
#include "remotectrl.h"

#include "encliveplay2.h"
#include "encliveplay.h"
#include "middle_control.h"
#include "MMP_control.h"

#include "app_video_control.h"
#include "app_audio_control.h"
#include "log_common.h"

#include "process_xml_cmd.h"
#include "app_head.h"
#include "sysparams.h"
#include "mid_socket.h"
#include "video.h"
#include "app_text_logo.h"


#define MMP_IND_INPUT SIGNAL_INPUT_2
#define MMP_IND_HIGH_CHANNEL CHANNEL_INPUT_2
#define MMP_IND_LOW_CHANNEL CHANNEL_INPUT_2_LOW
#define MMP_MP_INPUT  SIGNAL_INPUT_MP
#define MMP_MP_HIGH_CHNNAL CHANNEL_INPUT_MP
#define MMP_MP_LOW_CHANNEL CHANNEL_INPUT_MP_LOW
#define MMP_SYSINFO_INDEX   SECOND_MMP

#define  PORT_ONE 0
#define  PORT_TWO 1

#define HEAD_LEN			sizeof(MSGHEAD)

extern ENC2000_LINK_STRUCT	gEnc2000;

static int 	gRunStatus = 1;
static sem_t	g_sem;
static DSPCliParam		gDSPCliPara[DSP_NUM];
static pthread_mutex_t g_send_m;



static int WriteData(int s, void *pBuf, int nSize);
static unsigned long getCurrentTime2(void);
static void PackHeaderMSG(BYTE *data, BYTE type, WORD len);
static int GetNullClientData(unsigned char dsp);
static void ClearLostClient(unsigned char dsp);
static void SetAudioHeader(FRAMEHEAD *pAudio, AudioEncodeParam *pSys);
static int app_mmp_ainfo_get(int socket, int, int chid, unsigned char *data);
static int app_mmp_ainfo_set(int chid, int input , unsigned char *data);
static int app_mmp_vinfo_get(int socket, int chid, unsigned char *data);
static int app_mmp_vinfo_set(int chid, unsigned char *data);
static int app_mmp_sysinfo_get(int socket, int input, unsigned char *data, int mmp);
static int app_mmp_sysinfo_set(int input, unsigned char *data, int mmp);
static void SendBroadCastMsg(int index, BYTE cmd, unsigned char *data, int len);
static void LowRateBroadCastMsg(int index, BYTE cmd, unsigned char *data, int len);
static int AppSetQualityResize(unsigned char data[], int len, int channel);
static int lock_screen(unsigned char *data, int len, int channel);
static int CheckLowRateParam(const MMPLowVideoParam *pp);
static int RetLowRateParam(unsigned char *data, int *len, int channel);
static int RequestLowRate(int num, unsigned char *data, int len, int nPos, int channel);
static int mid_recv(SOCKET s, char *buff, int len, int flags);
static void EncoderProcess(void *pParams);
static void InitClientData(unsigned char port_num);
static int EncoderServerThread();
static int UploadImage(int input, int sSocket, char *data, unsigned char *logoname);

extern 	int MMP_config_update1(int high, int type , unsigned char *data, int len);
extern 	int mmp_set_audio_info(int input, MMPAudioParam *aparam, int *change);
extern int MMP_audio_info_get(int input, MMPAudioParam *aparam);
extern int web_get_audio_info(int input, AudioEncodeParam *aparam);


static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	//	pthread_mutex_lock(&gSetP_m.send_m);
	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			WARN_PRN("WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, errno, s);

			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr, "network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}

			//	pthread_mutex_unlock(&gSetP_m.send_m);
			return nRet;
		} else if(nRet == 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen,  errno, s);
			fprintf(stderr, "Send Net Data Error nRet= %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	//	pthread_mutex_unlock(&gSetP_m.send_m);
	return nWriteLen;
}


static unsigned long getCurrentTime2(void)
{
	unsigned long ultime = 0;
	ultime = getCurrentTime();
	return (ultime);
}

/*?????*/
static void PackHeaderMSG(BYTE *data,
                          BYTE type, WORD len)
{
	MSGHEAD  *p;
	p = (MSGHEAD *)data;
	memset(p, 0, HEAD_LEN);
	p->nLen = htons(len);
	p->nMsg = type;
	return ;
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


static void ClearLostClient(unsigned char dsp)
{
	int cli ;

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		if(!ISUSED(dsp, cli) && ISSOCK(dsp, cli)) {
			close(GETSOCK(dsp, cli));
			SETSOCK(dsp, cli, INVALID_SOCKET);
			SETCLILOGIN(dsp, cli, FALSE);
			SETCLIUSED(dsp, cli, FALSE);
			PRINTF("set lost client!\n");
			SETLOWRATEFLAG(dsp, cli, LOW_VIDEO_STOP);
			//		SETPPTINDEX(dsp, cli, FALSE);
			//		SETPPTQUERY(dsp, cli, FALSE);
		}
	}
}




/*set audio header infomation*/
static void SetAudioHeader(FRAMEHEAD *pAudio, AudioEncodeParam *pSys)
{
	pAudio->nDataCodec = 0x53544441;					//"ADTS"
	pAudio->nFrameRate = pSys->SampleRateIndex; 			//sample rate 1-----44.1KHz
	pAudio->nWidth = pSys->Channel;					//channel (default: 2)
	pAudio->nHight = pSys->SampleBit;				//sample bit (default: 16)
	pAudio->nColors = pSys->BitRate;					//bitrate  (default:64000)
}

//get mmp audio info
static int app_mmp_ainfo_get(int socket, int audio_input, int chid, unsigned char *data)
{
	int length = 0, ret;
	MMPAudioParam audio_param;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	int mp_status = get_mp_status();

	if(chid < 0 || chid > CHANNEL_INPUT_MAX) {
		fprintf(stderr, "msg_get_audio_param failed, chid error, %d\n", chid);
		return -1;
	}

	if(mp_status == IS_MP_STATUS) {
		input_get_audio_input(audio_input, &input);
	} else if(mp_status == IS_IND_STATUS) {
		channel_get_input_info(chid, &input, &high);
	}

	memset(&audio_param, 0, sizeof(MMPAudioParam));

	length = HEAD_LEN + sizeof(MMPAudioParam);
	PackHeaderMSG(data, MSG_GET_AUDIOPARAM, length);

	channel_get_input_info(chid, &input, &high);

	ret = MMP_audio_info_get(input, &audio_param);

	if(ret < 0) {
		PRINTF("get_video_param error!\n");
		return -1;
	}

	memcpy(data + HEAD_LEN, &audio_param, sizeof(MMPAudioParam));
	ret = WriteData(socket, data, length);

	if(ret < 0) {
		PRINTF("msg_get_video_param error %d socket:%d\n", ret, socket);
		return -1;
	}

	return 0;
}

//set mmp audio info
static int app_mmp_ainfo_set(int chid, int audio_input, unsigned char *data)
{
	MMPAudioParam *pnew_param;
	MMPAudioParam old_param;
	int ret = 0;
	int change = 0;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	int mp_status = get_mp_status();

	if(chid < 0 || chid > MAX_CHANNEL) {
		ERR_PRN("msg_set_video_param failed, chid error, %d\n", chid);
		return -1;
	}

	if(NULL == data) {
		ERR_PRN("msg_set_video_param, params error!\n");
		return -1;
	}

	pnew_param = (MMPAudioParam *)data;

	if(mp_status == IS_MP_STATUS) {
		input_get_audio_input(audio_input, &input);
	} else if(mp_status == IS_IND_STATUS) {
		channel_get_input_info(chid, &input, &high);
	}

	ret =  mmp_set_audio_info(input, pnew_param, &change);

	if(ret < 0) {
		ERR_PRN("set video info failed\n");
		return -1;
	}

	//if change == 1,i will send to all the socket
	if(change == 1) {
		ret = MMP_audio_info_get(input, &old_param);

		if(ret < 0) {
			PRINTF("get_video_param error!\n");
			return -1;
		}

		SendBroadCastMsg(PORT_ONE, MSG_GET_AUDIOPARAM, (unsigned char *)&old_param, sizeof(MMPAudioParam));
	}

	return 0;
}

//get mmp video info
static int app_mmp_vinfo_get(int socket, int chid, unsigned char *data)
{
	int length = 0, ret;
	MMPVideoParam video_param;

	if(chid < 0 || chid > MAX_CHANNEL) {
		ERR_PRN("msg_get_video_param failed, chid error, %d\n", chid);
		return -1;
	}

	memset(&video_param, 0, sizeof(MMPVideoParam));

	length = HEAD_LEN + sizeof(MMPVideoParam);
	PackHeaderMSG(data, MSG_GET_VIDEOPARAM, length);

	ret = MMP_video_info_get(chid, &video_param);

	if(ret < 0) {
		PRINTF("get_video_param error!\n");
		return -1;
	}

	PRINTF("channel=%d:%dx%d\n", chid, video_param.nWidth, video_param.nHight);

	memcpy(data + HEAD_LEN, &video_param, sizeof(MMPVideoParam));
	ret = WriteData(socket, data, length);

	if(ret < 0) {
		PRINTF("msg_get_video_param error %d socket:%d\n", ret, socket);
		return -1;
	}

	return 0;
}

//set mmp video info
static int app_mmp_vinfo_set(int chid, unsigned char *data)
{
	MMPVideoParam *pnew_param;
	MMPVideoParam old_param;
	int ret = 0;
	int change = 0;

	if(chid < 0 || chid > MAX_CHANNEL) {
		ERR_PRN("msg_set_video_param failed, chid error, %d\n", chid);
		return -1;
	}

	if(NULL == data) {
		ERR_PRN("msg_set_video_param, params error!\n");
		return -1;
	}

	pnew_param = (MMPVideoParam *)data;
	ret =  MMP_video_info_set(chid, pnew_param, &change);

	if(ret < 0) {
		ERR_PRN("set video info failed\n");
		return -1;
	}

	//if change == 1,i will send to all the socket
	if(change == 1) {
		ret = MMP_video_info_get(chid, &old_param);

		if(ret < 0) {
			PRINTF("get_video_param error!\n");
			return -1;
		}

		SendBroadCastMsg(PORT_ONE, MSG_GET_VIDEOPARAM, (unsigned char *)&old_param, sizeof(MMPVideoParam));

		if(chid  == CHANNEL_INPUT_MP) { //mp ->input1
			MMP_config_update1(HIGH_STREAM, 2 , (unsigned char *)&old_param, sizeof(MMPVideoParam));
		}
	}

	return 0;
}


static int app_mmp_sysinfo_get(int socket, int input, unsigned char *data, int mmp)
{
	MMPSysParam info;
	int length = 0;
	int ret = 0;
	memset(&info, 0, sizeof(MMPSysParam));
	MMP_get_sysparam(input, &info, mmp);

	length = HEAD_LEN + sizeof(MMPSysParam);
	PackHeaderMSG(data, MSG_SYSPARAMS, length);

	memcpy(data + HEAD_LEN, &info, sizeof(SYSPARAMS));
	ret = WriteData(socket, data, length);

	if(ret < 0) {
		PRINTF("msg_get_system_param error %d socket:%d\n", ret, socket);
		return -1;
	}

	return 0;

}
static int app_mmp_sysinfo_set(int input, unsigned char *data, int mmp)
{
	MMPSysParam *info = (MMPSysParam *)data;
	MMPSysParam newinfo;

	memset(&newinfo, 0, sizeof(MMPSysParam));
	//	int length = 0;
	//	int ret = 0;
	int change = 0;

	change = MMP_set_sysparam(input,  info, mmp);

	if(change == 1) {
		MMP_get_sysparam(input, &newinfo, mmp);
		SendBroadCastMsg(PORT_ONE, MSG_SYSPARAMS, (unsigned char *)&newinfo, sizeof(SYSPARAMS));
		PRINTF("the sys info is change \n");
		system("sync");
		//		sleep(4);
		//		PRINTF("----------ENCliveplay2---------reboot----------\n");
		//		system("reboot -f");
	}


	return 0;
}


/*broadcast message*/
static void SendBroadCastMsg(int index, BYTE cmd, unsigned char *data, int len)
{
	unsigned char temp[200];
	int length = 0, ret = 0, cnt = 0;

	length = HEAD_LEN + len;
	PackHeaderMSG(temp, cmd, length);
	memcpy(temp + HEAD_LEN, data, len);
	PRINTF("<><><><><><>SendBroadCastMsg()\n");

	for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
		if(ISUSED(PORT_ONE, cnt) && ISLOGIN(PORT_ONE, cnt))	{
			ret = WriteData(GETSOCK(PORT_ONE, cnt), temp, length);

			if(ret < 0) {
				ERR_PRN("SendBroadCastMsg()return:%d socket:%d ERROR:%d\n", ret, GETSOCK(PORT_ONE, cnt), errno);
				SETCLIUSED(PORT_ONE, cnt, FALSE);
				SETCLILOGIN(PORT_ONE, cnt, FALSE);
			}
		}
	}

	PRINTF("SendBroadCastMsg() succeed:%d \n", ret);
}

/*broadcast message*/
static void LowRateBroadCastMsg(int index, BYTE cmd, unsigned char *data, int len)
{
	unsigned char temp[200];
	int length = 0, ret = 0, cnt = 0;

	length = HEAD_LEN + len;
	PackHeaderMSG(temp, cmd, length);
	memcpy(temp + HEAD_LEN, data, len);

	for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
		if(ISUSED(PORT_ONE, cnt) && ISLOGIN(PORT_ONE, cnt) && GETLOWRATEFLAG(PORT_ONE, cnt) == LOW_VIDEO_START)	{
			ret = WriteData(GETSOCK(PORT_ONE, cnt), temp, length);

			if(ret < 0) {
				ERR_PRN("SendBroadCastMsg()return:%d socket:%d ERROR:%d\n", ret, GETSOCK(PORT_ONE, cnt), errno);
				SETCLIUSED(PORT_ONE, cnt, FALSE);
				SETCLILOGIN(PORT_ONE, cnt, FALSE);
			}
		}
	}

	//PRINTF( "SendBroadCastMsg() succeed:%d framerate:%d\n", ret, gSysParaT.videoPara[PORT_ONE].nFrameRate);
}





/*set Quality Resize*/
static int AppSetQualityResize(unsigned char data[], int len, int channel)
{
	int width = 0 , height = 0 , bitrate = 0, quality = 0;
	MMPVideoParam vparam;
	int change = 0;
	int ret = 0;

	if(len < 13) {
		//WARN_PRN( "SetQualityResize() len < %d\n", len);
		return -1;
	}

	quality = data[0];
	width = data[1] | (data[2] << 8) | data[3] << 16 | data[4] << 24;
	height = data[5] | (data[6] << 8) | data[7] << 16 | data[8] << 24;
	bitrate = data[9] | (data[10] << 8) | data[11] << 16 | data[12] << 24;
	PRINTF("channel=%d\n", channel);
	PRINTF("[AppSetQualityResize] Recv width = %d  height = %d  bitrate = %d,quality=%d\n", width , height, bitrate, quality);

	if(bitrate < MIN_BITRATE_VALUE) {
		bitrate = MIN_BITRATE_VALUE;
	}


	//change bitrate
	memset(&vparam, 0, sizeof(MMPVideoParam));
	MMP_video_info_get(channel, &vparam);

	if(/*(vparam.sCbr == VIDEO_QUALITY) || */(vparam.sBitrate != bitrate)) {
		PRINTF("video bitrate will change to %d kb/s\n", bitrate);
		vparam.sCbr = VIDEO_SBITRATE;
		vparam.sBitrate = bitrate;
		MMP_video_info_set(channel, &vparam, &change);
	}

	//高质量 不锁定分辨率为auto
	if(width > 1920) {
		PRINTF("width>1920\n");
		width = vparam.nWidth;
	}

	if(height > 1080) {
		PRINTF("height>1080\n");
		height = vparam.nHight;
	}

	//set quailty resize
	MMP_quailty_lock_set(channel, quality, width, height);


	//if change == 1,i will send to all the socket
	if(change == 1) {
		ret = MMP_video_info_get(channel, &vparam);

		if(ret < 0) {
			PRINTF("get_video_param error!\n");
			return -1;
		}

		SendBroadCastMsg(PORT_ONE, MSG_GET_VIDEOPARAM, (unsigned char *)&vparam, sizeof(MMPVideoParam));
	}

	return 0;
}


static int lock_screen(unsigned char *data, int len, int channel)
{
	int flag = 0;

	if(len) {
		PRINTF("[LockCurrentResolution] [%d]\n", data[0]);

		if(MMP_LOCK_SCREEN_IN == data[0]) {
			MMP_recode_lock_set(channel, LOCK_SCALE);
			PRINTF("[LockCurrentResolution] LOCK_IN\n");
		}

		if(MMP_LOCK_SCREEN_OUT == data[0]) {
			MMP_recode_lock_set(channel, UNLOCK_SCALE);
			PRINTF("[LockCurrentResolution] LOCK_OUT\n");
		}
	}

	return flag;
}



/*???????*/
static int CheckLowRateParam(const MMPLowVideoParam *pp)
{
	if(pp->nHeight > LOWRATE_MAX_HEIGHT || pp->nWidth > LOWRATE_MAX_WIDTH
	   || pp->nHeight < LOWRATE_MIN_HEIGHT || pp->nWidth < LOWRATE_MIN_WIDTH) {
		ERR_PRN("CheckLowRateParam() ERROR width = %d height = %d\n", pp->nWidth, pp->nHeight);
		return 0;
	}

	if(pp->nBitrate < MIN_BITRATE_VALUE || pp->nBitrate > 4096) {
		ERR_PRN("CheckLowRateParam() ERROR low rate = %d \n", pp->nBitrate);
		return 0;
	}

	/*
	    if(pp->nFrame < 1 || pp->nFrame > 60) {
	        ERR_PRN("CheckLowRateParam() ERROR frames = %d \n",pp->nFrame);
	        return 0;
	    }  */
	return 1;
}



/*?????*/
static int RetLowRateParam(unsigned char *data, int *len, int channel)
{
	MMPLowVideoParam param;
	MMPVideoParam videoparam;
	int 	width = 0;
	int 	height = 0;
	int     length = 0;
	memset(&param, 0, sizeof(MMPLowVideoParam));
	memset(&videoparam, 0, sizeof(MMPVideoParam));
	//get low video info
	MMP_video_info_get(channel, &videoparam);

	MMP_scale_lock_get(channel, &width, &height);

	param.nWidth = width;
	param.nHeight = height;
	param.nBitrate = videoparam.sBitrate;
	param.nFrame = videoparam.nFrameRate;
	length = sizeof(MMPLowVideoParam) ;
	memcpy(data, &param, sizeof(MMPLowVideoParam));
	*len = length ;
	return 0;
}

/*?????*/
static int RequestLowRate(int num, unsigned char *data, int len, int nPos, int channel)
{
	unsigned char iis_lowrate = 0;
	MMPLowVideoParam low;
	MMPVideoParam videoparam;
	unsigned char temp[50];
	int ret = 0, length = 0;
	int change = 0;


	iis_lowrate = data[0];
	PRINTF("RequestLowRate() is start low rate status = %d \n", iis_lowrate);
	memset(&low, 0, sizeof(MMPLowVideoParam));
	memset(&videoparam, 0, sizeof(MMPVideoParam));

	memcpy(&low, data + 1, sizeof(MMPLowVideoParam));
	PRINTF("<>lowbit.nWidth = %d,lowbit.nHeight = %d<>\n", low.nWidth, low.nHeight);
	PRINTF("<>lowbit.nBitrate = %d,lowbit.nFrame = %d<>\n", low.nBitrate, low.nFrame);

	if(LOW_VIDEO_START == iis_lowrate) {
		//check low rate
		ret = CheckLowRateParam(&low);

		if(0 == ret) {
			goto EXIT;
		}

		//change bitrate
		memset(&videoparam, 0, sizeof(MMPVideoParam));
		MMP_video_info_get(channel, &videoparam);

		if(/*(videoparam.sCbr == VIDEO_QUALITY) || */(videoparam.sBitrate != low.nBitrate)
		        || (videoparam.nFrameRate != low.nFrame)) {
			PRINTF("video bitrate will change to %d kb/s\n", low.nBitrate);
			videoparam.sCbr = VIDEO_SBITRATE;
			videoparam.sBitrate = low.nBitrate;
			MMP_video_info_set(channel, &videoparam, &change);
		}

		//set low rate
		MMP_lowstream_lock_set(channel, low.nWidth, low.nHeight);


		/*???????*/
		SETLOWRATEFLAG(num, nPos, LOW_VIDEO_START);
		PRINTF("RequestLowRate() is start low rate num = %d , nPos = %d\n", num, nPos);
	} else {
		//close low stream
		//SETLOWRATESTART(STOP);
	}

EXIT:
	RetLowRateParam(temp, &length, channel);
	LowRateBroadCastMsg(PORT_ONE, MSG_LOW_BITRATE, temp, length);
	return 0;
}
static int mid_recv(SOCKET s, char *buff, int len, int flags)
{
	int toplen = 0;
	int readlen = 0;

	while(len - toplen > 0) {
		readlen =  recv(s, buff + toplen, len - toplen, flags);

		if(readlen <= 0) {
			//	PRINTF("ERROR\n");
			//	PRINTF("ERROR,recv[%d]:[%s]\n", errno, strerror(errno));
			return -1;
		}

		if(readlen != len - toplen) {
			PRINTF("WARNNING,i read the buff len = %d,i need len = %d\n", readlen, len);
		}

		toplen += readlen;
	}

	return toplen;
}

static void EncoderProcess(void *pParams)
{
	int					sSocket = 0;
	int  			lowstream = 0;
	char			szData[256] = {0}, szPass[] = "123";;

	int 					nPos					= 0;
	int 					nMsgLen 				= 0;
	int 					*pnPos					= (int *)pParams;
	int 					nLen					= 0;
	MSGHEAD header, *phead;
	int old_status = get_mp_status();
	int new_status = 0;

	int input = MMP_IND_INPUT;
	int channel = MMP_IND_HIGH_CHANNEL;
	int low_channel = MMP_IND_LOW_CHANNEL;

	if(old_status == IS_MP_STATUS) {
		input = MMP_MP_INPUT;
		channel = MMP_MP_HIGH_CHNNAL;
		low_channel = MMP_MP_LOW_CHANNEL;
	} else if(IS_IND_STATUS == old_status) {
		input = MMP_IND_INPUT;
		channel = MMP_IND_HIGH_CHANNEL;
		low_channel = MMP_IND_LOW_CHANNEL;
	}


	PRINTF("Enter DSP1TcpComMsg() function!!\n");
	nPos = *pnPos;
	free(pParams);
	sSocket = GETSOCK(DSP1, nPos);
	PRINTF("socket=%d---Enter DSP1TcpComMsg() function!!\n", sSocket);

	phead = &header;
	memset(&header, 0, sizeof(MSGHEAD));
	/*??TCP???? 3s*/
	mid_socket_set_sendtimeout(sSocket, 3000);
	memset(szData, 0, sizeof(szData));
	sem_post(&g_sem);

	while(gRunStatus) {
		new_status = get_mp_status();

		if(old_status != new_status) {
			PRINTF("status is change ,new status =%d,old_status=%d\n", new_status, old_status);
			goto ExitClientMsg;
		}


		memset(szData, 0, sizeof(szData));

		if(sSocket <= 0) {
			fprintf(stderr, "sSocket<0 !\n");
			goto ExitClientMsg;
		}

		PRINTF("\n");
		nLen = mid_recv(sSocket, szData, HEAD_LEN, 0);
		//PRINTF("socket=%d---receive length:%d,errno=%d\n", sSocket, nLen, errno);

		if(nLen < HEAD_LEN) {
			PRINTF("recv nLen < 2 || nLen == -1  goto ExitClientMsg\n ");

			goto ExitClientMsg;
		}

		memcpy(phead, szData, HEAD_LEN);

		nMsgLen = ntohs(*((short *)szData));
		phead->nLen = nMsgLen;
		PRINTF("nMsgLen=%d\n", nMsgLen);
		nLen = mid_recv(sSocket, szData + HEAD_LEN, nMsgLen - HEAD_LEN, 0);
		//	PRINTF("nMsgLen = %d,szData[2]=%d!\n", nMsgLen, szData[2]);

		if(nLen == -1 || nLen < nMsgLen - HEAD_LEN) {
			PRINTF("ERROR!nLen < nMsgLen -2  goto ExitClientMsg\n ");
			goto ExitClientMsg;
		}

		PRINTF("------- sSocket = %d, type = 0x%02x, nLen = %d, nMsgLen = %d -------\n", sSocket, phead->nMsg, nLen, nMsgLen);

		/*??????*/
		switch(phead->nMsg) {
			case MSG_REQ_AUDIO:
				PRINTF("MSG_REQ_AUDIO\n");
				break;

			case MSG_GET_AUDIOPARAM:
				PRINTF("MSG_GET_AUDIOPARAM\n");
				app_mmp_ainfo_get(sSocket, input, channel, (BYTE *)&szData[HEAD_LEN]);
				break;

			case MSG_SET_AUDIOPARAM:
				PRINTF("MSG_SET_AUDIOPARAM\n");
				app_mmp_ainfo_set(channel, input, (BYTE *)&szData[HEAD_LEN]);
				break;

			case MSG_GET_VIDEOPARAM:
				PRINTF("MSG_GET_VIDEOPARAM\n");
				app_mmp_vinfo_get(sSocket, channel, (BYTE *)&szData[HEAD_LEN]);
				PRINTF(" test MSG_REQ_I\n");
				//if high stream
				app_video_request_iframe(channel);
				break;

			case MSG_SET_VIDEOPARAM:
				PRINTF("MSG_SET_VIDEOPARAM\n");
				app_mmp_vinfo_set(channel, (BYTE *)&szData[HEAD_LEN]);
				break;

			case MSG_SETVGAADJUST:
				PRINTF("MSG_SETVGAADJUST\n");
				break;

			case MSG_GSCREEN_CHECK:
				PRINTF("MSG_GSCREEN_CHECK\n");

				break;

			case MSG_QUALITYVALUE:
				PRINTF("MSG_QUALITYVALUE\n");

				if(0 == lowstream) {
					AppSetQualityResize((unsigned char *)(szData + HEAD_LEN), phead->nLen - HEAD_LEN, channel);
					PRINTF("MSG_QUALITYVALUE\n");
				}

				break;

			case MSG_SET_DEFPARAM:
				PRINTF("MSG_SET_DEFPARAM\n");
				break;

			case MSG_ADD_TEXT:	//??
				PRINTF("MSG_ADD_TEXT\n");
				MMP_set_osd_text(input, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN);
				break;

				//获取图片叠加参数
			case MMP_MSG_GET_LOGOINFO: {
				int length = HEAD_LEN + sizeof(LogoInfo);
				LogoInfo logo;
				PRINTF("--MSG_GET_LOGOINFO--\n");
				get_logo_info(input, &logo);
				PRINTF("input=%d\n", input);
				PackHeaderMSG((BYTE *)szData, MMP_MSG_GET_LOGOINFO, length);
				memcpy(szData + HEAD_LEN, &logo, sizeof(LogoInfo));
				send(sSocket, szData, length, 0);
				PRINTF("Get MSG_GET_LOGOINFO \n");
				break;
			}

			//设置图片叠加参数
			case MMP_MSG_SET_LOGOINFO: {

				PRINTF("--MMP_MSG_SET_LOGOINFO--\n");
				WORD length;
				int ret = 0;
				LogoInfo logo;

				if(phead->nLen - HEAD_LEN !=  sizeof(LogoInfo)) {
					ERR_PRN("phead->nLen - HEAD_LEN !=	sizeof(LogoInfo)\n");
					PRINTF("phead->nLen- HEAD_LEN =%d,sizeof(LogoInfo)=%d\n",
					       phead->nLen - HEAD_LEN , sizeof(LogoInfo));
					//goto ExitClientMsg;
				}

				memcpy(&logo, szData + HEAD_LEN + 1, sizeof(LogoInfo));

				PRINTF("enable=%d,alpha=%d,x/y=%d/%d\n", logo.enable, logo.alpha, logo.x, logo.y);
				PRINTF("szData[HEAD_LEN]=%d\n", szData[HEAD_LEN]);

				if(1 == szData[HEAD_LEN]) {
					ret = UploadImage(input, sSocket, szData + HEAD_LEN + 1 + sizeof(LogoInfo), (unsigned char *)logo.filename);

					if(1 == ret) {			// check the logo file
						ret =  check_logo_png(input, logo.filename);

						if(ret == 0) {
							char com[128];
							snprintf(com, sizeof(com), "mv %s %s_%d.png", logo.filename, LOGOFILE, input);
							system(com);
							PRINTF("success change logo file. com=%s\n", com);
							ret = MMP_set_logo_info(input, &logo);

							if(ret == 0) {
								ret = 1;
							} else if(ret < 0) {
								ret = -1;
							}
						} else {
							PRINTF("It's not PNG!\n");
							ret = -1;
						}
					} else {
						goto ExitClientMsg;
					}
				} else if(0 == szData[HEAD_LEN]) {
					ret = MMP_set_logo_info(input, &logo);

					if(ret == 0) {
						ret = 1;
					} else if(ret < 0) {
						ret = -1;
					}
				}

				PRINTF("ret=%d\n", ret);
				length = HEAD_LEN + 1;
				PackHeaderMSG((BYTE *)szData, MSG_SET_LOGOINFO, length);
				szData[HEAD_LEN] = ret;
				send(sSocket, szData, length, 0);
				PRINTF("Set MSG_SET_LOGOINFO \n");
			}
			break;


			case MSG_FARCTRL:
				PRINTF("MSG_FARCTRL\n");
				FarCtrlCamera(DSP1, (unsigned char *)&szData[HEAD_LEN], nMsgLen - HEAD_LEN, input_get_remote(1));
				break;

			case MSG_SET_SYSTIME: {
				PRINTF("MSG_SET_SYSTIME\n");
				CTime ctime;
				DATE_TIME_INFO n_time;
				memcpy(&ctime, szData + HEAD_LEN, phead->nLen - HEAD_LEN);
				n_time.year = ctime.tm_year;
				n_time.month = ctime.tm_mon + 1;
				n_time.mday = ctime.tm_mday;
				n_time.hours = ctime.tm_hour;
				n_time.min = ctime.tm_min;
				n_time.sec = ctime.tm_sec;
				set_encode_time(&n_time, sizeof(DATE_TIME_INFO));
			}
			break;

			case MSG_PASSWORD:
				PRINTF("MSG_PASSWORD\n");
				{
					if(!(strcmp("sawyer", szData + HEAD_LEN))) {
						SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);

					} else if(szData[HEAD_LEN] == 'A' && !strcmp(szPass, szData + HEAD_LEN + 1)) {
						SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);
						PRINTF("logo Admin!\n");
					} else if(szData[HEAD_LEN] == 'U' && !strcmp(szPass, szData + HEAD_LEN + 1)) {

						SETLOGINTYPE(PORT_ONE, nPos, LOGIN_USER);
						PRINTF("logo User!\n");
					} else {
						PackHeaderMSG((BYTE *)szData, MSG_PASSWORD_ERR, HEAD_LEN);
						send(sSocket, szData, HEAD_LEN, 0);
						PRINTF("logo error!\n");
						SETLOGINTYPE(PORT_ONE, nPos, LOGIN_ADMIN);
						goto ExitClientMsg;   //
					}

					PackHeaderMSG((BYTE *)szData, MSG_CONNECTSUCC, HEAD_LEN);
					send(sSocket, szData, HEAD_LEN, 0);
					PRINTF("send MSG_CONNECTSUCC!\n");

					//msg_get_system_param(sSocket, (unsigned char *)szData, gp_realtime_params);
					app_mmp_sysinfo_get(sSocket, input, (BYTE *)szData, MMP_SYSINFO_INDEX);

					/*set client login succeed*/
					SETCLIUSED(PORT_ONE, nPos, TRUE);
					SETCLILOGIN(PORT_ONE, nPos, TRUE);
					PRINTF("DSP:%d	 socket:%d\n", PORT_ONE, GETSOCK(PORT_ONE, nPos));
					PRINTF("ISUSED=%d, ISLOGIN=%d\n", ISUSED(PORT_ONE, nPos), ISLOGIN(PORT_ONE, nPos));
					break;
				}

			case MSG_SYSPARAMS:
				PRINTF("MSG_SYSPARAMS\n");
				app_mmp_sysinfo_get(sSocket, input, (BYTE *)&szData[HEAD_LEN], MMP_SYSINFO_INDEX);
				break;

			case MSG_SETPARAMS:
				//??????MAC????
				PRINTF("MSG_SETPARAMS\n");
				{
					unsigned char ParamBuf[200];
					SYSPARAMS *Newp;

					memcpy(ParamBuf, &szData[HEAD_LEN], nMsgLen - HEAD_LEN);

					Newp = (SYSPARAMS *)&ParamBuf[0];

					app_mmp_sysinfo_set(input, (unsigned char *)(&szData[HEAD_LEN]), MMP_SYSINFO_INDEX);
					PRINTF("==================================================\n");
					PRINTF("new:	unChannel = %d\n", Newp->unChannel);
					PRINTF("new:	dwAddr    = %d\n", Newp->dwAddr);
					PRINTF("new:	strName   = %s\n", Newp->strName);
					PRINTF("new:	bType     = %d\n", Newp->bType);
					PRINTF("==================================================\n");
				}
				break;

			case MSG_SAVEPARAMS:		//?????flash
				PRINTF("MSG_SAVEPARAMS\n");
				//	msg_save_param_table(gp_forever_params, gp_realtime_params);
				break;

			case MSG_RESTARTSYS:
				PRINTF("MSG_RESTARTSYS\n");
				system("sync");
				sleep(4);
				PRINTF("Restart sys\n");
				system("reboot -f");
				break;

			case MSG_UpdataFile: {
				PRINTF("MSG_UpdataFile\n");
				//升级过程
				int ret = 0 ;
				PRINTF("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n");
				ret = update_program(sSocket, szData, phead->nLen);

				if(ret <= 1) {
					goto ExitClientMsg;
				}
			}
			break;

			case MSG_UPDATEFILE_ROOT:
				PRINTF("MSG_UPDATEFILE_ROOT\n");
				break;

			case MSG_REQ_I:
				PRINTF("MSG_REQ_I\n");
				//if high stream
				app_video_request_iframe(channel);
				break;

			case MSG_MUTE:
				PRINTF("MSG_MUTE\n");
				break;

			case MSG_LOCK_SCREEN:
				PRINTF("MSG_LOCK_SCREEN\n");
				lock_screen((unsigned char *)&szData[3], nMsgLen - 3, channel);
				break;

			case MSG_LOW_BITRATE:
				PRINTF("MSG_LOW_BITRATE\n");
				{
					RequestLowRate(PORT_ONE, (BYTE *)&szData[HEAD_LEN], phead->nLen - HEAD_LEN, nPos, low_channel);
					lowstream = 1;
				}
				break;


			case MSG_SEND_INPUT:
				PRINTF("MSG_SEND_INPUT\n");
				break;

			default:
				PRINTF("unknown Cmd=0x%02x!\n", phead->nMsg);
				break;
		}

	}


ExitClientMsg:

	PRINTF("\nsSocket=%d---|||||||| go down  nPos=%d|||||||||\n\n", sSocket, nPos);
	PRINTF(" socket = %d--- %d\n", sSocket, GETSOCK(DSP1, nPos));

	SETCLIUSED(DSP1, nPos, FALSE);
	SETSOCK(DSP1, nPos, INVALID_SOCKET);
	SETLOGINTYPE(DSP1, nPos, LOGIN_USER);
	SETCLILOGIN(DSP1, nPos, FALSE);

	//SETPPTINDEX(DSP1, nPos, FALSE);
	//	SETPPTQUERY(DSP1, nPos, FALSE);
	//	SETLOWSTREAM(DSP1, nPos, 0);

	//	if(GETLOWSTREAMFLAG(DSP1, nPos)) {
	//		SETLOWRATEFLAG(DSP1, nPos, LOW_VIDEO_START);
	//	}
	SETLOWRATEFLAG(DSP1, nPos, LOW_VIDEO_STOP);
	close(sSocket);
	PRINTF("exit is ok\n");
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}


/*Initial client infomation*/
static void InitClientData(unsigned char port_num)
{
	int cli;

	for(cli = 0; cli < MAX_CLIENT; cli++) 	{
		SETCLIUSED(port_num, cli, FALSE);
		SETSOCK(port_num, cli, INVALID_SOCKET);
		SETCLILOGIN(port_num, cli, FALSE);
		SETLOGINTYPE(port_num, cli, LOGIN_USER);
		SETLOWRATEFLAG(port_num, cli, LOW_VIDEO_STOP);
	}

	return;
}

int get_MMP2_connent(void)
{
	int cli = 0;
	int num = 0;

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		if(ISUSED(DSP1, cli) && ISSOCK(DSP1, cli)) {
			num++;
		}
	}

	return num;
}


static void print_client_npos(void)
{
	int cli = 0;
	int sock = 0;

	for(cli = 0; cli < MAX_CLIENT; cli++) 	{
		sock = GETSOCK(DSP1, cli);
		PRINTF("..cli=%d,sock=%d,used=%d,..\n", cli, sock, ISUSED(DSP1, cli));
	}
}

int get_MMP2_client_num()
{
	int num = 0;
	int cli = 0, sock = 0;

	for(cli = 0; cli < MAX_CLIENT; cli++) 	{
		sock = GETSOCK(DSP1, cli);
		PRINTF("..cli=%d,sock=%d,used=%d,..\n", cli, sock, ISUSED(DSP1, cli));

		if(sock > 0 && ISUSED(DSP1, cli)) {
			num++;
		}
	}

	//	PRINTF("client client num = %d\n", num);
	return num;
}

static int EncoderServerThread()
{
	struct sockaddr_in		SrvAddr, ClientAddr;
	int						sClientSocket;
	int						ServSock;

	pthread_t				client_threadid[MAX_CLIENT] = {0};

	short					sPort					= PORT_LISTEN_DSP1;
	void					*ret 					= 0;

	int 					clientsocket				= 0;
	int 					nLen					= 0;;
	int 					ipos						= 0;
	int 					fileflags					= 0;
	int 					opt 					= 1;


	sem_init(&g_sem, 0, 0);
	int mp_status = get_mp_status();
	unsigned int ip = 0;
	unsigned int gateway = 0;
	unsigned int netmask = 0;
	int ret1 = 0;
	struct in_addr addr1;
	char newipconnect[20];
	//	sem_init(&g_sem, 0, 0);
	InitClientData(PORT_ONE);
SERVERSTARTRUN:

	ret1 = sys_get_network(ETH0_1, &ip, &gateway, &netmask);

	if(ret1 == -1) {
		PRINTF("Error,get ip is error\n");
		sleep(5);
		goto SERVERSTARTRUN;
	}

	memcpy(&addr1, &ip, 4);
	bzero(&SrvAddr, sizeof(struct sockaddr_in));
	SrvAddr.sin_family = AF_INET;
	SrvAddr.sin_port = htons(sPort);
	//	SrvAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	SrvAddr.sin_addr  = addr1;

	ServSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(ServSock < 0) {
		ERR_PRN("ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}

	setsockopt(ServSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(ServSock, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) < 0) {
		ERR_PRN("bind terror:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}

	if(listen(ServSock, 10) < 0) {
		ERR_PRN("listen error:%d,error msg: = %s", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}

	if((fileflags = fcntl(ServSock, F_GETFL, 0)) == -1) {
		ERR_PRN("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}

	if(fcntl(ServSock, F_SETFL, fileflags | O_NONBLOCK) == -1) {
		ERR_PRN("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		gRunStatus = 0;
		return -1;
	}


	nLen = sizeof(struct sockaddr_in);

	while(gRunStatus) {
		mp_status = get_mp_status();
		memset(&ClientAddr, 0, sizeof(struct sockaddr_in));

		nLen = sizeof(struct sockaddr_in);
		sClientSocket = accept(ServSock, (void *)&ClientAddr, (DWORD *)&nLen);

		if(mp_status == SWITCH_STATUS) {
			close(sClientSocket);

			if(ServSock > 0) {
				close(ServSock);
			}

			usleep(500000);
			goto SERVERSTARTRUN;
		}

		if(sClientSocket > 0) {
			inet_ntop(AF_INET, (void *) & (ClientAddr.sin_addr), newipconnect, 16);
			PRINTF("accept a valid connect!!!!-----------------, socket = %d,IP=%s\n\n\n\n", sClientSocket, newipconnect);
			int nPos = 0;
			//			print_client_npos();
			ClearLostClient(DSP1);
			nPos = GetNullClientData(DSP1);

			if(-1 == nPos) {
				char chData[20];
				PackHeaderMSG((BYTE *)chData, MSG_MAXCLIENT_ERR, HEAD_LEN);
				send(sClientSocket, chData, 3, 0);
				close(sClientSocket);
				ERR_PRN("MAXCLIENT ERR!\n");
			} else {
				int nSize = 0;
				int result;
				int *pnPos = malloc(sizeof(int));

				/* set client used */
				SETCLIUSED(DSP1, nPos, TRUE);
				SETSOCK(DSP1, nPos, sClientSocket) ;

				nSize = 1;

				if((setsockopt(sClientSocket, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
				               sizeof(nSize))) == -1) {
					fprintf(stderr, "setsockopt failed");
				}

				nSize = 0;
				nLen = sizeof(nLen);
				result = getsockopt(sClientSocket, SOL_SOCKET, SO_SNDBUF, &nSize , (DWORD *)&nLen);

				if(result) {
					fprintf(stderr, "getsockopt() errno:%d socket:%d  result:%d\n", errno, sClientSocket, result);
				}

				nSize = 1;

				if(setsockopt(sClientSocket, IPPROTO_TCP, TCP_NODELAY, &nSize , sizeof(nSize))) {
					fprintf(stderr, "Setsockopt error%d\n", errno);
				}

				PRINTF("Clent:%s connected,nPos:%d socket:%d!\n", inet_ntoa(ClientAddr.sin_addr), nPos, sClientSocket);
				*pnPos = nPos;

				result = pthread_create(&client_threadid[nPos], NULL, (void *)EncoderProcess, (void *)pnPos);
				usleep(300000);

				if(result) {
					close(sClientSocket);	//
					fprintf(stderr, "creat pthread ClientMsg error  = %d!\n" , errno);
					continue;
				}

				sem_wait(&g_sem);
				PRINTF("sClientSocket=%d---sem_wait() semphone inval!!!  result = %d\n", sClientSocket, result);
			}
		} else {
			if(errno == ECONNABORTED || errno == EAGAIN)
				//??????
			{
				usleep(100000);
				continue;
			}

			if(ServSock > 0) {
				close(ServSock);
			}

			goto SERVERSTARTRUN;
		}

	}

	fprintf(stderr, "begin exit the DSP1TCPTask \n");

	fprintf(stderr, "exit the drawtimethread \n");

	for(ipos = 0; ipos < MAX_CLIENT; ipos++) {
		if(client_threadid[ipos]) {
			clientsocket = GETSOCK(DSP1, ipos);

			if(clientsocket != INVALID_SOCKET) {
				close(clientsocket);
				SETSOCK(DSP1, ipos, INVALID_SOCKET);
			}

			if(pthread_join(client_threadid[ipos], &ret) == 0) {
			}
		}
	}

	fprintf(stderr, "close the encoder server socket \n");

	if(ServSock > 0) {
		fprintf(stderr, "close gserv socket \n");
		close(ServSock);
	}

	ServSock = 0;

	return 0;
}


int stop_encoder_mangerser2()
{
	gRunStatus = 0 ;
	return 0;
}


int StartEncoderMangerServer2()
{
	int result;
	pthread_t	serverThid;

	pthread_mutex_init(&g_send_m, NULL);
	result = pthread_create(&serverThid, NULL, (void *)EncoderServerThread, (void *)NULL);

	if(result < 0) {
		fprintf(stderr, "create EncoderServerThread() failed\n");
		return -1;
	}

	return 0;
}

static unsigned long 	g_audio_time = 0;
static unsigned long 	g_video_time = 0;
static unsigned int 	gnAudioCount = 0;
static unsigned int		gnSentCount  = 0;
static unsigned char	gszSendBuf[MAX_PACKET] = {0};
static unsigned char 	gszAudioBuf[MAX_PACKET] = {0};


static unsigned long 	g_low_audio_time = 0;
static unsigned long 	g_low_video_time = 0;
static unsigned int 	gnLowAudioCount = 0;
static unsigned int		gnLowSentCount  = 0;
static unsigned char	gszLowSendBuf[MAX_PACKET] = {0};
static unsigned char 	gszLowAudioBuf[MAX_PACKET] = {0};


/*send encode audio data to every client*/
void SendAudioToClient2(int nLen, unsigned char *pData,
                        int nFlag, unsigned char index, unsigned int samplerate)
{

	pthread_mutex_lock(&g_send_m);
	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	FRAMEHEAD  DataFrame;
	AudioEncodeParam aparam;
	int cnt = 0;
	MSGHEAD  *p;

	//	ParsePackageLock();
	bzero(&DataFrame, sizeof(FRAMEHEAD));
	memset(&aparam, 0, sizeof(AudioEncodeParam));
	DataFrame.dwPacketNumber = gnAudioCount++;

	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(FRAMEHEAD) - HEAD_LEN;

	DataFrame.ID = 0x56534434;		// " 4DSP"
	DataFrame.nFrameLength = nLen;


	unsigned int timeTick = 0;

	if(g_audio_time == 0) {
		timeTick = 0 ;
		g_audio_time = getCurrentTime2() ;
	} else {
		timeTick = getCurrentTime2() - g_audio_time;
	}

	DataFrame.nTimeTick = timeTick;
	//printf("audio[2]:[%I64d]\n", timeTick);
	web_get_audio_info(MMP_IND_INPUT, &aparam);
	SetAudioHeader(&DataFrame,  &aparam);


	if(nFlag == 1) {
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	while(nSent < nLen) {

		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;
			} else {
				DataFrame.dwSegment = 0;
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;
			} else {
				DataFrame.dwSegment = 1;
			}

			nPacketCount++;
		}

		memcpy(gszAudioBuf + HEAD_LEN, &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszAudioBuf + sizeof(FRAMEHEAD) + HEAD_LEN, pData + nSent, nSendLen);
		//PackHeaderMSG(gszAudioBuf,MSG_AUDIODATA,(nSendLen+sizeof(FRAMEHEAD)+HEAD_LEN));
		p = (MSGHEAD *)gszAudioBuf;
		memset(p, 0, HEAD_LEN);
		p->nLen = htons((nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN));
		p->nMsg = MSG_AUDIODATA;


		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(index, cnt) && ISLOGIN(index, cnt)
			   && (GETLOWRATEFLAG(0, cnt) != LOW_VIDEO_START)) {
				nRet = WriteData(GETSOCK(index, cnt), gszAudioBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);

				if(nRet < 0) {
					SETCLIUSED(index, cnt, FALSE);
					SETCLILOGIN(index, cnt, FALSE);
					PRINTF("Audio Send error index:%d, cnt:%d %d nSendLen:%d  ,nRet:%d\n", index, cnt, errno, nSendLen, nRet);
				}
			}
		}

#ifdef SUPPORT_IP_MATRIX
		SendAudioDataToIpMatrixClient(1, gszAudioBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);
#endif
		nSent += nSendLen;
	}

	pthread_mutex_unlock(&g_send_m);
	//	ParsePackageunLock();
}

/*send encode audio data to every client*/
void LowSendAudioToClient2(int nLen, unsigned char *pData,
                           int nFlag, unsigned char index, unsigned int samplerate)
{

	//pthread_mutex_lock(&g_send_m);
	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	FRAMEHEAD  DataFrame;
	AudioEncodeParam aparam;
	int cnt = 0;
	MSGHEAD  *p;

	//	ParsePackageLock();
	bzero(&DataFrame, sizeof(FRAMEHEAD));
	memset(&aparam, 0, sizeof(AudioEncodeParam));
	DataFrame.dwPacketNumber = gnLowAudioCount++;

	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(FRAMEHEAD) - HEAD_LEN;

	DataFrame.ID = 0x56534434;		// " 4DSP"
	DataFrame.nFrameLength = nLen;


	unsigned int timeTick = 0;

	if(g_low_audio_time == 0) {
		timeTick = 0 ;
		g_low_audio_time = getCurrentTime2() ;
	} else {
		timeTick = getCurrentTime2() - g_low_audio_time;
	}

	DataFrame.nTimeTick = timeTick;

	web_get_audio_info(MMP_IND_INPUT, &aparam);
	SetAudioHeader(&DataFrame,  &aparam);


	if(nFlag == 1) {
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}

	while(nSent < nLen) {

		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;
			} else {
				DataFrame.dwSegment = 0;
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;
			} else {
				DataFrame.dwSegment = 1;
			}

			nPacketCount++;
		}

		memcpy(gszLowAudioBuf + HEAD_LEN, &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszLowAudioBuf + sizeof(FRAMEHEAD) + HEAD_LEN, pData + nSent, nSendLen);
		//PackHeaderMSG(gszAudioBuf,MSG_AUDIODATA,(nSendLen+sizeof(FRAMEHEAD)+HEAD_LEN));
		p = (MSGHEAD *)gszLowAudioBuf;
		memset(p, 0, HEAD_LEN);
		p->nLen = htons((nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN));
		p->nMsg = MSG_AUDIODATA;


		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(index, cnt) && ISLOGIN(index, cnt)
			   && (GETLOWRATEFLAG(0, cnt) == LOW_VIDEO_START)) {
				nRet = WriteData(GETSOCK(index, cnt), gszLowAudioBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);

				if(nRet < 0) {
					SETCLIUSED(index, cnt, FALSE);
					SETCLILOGIN(index, cnt, FALSE);
					PRINTF("Audio Send error index:%d, cnt:%d %d nSendLen:%d  ,nRet:%d\n", index, cnt, errno, nSendLen, nRet);
				}
			}
		}

		nSent += nSendLen;
	}

	//	pthread_mutex_unlock(&g_send_m);
	//	ParsePackageunLock();
}
void SendDataToClient2(int nLen, unsigned char *pData,
                       int nFlag, unsigned char index, int width, int height)
{

	pthread_mutex_lock(&g_send_m);

	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	FRAMEHEAD	DataFrame;
	int cnt = 0;
	SOCKET	sendsocket = 0;
	MSGHEAD	*p;

	bzero(&DataFrame, sizeof(FRAMEHEAD));
	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(FRAMEHEAD) - HEAD_LEN;


	DataFrame.ID = 0x56534434;		// "4DSP"
	DataFrame.nFrameLength = nLen;
	DataFrame.nDataCodec = 0x34363248;		//"H264"

	unsigned int timeTick = 0;

	if(g_video_time == 0) {
		timeTick = 0 ;
		g_video_time = getCurrentTime2();
	} else {

		timeTick = getCurrentTime2() - g_video_time;

	}

	DataFrame.nTimeTick = timeTick;
	//printf("video[2]:[%I64d]\n", timeTick);
	DataFrame.nWidth = width;		//video width
	DataFrame.nHight = height;		//video height

	if(DataFrame.nHight == 1088) {
		DataFrame.nHight = 1080;
	}


	if(nFlag == 1)
		//if I frame
	{
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}


	while(nSent < nLen) {
		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;    //start frame
			} else {
				DataFrame.dwSegment = 0;    //middle frame
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;    //first frame and last frame
			} else {
				DataFrame.dwSegment = 1;    //last frame
			}

			nPacketCount++;
		}

		DataFrame.dwPacketNumber = gnSentCount++;
		memcpy(gszSendBuf + HEAD_LEN, &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszSendBuf + sizeof(FRAMEHEAD) + HEAD_LEN, pData + nSent, nSendLen);
		p = (MSGHEAD *)gszSendBuf;
		memset(p, 0, HEAD_LEN);
		p->nLen = htons((nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN));
		p->nMsg = MSG_SCREENDATA;

		//send multi client
		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(0, cnt) && ISLOGIN(0, cnt) && (GETLOWRATEFLAG(0, cnt) != LOW_VIDEO_START)) {
				sendsocket = GETSOCK(0, cnt);

				if(sendsocket > 0) {
					nRet = WriteData(sendsocket, gszSendBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);

					if(nRet < 0) {
						SETCLIUSED(0, cnt, FALSE);
						SETCLILOGIN(0, cnt, FALSE);
						//		SETPPTQUERY(0, cnt, FALSE);
						PRINTF("Error: SOCK = %d count = %d  errno = %d  ret = %d\n", sendsocket, cnt, errno, nRet);
					}
				}
			}

		}

#ifdef SUPPORT_IP_MATRIX
		SendVideoDataToIpMatrixClient(1, gszSendBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);
#endif
		nSent += nSendLen;
	}

	pthread_mutex_unlock(&g_send_m);
}

void LowSendDataToClient2(int nLen, unsigned char *pData,
                          int nFlag, unsigned char index, int width, int height)
{

	//pthread_mutex_lock(&g_send_m);

	int nRet, nSent, nSendLen, nPacketCount, nMaxDataLen;
	FRAMEHEAD	DataFrame;
	int cnt = 0;
	SOCKET	sendsocket = 0;
	MSGHEAD	*p;

	bzero(&DataFrame, sizeof(FRAMEHEAD));
	nSent = 0;
	nSendLen = 0;
	nPacketCount = 0;
	nMaxDataLen = MAX_PACKET - sizeof(FRAMEHEAD) - HEAD_LEN;


	DataFrame.ID = 0x56534434;		// "4DSP"
	DataFrame.nFrameLength = nLen;
	DataFrame.nDataCodec = 0x34363248;		//"H264"

	unsigned int timeTick = 0;

	if(g_low_video_time == 0) {
		timeTick = 0 ;
		g_low_video_time = getCurrentTime2();
	} else {

		timeTick = getCurrentTime2() - g_low_video_time;

	}

	DataFrame.nTimeTick = timeTick;

	DataFrame.nWidth = width;		//video width
	DataFrame.nHight = height;		//video height

	if(DataFrame.nHight == 1088) {
		DataFrame.nHight = 1080;
	}


	if(nFlag == 1)
		//if I frame
	{
		DataFrame.dwFlags = AVIIF_KEYFRAME;
	} else {
		DataFrame.dwFlags = 0;
	}


	while(nSent < nLen) {
		if(nLen - nSent > nMaxDataLen) {
			nSendLen = nMaxDataLen;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 2;    //start frame
			} else {
				DataFrame.dwSegment = 0;    //middle frame
			}

			nPacketCount++;
		} else {
			nSendLen = nLen - nSent;

			if(nPacketCount == 0) {
				DataFrame.dwSegment = 3;    //first frame and last frame
			} else {
				DataFrame.dwSegment = 1;    //last frame
			}

			nPacketCount++;
		}


		DataFrame.dwPacketNumber = gnLowSentCount++;
		memcpy(gszLowSendBuf + HEAD_LEN, &DataFrame, sizeof(FRAMEHEAD));
		memcpy(gszLowSendBuf + sizeof(FRAMEHEAD) + HEAD_LEN, pData + nSent, nSendLen);
		p = (MSGHEAD *)gszLowSendBuf;
		memset(p, 0, HEAD_LEN);
		p->nLen = htons((nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN));
		p->nMsg = MSG_SCREENDATA;

		//send multi client
		for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
			if(ISUSED(0, cnt) && ISLOGIN(0, cnt) && (GETLOWRATEFLAG(0, cnt) == LOW_VIDEO_START)) {
				sendsocket = GETSOCK(0, cnt);

				if(sendsocket > 0) {
					nRet = WriteData(sendsocket, gszLowSendBuf, nSendLen + sizeof(FRAMEHEAD) + HEAD_LEN);
					//PRINTF(" socket = %d\n", sendsocket);

					if(nRet < 0) {
						SETCLIUSED(0, cnt, FALSE);
						SETCLILOGIN(0, cnt, FALSE);
						//		SETPPTQUERY(0, cnt, FALSE);
						PRINTF("Error: SOCK = %d count = %d  errno = %d  ret = %d\n", sendsocket, cnt, errno, nRet);
					}
				}
			}
		}

		nSent += nSendLen;
	}

	//pthread_mutex_unlock(&g_send_m);
}

// high :HIGH_STREAM,LOW_STREAM
// type :1 audio get  ,2 video get, 3 ,sysinfo get
int MMP_config_update2(int high, int type , unsigned char *data, int len)
{
	int cmd = 0;
	int needlen = 0;

	if(type == 1) {
		cmd = MSG_GET_AUDIOPARAM;
		needlen = sizeof(MMPAudioParam);
	} else if(type == 2) {
		cmd = MSG_GET_VIDEOPARAM;
		needlen = sizeof(MMPVideoParam);
	} else if(type == 3) {
		cmd = MSG_SYSPARAMS;
		needlen = sizeof(MMPSysParam);
	}

	cmd = cmd & 0xff;

	if(len != needlen) {
		ERR_PRN("len= %d,needlen=%d\n", len, needlen);
		return -1;
	}

	if(high == HIGH_STREAM) {
		SendBroadCastMsg(0,  cmd,  data,  len);
	} else if(high == LOW_STREAM) {
		LowRateBroadCastMsg(0,  cmd,  data,  len);
	}

	return 0;
}




/*升级png图片过程*/
static int UploadImage(int input, int sSocket, char *data, unsigned char *logoname)
{
	unsigned long filesize;
	unsigned char *p;
	int nLen;
	unsigned char temp[20];
	char filename[20];
	//	char com[128];
	int ret;
	FILE *fp = NULL;
	p = NULL;
	p = malloc(8 * 1024);
	nLen = HEAD_LEN + 1;
	PackHeaderMSG((BYTE *)temp, MSG_UPLOADIMG, nLen);
	temp[HEAD_LEN] = 1;
	PRINTF("The File Updata Buffer Is 8KB!\n");

	if(!p) {
		ERR_PRN("Malloc 8KB Buffer Failed!\n");
	} else {
		PRINTF("The Buffer Addr 0x%x\n", (unsigned int)p);
	}

	sprintf(filename, "%s", logoname);
	nLen = recv(sSocket, data, 4, 0);
	PRINTF("nLennLen=%d!\n", nLen);

	if(nLen < 4) {
		free(p);
		return -1;
	}

	filesize = ((unsigned char)data[0]) | ((unsigned char)data[1]) << 8 |
	           ((unsigned char)data[2]) << 16 | ((unsigned char)data[3]) << 24;
	PRINTF("Updata file size = 0x%x! \n", (unsigned int)filesize);

	if(filesize > 10 * 1024) {
		free(p);
		return -1;
	}

	//if(strcpy(filename,"logo.png")){
	//free(p);
	//return 0;
	//}
	if((fp = fopen(filename, "w+")) == NULL) {
		ERR_PRN("open %s error \n", filename);
		return 0;
	}

	while(filesize) {
		nLen = recv(sSocket, p, 8 * 1024, 0);

		if(nLen < 1) {
			return -2;
		}

		PRINTF("recv Updata File 0x%x Bytes!\n", nLen);
		ret = fwrite(p, 1, nLen, fp);

		if(ret < 0) {
			ERR_PRN("product update faile!\n");
			free(p);
			fclose(fp);
			p = NULL;
			temp[HEAD_LEN] = 0;
			WriteData(sSocket, temp, HEAD_LEN + 1);
			return -2;
		}

		PRINTF("write data ...........................:0x%x\n", (unsigned int)filesize);
		filesize = filesize - nLen;
	}

	PRINTF("recv finish......................\n");
	free(p);
	fclose(fp);
	p = NULL;

	PRINTF("upload image succeed !! \n");
	system("sync");
	return 1;
}

int MMP_close_all_client2()
{
	InitClientData(0);
	return 0;
}


