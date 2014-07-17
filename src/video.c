#include <stdio.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>


#include "reach_enc2000.h"
#include "common.h"
#include "video.h"
#include "capture.h"
#include "encliveplay.h"
#include "params.h"
#include "liveplay.h"

#ifdef HAVE_RTSP_MODULE
#include "app_stream_output.h"
#endif
#include "mid_mutex.h"
#include "mid_timer.h"
#include "log_common.h"
#include "app_protocol.h"
#include "app_head.h"

#include "reach_enc2000.h"

extern void SendDataToClient1(int nLen, unsigned char *pData, int nFlag, unsigned char index, int width, int height);
extern void SendDataToClient2(int nLen, unsigned char *pData, int nFlag, unsigned char index, int width, int height);
extern void LowSendDataToClient1(int nLen, unsigned char *pData, int nFlag, unsigned char index, int width, int height);
extern void LowSendDataToClient2(int nLen, unsigned char *pData, int nFlag, unsigned char index, int width, int height);
extern  int read_gaps_in_frame(unsigned char *p,  int *width,  int *hight, unsigned int len);

#ifdef HAVE_RTSP_MODULE
static	unsigned long long last_time[CHANNEL_INPUT_MAX]  =  {0};
#endif
#define HP_PROFILE_LEVEL   100   	//IH264_HIGH_PROFILE
#define MP_PROFILE_LEVEL   66		//IH264_BASELINE_PROFILE
static int enc_state = 0;
static int s_encoding = 1; //0--IH264_BASELINE_PROFILE  1--IH264_HIGH_PROFILE

int get_encoding_mode(void)
{
	return s_encoding;
}

void set_encoding_mode(int state)
{
	s_encoding = state;
}

int get_enc_state(void)
{
	return enc_state;
}

void set_enc_state(int state)
{
	enc_state = state;
}

