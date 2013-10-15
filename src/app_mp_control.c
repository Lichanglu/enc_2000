#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>



#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>


#include "reach_enc2000.h"
#include "capture.h"
#include "select.h"
#include "common.h"
#include "log.h"
#include "log_common.h"
#include "middle_control.h"
#include "sd_demo_osd.h"
#include "app_video_control.h"
#include "app_mp_control.h"

#include "video.h"
#include "capture.h"
#include "encliveplay.h"
#include "params.h"
#include "liveplay.h"
#include "select.h"

#ifdef HAVE_RTSP_MODULE
#include "app_stream_output.h"
#endif
#include "mid_mutex.h"
#include "mid_timer.h"
#include "app_protocol.h"
#include "MMP_control.h"

#include "app_video_control.h"
#include "app_head.h"


typedef struct _Layout_Pos {
	short win1;
	short win2;
} Layout_Pos;

static Layout_Pos s_layout_win[MP_LAYOUT_MAX] = {{0, 0}, {0, 0},
	{0, 0}, {0, 0}, {0, 0}, {0, 0}
};


static int g_mp_layout = 1;

extern VideoCommonHandle g_video_control_handle ;
int get_mp_layout()
{
	return g_mp_layout;
}

int set_mp_layout(int layout)
{
	g_mp_layout = layout;
	return 0;
}
int app_get_swap_layout(int layout, int *win1, int *win2)
{
	if(layout < MP_LAYOUT_1 && layout > MP_LAYOUT_6) {
		PRINTF("WARNING:layout =%d\n", layout);
		return -1;
	}

	*win1 = s_layout_win[layout].win1;
	*win2 = s_layout_win[layout].win2;
	return 0;
}

static int app_set_swap_layout(UInt32 layout, UInt32 win1, UInt32 win2)
{
	if((win1 < SIGNAL_INPUT_1 || win1 > SIGNAL_INPUT_2)
	   || (win2 < SIGNAL_INPUT_1 || win2 > SIGNAL_INPUT_2)) {
		PRINTF("WARNING:win1 =%d,win2=%d\n", win1, win2);
		return -1;
	}

	if(win1  == win2) {
		PRINTF("win2=win1=%d", win1);
		return -1;
	}

	s_layout_win[layout].win1 = win1;
	s_layout_win[layout].win2 = win2;
	return 0;
}


Void app_web_get_mp_info(Mp_Info *info)
{
	info->layout = get_mp_layout();
	info->mp_status = get_mp_status();
	app_get_swap_layout(info->layout, &(info->win1), &(info->win2));
	PRINTF("layout=%d\n", info->layout);
}

