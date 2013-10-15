/*****************************************************************************************
*     描述：1. 用于ENCODEMANGE控制video相关参数
*           2. WEB控制VIDEO ENCODE相关参数
*           3.其他需要控制video encode地方，比如rtsp
*			4.保存和读取flash里面的video encode的设置
*
*
*
*
*
*
*
******************************************************************************************/
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
#include "select.h"

#ifdef HAVE_RTSP_MODULE
#include "app_stream_output.h"
//#include "rtsp_output_server.h"
//#include "multicast_output_server.h"
#endif
#include "mid_mutex.h"
#include "mid_timer.h"
#include "log_common.h"
#include "app_protocol.h"
#include "MMP_control.h"

#include "app_video_control.h"
#include "app_mp_control.h"

#include "xml/xml_base.h"
#include "app_signal.h"

VideoCommonHandle g_video_control_handle = NULL;


#define CHECK(id) \
	if(id <0 || id > MAX_CHANNEL)    \
	{ 							\
		ERR_PRN("id = %d is invaild\n",id);    \
		return -1;        \
	}              \
	 


static void detele_video_control_handle(VideoCommonHandle *handle);
static void create_video_control_handle();
static int read_video_global_param(xmlDocPtr pdoc, xmlNodePtr pnode, int *hp);
static int add_video_global_param(xmlNodePtr pnode, int  hp);
static int read_video_encode_param(xmlDocPtr pdoc, xmlNodePtr pnode, VideoEnodeInfo *pvideo);
static int add_video_encode_param(xmlNodePtr pnode, VideoEnodeInfo *pvideo);
static int read_video_scale_param(xmlDocPtr pdoc, xmlNodePtr pnode, VideoScaleInfo *pvideo);
static int add_video_scale_param(xmlNodePtr pnode, VideoScaleInfo *pvideo);
static int read_video_control_file();
static int write_video_control_file();

static void app_video_control();



//配套的encode mange 协议的结构体的获取  ok :ret = 0 . error: ret = -1;
int MMP_video_info_get(int channel, MMPVideoParam *vparam)
{
	CHECK(channel);
	VideoEnodeInfo *info = NULL;
	int width = 0, hight = 0;
	int input = 0, high = 0;
	channel_get_input_info(channel, &input, &high);
	capture_get_input_hw(input, &width, &hight);

	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);
	vparam->nDataCodec = info->ndatacodec;
	vparam->nFrameRate = info->framerate;
	vparam->nWidth = width;
	vparam->nHight = hight;
	vparam->nColors = info->ncolors;
	vparam->nQuality = info->quality;
	vparam->sCbr = info->scbr;
	vparam->sBitrate = info->bitrate;
	mid_mutex_unlock(g_video_control_handle->mutex[channel]);
	return 0;
}

// if change ,return 1
int MMP_video_info_set(int channel, MMPVideoParam *vparam, int *change)
{
	int bitrate_channge = 0 ;
	CHECK(channel);
	VideoEnodeInfo *info = NULL;
	int input = 0, high = 0;
	int enc_chid = channel_get_enc_chid(channel);

	if(enc_chid < 0) {
		ERR_PRN("enc_chid =%d is invaid\n", enc_chid);
		return -1;
	}

	channel_get_input_info(channel, &input, &high);

	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);


	if(vparam->nFrameRate > 0 && vparam->nFrameRate <= 60) {
		if(vparam->nFrameRate != info->framerate) {
			*change = 1;
			PRINTF("new frame = %d,old frame = %d \n", vparam->nFrameRate, info->framerate);

			info->framerate = vparam->nFrameRate;
		}
	}

	if((vparam->sCbr == VIDEO_SBITRATE && vparam->sBitrate != info->bitrate)) {
		*change = 1;
		bitrate_channge = 1;
		info->scbr = vparam->sCbr;
		info->quality = vparam->nQuality ;
		info->bitrate = vparam->sBitrate;
		PRINTF("new cbr=%d,quality=%d,bitrate=%d,old cbr=%d,quality=%d,bitrate=%d\n", vparam->sCbr,
		       vparam->nQuality, vparam->sBitrate, info->scbr, info->quality, info->bitrate);
	}


	if(*change == 1) {
		int framerate = check_video_fps(input, (info->framerate));

		if(info->scbr == 0) {
			enc_set_fps(gEnc2000.encLink.link_id, enc_chid,
			            (framerate), (info->quality) * 100 * 1100);
		} else {
			enc_set_fps(gEnc2000.encLink.link_id, enc_chid,
			            (framerate) , (info->bitrate) * 1100);
		}
	}

	mid_mutex_unlock(g_video_control_handle->mutex[channel]);

	if(*change == 1) {
		//may be need mid_timer
		write_video_control_file();
	}

#ifdef HAVE_RTSP_MODULE

	if(bitrate_channge == 1) {
		app_mult_set_all_tc(channel);
		stream_rtsp_set_all_tc(channel);
	}

#endif
	return 0;
}



int web_video_info_get(int channel, WebVideoEncodeInfo *vparam)
{
	CHECK(channel);
	VideoEnodeInfo *info = NULL;
	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);
	vparam->IFrameinterval = info->gop;
	vparam->nFrameRate = info->framerate;
	vparam->sBitrate = info->bitrate;
	vparam->nQuality = info->quality;
	vparam->sCbr = info->scbr;
	vparam->preset = info->preset;
	mid_mutex_unlock(g_video_control_handle->mutex[channel]);
	return 0;
}

static int enc_preset_frame_qp(int enc_chid, int high, SceneConfiguration preset)
{
	//return 0;
	PRINTF("enc_chid:[%d] high:[%d] preset:[%d]\n", enc_chid, high, preset);
	EncLink_ChQPParams params;

	if(high) {
		if(MOTION == preset) {
			params.chId = enc_chid;
			params.qpInit = 30;
			params.qpMax = 48;
			params.qpMin = 15;
		} else {
			params.chId = enc_chid;
			params.qpInit = 20;
			params.qpMax = 30;
			params.qpMin = 15;
		}
	} else {
		params.chId = enc_chid;
		params.qpInit = 30;
		params.qpMax = 48;
		params.qpMin = 15;
	}

	PRINTF("qpInit:[%d] qpMax:[%d] qpMin:[%d]\n", params.qpInit, params.qpMax, params.qpMin);
	enc_set_pframe_qp(gEnc2000.encLink.link_id, &params);
	enc_set_iframe_qp(gEnc2000.encLink.link_id, &params);
}