static void *InHostStreamProcess(bits_user_param *param)
{
	Int32 status = -1;
	int64_t t[4] = {0};
	Int32 idr_header_length0 = 0;
	Int32 idr_header_length1 = 0;
	Int32 idr_header_length2 = 0;
	Int32 idr_header_length3 = 0;

	Int32 *temp_header_length = &idr_header_length0;
	Int32 length[4] = {0};
	Int32 framecount[4] = {0};
	Bitstream_Buf	*pFullBuf;
	UInt8 *p = NULL;
	int mp_status = 0;

	Uint32 width = 1920;
	Uint32 height = 1080;

#ifdef HAVE_RTSP_MODULE
	APP_VIDEO_DATA_INFO info;
	unsigned int capture_time = {0};
	unsigned long long rtp_time = {0};
	unsigned long long start_time = 0;
	unsigned long long 	temp_time = 0;
	unsigned char *vdata = NULL;
	int vlen = 0;
#endif

	if(param == NULL) {
		PRINTF("InHostStreamProcess: param is NULL!\n");
		return NULL;
	}

#ifdef HAVE_RECPLAY
	setLiveMaxUserNum(4);

	startLiveplayTask();
#endif

	OSA_QueHndl *full_que = &(param->bits_full_que);
	OSA_QueHndl *empty_que = &(param->bits_empty_que);

	char *hTempBuf = (char *)malloc(1920 * 1200);

	unsigned char h264Header0[0x40] = {0};
	unsigned char h264Header1[0x40]	= {0};
	unsigned char h264Header2[0x40]	= {0};
	unsigned char h264Header3[0x40]	= {0};

	unsigned char *temp_h264Header = NULL;

	int is_send_HIGH = 1;
	int is_send_LOW = 1;
	int temp_channel = CHANNEL_INPUT_1;
	int rtsp_channel = CHANNEL_INPUT_1;
	int is_keyframe = 0;
#ifdef HAVE_RECPLAY
	LiveVideoParam videoParam;
#endif

	while(TRUE == param->run_status) {
		status = OSA_queGet(full_que, (Int32 *)(&pFullBuf), OSA_TIMEOUT_FOREVER);
		OSA_assert(status == 0);
		set_enc_state(1);

#if PRINTF_BITSTREAM_INFO
		length[pFullBuf->channelNum] += pFullBuf->fillLength;

		if(pFullBuf->fillLength > 0) {
			framecount[pFullBuf->channelNum]++;
		}

		if(getostime() - t[pFullBuf->channelNum] >= 3000) {

			PRINTF("\n ##### ch%d: %d x %d	video bitrate = %d kb/s, framerate = %d fps\n",
			       pFullBuf->channelNum,
			       pFullBuf->frameWidth, pFullBuf->frameHeight,
			       length[pFullBuf->channelNum] / 1 / 1024 * 8 / 5,
			       framecount[pFullBuf->channelNum] / 5);

			framecount[pFullBuf->channelNum] = 0;
			length[pFullBuf->channelNum] = 0;
			t[pFullBuf->channelNum] = getostime();
		}

#endif
		mp_status  = get_mp_status();
		writeWatchDog();


		//	PRINTF("mp_status = %d ,pFullBuf->channelNum=%d .\n",mp_status,pFullBuf->channelNum);
		if(IS_IND_STATUS == mp_status) {
			if(pFullBuf->channelNum == 0) {
				//temp_channel =	CHANNEL_INPUT_1_LOW;
				temp_channel = enc_chid_get_channel(pFullBuf->channelNum, mp_status);
				temp_header_length = &idr_header_length0;
				temp_h264Header = h264Header0;
				is_send_HIGH = 0;
				is_send_LOW = 1;
			} else if(pFullBuf->channelNum == 1) {
				//temp_channel = CHANNEL_INPUT_1 ;
				temp_channel = enc_chid_get_channel(pFullBuf->channelNum, mp_status);
				temp_header_length = &idr_header_length1;
				temp_h264Header = h264Header1;
				is_send_HIGH = 1;
				is_send_LOW = 0;
			} else if(pFullBuf->channelNum == 2) {
				//temp_channel = CHANNEL_INPUT_2_LOW;
				temp_channel = enc_chid_get_channel(pFullBuf->channelNum, mp_status);
				temp_header_length = &idr_header_length2;
				temp_h264Header = h264Header2;
				is_send_HIGH = 0;
				is_send_LOW = 1;
			} else if(pFullBuf->channelNum == 3) {
				//temp_channel = CHANNEL_INPUT_2 ;
				temp_channel = enc_chid_get_channel(pFullBuf->channelNum, mp_status);
				temp_header_length = &idr_header_length3;
				temp_h264Header = h264Header3;
				is_send_HIGH = 1;
				is_send_LOW = 0;
			} else {
				is_send_HIGH = 0;
				is_send_LOW = 0;
				ERR_PRN("ERROR,please ,must check this code\n");
				continue;
			}
		} else if(IS_MP_STATUS == mp_status) {
			if(pFullBuf->channelNum == 0) {
				//temp_channel =	CHANNEL_INPUT_MP_LOW;
				//调用enc_chid_get_channel的原因mp的0通道与no_mp的重叠
				temp_channel = enc_chid_get_channel(pFullBuf->channelNum, mp_status); //0;
				temp_header_length = &idr_header_length0;
				temp_h264Header = h264Header0;
				is_send_HIGH = 0;
				is_send_LOW = 1;
			} else if(pFullBuf->channelNum == 1) {
				//temp_channel = CHANNEL_INPUT_MP ;
				temp_channel = enc_chid_get_channel(pFullBuf->channelNum, mp_status); // 1
				temp_header_length = &idr_header_length1;
				temp_h264Header = h264Header1;
				is_send_HIGH = 1;
				is_send_LOW = 0;
			} else {
				is_send_HIGH = 0;
				is_send_LOW = 0;
				ERR_PRN("ERROR,please ,must check this code\n");
				continue;
			}
		} else {
			PRINTF("WARNING:...........MP-status Warning\n");
			status = OSA_quePut(empty_que, (Int32)pFullBuf, OSA_TIMEOUT_NONE);
			OSA_assert(status == 0);
			continue;
		}

		//	PRINTF("mp_status = %d ,pFullBuf->channelNum=%d temp_channel =%d\n",mp_status,pFullBuf->channelNum,temp_channel);

		width = pFullBuf->frameWidth;
		height = pFullBuf->frameHeight;
		p = (unsigned char *)(pFullBuf->addr);

		//	if(pFullBuf->channelNum == 3 )
		//		{
		//			PRINTF("w=%d,H=%d\n",width,height);
		//		}
		static int i1 = 0;
		static int i2 = 0;
		static int i3 = 0;
		static int i4 = 0;
		i1++;
		i2++;
		i3++;
		i4++;


		if(((i1 % 500 == 0) && pFullBuf->channelNum == 0) ||
		   ((i2 % 500 == 0) && pFullBuf->channelNum == 1) ||
		   ((i3 % 500 == 0) && pFullBuf->channelNum == 2) ||
		   ((i4 % 500 == 0) && pFullBuf->channelNum == 3)) {
			PRINTF("CH=%d,width=%d,height=%d,len=%d\n", pFullBuf->channelNum, width, height, pFullBuf->fillLength);
			PRINTF("temp_channel=%d,is_send_MMP=%d,is_keyframe=%d\n", temp_channel, is_send_HIGH, is_keyframe);
		}


		if(pFullBuf->isKeyFrame == TRUE) {
			//			int w = 0, h = 0;

			is_keyframe = 1;

			if(p[4] == 0x27 || p[4] == 0x67) {

				//if(pFullBuf->channelNum == 1)
				{
					//					printf("IDR FRAME--------p[4] = %02x,%02x,%02x====pFullBuf->channelNum=%d\n", p[4], p[5], p[6], pFullBuf->channelNum);
				}

				*temp_header_length = ParseIDRHeader((unsigned char *)(pFullBuf->addr));
				memcpy(temp_h264Header, (unsigned char *)(pFullBuf->addr), *temp_header_length);
				vdata = (unsigned char *)(pFullBuf->addr);
				vlen = pFullBuf->fillLength;
				PRINTF("--->Ch=%d,IFrame len=%d\n",pFullBuf->channelNum,vlen);

				//read_gaps_in_frame(p, &w, &h, temp_header_length);
				//				PRINTF("video relusion:%dx%d=====\n", width, height);

				//width
			} else {

				if((pFullBuf->fillLength + *temp_header_length) > 1600000) {
					fprintf(stderr, "\n\n\n\n\n fatal err \n\n\n\n\n");
				}

				memcpy(hTempBuf, temp_h264Header, *temp_header_length);
				memcpy(hTempBuf + *temp_header_length, (unsigned char *)(pFullBuf->addr), pFullBuf->fillLength);
				vdata = (unsigned char *)(hTempBuf);
				vlen = pFullBuf->fillLength + *temp_header_length;
			}

		} else {
			vdata = (unsigned char *)(pFullBuf->addr);
			vlen = pFullBuf->fillLength;
			is_keyframe = 0;
		}

		if(is_send_HIGH == 1) {

			///????
			//	if(height > 1080) {
			//		height = 1080;
			//	}

			if(mp_status == IS_MP_STATUS) {
				if(temp_channel == CHANNEL_INPUT_MP) {
					//	PRINTF("==vlen=%d==\n",vlen);
					SendDataToClient1(vlen, vdata,
					                  is_keyframe, 0, width, height);

					SendDataToClient2(vlen, vdata,
					                  is_keyframe, 0, width, height);
				}
			} else if(IS_IND_STATUS == mp_status) {
				if(temp_channel == CHANNEL_INPUT_1) {
					unsigned int tm_time = getostime();
					SendDataToClient1(vlen, vdata,
					                  is_keyframe, 0, width, height);
					if(getostime()-tm_time > 10){
						PRINTF("------------1  Send Warning TimeOut!!!\n");
					}
				} else if(temp_channel == CHANNEL_INPUT_2) {
					unsigned int tm_time = getostime();
					SendDataToClient2(vlen, vdata,
					                  is_keyframe, 0, width, height);
					if(getostime()-tm_time > 10){
						PRINTF("------------2  Send Warning TimeOut!!!\n");
					}
				}
			}

		}

		if(is_send_LOW == 1) {

#ifdef HAVE_RECPLAY
			videoParam.nWidth = pFullBuf->frameWidth;
			videoParam.nHight = pFullBuf->frameHeight;
#endif

			if(mp_status == IS_MP_STATUS) {
				if(temp_channel == CHANNEL_INPUT_MP_LOW) {
					LowSendDataToClient1(vlen, vdata,
					                     is_keyframe, 0, width, height);
					LowSendDataToClient2(vlen, vdata,
					                     is_keyframe, 0, width, height);

				}
			} else if(IS_IND_STATUS == mp_status) {
				if(temp_channel == CHANNEL_INPUT_1_LOW) {
					LowSendDataToClient1(vlen, vdata,
					                     is_keyframe, 0, width, height);
				} else if(temp_channel == CHANNEL_INPUT_2_LOW) {
					LowSendDataToClient2(vlen, vdata,
					                     is_keyframe, 0, width, height);
				}
			}

		}

#if 0

		if(is_send_LOW == 1) {
			videoParam.nWidth = pFullBuf->frameWidth;
			videoParam.nHight = pFullBuf->frameHeight;

			if(mp_status == MP_STATUS) {
				if(temp_channel == 0) {
					LiveSendVideoDataToRecPlay(vlen, vdata,
					                           is_keyframe, 0, 0, &videoParam);
					LiveSendVideoDataToRecPlay(vlen, vdata,
					                           is_keyframe, 1, 0, &videoParam);
				}
			} else if(NO_MP_STATUS == mp_status) {
				if(temp_channel == CHANNEL_INPUT_1_LOW) {
					LiveSendVideoDataToRecPlay(vlen, vdata,
					                           is_keyframe, 0, 0, &videoParam);
				} else if(temp_channel == CHANNEL_INPUT_2_LOW) {
					LiveSendVideoDataToRecPlay(vlen, vdata,
					                           is_keyframe, 1, 0, &videoParam);
				}
			}

		}

#endif

#ifdef HAVE_RTSP_MODULE
		rtsp_channel = temp_channel;

		if(temp_channel == CHANNEL_INPUT_MP) {
			rtsp_channel = CHANNEL_INPUT_1;
		} else if(temp_channel == CHANNEL_INPUT_MP_LOW) {
			rtsp_channel = CHANNEL_INPUT_1_LOW;
		}


		capture_time = get_run_time();
		start_time = app_get_start_time();
		temp_time = rtp_time = getCurrentTime();

		if(rtp_time < start_time) {
			PRINTF("warnning,the rtp_time=%lld,the start time = %lld\n", rtp_time, start_time);
			app_set_start_time();
			start_time = app_get_start_time();
			rtp_time = 0;
		} else {
			rtp_time = rtp_time - start_time;
		}

		if(last_time[rtsp_channel] > rtp_time) {
			rtp_time = last_time[rtsp_channel] + 1;
			//last_time = last_time;
		}

		memset(&info, 0, sizeof(info));
		info.stream_channel = rtsp_channel;
		info.width = pFullBuf->frameWidth;
		info.height = pFullBuf->frameHeight;
		info.timestamp = capture_time;
		info.rtp_time = rtp_time & 0xffffffff;
		last_time[rtsp_channel] = rtp_time;
		info.IsIframe = is_keyframe;
#if 0
		static int g_test = 0;

		if(info.stream_channel == CHANNEL_INPUT_1) {
			g_test ++;

			if(g_test % 30 == 0)
				;//	PRINTF("ch=%d,timestamp=%d,rtp_time=%d==%lld=%lld==%d\n",info.stream_channel,info.timestamp,info.rtp_time,start_time,temp_time,get_run_time());
		}

#endif

		if(app_get_have_stream() == 1) {
			app_set_media_info(info.width, info.height, -1 , info.stream_channel);
		}

		if(app_get_have_stream() == 1) {
			app_stream_video_output(vdata, vlen , &info);
		}

		//mp 复制一份
		if(temp_channel == CHANNEL_INPUT_MP || temp_channel == CHANNEL_INPUT_MP_LOW) {
			//PRINTF("\n");
			if(temp_channel == CHANNEL_INPUT_MP) {
				rtsp_channel = CHANNEL_INPUT_2;
			} else {
				rtsp_channel = CHANNEL_INPUT_2_LOW;
			}

			info.stream_channel = rtsp_channel;
			last_time[rtsp_channel] = rtp_time;

			if(app_get_have_stream() == 1) {
				app_set_media_info(info.width, info.height, -1 , info.stream_channel);
			}

			if(app_get_have_stream() == 1) {
				app_stream_video_output(vdata, vlen , &info);
			}
		}

#endif
		status = OSA_quePut(empty_que, (Int32)pFullBuf, OSA_TIMEOUT_NONE);
		OSA_assert(status == 0);
	}

	PRINTF("----video host------end-------\n");
	PRINTF("----video host------end-------\n");
	PRINTF("----video host------end-------\n");
	return NULL;
}