Void app_web_set_mp_layout(UInt32 layout, UInt32 win1, UInt32 win2)
{
	UInt32 devId = 0;
	Int32 ret = 0;
	SwMsLink_CreateParams *swMsCreateArgs =  &(gEnc2000.swmsLink[0].create_params);
	SwMsLink_LayoutPrm *layoutInfo;
	SwMsLink_LayoutWinInfo *winInfo, *winInfo2;
	UInt32 outWidth, outHeight, widthAlign, heightAlign;
	UInt32 outputfps;
	//	UInt32 old_layout = get_mp_layout();
	PRINTF("layout=%d\n", layout);

	getoutsize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

	widthAlign = 8;
	heightAlign = 1;

	if(devId > 1) {
		devId = 0;
	}

	layoutInfo = &swMsCreateArgs->layoutPrm;
	outputfps = layoutInfo->outputFPS;

	memset(layoutInfo, 0, sizeof(*layoutInfo));
	layoutInfo->onlyCh2WinMapChanged = FALSE;
	layoutInfo->outputFPS = outputfps;

	layoutInfo->numWin = 2;
	PRINTF("win1=%d,win2=%d\n", win1, win2);


	if((win1 < SIGNAL_INPUT_1 || win1 > SIGNAL_INPUT_2)
	   || (win2 < SIGNAL_INPUT_1 || win2 > SIGNAL_INPUT_2)
	   || (win2 == win1)) {

		ERR_PRN("WARNING:win1 =%d|| win2=%d\n", win1, win2);
		return ;
	}

	switch(layout) {
		case MP_LAYOUT_1 : {
			PRINTF("---MP_LAYOUT_1---\n");
			winInfo = &layoutInfo->winInfo[0];
			winInfo->width	= SystemUtils_align(outWidth / 3, widthAlign);
			winInfo->height = SystemUtils_align(outHeight / 3, heightAlign);
			winInfo->startX = 0;
			winInfo->startY = 0;
			winInfo->bypass = FALSE;
			winInfo->channelNum = win1;

			winInfo2 = &layoutInfo->winInfo[1];
			winInfo2->width	= SystemUtils_align(outWidth * 2 / 3, widthAlign);
			winInfo2->height = SystemUtils_align(outHeight, heightAlign);
			winInfo2->startX = winInfo->width;
			winInfo2->startY = 0;
			winInfo2->bypass = FALSE;
			winInfo2->channelNum = win2;
			break;
		}

		case MP_LAYOUT_2 : {
			PRINTF("---MP_LAYOUT_2---\n");
			winInfo = &layoutInfo->winInfo[0];
			winInfo->width	= SystemUtils_align(outWidth / 2, widthAlign);
			winInfo->height = SystemUtils_align(outHeight, heightAlign);
			winInfo->startX = 0;
			winInfo->startY = 0;
			winInfo->bypass = FALSE;
			winInfo->channelNum = win1;

			winInfo2 = &layoutInfo->winInfo[1];
			winInfo2->width	= SystemUtils_align(outWidth / 2, widthAlign);
			winInfo2->height = SystemUtils_align(outHeight, heightAlign);
			winInfo2->startX = winInfo->width;
			winInfo2->startY = 0;
			winInfo2->bypass = FALSE;
			winInfo2->channelNum = win2;
			break;
		}

		case MP_LAYOUT_3: {
			PRINTF("---MP_LAYOUT_3---\n");
			winInfo = &layoutInfo->winInfo[1];
			winInfo->width	= SystemUtils_align(outWidth / 3, widthAlign);
			winInfo->height = SystemUtils_align(outHeight / 3, heightAlign);
			winInfo->startX = 0;
			winInfo->startY = 0;
			winInfo->bypass = FALSE;
			winInfo->channelNum = win1;

			winInfo2 = &layoutInfo->winInfo[0];
			winInfo2->width	= SystemUtils_align(outWidth, widthAlign);
			winInfo2->height = SystemUtils_align(outHeight, heightAlign);
			winInfo2->startX = 0;
			winInfo2->startY = 0;
			winInfo2->bypass = FALSE;
			winInfo2->channelNum = win2;

			break;
		}

		case MP_LAYOUT_4: {
			PRINTF("---MP_LAYOUT_4---\n");
			winInfo = &layoutInfo->winInfo[1];
			winInfo->width	= SystemUtils_align(outWidth / 3, widthAlign);
			winInfo->height = SystemUtils_align(outHeight / 3, heightAlign);
			winInfo->startX = 0;
			winInfo->startY = outHeight - winInfo->height;
			winInfo->bypass = FALSE;
			winInfo->channelNum = win1;

			winInfo2 = &layoutInfo->winInfo[0];
			winInfo2->width	= SystemUtils_align(outWidth, widthAlign);
			winInfo2->height = SystemUtils_align(outHeight, heightAlign);
			winInfo2->startX = 0;
			winInfo2->startY = 0;
			winInfo2->bypass = FALSE;
			winInfo2->channelNum = win2;
			break;
		}

		case MP_LAYOUT_5: {
			PRINTF("---MP_LAYOUT_5---\n");
			winInfo = &layoutInfo->winInfo[1];
			winInfo->width	= SystemUtils_align((outWidth) / 3, widthAlign);
			winInfo->height = SystemUtils_align(outHeight / 3, heightAlign);
			winInfo->startX = outWidth - winInfo->width;
			winInfo->startY = 0;
			winInfo->bypass = FALSE;
			winInfo->channelNum = win1;

			winInfo2 = &layoutInfo->winInfo[0];
			winInfo2->width	= SystemUtils_align((outWidth), widthAlign);
			winInfo2->height = SystemUtils_align(outHeight, heightAlign);
			winInfo2->startX = 0;
			winInfo2->startY = 0;
			winInfo2->bypass = FALSE;
			winInfo2->channelNum = win2;
			break;
		}

		case MP_LAYOUT_6: {
			PRINTF("---MP_LAYOUT_6---\n");

			winInfo = &layoutInfo->winInfo[1];
			winInfo->width	= SystemUtils_align((outWidth) / 3, widthAlign);
			winInfo->height = SystemUtils_align(outHeight / 3, heightAlign);
			winInfo->startX = outWidth - winInfo->width;
			winInfo->startY = outHeight - winInfo->height;
			winInfo->bypass = FALSE;
			winInfo->channelNum = win1;

			winInfo2 = &layoutInfo->winInfo[0];
			winInfo2->width	= SystemUtils_align((outWidth), widthAlign);
			winInfo2->height = SystemUtils_align(outHeight, heightAlign);
			winInfo2->startX = 0;
			winInfo2->startY = 0;
			winInfo2->bypass = FALSE;
			winInfo2->channelNum = win2;

			break;
		}

		default:
			PRINTF("---MP_LAYOUT_2---\n");
			winInfo = &layoutInfo->winInfo[0];
			winInfo->width	= SystemUtils_align(outWidth / 2, widthAlign);
			winInfo->height = SystemUtils_align(outHeight, heightAlign);
			winInfo->startX = 0;
			winInfo->startY = 0;
			winInfo->bypass = FALSE;
			winInfo->channelNum = win1;

			winInfo2 = &layoutInfo->winInfo[1];
			winInfo2->width	= SystemUtils_align(outWidth / 2, widthAlign);
			winInfo2->height = SystemUtils_align(outHeight, heightAlign);
			winInfo2->startX = winInfo->width;
			winInfo2->startY = 0;
			winInfo2->bypass = FALSE;
			winInfo2->channelNum = win2;
			break;
	}

	app_set_swap_layout(layout, win1, win2);

	ret = swms_set_layout(gEnc2000.swmsLink[0].link_id, layoutInfo);

	set_mp_layout(layout);

	PRINTF("ret=%d\n", ret);
}