int web_video_info_set(int channel, WebVideoEncodeInfo *vparam, int *change)
{
	int bitrate_channge = 0 ;
	int other_chang = 0;
	CHECK(channel);
	VideoEnodeInfo *info = NULL;
	int input = 0, high = 0;
	int enc_chid = channel_get_enc_chid(channel);

	if(enc_chid < 0) {
		ERR_PRN("enc_chid =%d is invaid\n", enc_chid);
		return -1;
	}

	channel_get_input_info(channel, &input, &high);

	PRINTF("g_video_control_handle[%d]\n", channel);
	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);

	if(vparam->nFrameRate > 0 && vparam->nFrameRate <= 60) {
		if(vparam->nFrameRate != info->framerate) {
			*change = 1;
			info->framerate = vparam->nFrameRate;
			PRINTF("new frame = %d,old frame = %d \n", vparam->nFrameRate, info->framerate);
		}
	}

	//不支持定质量
	if((vparam->sCbr == VIDEO_SBITRATE && vparam->sBitrate != info->bitrate)) {
		*change = 1;
		bitrate_channge = 1;
		info->scbr = vparam->sCbr;
		info->quality = vparam->nQuality ;
		info->bitrate = vparam->sBitrate;
		PRINTF("new cbr=%d,quality=%d,bitrate=%d,old cbr=%d,quality=%d,bitrate=%d\n", vparam->sCbr,
		       vparam->nQuality, vparam->sBitrate, info->scbr, info->quality, info->bitrate);
	}

	if(vparam->IFrameinterval >= 0 && vparam->IFrameinterval <= 1000) {
		if(vparam->IFrameinterval != info->gop) {
			//maybe just encode mange need this
			//*change = 1;
			other_chang = 1;
			info->gop = vparam->IFrameinterval;
			enc_set_interval(gEnc2000.encLink.link_id, enc_chid, info->gop);

		}
	}


	if(*change == 1) {
		PRINTF("scbr=%d,enc_chid=%d\n", info->scbr, enc_chid);
		int framerate = check_video_fps(input, (info->framerate));

		if(info->scbr == 0) {//定质量，不支持
			enc_set_fps(gEnc2000.encLink.link_id, enc_chid,
			            (framerate), (info->quality) * 100 * 1100);
		} else {
			PRINTF("--------bitrate=%d\n", info->bitrate);
			enc_set_fps(gEnc2000.encLink.link_id, enc_chid,
			            (framerate) , (info	->bitrate) * 1100);
		}
	}

	if(vparam->preset >= 0 && vparam->preset != info->preset) {
		info->preset = vparam->preset;
		enc_preset_frame_qp(enc_chid, high, vparam->preset);
		other_chang = 1;
	}

	mid_mutex_unlock(g_video_control_handle->mutex[channel]);

	if(*change == 1 || other_chang  == 1) {
		//may be need mid_timer
		write_video_control_file();
	}


#ifdef HAVE_RTSP_MODULE

	if(bitrate_channge == 1) {
		app_mult_set_all_tc(channel);
		stream_rtsp_set_all_tc();
	}

#endif
	return 0;
}

int app_video_get_rate(int channel, int *rate, int *scbr)
{
	//int rate = 0;
	CHECK(channel);
	VideoEnodeInfo *info = NULL;
	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);

	*rate = info->bitrate;
	*scbr = info->scbr;
	mid_mutex_unlock(g_video_control_handle->mutex[channel]);
	return 0;
}

int update_enc_preset_frame_qp(int input)
{
	PRINTF("input:[%d]\n", input);
	int channel = input_get_high_channel(input);
	int enc_chid = channel_get_enc_chid(channel);
	int high = 0;
	channel_get_input_info(channel, &input, &high);

	if(enc_chid < 0) {
		ERR_PRN("enc_chid =%d is invaid\n", enc_chid);
		return -1;
	}

	VideoEnodeInfo  *info = NULL;

	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);
	enc_preset_frame_qp(enc_chid, high, info->preset);
	mid_mutex_unlock(g_video_control_handle->mutex[channel]);
	return 0;
}

int update_video_fps(int input)
{
	int channel = input_get_high_channel(input);
	//int rate = 0 ;
	//int scbr = 1;
	int enc_chid = channel_get_enc_chid(channel);

	if(enc_chid < 0) {
		ERR_PRN("enc_chid =%d is invaid\n", enc_chid);
		return -1;
	}

	VideoEnodeInfo  *info = NULL;

	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);
	int framerate = check_video_fps(input, (info->framerate));
	PRINTF("framerate=%d,enc_chid=%d\n", info->framerate, enc_chid);
	enc_set_fps(gEnc2000.encLink.link_id, enc_chid,
	            (framerate) , (info->bitrate) * 1100);
	mid_mutex_unlock(g_video_control_handle->mutex[channel]);
	return 0;
}

int app_video_get_profile()
{
	PRINTF("the video profile =%d\n", g_video_control_handle->profile);
	return g_video_control_handle->profile;
}

int app_video_set_profile(int profile)
{
	if(profile != 100 && profile != 66) {
		PRINTF("profile =%d is invailed\n", profile);
		return -1;
	}

	if(profile != g_video_control_handle->profile) {
		PRINTF("profile change,old =%d,new=%d\n", g_video_control_handle->profile, profile);
		g_video_control_handle->profile = profile;
		//may be need mid_timer
		write_video_control_file();
		return 0;
	}

	return 0;
}



//初始化之后，才能设置
void app_video_control_set(int mp_status)
{
	if(g_video_control_handle == NULL) {
		ERR_PRN("g_video_control_handle is not init\n");
		return ;
	}

	int channel = 0;
	int enc_chid = -1;
	VideoEnodeInfo *info = NULL;
	int max_channel = 0;
	int input = 0, high = 0;

	//	int chid_diff = 0;
	if(mp_status == IS_IND_STATUS) {
		max_channel = NO_MP_MAX_CHANNEL;
		channel = CHANNEL_INPUT_1;
	}

	for(; channel < max_channel; channel++) {
		channel_get_input_info(channel, &input, &high);
		PRINTF("channel = %d \n", channel);
		mid_mutex_lock(g_video_control_handle->mutex[channel]);
		info = &(g_video_control_handle->info[channel]);
		enc_chid = channel_get_enc_chid(channel);

		if(enc_chid < 0) {
			ERR_PRN("enc_chid = %d is invaid\n", enc_chid);
			mid_mutex_unlock(g_video_control_handle->mutex[channel]);
			return ;
		}

		PRINTF("info->scbr=%d,fps=%d,bitrate=%d\n", info->scbr, info->framerate, info->bitrate);

		int framerate = check_video_fps(input, (info->framerate));

		if(info->scbr == 0) {
			PRINTF("WARNING ... \n");
			enc_set_fps(gEnc2000.encLink.link_id, enc_chid,
			            (framerate), (info->quality) * 100 * 1100);
		} else {
			enc_set_fps(gEnc2000.encLink.link_id, enc_chid,
			            (framerate) , (info->bitrate) * 1100);
		}

		//set gop
		enc_set_interval(gEnc2000.encLink.link_id, enc_chid, (info->gop));
		mid_mutex_unlock(g_video_control_handle->mutex[channel]);
	}

	if(mp_status == IS_IND_STATUS) {
		app_video_control();
	}

	return ;
}