Void *g_bit_handle = NULL;

Int32 reach_host_bits_process(ENC2000_LINK_STRUCT *pstruct)
{
	IpcBitsInLinkHLOS_CreateParams	*pIpcBitsInHostPrm;

	pIpcBitsInHostPrm = &(pstruct->ipcbit_inhost_Link.create_params);
	pIpcBitsInHostPrm->baseCreateParams.inQueParams.prevLinkId		= pstruct->ipcbit_outvideo_Link.link_id;
	pIpcBitsInHostPrm->baseCreateParams.inQueParams.prevLinkQueId	= 0;
	pIpcBitsInHostPrm->baseCreateParams.outQueParams[0].nextLink	= SYSTEM_LINK_ID_INVALID;
	pIpcBitsInHostPrm->baseCreateParams.noNotifyMode = TRUE;
	pIpcBitsInHostPrm->cbFxn = NULL;
	pIpcBitsInHostPrm->cbCtx = NULL;
	pIpcBitsInHostPrm->baseCreateParams.notifyNextLink = FALSE;
	pIpcBitsInHostPrm->baseCreateParams.notifyPrevLink = FALSE;


	PRINTF("start InHostStreamProcess\n\n");
	ipcbit_create_bitsprocess_inst(&g_bit_handle,
	                               pstruct->ipcbit_inhost_Link.link_id,
	                               SYSTEM_LINK_ID_INVALID,
	                               (bits_callback_fxn)InHostStreamProcess,
	                               NULL,
	                               NULL,
	                               NULL);

	return 0;
}