Int32 mp_set_resolution(Int32 input, Int32 streamId, UInt32 outWidth, UInt32 outHeight, UInt32 inWidth, UInt32 inHeight)
{

	if((input !=  SIGNAL_INPUT_MP) || (streamId >= MAX_STREAM)) {
		PRINTF("WARNNING:input =%d \n", input);
		return -1;
	}


	if((outWidth <= 0) || (outHeight <= 0)) {
		return -1;
	}

	PRINTF("input=%d,streamId=%d,outWidth=%d,outHeight=%d,inWidth=%d,inHeight=%d\n",
	       input, streamId, outWidth, outHeight, inWidth, inHeight);

	outWidth  = Reach_align(outWidth, 16u);
	outHeight = Reach_align(outHeight, 8u);

	if(LOW_STREAM == streamId) {
		SclrLink_chDynamicSetOutRes params;

		/* 设置各通道输出分辨率  */
		params.width  = outWidth;
		params.height = outHeight;
		params.pitch[0]  = outWidth * 2;
		params.pitch[1]  = outWidth * 2;
		params.pitch[2]  = outWidth * 2;
		params.chId = 0;

		sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_0, &params);
		sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_0, input, 60, 30);
	} else if(HIGH_STREAM == streamId) {

		SelectLink_OutQueChInfo prm;
		SclrLink_chDynamicSetOutRes params;
		prm.outQueId = 0;
		/*
		capture_get_input_hw(input, &width, &height);

		if(height == 540 && width == 1920) {
			height = height * 2;
		}

		if(inWidth != 0) {
			width = inWidth;
		}

		if(inHeight != 0) {
			height = inHeight;
		}
		*/
		/* 进行缩放 */
		params.width  = outWidth;
		params.height = outHeight;
		params.pitch[0]  = outWidth * 2;
		params.pitch[1]  = outWidth * 2;
		params.pitch[2]  = outWidth * 2;
		params.chId = 1;
		sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_0, &params);
		sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_0, 3, 60, 60);

	}

	return 0;
}