static void detele_video_control_handle(VideoCommonHandle *handle)
{
	int i = 0;

	if(*handle != NULL) {
		for(i = 0; i < MAX_CHANNEL; i++) {
			mid_mutex_destroy(&((*handle)->mutex[i]));
		}

		free(*handle);
	}

	*handle = NULL;
	return ;
}

static void create_video_control_handle()
{
	if(g_video_control_handle != NULL) {
		ERR_PRN("g_video_control_handle is already init\n");
		return ;
	}

	int i = 0;
	//	int mp_status = get_mp_status();
	int input = CHANNEL_INPUT_1;
	int high_stream = HIGH_STREAM;
	g_video_control_handle = (VideoCommonHandle)malloc(sizeof(VideoCommonInfo));

	if(g_video_control_handle == NULL) {
		ERR_PRN("malloc failed\n");
		return ;
	}

	memset(g_video_control_handle, 0, sizeof(VideoCommonInfo));

	g_video_control_handle->profile = 100;  //hp

	//init mutex
	for(i = 0; i < MAX_CHANNEL; i++) {
		g_video_control_handle->mutex[i] = mid_mutex_create();

		if(g_video_control_handle->mutex[i] == NULL) {
			ERR_PRN("create %d mutex  failed\n", i);
			goto CREATE_ERR;
		}

		g_video_control_handle->smutex[i] = mid_mutex_create();

		if(g_video_control_handle->smutex[i] == NULL) {
			ERR_PRN("create %d mutex  failed\n", i);
			goto CREATE_ERR;
		}

		g_video_control_handle->info[i].ndatacodec = 0x34363248;	//"H264"
		g_video_control_handle->info[i].width = 1920;			//video width
		g_video_control_handle->info[i].hight = 1080;			//video height
		g_video_control_handle->info[i].quality = 45;				//quality (5---90)
		g_video_control_handle->info[i].scbr = 1;					// 0---quality  1---bitrate
		g_video_control_handle->info[i].framerate = 60;			//current framerate
		g_video_control_handle->info[i].bitrate = 4096;				//bitrate (128k---4096k)
		g_video_control_handle->info[i].ncolors = 24;			//24
		g_video_control_handle->info[i].gop = 120;

		g_video_control_handle->sinfo[i].quality_lock_flag = UNLOCK_SCALE;
		g_video_control_handle->sinfo[i].record_lock_flag = UNLOCK_SCALE;
		g_video_control_handle->sinfo[i].web_lock_flag = UNLOCK_SCALE;
		g_video_control_handle->sinfo[i].low_lock_flag = UNLOCK_SCALE;
		g_video_control_handle->sinfo[i].outwidth = 0;
		g_video_control_handle->sinfo[i].outheight = 0;
		g_video_control_handle->sinfo[i].scalemode = 0;


		channel_get_input_info(i, &input, &high_stream);

		if(high_stream == LOW_STREAM) {
			//			g_video_control_handle->info[i].width = 352;			//video width
			//			g_video_control_handle->info[i].hight = 288;			//video height
			g_video_control_handle->info[i].framerate = 20;			//current framerate
			g_video_control_handle->info[i].bitrate = 512;				//bitrate (128k---4096k)

			g_video_control_handle->sinfo[i].web_lock_flag = LOCK_SCALE;
			g_video_control_handle->sinfo[i].low_lock_flag = LOCK_SCALE;
			g_video_control_handle->sinfo[i].outwidth = 352;
			g_video_control_handle->sinfo[i].outheight = 288;
			g_video_control_handle->sinfo[i].scalemode = 0;
		}

	}

	return ;
CREATE_ERR:
	detele_video_control_handle(&g_video_control_handle);
	return ;
}


//缩放模式0--按比例1---全局拉伸
int set_scale_mode(int link_id, int mode)
{
	int ret = 0;
	SclrLink_SclrMode  sclr ;
	PRINTF("mode=%d\n", mode);
	sclr.SclrMode = (Bool)mode;
	ret = sclr_set_SclrMode(link_id, &sclr);
	return ret;
}

int app_get_scale_status(int channel, int *scale, int *width, int *height)
{
	if(g_video_control_handle == NULL) {
		ERR_PRN("g_video_control_handle is not init\n");
		return -1;
	}

	CHECK(channel);
	VideoScaleInfo *info = NULL;
	int input , high_stream;
	input = high_stream = -1;
	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);
	channel_get_input_info(channel, &input, &high_stream);

	if(high_stream == HIGH_STREAM) {
		if(info->quality_lock_flag == LOCK_SCALE || info->record_lock_flag == LOCK_SCALE || info->web_lock_flag == LOCK_SCALE) {
			*scale = LOCK_SCALE;

			if(info->outwidth < 352 || info->outheight < 288) {
				*scale = UNLOCK_SCALE;
			} else {

				*width = info->outwidth;
				*height = info->outheight;
			}
		}
	} else if(high_stream == LOW_STREAM) {
		if(info->low_lock_flag == LOCK_SCALE) {
			*scale = LOCK_SCALE;

			if(info->outwidth < 352 || info->outheight < 288) {
				*scale = UNLOCK_SCALE;
			} else {

				*width = info->outwidth;
				*height = info->outheight;
			}
		}
	}

	PRINTF("*scale=%d=ch=%d,pvideo->web_lock_flag=%d,=%d=%d\n", *scale, channel, g_video_control_handle->sinfo[channel].web_lock_flag, g_video_control_handle->sinfo[channel].outwidth, g_video_control_handle->sinfo[channel].outheight);

	mid_mutex_unlock(g_video_control_handle->smutex[channel]);
	return 0;
}
//初始化之后，才能设置
static void app_video_control()
{
	if(g_video_control_handle == NULL) {
		ERR_PRN("g_video_control_handle is not init\n");
		return ;
	}

	int channel = 0;
	//	VideoScaleInfo *info = NULL;
	int scale, outwidth, outheight, width, height, input, high_stream;

	input = high_stream = outwidth = outheight = scale = width = height = -1;

	for(channel = 0; channel < NO_MP_MAX_CHANNEL; channel++) {
		input = high_stream = outwidth = outheight = scale = width = height = -1;
		app_get_scale_status(channel, &scale, &outwidth, &outheight);

		if(scale == LOCK_SCALE) {
			channel_get_input_info(channel, &input, &high_stream);
			capture_get_input_hw(input, &width, &height);

			if(height == 540 && width == 1920) {
				height = height * 2;
			}

			PRINTF("the %d channel is lock scale ,scale widht=%d,hight=%d\n", channel, outwidth, outheight);
			set_resolution(input, high_stream, outwidth, outheight, width, height);
		} else {
			PRINTF("the %d channle is not lock scale\n", channel);
		}
	}

	return ;

}