#if 0
Int32 reach_venc_process(ENC2000_LINK_STRUCT *pstruct, int numInst, int mp_status)
{
	Int32 encInst = 0;
	Int32 queId = 0;
	int profile = 100;
	//	VideoParam video_param;

	EncLink_CreateParams			*pEncPrm;
	IpcLink_CreateParams			*pIpcInVideoPrm;
	IpcBitsOutLinkRTOS_CreateParams *pIpcBitsOutVideoPrm;

	pIpcInVideoPrm = &(pstruct->ipc_invideo_Link.create_params);
	pIpcInVideoPrm->inQueParams.prevLinkId		= pstruct->ipc_outvpss_Link.link_id;
	pIpcInVideoPrm->inQueParams.prevLinkQueId	= 0;
	pIpcInVideoPrm->numOutQue					= 1;
	pIpcInVideoPrm->outQueParams[0].nextLink	= pstruct->encLink.link_id;
	pIpcInVideoPrm->notifyNextLink				= TRUE;
	pIpcInVideoPrm->notifyPrevLink				= TRUE;
	pIpcInVideoPrm->noNotifyMode				= FALSE;

	pEncPrm = &(pstruct->encLink.create_params);
	encInst = numInst;

	profile = app_video_get_profile();

	for(queId = 0; queId < encInst; queId++) {
		//get_video_param(gp_realtime_params, queId, &video_param);
		//pEncPrm->chCreateParams[queId].format 	= IVIDEO_H264BP;//IVIDEO_H264HP;
		//pEncPrm->chCreateParams[queId].profile	= 66;//IH264_HIGH_PROFILE;
		pEncPrm->chCreateParams[queId].format	= IVIDEO_H264HP;//IVIDEO_H264HP;

		if(1 ==  get_encoding_mode()) {
			pEncPrm->chCreateParams[queId].profile	= IH264_HIGH_PROFILE;//IH264_HIGH_PROFILE
		} else if(0 == get_encoding_mode())	{
			pEncPrm->chCreateParams[queId].profile	= IH264_BASELINE_PROFILE;//IH264_BASELINE_PROFILE
		}

		pEncPrm->chCreateParams[queId].dataLayout = IVIDEO_FIELD_SEPARATED;
		pEncPrm->chCreateParams[queId].fieldMergeEncodeEnable  = FALSE;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.intraFrameInterval = 5;
		pEncPrm->chCreateParams[queId].encodingPreset = XDM_USER_DEFINED;
		pEncPrm->chCreateParams[queId].enableAnalyticinfo = 0;

		//if(video_param.sCbr == 1 && 0)
		//{
		//pEncPrm->chCreateParams[queId].rateControlPreset = IVIDEO_LOW_DELAY;
		//}
		//else
		{
			pEncPrm->chCreateParams[queId].rateControlPreset = IVIDEO_LOW_DELAY;
		}

		if(mp_status == IS_IND_STATUS) {
			pEncPrm->numBufPerCh[0] = 6;
			pEncPrm->numBufPerCh[1] = 6;
			pEncPrm->numBufPerCh[2] = 6;
			pEncPrm->numBufPerCh[3] = 6;
		} else if(mp_status == IS_MP_STATUS) {
			pEncPrm->numBufPerCh[0] = 6;
			pEncPrm->numBufPerCh[1] = 6;
		}


		pEncPrm->chCreateParams[queId].defaultDynamicParams.inputFrameRate = 60;//video_param.nFrameRate;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.targetBitRate = 2000 * 1000; //video_param.sBitrate*1000;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.interFrameInterval = 1;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.qpInit = 30;
		/*qpMax与qpMin取值范围(0-51)其值越小视频质量
		越高但相应码率也会有所提高故其值也不可太小*/
		pEncPrm->chCreateParams[queId].defaultDynamicParams.qpMax = 48;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.qpMin = 15;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.rcAlg = 1;
		pEncPrm->chCreateParams[queId].defaultDynamicParams.mvAccuracy = IVIDENC2_MOTIONVECTOR_QUARTERPEL;
	}

	pEncPrm->inQueParams.prevLinkId	= pstruct->ipc_invideo_Link.link_id;
	pEncPrm->inQueParams.prevLinkQueId = 0;
	pEncPrm->outQueParams.nextLink = pstruct->ipcbit_outvideo_Link.link_id;


	pIpcBitsOutVideoPrm = &(pstruct->ipcbit_outvideo_Link.create_params);
	pIpcBitsOutVideoPrm->baseCreateParams.inQueParams.prevLinkId	= pstruct->encLink.link_id;
	pIpcBitsOutVideoPrm->baseCreateParams.inQueParams.prevLinkQueId	= 0;
	pIpcBitsOutVideoPrm->baseCreateParams.numOutQue					= 1;
	pIpcBitsOutVideoPrm->baseCreateParams.outQueParams[0].nextLink	= pstruct->ipcbit_inhost_Link.link_id;
	pIpcBitsOutVideoPrm->baseCreateParams.noNotifyMode				= TRUE;
	pIpcBitsOutVideoPrm->baseCreateParams.notifyNextLink			= FALSE;
	pIpcBitsOutVideoPrm->baseCreateParams.notifyPrevLink			= TRUE;

	return 0;
}
#endif