int app_set_mp_info(Mp_Info *mp_info)
{
	int mp_staus = get_mp_status();
	int win1 = SIGNAL_INPUT_2, win2 = SIGNAL_INPUT_1;
	int save = 0;
	PRINTF("mp=%d,layout=%d,win1,win2=%d/%d\n", mp_info->mp_status,
	       mp_info->layout, mp_info->win1, mp_info->win2);
	app_get_swap_layout(mp_info->layout, &win1, &win2);

	if(mp_info->mp_status != IS_IND_STATUS && mp_info->mp_status != IS_MP_STATUS) {
		PRINTF("new_mp_stats=%d\n", mp_info->mp_status);
		return 0;
	}

	writeWatchDog();

	if(mp_staus != mp_info->mp_status) {
		if(mp_info->mp_status == IS_IND_STATUS) {
			mp_reach_stop();
			reach_begin();
		} else if(mp_info->mp_status == IS_MP_STATUS) {
			reach_stop();
			mp_reach_begin();
		}

		save = 1;
	}

	if(IS_MP_STATUS == mp_info->mp_status) {
		if(mp_info->layout != get_mp_layout()) {
			save = 1;
		}

		if(mp_info->win1 != win1 && mp_info->win2 != win2) {
			save = 1;
		}

		//		app_get_swap_layout(mp_info.layout, &win1, &win2);
		app_web_set_mp_layout(mp_info->layout, mp_info->win1, mp_info->win2);
	}

	if(1 ==  save) {
		write_mp_config();
	}

	return 0;
}


void app_mp_video_control()
{
	if(g_video_control_handle == NULL) {
		ERR_PRN("g_video_control_handle is not init\n");
		return ;
	}

	int channel = 0;
	int enc_chid = 0;
	VideoEnodeInfo *info = NULL;
	int scale, outwidth, outheight, width, height, input, high_stream;

	input = high_stream = outwidth = outheight = scale = width = height = -1;


	for(channel = CHANNEL_INPUT_MP; channel < MAX_CHANNEL; channel++) {
		channel_get_input_info(channel, &input, &high_stream);
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

		//---------
		outwidth = outheight = scale = width = height = -1;
		app_get_scale_status(channel, &scale, &outwidth, &outheight);

		if(scale == LOCK_SCALE) {
			capture_get_input_hw(input, &width, &height);

			if(height == 540 && width == 1920) {
				height = height * 2;
			}

			PRINTF("the %d channel is lock scale ,scale widht=%d,hight=%d\n", channel, outwidth, outheight);
			mp_set_resolution(input, high_stream, outwidth, outheight, width, height);
		} else {
			if(CHANNEL_INPUT_MP == channel) {
				mp_set_resolution(input, high_stream, 1920, 1080, 1920, 1080);
			} else {
				mp_set_resolution(input, high_stream, 352, 288, 1920, 1080);
			}

			PRINTF("the %d channle is not lock scale\n", channel);
		}
	}

	return ;
}


int app_get_original_resolution(int *width, int *height)
{
	*width = 1920;
	*height = 1080;
	return 0;
}