//a) 录制锁定分辨率。输入0/1，锁定到当前编码的宽高
//一旦用户发出锁定分辨率的消息，默认就锁定当前的编码的宽高，不管是否处于WEB或者质量锁定的状态
int MMP_recode_lock_set(int channel, int islock)
{
	CHECK(channel);

	VideoScaleInfo *info = NULL;
	int input , high_stream;
	int width, height;
	int mp_status = get_mp_status();
	int ret = 0;

	int need_lock = UNLOCK_SCALE;
	width = height = input = high_stream = -1;
	channel_get_input_info(channel, &input, &high_stream);

	if(high_stream == LOW_STREAM) {
		ERR_PRN("the %d channel is low stream ,not have this set\n", channel);
		return -1;
	}

	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);

#ifdef HAVE_IP_XML

	if(info->ip_xml_lock_flag > 0) {
		PRINTF("IP XML locking,This will return!\n");
		ret = IP_XML_LOCK_SCALE;
		goto XML_LOCK;
	}

#endif

	if(mp_status == IS_IND_STATUS) {
		capture_get_input_hw(input, &width, &height);

		if(height == 540 && width == 1920) {
			height = height * 2;
		}
	} else {
		app_get_original_resolution(&width, &height);

	}

	if(islock == LOCK_SCALE) {
		info->record_lock_flag = LOCK_SCALE;

		//处于锁定状态，继续锁定,小流逻辑不走入这个
		if(info->quality_lock_flag == LOCK_SCALE || info->record_lock_flag == LOCK_SCALE
		   || info->web_lock_flag == LOCK_SCALE) {
			PRINTF("the %d channel is already lock scale\n", channel);
			//info->record_lock_flag = LOCK_SCALE;
			need_lock = LOCK_SCALE;
		} else { //开始进入锁定状态

			info->outwidth = width;
			info->outheight = height;
			PRINTF("the %d channel is begin to scale to %d x %d\n", channel, width, height);
			need_lock = LOCK_SCALE;
		}
	} else if(islock == UNLOCK_SCALE) {
		info->record_lock_flag = UNLOCK_SCALE;

		//解锁只解锁record lock,不解锁其他操作
		if(info->quality_lock_flag == LOCK_SCALE || info->web_lock_flag == LOCK_SCALE) {
			PRINTF("the %d channel is already lock scale\n", channel);

			need_lock = LOCK_SCALE;
		} else { //不需要任何锁定
			info->outwidth = width;
			info->outheight = height;
			PRINTF("the %d channel is begin to unlock to %d x %d\n", channel, width, height);
			need_lock = UNLOCK_SCALE;
		}
	}

	//set scale
	PRINTF("the %d channel is lock scale ,from %d x %d to scale widht=%d,hight=%d\n", channel, width, height, info->outwidth, info->outheight);

	if(mp_status == IS_IND_STATUS) {
		set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	} else if(IS_MP_STATUS == mp_status) {
		mp_set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	}

XML_LOCK:

	mid_mutex_unlock(g_video_control_handle->smutex[channel]);

	return ret;
}


// b) 高中低质量以及电影模式。 传入需要锁定的宽高，码率，需要互斥
// 进入之后，如果遇到存在record lock，默认不操作，只设置状态值
//暂时录播不发送质量锁定的解锁消息
// 返回值 0 表示设置成功， 不成功返回-1
int MMP_quailty_lock_set(int channel, int qulity, int outwidth, int outheight)
{
	CHECK(channel);

	VideoScaleInfo *info = NULL;
	int input , high_stream;
	int width, height;
	int mp_status = get_mp_status();

	int ret = 0;
	int islock = LOCK_SCALE;
	//int need_lock = UNLOCK_SCALE;
	width = height = input = high_stream = -1;
	channel_get_input_info(channel, &input, &high_stream);

	if(high_stream == LOW_STREAM) {
		ERR_PRN("the %d channel is low stream ,not have this set\n", channel);
		return -1;
	}


	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);

#ifdef HAVE_IP_XML

	if(info->ip_xml_lock_flag > 0) {
		PRINTF("IP XML locking,This will return!\n");
		ret = IP_XML_LOCK_SCALE;
		goto XML_LOCK;
	}

#endif

	if(mp_status == IS_IND_STATUS) {
		capture_get_input_hw(input, &width, &height);

		if(height == 540 && width == 1920) {
			height = height * 2;
		}
	} else if(mp_status == IS_MP_STATUS) {
		app_get_original_resolution(&width, &height);

		if(0 == outwidth ||  0 == outheight) {
			outwidth = width;
			outheight = height;
		}
	}


	if(islock == LOCK_SCALE) {
		//处于锁定状态，继续锁定,小流逻辑不走入这个
		if(info->record_lock_flag == LOCK_SCALE) {
			ret = -1;
			PRINTF("Warnning,the record lock is lock ,will return \n");
		} else {
			info->outwidth = outwidth;
			info->outheight = outheight;
			PRINTF("the %d channel is begin to scale to %d x %d\n", channel, info->outwidth, info->outheight);
			//need_lock = LOCK_SCALE;
			info->quality_lock_flag = LOCK_SCALE;
		}
	} else if(islock == UNLOCK_SCALE) {
		info->quality_lock_flag = UNLOCK_SCALE;

		//解锁只解锁record lock,不解锁其他操作
		if(info->record_lock_flag == LOCK_SCALE || info->web_lock_flag == LOCK_SCALE) {
			PRINTF("the %d channel is already lock scale\n", channel);

			//	need_lock = LOCK_SCALE;
		} else { //不需要任何锁定
			info->outwidth = width;
			info->outheight = height;
			PRINTF("the %d channel is  begin to unscale to %d x %d\n", channel, info->outwidth, info->outheight);
			//	need_lock = UNLOCK_SCALE;
		}
	}

	//set scale
	PRINTF("the %d channel is lock scale ,from %d x %d to scale widht=%d,hight=%d\n", channel, width, height, info->outwidth, info->outheight);

	if(mp_status == IS_IND_STATUS) {
		set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	} else if(IS_MP_STATUS == mp_status) {
		mp_set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	}