#if 0
EncLink_ChCreateParams ENCLINK_DEFAULTCHCREATEPARAMS_H264 = {
	.format = IVIDEO_H264HP,
	.profile = IH264_HIGH_PROFILE,
	.dataLayout = IVIDEO_FIELD_SEPARATED,
	.fieldMergeEncodeEnable = FALSE,
	.encodingPreset = ENC_LINK_DEFAULT_ALGPARAMS_ENCODINGPRESET,
	.enableAnalyticinfo = ENC_LINK_DEFAULT_ALGPARAMS_ANALYTICINFO,
	.enableWaterMarking = ENC_LINK_DEFAULT_ALGPARAMS_ENABLEWATERMARKING,
	.maxBitRate = ENC_LINK_DEFAULT_ALGPARAMS_MAXBITRATE,
	.rateControlPreset = ENC_LINK_DEFAULT_ALGPARAMS_RATECONTROLPRESET,
	.defaultDynamicParams =
	{
		.intraFrameInterval = ENC_LINK_DEFAULT_ALGPARAMS_INTRAFRAMEINTERVAL,
		.targetBitRate      = ENC_LINK_DEFAULT_ALGPARAMS_TARGETBITRATE,
		.interFrameInterval = ENC_LINK_DEFAULT_ALGPARAMS_MAXINTERFRAMEINTERVAL,
		.mvAccuracy         = ENC_LINK_DEFAULT_ALGPARAMS_MVACCURACY,
		.inputFrameRate     = ENC_LINK_DEFAULT_ALGPARAMS_INPUTFRAMERATE,
		.qpMin              = ENC_LINK_DEFAULT_ALGPARAMS_QPMIN,
		.qpMax              = ENC_LINK_DEFAULT_ALGPARAMS_QPMAX,
		.qpInit             = ENC_LINK_DEFAULT_ALGPARAMS_QPINIT,
		.vbrDuration        = ENC_LINK_DEFAULT_ALGPARAMS_VBRDURATION,
		.vbrSensitivity     = ENC_LINK_DEFAULT_ALGPARAMS_VBRSENSITIVITY
	}
};