int read_mp_config(int *mp_status)
{
	char 			temp[64] = {0};
	int 			ret  = 0 ;
	int layout = 1;
	int i = 0;
	const char config_file[64] = MP_CONFIG;
	//	int input=SIGNAL_INPUT_1;
	int  audio_input = SIGNAL_INPUT_1;

	if(mp_status == NULL) {
		return -1;
	}

	ret =  ConfigGetKey((char *)config_file, "MP", "mp", temp);

	if(ret != 0) {
		ERR_PRN("Failed	 to Get mp \n");
		return -1;
	}

	*mp_status = atoi(temp);


	ret =  ConfigGetKey((char *)config_file, "MP", "audio_input", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get audio_input \n");
		return -1;
	}

	audio_input = atoi(temp);
	input_set_audio_input(audio_input);


	ret =  ConfigGetKey((char *)config_file, "MP", "layout", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get layout \n");
		return -1;
	}

	layout = atoi(temp);
	set_mp_layout(layout);

	for(i = MP_LAYOUT_1; i < MP_LAYOUT_MAX; i++) {
		char title[16] = {0};
		int win1 = SIGNAL_INPUT_1, win2 = SIGNAL_INPUT_1;

		sprintf(title, "layout=%d", i);
		ret =  ConfigGetKey((char *)config_file, title, "win1", temp);

		if(ret != 0) {
			ERR_PRN("Failed to Get win1 \n");
			return -1;
		}

		win1 = atoi(temp);

		ret =  ConfigGetKey((char *)config_file, title, "win2", temp);

		if(ret != 0) {
			ERR_PRN("Failed to Get win2 \n");
			return -1;
		}

		win2 = atoi(temp);

		app_set_swap_layout(i, win1, win2);
		PRINTF("layout_%d:win1=%d,win2=%d\n", i, win1, win2);
	}

	PRINTF("mp_status=%d,layout=%d,audio_input=%d\n", *mp_status, layout, audio_input);
	return 0;
}

int write_mp_config()
{
	char temp[64] = {0};
	int ret  = 0 ;
	int i = 0 ;
	int input = SIGNAL_INPUT_MP;
	int layout = 1;
	int  audio_input = SIGNAL_INPUT_1;
	const char config_file[64] = MP_CONFIG ;

	int mp_status = get_mp_status();
	sprintf(temp, "%d", mp_status);
	ret =  ConfigSetKey((char *)config_file, "MP", "mp", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set mp \n");
		return -1;
	}

	input_get_audio_input(input, &audio_input);
	sprintf(temp, "%d", audio_input);
	ret =  ConfigSetKey((char *)config_file, "MP", "audio_input", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set audio_input \n");
		return -1;
	}

	layout = get_mp_layout();
	sprintf(temp, "%d", layout);
	ret =  ConfigSetKey((char *)config_file, "MP", "layout", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set layout \n");
		return -1;
	}

	for(i = MP_LAYOUT_1; i < MP_LAYOUT_MAX; i++) {
		char title[16] = {0};
		int win1 = SIGNAL_INPUT_1, win2 = SIGNAL_INPUT_2;
		app_get_swap_layout(i, &win1, &win2);

		sprintf(title, "layout=%d", i);
		sprintf(temp, "%d", win1);
		ret =  ConfigSetKey((char *)config_file, title, "win1", temp);

		if(ret != 0) {
			ERR_PRN("Failed to Set win1 \n");
			return -1;
		}

		sprintf(temp, "%d", win2);
		ret =  ConfigSetKey((char *)config_file, title, "win2", temp);

		if(ret != 0) {
			ERR_PRN("Failed to Set win2 \n");
			return -1;
		}
	}

	PRINTF("mp_status=%d,layout=%d,audio_input=%d\n", mp_status, layout, audio_input);
	return ret;
}


void init_MP_info(int *mp_status)
{
	int ret = 0;
	int i = 0;
	*mp_status = IS_IND_STATUS;

	for(i = MP_LAYOUT_1 ; i < MP_LAYOUT_MAX; i++) {
		app_set_swap_layout(i, SIGNAL_INPUT_1, SIGNAL_INPUT_2);
	}

	ret = read_mp_config(mp_status);
}