XML_LOCK:

	mid_mutex_unlock(g_video_control_handle->smutex[channel]);

	return ret;
}

// c) 低质量码流。出入宽，高，帧率，码率。(保存到FLASH)
int MMP_lowstream_lock_set(int channel, int outwidth, int outheight)
{
	CHECK(channel);

	VideoScaleInfo *info = NULL;
	int input , high_stream;
	int width, height;
	int mp_status =  get_mp_status();

	int ret = 0;
	int islock = LOCK_SCALE;
	//int need_lock = UNLOCK_SCALE;
	width = height = input = high_stream = -1;
	channel_get_input_info(channel, &input, &high_stream);

	if(high_stream == HIGH_STREAM) {
		ERR_PRN("the %d channel is low stream ,not have this set\n", channel);
		return -1;
	}

	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);

	if(mp_status == IS_IND_STATUS) {
		capture_get_input_hw(input, &width, &height);

		if(height == 540  && width == 1920) {
			height = height * 2;
		}
	} else if(mp_status == IS_MP_STATUS) {
		app_get_original_resolution(&width, &height);
	}

	if(islock == LOCK_SCALE) {
		//处于锁定状态，继续锁定,小流逻辑不走入这个
		info->low_lock_flag = LOCK_SCALE ;

		info->outwidth = outwidth;
		info->outheight = outheight;
		PRINTF("the %d channel is begin to scale to %d x %d\n", channel, info->outwidth, info->outheight);

	} else if(islock == UNLOCK_SCALE) {

		info->outwidth = 352;
		info->outheight = 288;
		PRINTF("the %d channel is begin to unlock to %d x %d\n", channel, info->outwidth, info->outheight);
	}

	//set scale
	PRINTF("the %d channel is lock scale ,from %d x %d to scale widht=%d,hight=%d\n", channel, width, height, info->outwidth, info->outheight);

	if(mp_status == IS_IND_STATUS) {
		set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	} else if(IS_MP_STATUS == mp_status) {
		mp_set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	}

	//save to flash
	mid_mutex_unlock(g_video_control_handle->smutex[channel]);

	return ret;
}


int MMP_scale_lock_get(int channel, int *width, int *height)
{
	CHECK(channel);

	VideoScaleInfo *info = NULL;

	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);

	*width = info->outwidth;
	*height = info->outheight;
	mid_mutex_unlock(g_video_control_handle->smutex[channel]);
	return 0;
}



//d) 页面的锁定分辨率，包含全屏缩放以及按比例缩放和宽高 (保存到FLASH)
int web_scale_lock_set(int channel, int outwidth, int outheight, int scalemode)
{
	CHECK(channel);

	VideoScaleInfo *info = NULL;
	VideoScaleInfo oldinfo;
	int input , high_stream;
	int width, height;

	int mp_status  = get_mp_status();
	int ret = 0;
	int islock = LOCK_SCALE;
	int change = 1;

	if(outwidth == 0 || outheight == 0) {
		PRINTF("the %d channel,web will not need lock\n", channel);
		islock = UNLOCK_SCALE;
	}

	width = height = input = high_stream = -1;

	//mp -> input1
	/*
		if(IS_MP_STATUS == mp_status) {
			channel = CHANNEL_INPUT_MP;
		}
	*/
	PRINTF("---channel=%d---\n", channel);
	channel_get_input_info(channel, &input, &high_stream);


	memset(&oldinfo, 0, sizeof(VideoScaleInfo));

	//web 不支持小流设置
	if(high_stream == LOW_STREAM) {
		ERR_PRN("the %d channel is low stream ,not have this set\n", channel);
		return -1;
	}

	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);

#ifdef HAVE_IP_XML

	if(info->ip_xml_lock_flag > 0) {
		PRINTF("IP XML locking,This will return!\n");
		ret = 2;
		goto XML_LOCK;

	}

#endif
	memcpy(&oldinfo, info, sizeof(VideoScaleInfo));

	if(mp_status == IS_IND_STATUS) {
		capture_get_input_hw(input, &width, &height);

		if(height == 540 && width == 1920) {
			height = height * 2;
		}
	} else if(mp_status == IS_MP_STATUS) {
		app_get_original_resolution(&width, &height);

		//不需要锁定设置1920x1080
		if(outwidth == 0 && 0 == outheight) {
			outwidth = width;
			outheight = height;
		}
	}

	if(islock == LOCK_SCALE) {
		//处于锁定状态，继续锁定,小流逻辑不走入这个
		if(info->record_lock_flag == LOCK_SCALE || info->quality_lock_flag == LOCK_SCALE) {
			ret = 2;//-2 服务器锁定消息
			PRINTF("Warnning,the record lock is lock ,will return \n");
		} else {
			info->outwidth = outwidth;
			info->outheight = outheight;
			PRINTF("the %d channel is begin to scale to %d x %d\n", channel, info->outwidth, info->outheight);
			//need_lock = LOCK_SCALE;
			info->web_lock_flag = LOCK_SCALE;
			change = 1;
		}
	} else if(islock == UNLOCK_SCALE) {
		info->web_lock_flag = UNLOCK_SCALE;

		//解锁只解锁record lock,不解锁其他操作
		if(info->record_lock_flag == LOCK_SCALE || info->quality_lock_flag == LOCK_SCALE) {
			PRINTF("the %d channel is already lock scale\n", channel);

			//	need_lock = LOCK_SCALE;
		} else { //不需要任何锁定
			//设为源分辨率
			info->outwidth = width ;//outwidth
			info->outheight = height ;//outheight;
			PRINTF("the %d channel is  begin to unscale to %d x %d\n", channel, info->outwidth, info->outheight);
			//	need_lock = UNLOCK_SCALE;
		}
	}

	//set scale
	PRINTF("the %d channel input =%d,is lock scale ,from %d x %d to scale widht=%d,hight=%d\n", channel, input, width, height, info->outwidth, info->outheight);
	PRINTF("mp_status=%d,input=%d\n", mp_status, input);

	if(IS_IND_STATUS == mp_status) {
		set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	} else if(IS_MP_STATUS == mp_status) {
		mp_set_resolution(input, high_stream, info->outwidth, info->outheight, width, height);
	}

	//save to flash
	if(memcmp(&oldinfo, info, sizeof(VideoScaleInfo)) == 0) {
		change = 0;
	}

	if(scalemode != info->scalemode) {
		info->scalemode = scalemode;
		change = 1;
	}