#endif
#if 1
Int32 reach_venc_process(ENC2000_LINK_STRUCT *pstruct, int numInst, int mp_status)
{
	Int32 encInst = 0;
	Int32 queId = 0;
	int profile = 100;
	//	VideoParam video_param;

	EncLink_CreateParams			*pEncPrm;
	IpcLink_CreateParams			*pIpcInVideoPrm;
	IpcBitsOutLinkRTOS_CreateParams *pIpcBitsOutVideoPrm;

	pIpcInVideoPrm = &(pstruct->ipc_invideo_Link.create_params);
	pIpcInVideoPrm->inQueParams.prevLinkId		= pstruct->ipc_outvpss_Link.link_id;
	pIpcInVideoPrm->inQueParams.prevLinkQueId	= 0;
	pIpcInVideoPrm->numOutQue					= 1;
	pIpcInVideoPrm->outQueParams[0].nextLink	= pstruct->encLink.link_id;
	pIpcInVideoPrm->notifyNextLink				= TRUE;
	pIpcInVideoPrm->notifyPrevLink				= TRUE;
	pIpcInVideoPrm->noNotifyMode				= FALSE;

	pEncPrm = &(pstruct->encLink.create_params);
	encInst = numInst;

	profile = app_video_get_profile();

	for(queId = 0; queId < encInst; queId++) {
		//	pEncPrm->chCreateParams[queId] = ENCLINK_DEFAULTCHCREATEPARAMS_H264;
		//create params
		{
			pEncPrm->chCreateParams[queId].encLevel = IH264_LEVEL_51;
			pEncPrm->chCreateParams[queId].format	= IVIDEO_H264HP;//IVIDEO_H264HP;

			if(1 ==  get_encoding_mode()) {
				pEncPrm->chCreateParams[queId].profile	= IH264_HIGH_PROFILE;//IH264_HIGH_PROFILE
			} else if(0 == get_encoding_mode())	{
				pEncPrm->chCreateParams[queId].profile	= IH264_BASELINE_PROFILE;//IH264_BASELINE_PROFILE
			}

			pEncPrm->chCreateParams[queId].dataLayout = IVIDEO_FIELD_SEPARATED;
			pEncPrm->chCreateParams[queId].fieldMergeEncodeEnable  = FALSE;
			pEncPrm->chCreateParams[queId].enableAnalyticinfo = FALSE;
			pEncPrm->chCreateParams[queId].enableWaterMarking = FALSE;
			pEncPrm->chCreateParams[queId].maxBitRate = 2000 * 1000;

			//设置编码场景，比如高质量还是高速度，默认中速度中质量
			pEncPrm->chCreateParams[queId].encodingPreset = XDM_USER_DEFINED;
			//pEncPrm->chCreateParams[queId].encodingPreset = XDM_DEFAULT;// XDM_DEFAULT;//XDM_PRESET_DEFAULT;// ;
			//设置码率控制配置，默认使用low delay(CBR)
			//pEncPrm->chCreateParams[queId].rateControlPreset =IVIDEO_USER_DEFINED;// IVIDEO_LOW_DELAY;//IVIDEO_USER_DEFINED;// IVIDEO_LOW_DELAY;
			pEncPrm->chCreateParams[queId].rateControlPreset = IVIDEO_USER_DEFINED;

			pEncPrm->chCreateParams[queId].enableHighSpeed = 0;
			pEncPrm->chCreateParams[queId].enableSVCExtensionFlag = 0;
			pEncPrm->chCreateParams[queId].numTemporalLayer = 1;

			//只有encodingPreset == XDM_USER_DEFINED,下面设置才生效
			if(pEncPrm->chCreateParams[queId].encodingPreset == XDM_USER_DEFINED) {
				pEncPrm->chCreateParams[queId].enableHighSpeed = 0;
			}

			pEncPrm->chCreateParams[queId].enableSVCExtensionFlag = 0;
			pEncPrm->chCreateParams[queId].numTemporalLayer = 0;
		}

		//set default dynamicparams
		{
			pEncPrm->chCreateParams[queId].defaultDynamicParams.intraFrameInterval = 120;
			pEncPrm->chCreateParams[queId].defaultDynamicParams.targetBitRate = 2000 * 1000; //video_param.sBitrate*1000;
			pEncPrm->chCreateParams[queId].defaultDynamicParams.interFrameInterval = 1;

			pEncPrm->chCreateParams[queId].defaultDynamicParams.mvAccuracy = IVIDENC2_MOTIONVECTOR_QUARTERPEL ; //IVIDENC2_MOTIONVECTOR_QUARTERPEL;
			pEncPrm->chCreateParams[queId].defaultDynamicParams.inputFrameRate = 60;//video_param.nFrameRate;

			//只有IVIDEO_USER_DEFINED才生效
			//pEncPrm->chCreateParams[queId].defaultDynamicParams.rcAlg= IH264_RATECONTROL_PRC;
			pEncPrm->chCreateParams[queId].defaultDynamicParams.rcAlg = IH264_RATECONTROL_PRC;

			/*qpMax与qpMin取值范围(0-51)其值越小视频质量越高但相应码率也会有所提高故其值也不可太小*/
			pEncPrm->chCreateParams[queId].defaultDynamicParams.qpMin = 15;
			pEncPrm->chCreateParams[queId].defaultDynamicParams.qpMax = 48;
			pEncPrm->chCreateParams[queId].defaultDynamicParams.qpInit = -1;

			pEncPrm->chCreateParams[queId].defaultDynamicParams.vbrDuration = 8;
			pEncPrm->chCreateParams[queId].defaultDynamicParams.vbrSensitivity = 0;

			//只有IVIDEO_USER_DEFINED才生效
			if(pEncPrm->chCreateParams[queId].rateControlPreset == IVIDEO_USER_DEFINED) {
				pEncPrm->chCreateParams[queId].defaultDynamicParams.rcAlg = IH264_RATECONTROL_PRC;
				//pEncPrm->chCreateParams[queId].defaultDynamicParams.rcAlg=IH264_RATECONTROL_PRC;
			}

		}

		if(mp_status == IS_IND_STATUS) {
			pEncPrm->numBufPerCh[0] = 6;
			pEncPrm->numBufPerCh[1] = 6;
			pEncPrm->numBufPerCh[2] = 6;
			pEncPrm->numBufPerCh[3] = 6;
		} else if(mp_status == IS_MP_STATUS) {
			pEncPrm->numBufPerCh[0] = 6;
			pEncPrm->numBufPerCh[1] = 6;
		}


	}

	pEncPrm->inQueParams.prevLinkId	= pstruct->ipc_invideo_Link.link_id;
	pEncPrm->inQueParams.prevLinkQueId = 0;
	pEncPrm->outQueParams.nextLink = pstruct->ipcbit_outvideo_Link.link_id;

	pIpcBitsOutVideoPrm = &(pstruct->ipcbit_outvideo_Link.create_params);
	pIpcBitsOutVideoPrm->baseCreateParams.inQueParams.prevLinkId	= pstruct->encLink.link_id;
	pIpcBitsOutVideoPrm->baseCreateParams.inQueParams.prevLinkQueId	= 0;
	pIpcBitsOutVideoPrm->baseCreateParams.numOutQue					= 1;
	pIpcBitsOutVideoPrm->baseCreateParams.outQueParams[0].nextLink	= pstruct->ipcbit_inhost_Link.link_id;
	pIpcBitsOutVideoPrm->baseCreateParams.noNotifyMode				= TRUE;
	pIpcBitsOutVideoPrm->baseCreateParams.notifyNextLink			= FALSE;
	pIpcBitsOutVideoPrm->baseCreateParams.notifyPrevLink			= TRUE;

	return 0;
}
#endif

//强制请求I帧
int app_video_request_iframe(int channel)
{
	int enc_chid = channel_get_enc_chid(channel);

	if(enc_chid < 0) {
		ERR_PRN("enc_chid =%d is invaid\n", enc_chid);
		return -1;
	}

	enc_force_iframe(gEnc2000.encLink.link_id, enc_chid);
	return 0;
}