XML_LOCK:

	mid_mutex_unlock(g_video_control_handle->smutex[channel]);

	if(change == 1) {
		write_video_control_file();
	}

	return ret;
}

int web_scale_lock_get(int channel, int *width, int *height, int *scalemode)
{
	CHECK(channel);

	VideoScaleInfo *info = NULL;

	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);

	*width = info->outwidth;
	*height = info->outheight;
	*scalemode =  info->scalemode;
	mid_mutex_unlock(g_video_control_handle->smutex[channel]);
	return 0;
}
#ifdef HAVE_IP_XML
int g_bak_resolution = 0;

int ip_xml_scale_lock_get(int channel, MMPVideoParam *vparam)
{

	CHECK(channel);
	VideoEnodeInfo *info = NULL;
	int width = 0, height = 0;
	Uint32 input = 0, high = 0;
	int mp_status = get_mp_status();
	channel_get_input_info(channel, &input, &high);

	if(IS_IND_STATUS == mp_status) {
		capture_get_input_hw(input, &width, &height);
	} else if(IS_MP_STATUS == mp_status) {
		app_get_original_resolution(&width, &height);
	}

	mid_mutex_lock(g_video_control_handle->mutex[channel]);
	info = &(g_video_control_handle->info[channel]);
	vparam->nDataCodec = info->ndatacodec;
	vparam->nFrameRate = info->framerate;
	vparam->nWidth = width;
	vparam->nHight = height;
	vparam->nColors = info->ncolors;
	vparam->nQuality = info->quality;
	vparam->sCbr = info->scbr;
	vparam->sBitrate = info->bitrate;
	mid_mutex_unlock(g_video_control_handle->mutex[channel]);
}

int ip_xml_scale_lock_set(int channel, int scalemode)
{

	CHECK(channel);
	VideoScaleInfo *info = NULL;
	VideoScaleInfo oldinfo;
	int high, input;
	int width = 0, height = 0;

	int mp_status  = get_mp_status();
	int ret = 0;
	int islock = LOCK_SCALE;
	int change = 1;
	channel_get_input_info(channel, &input, &high);

	if(LOCK_SCALE == scalemode) {
		PRINTF("the %d channel,web will need lock\n", channel);
	}

	//mp -> input1
	if(IS_MP_STATUS == mp_status) {
		app_get_original_resolution(&width, &height);
	} else if(IS_IND_STATUS == mp_status) {
		// set source res
		capture_get_input_hw(input, &width, &height);
	}

	memset(&oldinfo, 0, sizeof(VideoScaleInfo));

	//web 不支持小流设置
	if(LOW_STREAM == high) {
		ERR_PRN("the %d channel is low stream ,not have this set\n", channel);
		return -1;
	}

	mid_mutex_lock(g_video_control_handle->smutex[channel]);
	info = &(g_video_control_handle->sinfo[channel]);
	memcpy(&oldinfo, info, sizeof(VideoScaleInfo));

	//need lock
	if(LOCK_SCALE == scalemode) {
		//忽略低级别锁定
		if(info->record_lock_flag == LOCK_SCALE
		   || info->quality_lock_flag == LOCK_SCALE
		   || info->web_lock_flag == LOCK_SCALE) {
			PRINTF("Warnning,the record_lock=%d,quality_lock=%d,web_lock=%d ,will return \n",
			       info->record_lock_flag, info->quality_lock_flag,
			       info->web_lock_flag);
		}

		info->outwidth = width;
		info->outheight = height;
		PRINTF("the %d channel is begin to scale to %d x %d\n", channel, info->outwidth, info->outheight);
		info->ip_xml_lock_flag += LOCK_SCALE;//add lock scale

		//set res
		if(IS_IND_STATUS == mp_status) {
			set_resolution(input, high, info->outwidth, info->outheight, width, height);
		} else if(IS_MP_STATUS == mp_status) {
			mp_set_resolution(input, high, info->outwidth, info->outheight, width, height);
		}

	} else if(scalemode == UNLOCK_SCALE) {
		if(0 == info->ip_xml_lock_flag) {
			PRINTF("WARNING: ip_xml_lock_flag =%d\n", info->ip_xml_lock_flag);
		} else if(info->ip_xml_lock_flag > 0) {
			info->ip_xml_lock_flag -= LOCK_SCALE;//sub lock scale
		}

		//解锁record quality lock
		if(info->record_lock_flag == LOCK_SCALE
		   || info->quality_lock_flag == LOCK_SCALE) {
			PRINTF("Warnning,the record_lock=%d,quality_lock=%d will return \n",
			       info->record_lock_flag, info->quality_lock_flag);
			info->record_lock_flag = UNLOCK_SCALE;
			info->quality_lock_flag = UNLOCK_SCALE;
		}

		if(info->web_lock_flag == LOCK_SCALE && g_bak_resolution != LOCK_AUTO) {
			WebScaleInfo in, out;
			int in_width = 0, in_height = 0;
			resolution_to_width(g_bak_resolution , &in_width, &in_height);
			in.outheight = in_height;
			in.outwidth = in_width;
			in.scalemode = g_bak_resolution;
			in.scalemode = info->scalemode;
			app_web_set_scaleinfo(channel, &in, &out);
		}
	}

	mid_mutex_unlock(g_video_control_handle->smutex[channel]);
	PRINTF("mp_status=%d,input=%d\n", mp_status, input);
	PRINTF("the %d channel input =%d,is lock scale ,from %d x %d to scale widht=%d,hight=%d\n", channel, input, width, height, info->outwidth, info->outheight);
	return 0;
}

#endif
static int read_video_global_param(xmlDocPtr pdoc, xmlNodePtr pnode, int *hp)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;
	int				ret = 0;
	int value = 0;

	if(NULL == pdoc || NULL == pnode) {
		fprintf(stderr, "read_video_param error, params invalid!\n");
		return -1;
	}

	node = get_children_node(pnode, BAD_CAST "encode_profile");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == 66 || value == 100) {
				*hp = atoi(temp);
			}

			PRINTF("\nprofile mode: %s\n", temp);
		}
	}

	return 0;
}




static int add_video_global_param(xmlNodePtr pnode, int  hp)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", hp);
	xmlNewTextChild(pnode, NULL, BAD_CAST"encode_profile", BAD_CAST(temp));

	return 0;
}



static int read_video_encode_param(xmlDocPtr pdoc, xmlNodePtr pnode, VideoEnodeInfo *pvideo)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;
	int				ret = 0;
	int value = 0;

	if(NULL == pdoc || NULL == pnode || NULL == pvideo) {
		fprintf(stderr, "read_video_param error, params invalid!\n");
		return -1;
	}

	node = get_children_node(pnode, BAD_CAST "nDataCodec");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			pvideo->ndatacodec = atoi(temp);

			PRINTF("\nencode mode: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nWidth");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			pvideo->width = atoi(temp);
			PRINTF("width: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nHight");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			pvideo->hight = atoi(temp);
			PRINTF("height: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nQuality");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value > 0 && value <= 90) {
				pvideo->quality = value;
			}

			PRINTF("quality: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "sCbr");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == 0 || value == 1) {
				pvideo->scbr = value;
			}

			PRINTF("cbr: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nFrameRate");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value > 0 && value <= 60) {
				pvideo->framerate = value;
			}

			PRINTF("framerate: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "sBitrate");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value >= MIN_BITRATE_VALUE && value <= 20000) {
				pvideo->bitrate = value;
			}

			PRINTF("bitrate: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nColors");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			pvideo->ncolors = atoi(temp);
			PRINTF("colors: %s\n", temp);
		}
	}


	node = get_children_node(pnode, BAD_CAST "gop");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value > 0 && value <=	1000) {
				pvideo->gop = value;
			}

			PRINTF("gop: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "preset");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value >= 0) {
				pvideo->preset = value;
			}

			PRINTF("preset: [%s]\n", temp);
		}
	}

	return 0;
}



static int add_video_encode_param(xmlNodePtr pnode, VideoEnodeInfo *pvideo)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->ndatacodec);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nDataCodec", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->width);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nWidth", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->hight);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nHight", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->quality);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nQuality", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->scbr);
	xmlNewTextChild(pnode, NULL, BAD_CAST"sCbr", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->framerate);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nFrameRate", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->bitrate);
	xmlNewTextChild(pnode, NULL, BAD_CAST"sBitrate", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->ncolors);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nColors", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->gop);
	xmlNewTextChild(pnode, NULL, BAD_CAST"gop", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->preset);
	xmlNewTextChild(pnode, NULL, BAD_CAST"preset", BAD_CAST(temp));
	return 0;
}



static int read_video_scale_param(xmlDocPtr pdoc, xmlNodePtr pnode, VideoScaleInfo *pvideo)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;
	int				ret = 0;
	int value = 0;

	if(NULL == pdoc || NULL == pnode || NULL == pvideo) {
		fprintf(stderr, "read_video_param error, params invalid!\n");
		return -1;
	}

	node = get_children_node(pnode, BAD_CAST "web_lock_flag");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == LOCK_SCALE || value == UNLOCK_SCALE) {
				pvideo->web_lock_flag = atoi(temp);
			}

			PRINTF("\nweb_lock_flag: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "outwidth");


	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value >= 0 && value <= 1920) {
				pvideo->outwidth = atoi(temp);
			}

			PRINTF("outwidth: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "outheight");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value >= 0 && value <= 1080) {
				pvideo->outheight = atoi(temp);
			}

			PRINTF("outheight: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "scalemode");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == 0 || value == 1) {
				pvideo->scalemode = value;
			}

			PRINTF("scalemode: %s\n", temp);
		}
	}

	if(pvideo->web_lock_flag == UNLOCK_SCALE) {
		pvideo->outwidth = 0;
		pvideo->outheight = 0;
	}

	PRINTF("pvideo->web_lock_flag=%d,=%d=%d\n", pvideo->web_lock_flag, pvideo->outwidth, pvideo->outheight);
	return 0;
}


static int add_video_scale_param(xmlNodePtr pnode, VideoScaleInfo *pvideo)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->web_lock_flag);
	xmlNewTextChild(pnode, NULL, BAD_CAST"web_lock_flag", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->outwidth);
	xmlNewTextChild(pnode, NULL, BAD_CAST"outwidth", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->outheight);
	xmlNewTextChild(pnode, NULL, BAD_CAST"outheight", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->scalemode);
	xmlNewTextChild(pnode, NULL, BAD_CAST"scalemode", BAD_CAST(temp));

	return 0;
}



static int read_video_control_file()
{
	char xml_file[256] = VIDEO_CONFIG;
	int index = 0;
	int ret = 0;
	int input = CHANNEL_INPUT_1;
	int high_stream = HIGH_STREAM;
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr video_global_node;
	xmlNodePtr video_encode_node[MAX_CHANNEL];
	xmlNodePtr video_scale_node[MAX_CHANNEL];

	parse_xml_t px;
	int hp = 0;

	//	pthread_mutex_lock(&ptable->lock);

	init_file_dom_tree(&px, xml_file);

	if(NULL == px.pdoc || NULL == px.proot) {
		PRINTF("init_file_dom_tree failed, xml file: %s\n", xml_file);
		ret = -1;
		goto cleanup;
	}

	video_global_node = get_children_node(px.proot, BAD_CAST"video_global");

	if(NULL == video_global_node) {
		fprintf(stderr, "get sys_param node failed, xml file: %s\n", xml_file);
		ret = -1;
		goto cleanup;
	}

	read_video_global_param(px.pdoc, video_global_node, &hp);
	g_video_control_handle->profile = hp;

	for(index = 0; index < MAX_CHANNEL; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "video_encode_%d", index);
		video_encode_node[index] = get_children_node(px.proot, BAD_CAST(temp));

		if(video_encode_node[index]) {
			mid_mutex_lock(g_video_control_handle->mutex[index]);
			read_video_encode_param(px.pdoc, video_encode_node[index], &(g_video_control_handle->info[index]));
			mid_mutex_unlock(g_video_control_handle->mutex[index]);
		}
	}

	for(index = 0; index < MAX_CHANNEL; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "video_scale_%d", index);
		video_scale_node[index] = get_children_node(px.proot, BAD_CAST(temp));

		if(video_scale_node[index]) {
			mid_mutex_lock(g_video_control_handle->smutex[index]);
			PRINTF("scale index = %d\n", index);
			read_video_scale_param(px.pdoc, video_scale_node[index], &(g_video_control_handle->sinfo[index]));
			mid_mutex_unlock(g_video_control_handle->smutex[index]);
		}

		channel_get_input_info(index, &input, &high_stream);

		if(high_stream == LOW_STREAM) {
			g_video_control_handle->sinfo[index].low_lock_flag = LOCK_SCALE;
		}

	}

cleanup:
	release_dom_tree(px.pdoc);


	return ret;
}




static int write_video_control_file()
{
	char xml_file[256] = VIDEO_CONFIG;


	int index = 0;
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlDocPtr doc;
	xmlNodePtr root_node;
	xmlNodePtr video_global_node;
	xmlNodePtr video_encode_node[MAX_CHANNEL];
	xmlNodePtr video_scale_node[MAX_CHANNEL];

	//	pthread_mutex_lock(&ptable->lock);

	doc = xmlNewDoc(BAD_CAST"1.0"); 			//定义文档和节点指针

	if(NULL == doc) {
		PRINTF("xmlNewDoc failed, file: %s\n", xml_file);
		//pthread_mutex_unlock(&ptable->lock);
		return -1;
	}

	root_node = xmlNewNode(NULL, BAD_CAST"video_control");
	xmlDocSetRootElement(doc, root_node);		//设置根节点

	video_global_node = xmlNewNode(NULL, BAD_CAST "video_global");
	xmlAddChild(root_node, video_global_node);
	add_video_global_param(video_global_node, g_video_control_handle->profile);

	for(index = 0; index < MAX_CHANNEL; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "video_encode_%d", index);
		video_encode_node[index] = xmlNewNode(NULL, BAD_CAST(temp));
		xmlAddChild(root_node, video_encode_node[index]);
		mid_mutex_lock(g_video_control_handle->mutex[index]);
		add_video_encode_param(video_encode_node[index], &(g_video_control_handle->info[index]));
		mid_mutex_unlock(g_video_control_handle->mutex[index]);
	}

	for(index = 0; index < MAX_CHANNEL; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "video_scale_%d", index);
		video_scale_node[index] = xmlNewNode(NULL, BAD_CAST(temp));
		xmlAddChild(root_node, video_scale_node[index]);
		mid_mutex_lock(g_video_control_handle->smutex[index]);
		add_video_scale_param(video_scale_node[index], &(g_video_control_handle->sinfo[index]));
		mid_mutex_unlock(g_video_control_handle->smutex[index]);
	}

	/*保存新的xml文件*/
	int ret = xmlSaveFormatFileEnc(xml_file, doc, "UTF-8", 1);

	if(-1 == ret) {
		PRINTF("xml save params table failed !!!\n");
		ret = -1;
		goto cleanup;
	}

	//释放文档内节点动态申请的内存
cleanup:
	xmlFreeDoc(doc);

	system("sync");


	return ret;
}

static int s_scale_mode = 0; //
int get_scale_status(void)
{
	return s_scale_mode;
}

void app_set_scale_mode(int state)
{
	int ret = 0;
	int mp_status = get_mp_status();
	PRINTF("mode=%d\n", state);

	if(IS_IND_STATUS == mp_status) {
		ret = set_scale_mode(gEnc2000.sclrLink[1].link_id, state);
	} else if(IS_MP_STATUS == mp_status) {
		ret = set_scale_mode(gEnc2000.sclrLink[0].link_id, state);
	}

	if(s_scale_mode != state && 0 == ret) {
		s_scale_mode = state;
		write_encoding_mode(get_encoding_mode(), 1);
	}
}
int write_encoding_mode(int enclevel, int save)
{
	static int g_pre_enclevel = 0;
	char 			temp[32] = {0};
	int 			ret  = 0 ;
	int 			mode = -1;
	char    		title[24];
	const char config_file[64] = ENCODING_CONFIG;
	sprintf(title, "encoding");

	//确定谁调的
	PRINTF("old g_pre_enclevel=%d\n", g_pre_enclevel);

	if((enclevel == g_pre_enclevel) && 0 == save) {
		PRINTF("NO need write ENCODING_CONFIG!\n");
		return 0;
	}

	g_pre_enclevel  = enclevel;

	sprintf(temp, "%d", g_pre_enclevel);
	ret =  ConfigSetKey((char *)config_file, title, "enclevel", temp);

	if(ret != 0) {
		PRINTF("Failed to Set mode \n");
		return -1;
	}

	mode = 0;
	mode = get_scale_status();
	sprintf(temp, "%d", mode);
	ret =  ConfigSetKey((char *)config_file, title, "scale", temp);

	if(ret != 0) {
		PRINTF("Failed to Get input \n");
		return -1;
	}

	PRINTF("write finished!enclevel=%d,scrl=%d\n", g_pre_enclevel, mode);
	return 0;
}

int read_encoding_mode()
{
	char 			temp[32] = {0};
	int 			ret  = 0 ;
	int 			mode = -1;
	char    		title[24];
	const char config_file[64] = ENCODING_CONFIG;
	sprintf(title, "encoding");

	ret =  ConfigGetKey((char *)config_file, title, "enclevel", temp);

	if(ret != 0) {
		PRINTF("Failed to Set mode \n");
		return -1;
	}

	mode = atoi(temp);
	set_encoding_mode(mode);

	mode = 0;
	ret =  ConfigGetKey((char *)config_file, title, "scale", temp);

	if(ret != 0) {
		PRINTF("Failed to Get input \n");
		return -1;
	}

	mode = atoi(temp);
	s_scale_mode = mode;
	PRINTF("write finished!\n");
	return 0;
}

int app_init_video_control()
{
	create_video_control_handle();
	read_video_control_file();
	read_encoding_mode();//只在main函数起来读一次
	//init handle
	//init mutex
	//read from file
	return 0;
}

int app_init_no_signal_res(void)
{
	//	int input = 0;
	int width = 0, height = 0;
	int scalemod = 0;
	app_set_scale_mode(get_scale_status());

	if(IS_MP_STATUS == get_mp_status()) {
		;//return 0;
	}

	if(0 == get_capture_state(SIGNAL_INPUT_1)) {
		PRINTF("input 1  NO signal!\n");
		web_scale_lock_get(CHANNEL_INPUT_1, &width, &height, &scalemod);

		if(width == 0 || height == 0) {
			width = 1920;
			height = 1080;
		}

		PRINTF("%dx%d\n", width, height);
		set_resolution(SIGNAL_INPUT_1, HIGH_STREAM, width, height, 1920, 1200);
	}

	if(0 == get_capture_state(SIGNAL_INPUT_2)) {
		PRINTF("input 2 NO signal!\n");
		web_scale_lock_get(CHANNEL_INPUT_2, &width, &height, &scalemod);

		if(width == 0 || height == 0) {
			width = 1920;
			height = 1080;
		}

		PRINTF("%dx%d\n", width, height);
		set_resolution(SIGNAL_INPUT_2, HIGH_STREAM, width, height, 1920, 1200);
	}

	PRINTF("\n");
	return 0 ;
}

