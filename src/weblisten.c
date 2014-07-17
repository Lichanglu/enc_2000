#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>



#include "rtsp_server.h"

#include "./stream_output/stream_output_struct.h"
#include "weblisten.h"
#include "middle_control.h"
#include "mid_timer.h"
#include "rtsp_output_server.h"
#include "app_protocol.h"
#include "multicast_output_server.h"
#include "common.h"
#include "remotectrl.h"
#include "params.h"
#include "sysparams.h"
#include "reach_enc2000.h"
#include "reach_capture.h"
#include "MMP_control.h"
#include "app_video_control.h"
#include "app_audio_control.h"
#include "app_signal.h"
#include "log_common.h"
#include "encliveplay.h"
#include "encliveplay2.h"
#include "app_head.h"
#include "video.h"
#include "app_update.h"
#include "app_mp_control.h"
#include "app_text_logo.h"

#define THREAD_SUCCESS      (void *) 0
#define THREAD_FAILURE      (void *) -1

#define MAX_LISTEN		 10
#define DSP1			0
static int app_web_get_stream_protocol(int identifier, int fd, char *data, int valen);
static int app_web_stream_output_process_int(int channel, int cmd, int in, int *out);
static int app_web_stream_output_process_struct(int identifier, int fd, int cmd, char *data, int valen);
static int app_web_get_remote_protole(int , int *);
static int app_web_set_remote_protole(int , int);
static int app_web_PTZ_ctrl(unsigned char *data, int len, int index);
//static int app_web_get_audioparam(int identifier, int fd, char *data, int valen);
//static int app_web_get_mute(int cmd, int in, int *out);
//static int app_web_set_audioparam(int identifier, int fd, int cmd, char *data, int valen);
//static int app_web_set_mute(int cmd, int in, int *out);

extern 	int MMP_config_update2(int high, int type , unsigned char *data, int len);
/*消息头打包*/
static void msgPacket(int identifier, unsigned char *data, webparsetype type, int len, int cmd, int ret)
{
	webMsgInfo_1260  msghead;
	int	cmdlen = sizeof(type);
	int retlen = sizeof(ret);
	msghead.identifier = identifier;
	msghead.type = type;
	msghead.len = len;
	memcpy(data, &msghead, MSGINFOHEAD);
	memcpy(data + MSGINFOHEAD, &cmd, cmdlen);
	memcpy(data + MSGINFOHEAD + cmdlen, &ret, retlen);
	//PRINTF("msghead.len=%d\n", msghead.len);
	return ;
}


//get signal hdcp
static int app_web_get_HDCP(int *out, int channel)
{
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	Signal_Info info;
	int ret = 0;
	memset(&info, 0, sizeof(Signal_Info));
	channel_get_input_info(channel, &input , &high);
	ret  = app_get_signal_info(input, &info);
	printf_signal_info(input, &info);
	*out = info.hdcp;

	if(ret < 0) {
		*out = 0;
		PRINTF("WARNING:get hdcp failed!\n");
		return -1;
	}

	return 0;
}

typedef struct hv_info_ {
	int input;
	int hporch;
	int vporch;
	int save;
} hv_info_t;

static int set_hv_processed = 1;
static hv_info_t hv_info;
static void *app_set_hv_process(void *arg)
{
	app_set_hv(hv_info.input, hv_info.hporch, hv_info.vporch, hv_info.save);
	usleep(500000);
	set_hv_processed = 1;
	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return arg;
}

static int set_hv(void)
{
	if(set_hv_processed) {
		PRINTF("process set hv,set_hv_processed=%d\n", set_hv_processed);
		pthread_t p;

		if(pthread_create(&p, NULL, app_set_hv_process, (void *)NULL)) {
			PRINTF("pthread_create failed !!!\n");
			return -1;
		}

		set_hv_processed = 0;
	} else {
		PRINTF("miss set hv,set_hv_processed=%d\n", set_hv_processed);
	}

	return 0;
}

static int app_web_set_revise(int hv, int channel)
{
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	memset(&hv_info, 0, sizeof(hv_info_t));
	//	int in = 0;
	short hporch = 0, vporch = 0;
	channel_get_input_info(channel, &input , &high);
	hporch = hv >> 16;
	vporch = hv & 0xffff;
	PRINTF("input=%d,hporch=%d,vporch=%d\n", input, hporch, vporch);

	//	in = hporch ? hporch : vporch;
	//	PRINTF("In=%d\n", in);
	hv_info.hporch = hporch;
	hv_info.vporch = vporch;
	hv_info.input = input;
	hv_info.save = 1;
	//app_set_hv(input, hporch, vporch, 1);
	set_hv();
	return 0;
}

int app_web_set_CbCr(int channel)
{
	PRINTF("channel:[%d]\n", channel);
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	channel_get_input_info(channel, &input , &high);
	app_set_CbCr(input);
	return 0;
}
/*
static int app_web_set_revise(int hv, int channel)
{
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	//	int in = 0;
	short hporch = 0, vporch = 0;
	channel_get_input_info(channel, &input , &high);
	hporch = hv >> 16;
	vporch = hv & 0xffff;

	PRINTF("input=%d,hporch=%d,vporch=%d\n", input, hporch, vporch);

	//	in = hporch ? hporch : vporch;
	//	PRINTF("In=%d\n", in);

	app_set_hv(input, hporch, vporch, 1);
	return 0;
}
*/
static int app_web_get_vinfo(int channel, WebVideoEncodeInfo *out)
{
	PRINTF("-channel=%d-", channel);
	//	WebVideoEncodeInfo *info = (WebVideoEncodeInfo *)out;
	web_video_info_get(channel, out);
	PRINTF("out:IFrameinterval=%d--sBitrate=%d\n", out->IFrameinterval, out->sBitrate);
	return 0;
}

static int app_web_set_vinfo(int channel, char *in, char *out)
{
	int change = 0;
	int ret = 0;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	/*
		int mp_status = get_mp_status();

		if(mp_status == IS_MP_STATUS) {
			channel = CHANNEL_INPUT_MP;
		}
	*/
	channel_get_input_info(channel, &input, &high);

	WebVideoEncodeInfo *oldinfo = (WebVideoEncodeInfo *)in;
	WebVideoEncodeInfo *newinfo = (WebVideoEncodeInfo *)out;
	web_video_info_set(channel, oldinfo, &change);
	web_video_info_get(channel, newinfo);
	PRINTF("-INPUT=%d-", input);
	//if change == 1,i will do something

	if(change == 1) {
		MMPVideoParam old_param;
		ret = MMP_video_info_get(channel, &old_param);

		if(ret < 0) {
			PRINTF("get_video_param error!\n");
			return -1;
		}

		if(input == SIGNAL_INPUT_2) {
			MMP_config_update2(high, 2 , (unsigned char *)&old_param, sizeof(MMPVideoParam));
		} else if(input == SIGNAL_INPUT_1) {
			MMP_config_update1(high, 2 , (unsigned char *)&old_param, sizeof(MMPVideoParam));
		} else if(input == SIGNAL_INPUT_MP) { //mp ->input1
			MMP_config_update1(high, 2 , (unsigned char *)&old_param, sizeof(MMPVideoParam));
			MMP_config_update2(high, 2 , (unsigned char *)&old_param, sizeof(MMPVideoParam));
		}

	}

	return 0;
}


int resolution_to_width(int mode , int *width, int *height)
{
	if(mode == LOCK_AUTO) {
		*width = 0;
		*height = 0;
	} else if(mode == LOCK_CUSTOM) {
		//*width = 0;
		//*height = 0;
	} else if(mode == LOCK_1920x1080P) {
		*width = 1920;
		*height = 1080;
	} else if(mode == LOCK_1280x720P) {
		*width = 1280;
		*height = 720;
	} else if(mode == LOCK_1400x1050_VGA) {
		*width = 1400;
		*height = 1050;
	} else if(mode == LOCK_1366x768_VGA) {
		*width = 1366;
		*height = 768;
	} else if(mode == LOCK_1280x1024_VGA) {
		*width = 1280;
		*height = 1024;
	} else if(mode == LOCK_1280x768_VGA) {
		*width = 1280;
		*height = 768;
	} else if(mode == LOCK_1024x768_VGA) {
		*width = 1024;
		*height = 768;
	} else if(mode == LOCK_720x480_D1) {
		*width = 720;
		*height = 480;
	} else if(mode == LOCK_352x288_D1) {
		*width = 352;
		*height = 288;
	} else {
		*width = 0;
		*height = 0;
	}

	return 0;
}

static int width_to_resolution(int width, int height, int *mode)
{
	if(width == 352 && height == 288) {
		*mode = LOCK_352x288_D1;
	} else if(width == 720 && height == 480) {
		*mode = LOCK_720x480_D1;
	} else if(width == 1024 && height == 768) {
		*mode = LOCK_1024x768_VGA;
	} else if(width == 1280 && height == 768) {
		*mode = LOCK_1280x768_VGA;
	} else if(width == 1280 && height == 1024) {
		*mode = LOCK_1280x1024_VGA;
	} else if(width == 1366 && height == 768) {
		*mode = LOCK_1366x768_VGA;
	} else if(width == 1400 && height == 1050) {
		*mode = LOCK_1400x1050_VGA;
	} else if(width == 1280 && height == 720) {
		*mode = LOCK_1280x720P;
	} else if(width == 1920 && height == 1080) {
		*mode = LOCK_1920x1080P;
	} else if(width == 0 && height == 0) {
		*mode = LOCK_AUTO;
	} else {
		*mode = LOCK_AUTO;
	}

	return 0;
}


static int scale_res_flag = 0; // 1 ->lock auto
int app_web_get_scaleinfo(int channel, char *out)
{
	WebScaleInfo sinfo;
	int ret = -1;

	memset(&sinfo, 0, sizeof(WebScaleInfo));
	ret = web_scale_lock_get(channel, &(sinfo.outwidth), &(sinfo.outheight), &(sinfo.scalemode));

	if(0 == scale_res_flag) {
		width_to_resolution(sinfo.outwidth, sinfo.outheight, &(sinfo.resolution));
	} else {
		sinfo.resolution = LOCK_AUTO;
	}

	PRINTF("the widht=%d,height=%d,resolution=%d\n", sinfo.outwidth, sinfo.outheight, sinfo.resolution);
	memcpy(out, &sinfo, sizeof(WebScaleInfo));

	return ret;
}

#ifdef HAVE_IP_XML
extern int g_bak_resolution;
#endif
int app_web_set_scaleinfo(int channel, char *in, char *out)
{
	WebScaleInfo *info = (WebScaleInfo *)in;
	WebScaleInfo *newinfo = (WebScaleInfo *)out;
	int outwidth = -1;
	int outheight = -1;
	int scalemode = 0; //不再有scalemode消息
	int ret = -1;
	//	int mp_status = get_mp_status();
	resolution_to_width(info->resolution, &outwidth, &outheight);

	PRINTF("info->resolution= %d,width=%d,height=%d\n", info->resolution, outwidth, outheight);

	if(outwidth != -1 && outheight != -1 && scalemode != -1) {
		ret = web_scale_lock_set(channel, outwidth, outheight, scalemode);
	}

	web_scale_lock_get(channel, &(newinfo->outwidth), &(newinfo->outheight), &(newinfo->scalemode));

	if(0 != info->resolution) {
		scale_res_flag = 0;
		width_to_resolution(newinfo->outwidth, newinfo->outheight, &(newinfo->resolution));
	} else {
		newinfo->resolution = 0;
		scale_res_flag = 1;
	}

	PRINTF("the width=%d,height=%d,resolution=%d\n", newinfo->outwidth, newinfo->outheight, newinfo->resolution);

#ifdef HAVE_IP_XML
	g_bak_resolution = newinfo->resolution;
#endif
	return ret;
}

static int web_set_restore(int restore, int *out)
{
	PRINTF("restore=%d\n", restore);
	system("rm .config -Rf");
	*out = 1;
	PRINTF("reboot ...\n");
	system("reboot -f");
	return 0;
}

int midParseInt(int identifier, int fd, char *data, int len)
{
	int recvlen;
	int channel = 0, subcmd = 0, tcmd = 0;
	int actdata = 0;
	//char logoname[15] = "logo.png";
	int ret = 0;
	int web_ret = SERVER_RET_OK;

	int need_send = 0;


	char senddata[1024] = {0};
	int totallen = 0;

	recvlen = recv(fd, data, len, 0);
	int out = 0;


	if(recvlen < 0 || len != sizeof(tcmd) + sizeof(int)) {
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}


	memcpy(&tcmd, data, sizeof(int));
	memcpy(&actdata, data + sizeof(int), len - sizeof(int));

	channel = ((tcmd >> 16) & 0xf) - 1;
	subcmd = tcmd & 0xffff;
	PRINTF("channel = %d,subsmd =0x%x,tcmd= 0x%x\n", channel, subcmd, tcmd);

	if(channel < -1 || channel > MAX_CHANNEL) {
		need_send = 1;
		return -1;
	}

	PRINTF("tcmd = 0x%04x,subcmd=0x%04x\n", tcmd, subcmd);

	switch(subcmd) {

#ifdef HAVE_RTSP_MODULE

			//stream_ouput
		case MSG_RTSP_GET_USED:
		case MSG_RTSP_DEL_SERVER:
		case MSG_MULT_GET_NUM:
		case MSG_MULT_GET_TS_RATE:
		case MSG_MULT_GET_RTP_RATE:
		case MSG_MULT_GET_RTPTS_RATE:
			PRINTF("MSG_STREAM\n");
			app_web_stream_output_process_int(channel, subcmd, actdata, &out);
			need_send = 1;
			break;
#endif

		case MSG_GET_CTRL_PROTO:
			PRINTF("MSG_GET_CTRL_PROTO\n");
			app_web_get_remote_protole(channel, &out);

			need_send = 1;
			break;

		case MSG_SET_CTRL_PROTO: {
			int ret = 0;
			PRINTF("MSG_SET_CTRL_PROTO\n");
			ret = app_web_set_remote_protole(channel, actdata);
			PRINTF("ret=%d\n", ret);
			need_send = 1;
		}
		break;

		case MSG_GETMUTE:
			PRINTF("MSG_GETMUTE\n");
			//	app_web_get_mute(subcmd, actdata, &out);
			need_send = 1;
			break;

		case MSG_SETMUTE:
			PRINTF("MSG_SETMUTE\n");
			//			app_web_set_mute(subcmd, actdata, &out);
			need_send = 1;
			break;

		case MSG_GETCPULOAD:
			PRINTF("--MSG_GETCPULOAD\n");
			out = 0;
			need_send = 1;
			//cpu load
			break;

		case MSG_RESTORESET:
			PRINTF("--MSG_RESTORESET--\n");
			web_set_restore(actdata, &out);
			need_send = 1;
			break;


		case MSG_CHANNEL_NUM:
			PRINTF("MSG_CHANNEL_NUM\n");
			PRINTF("**---channel = %d ---**\n", actdata);
			need_send = 1;
			break;

			// signal input set
		case MSG_SETINPUTTYPE: {
			PRINTF("----MSG_SETINPUTTYPE\n");
			app_web_set_input_type(actdata, &out, channel);
			need_send = 1;
			break;
		}

		case MSG_GETINPUTTYPE:
			PRINTF("---MSG_GETINPUTTYPE\n");
			get_input_type(&out, channel);
			need_send = 1;
			break;

		case MSG_GETCOLORSPACE:
			PRINTF("--MSG_GETCOLORSPACE\n");
			out = app_get_color_space(channel);
			need_send = 1;
			break;


		case MSG_SETCOLORSPACE:
			PRINTF("MSG_SETCOLORSPACE\n");
			app_set_color_space(channel, actdata, &out);
			need_send = 1;
			break;

		case MSG_GETHDCPVALUE:
			PRINTF("------MSG_GETHDCPVALUE\n");
			app_web_get_HDCP(&out, channel);
			need_send = 1;
			break;

		case MSG_REVISE_PICTURE:
			PRINTF("--MSG_REVISE_PICTURE\n");
			PRINTF("actdata=%d\n", actdata);
			app_web_set_revise(actdata,  channel);
			need_send = 1;
			break;

		case MSG_SDI_REVISE_PICTURE:
			PRINTF("--MSG_REVISE_PICTURE\n");
			PRINTF("actdata=%d\n", actdata);
			app_enable_SDI_adjust_flag(channel);
			app_web_set_revise(actdata,  channel);
			need_send = 1;
			break;

		case MSG_REBOOTSYS:
			PRINTF("cmd = 0x%04x ---MSG_REBOOTSYS.\n", subcmd);
			need_send = 1;
			app_web_rebootSys(actdata, &out);
			break;

		case MSG_GET_MAXPOS:
			PRINTF("cmd = 0x%04x ---MSG_GET_MAXPOS.\n", subcmd);
			app_get_osd_max_xypos(channel, &out);
			need_send = 1;
			break;

		case MSG_GET_ENCODING_PROFILE: {
			PRINTF("--MSG_GET_ENCODING_PROFILE--\n");
			out = get_encoding_mode();
			need_send = 1;
			break;
		}

		case MSG_SET_ENCODING_PROFILE: {
			PRINTF("--MSG_SET_ENCODING_PROFILE--\n");
			need_send = 1;
			write_encoding_mode(actdata, 1); //直接写，不改变全局变量
			break;
		}

		case MSG_SET_SCALE_MODE: {
			PRINTF("--MSG_SET_SCALE_MODE--\n");
			app_set_scale_mode(actdata);
			need_send = 1;
			break;
		}

		case MSG_GET_SCALE_MODE: {

			PRINTF("--MSG_GET_SCALE_MODE--\n");
			out = get_scale_status();
			need_send = 1;
			break;
		}

		case MSG_SETCBCR: {
			PRINTF("--MSG_SETCBCR--\n");
			app_web_set_CbCr(channel);
			break;
		}

		default:
			PRINTF("unkonwn cmd = %04x\n", subcmd);
			need_send = 1;
			web_ret = SERVER_RET_UNKOWN_CMD;
			break;
			//	case
	}

	if(ret < 0) {
		web_ret = SERVER_RET_INVAID_PARM_VALUE;
	}

EXIT:

	if(need_send == 1) {
		totallen = MSGINFOHEAD + sizeof(subcmd) + sizeof(web_ret) + sizeof(out);
		msgPacket(identifier, (unsigned char *)senddata, INT_TYPE, totallen, tcmd, web_ret);
		memcpy(senddata + (totallen - sizeof(out)), &out, sizeof(out));
		PRINTF("the cmd =%04x,the value=%d,the ret=%04x\n", tcmd, out, web_ret);
		send(fd, senddata, totallen, 0);

		if(web_ret != SERVER_RET_OK) {
			PRINTF("ERROR,the cmd =0x%x,ret= 0x%x\n", tcmd, web_ret);
		}
	}

	return 0;
}


static int webupdateFile(int identifier, int fd, int cmd, char *data, int vallen)
{
	unsigned char sdata[256] = {0};
	int totallen = 0;
	int cmdlen = sizeof(int);
	int retlen = sizeof(int);
	char filename[1024] = {0};
	int ret = 0;
	memset(filename, 0x000, vallen);
	memcpy(filename, data, vallen);
	PRINTF("update Ok!!!!!!!  filename = %s\n", filename);

	ret = updatesystem(filename);

	return ret;
}


int midParseString(int identifier, int fd, char *data, int len)
{
	int recvlen;
	//	int cmd = 0;
	char actdata[4096] = {0};
	int vallen = 0;

	char senddata[1024] = {0};
	int totallen = 0;
	int channel = 0 ;
	int tcmd = 0, subcmd = 0;

	char  out[4096] = "unknown cmd.";
	int outlen = 0;
	int web_ret = SERVER_RET_OK;
	int update_ret = -1;
	int need_send = 0;

	recvlen = recv(fd, data, len, 0);

	vallen = len - sizeof(int);

	if(recvlen < 0 || vallen > sizeof(actdata)) {
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}


	memcpy(&tcmd, data, sizeof(int));
	memcpy(actdata, data + sizeof(int), len - sizeof(int));
	//PRINTF("cmd = 0x%04x,vallen=%d=len=%d\n", cmd, vallen, len);
	channel = ((tcmd >> 16) & 0xf) - 1;
	subcmd = tcmd & 0xffff;
	PRINTF("channel = %d,subsmd =0x%x,tcmd= 0x%x\n", channel, subcmd, tcmd);
	printf("channel = %d,subsmd =0x%x,tcmd= 0x%x\n", channel, subcmd, tcmd);

	if(channel < -1 || channel > MAX_CHANNEL) {
		need_send = 1;
		web_ret = SERVER_RET_INVAID_PARM_VALUE;
		goto EXIT;
	}

	switch(subcmd) {

		case MSG_FAR_CTRL:
			app_web_PTZ_ctrl((unsigned char *)actdata, vallen, channel);
			need_send = 1;
			break;

		case MSG_GETINPUTSIGNALINFO:
			PRINTF("cmd = 0x%04x is MSG_GETINPUTSIGNALINFO.\n", subcmd);
			app_get_signal_resolution(channel, out, &outlen);
			need_send = 1;
			break;

		case MSG_UPLOADIMG:
			PRINTF("cmd = 0x%04x is MSG_UPLOADIMG.\n", subcmd);
			web_ret = app_update_logo(channel, actdata, out);

			need_send = 1;
			break;

		case MSG_GETSOFTCONFIGTIME:
			PRINTF("cmd = 0x%04x is MSG_GETSOFTCONFIGTIME.\n", subcmd);
			app_get_build_time(out);
			need_send = 1;
			break;

		case MSG_GETDEVICETYPE:
			PRINTF("cmd = 0x%04x is MSG_GETDEVICETYPE.\n", subcmd);
			app_get_device_type(out, vallen);

			need_send = 1;
			break;

		case MSG_GETSERIALNO:
			PRINTF("cmd = 0x%04x is MSG_GETSERIALNO.\n", subcmd);
			app_get_serialno(out);
			need_send = 1;
			break;

		case MSG_GETSOFTVERSION:
			PRINTF("cmd = 0x%04x is MSG_GETSOFTVERSION.\n", subcmd);
			app_get_soft_version(out, sizeof(out));
			need_send = 1;
			break;

		case MSG_SIGNALDETAILINFO:
			PRINTF("cmd = 0x%04x is MSG_SIGNALDETAILINFO.\n", subcmd);
			app_web_get_signal_detail_info(channel, out, outlen);
			need_send = 1;
			break;

		case MSG_UPDATESYS: {
			char *filename = malloc(64);
			int ret = 0;
			memset(filename, 0x000, 64);
			memcpy(filename, data + sizeof(int), vallen);
			// recvlen = recv(fd, filename, vallen, 0);

			if(recvlen < vallen) {
				ERR_PRN("update FAILED!!!!!!!\n");
				web_ret = SERVER_UPDATE_FAILED;
				//ret = -1;
			} else {
				update_ret = webupdateFile(identifier, fd, subcmd, filename, vallen);

				if(0 != update_ret) {
					web_ret = SERVER_UPDATE_FAILED;
				}
			}

			need_send = 1;

			PRINTF("web update ret =%d\n", update_ret);
			//	send(fd, &ret, sizeof(int), 0);
			free(filename);
			//system("sync");
			//	sleep(4);

			//	if(ret == 0) {
			//		sleep(3);
			//		PRINTF("----------------reboot----------\n");
			//		system("reboot -f");
			//	}
		}
		break;

		case MSG_SETSERIALNO: {
			PRINTF("--MSG_SETSERIALNO--\n");
			app_set_serialno(actdata);
			need_send = 1;
		}
		break;

		default:
			PRINTF("Warnning,the cmd %d is UNKOWN\n", subcmd);
			need_send = 1;
			break;
	}

EXIT:

	if(need_send == 1) {
		totallen = MSGINFOHEAD + sizeof(tcmd) + sizeof(web_ret) + strlen(out);
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, tcmd, web_ret);
		memcpy(senddata + (totallen - strlen(out)), out, strlen(out));
		PRINTF("the cmd =%04x,the out=%s,the ret=%04x\n", tcmd, out, web_ret);
		send(fd, senddata, totallen, 0);

		if(0 == update_ret) {
			sleep(3);
			system("reboot -f");
		}
	}


	return 0;
}


int midParseStruct(int identifier, int fd, char *data, int len)
{
	int recvlen;
	int tcmd = 0;
	char actualdata[1024] = {0};
	char out[2048] = {0};
	int  vallen = 0;
	int  status = 0;
	int  ret = 0;


	char senddata[1024] = {0};
	int totallen = 0;

	int web_ret = SERVER_RET_OK;
	int need_send = 0;

	int channel = CHANNEL_INPUT_1;
	int subcmd = 0;
	recvlen = recv(fd, data, len, 0);

	if(recvlen < 0) {
		ERR_PRN("recv failed,errno = %d,error message:%s \n", errno, strerror(errno));
		//return -1;
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		status = -1;
		goto EXIT;
	}

	vallen = len - sizeof(int);

	memcpy(&tcmd, data, sizeof(int));
	memcpy(actualdata, data + sizeof(int), len - sizeof(int));
	//PRINTF("cmd = 0x%04x,vallen=%d=len=%d\n", cmd, vallen, len);
	channel = ((tcmd >> 16) & 0xf) - 1;
	subcmd = tcmd & 0xffff;
	PRINTF("channel = %d,subsmd =0x%x,tcmd= 0x%x\n", channel, subcmd, tcmd);

	if(channel < -1 || channel > MAX_CHANNEL) {
		need_send = 1;
		ret = SERVER_RET_INVAID_PARM_VALUE;
		goto EXIT;
	}

	switch(subcmd) {

#ifdef HAVE_RTSP_MODULE

		case MSG_RTSP_GET_GINFO:
		case MSG_RTSP_SET_GINFO:
		case MSG_RTSP_GET_CINFO:
		case MSG_RTSP_SET_CINFO:
		case MSG_RTSP_ADD_SERVER:
		case MSG_MULT_ADD_SERVER:
		case MSG_MULT_GET_CINFO:
		case MSG_MULT_SET_CINFO:
		case MSG_RTSP_SET_STATUS:
		case MSG_MULT_SET_STATUS:
		case MSG_MULT_DEL_SERVER:
		case MSG_GET_SDP_INFO:
			PRINTF("cmd = 0x%04x is MSG_STREAMOUTPUT\n", subcmd);
			app_web_stream_output_process_struct(identifier, fd, subcmd, actualdata, vallen);
			break;
			//		case

		case MSG_STREAM_GET_PROTOCOL:
			need_send = 1;

			if(vallen != sizeof(Protocol)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			PRINTF("cmd = 0x%04x is MSG_STREAM_GET_PROTOCOL\n", subcmd);
			app_web_get_stream_protocol(identifier, fd, out, vallen);
			break;
#endif

		case MSG_GETAUDIOPARAM:
			need_send = 1;

			if(vallen != sizeof(AudioEncodeParam)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			app_web_get_ainfo(channel, out);
			break;

		case MSG_SETAUDIOPARAM:
			need_send = 1;

			if(vallen != sizeof(AudioEncodeParam)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			app_web_set_ainfo(channel, actualdata, out);
			need_send = 1;
			break;

		case MSG_SET_TEXTINFO:
			PRINTF("-----MSG_SET_TEXTINFO----\n");
			need_send = 1;

			if(vallen != sizeof(TextInfo)) {
				PRINTF("vallen=%d \n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				return -1;
			}

			ret = app_set_text(channel, actualdata, vallen, out);
			break;

		case MSG_GET_TEXTINFO:
			PRINTF("-----MSG_GET_TEXTINFO----\n");
			need_send = 1;

			if(vallen != sizeof(TextInfo)) {
				PRINTF("vallen=%d \n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				return -1;
			}

			ret = app_get_text(channel, actualdata, vallen, out);
			break;

		case MSG_SET_LOGOINFO:
			PRINTF("-----MSG_SET_LOGOINFO----\n");
			need_send  = 1;

			if(vallen != sizeof(LogoInfo)) {
				ret = -1;
				PRINTF("WRONG:vallen != sizeof(LogoInfo) \n");
				return ret;
			}

			ret = app_set_logo(channel, actualdata, vallen, out);

			if(ret != 0) {
				web_ret = SERVER_RET_INVAID_PARM_VALUE;
			}



			break;

		case MSG_GET_LOGOINFO:
			PRINTF("-----MSG_GET_LOGOINFO----\n");
			ret = app_get_logo(channel, actualdata, vallen, out);

			if(ret != 0) {
				web_ret = SERVER_RET_INVAID_PARM_VALUE;
			}


			need_send = 1;
			break;

		case MSG_GETSCALEINFO:
			need_send = 1;

			if(vallen != sizeof(WebScaleInfo)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			app_web_get_scaleinfo(channel, out);

			PRINTF("cmd = 0x%04x is MSG_GETSCALEINFO\n", subcmd);
			break;

		case MSG_SETSCALEINFO:
			need_send = 1;

			if(vallen != sizeof(WebScaleInfo)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			ret = app_web_set_scaleinfo(channel, actualdata, out);

			if(ret == 2) {
				PRINTF("server lock!\n");
				web_ret = SERVER_RET_USER_INVALIED;
			}

			//test_scale(actualdata, 0, channel);
			PRINTF("cmd = 0x%04x is MSG_SETSCALEINFO\n", subcmd);
			break;

		case MSG_GETVIDEOENCODEINFO:
			need_send = 1;
			PRINTF("cmd = 0x%04x is MSG_GETVIDEOENCODEINFO\n", subcmd);

			if(vallen != sizeof(WebVideoEncodeInfo)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			app_web_get_vinfo(channel, out);
			break;

		case MSG_SETVIDEOENCODEINFO:
			need_send = 1;

			PRINTF("cmd = 0x%04x is MSG_SETVIDEOENCODEINFO\n", subcmd);

			if(vallen != sizeof(WebVideoEncodeInfo)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			app_web_set_vinfo(channel, actualdata, out);
			break;

#if 0

		case MSG_GETENABLETEXTINFO:
			need_send = 1;

			if(vallen != sizeof(WebEnableTextInfo)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			app_osd_enable(out, 1, channel);
			PRINTF("cmd = 0x%04x is MSG_GETENABLETEXTINFO\n", subcmd);
			break;

		case MSG_SETENABLETEXTINFO:
			need_send = 1;

			if(vallen != sizeof(WebEnableTextInfo)) {
				PRINTF("vallen=%d\n", vallen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			app_osd_enable(actualdata, 0, channel);
			PRINTF("cmd = 0x%04x is MSG_SETENABLETEXTINFO\n", subcmd);
			break;
#endif

		case MSG_GETSYSPARAM:
			PRINTF("------cmd = 0x%04x--MSG_GETSYSPARAM\n", subcmd);

			if(vallen != sizeof(Enc2000_Sys)) {
				PRINTF("WARNING:vallen != sizeof(Enc2000_Sys)!\n");
				goto EXIT;
			}

			app_get_network(out, &vallen);
			need_send = 1;
			break;

		case MSG_SETSYSPARAM:
			if(vallen != sizeof(Enc2000_Sys)) {
				PRINTF("WARNING:vallen != sizeof(Enc2000_Sys)!\n");
				goto EXIT;
			}

			Enc2000_Sys *network = (Enc2000_Sys *)actualdata;
			ret = app_set_network(&(network->eth0), &(network->eth1));

			if(ret < 0) {
				need_send = 1;
				web_ret = SERVER_RET_USER_INVALIED;
			} else if(1 == ret) {
				/*
					system("sync");
					sleep(4);

					PRINTF("----------------reboot----------\n");
					system("reboot -f");
					*/
				need_send = 1;
				web_ret = SERVER_RET_OK;
			} else if(0 == ret) {
				need_send = 1;
				web_ret = SERVER_RET_USER_INVALIED ;
			}

			break;

		case MSG_GETENCODETIME:
			PRINTF("cmd = 0x%04x --- MSG_GETENCODETIME.\n", subcmd);
			need_send = 1;

			if(vallen != sizeof(DATE_TIME_INFO)) {
				PRINTF("WARNING:vallen != sizeof(DATE_TIME_INFO)!\n");
				goto EXIT;
			}

			get_encode_time(out, vallen);

			break;

		case MSG_SYNCTIME:
			PRINTF("cmd = 0x%04x is MSG_SYNCTIME.\n", subcmd);
			need_send = 1;

			if(vallen != sizeof(DATE_TIME_INFO)) {
				PRINTF("WARNING:vallen != sizeof(DATE_TIME_INFO)!\n");
				goto EXIT;
			}

			set_encode_time(actualdata, vallen);

			break;

		case MSG_GET_MP_INFO: {
			PRINTF("cmd = 0x%04x is MSG_GET_MP_INFO.\n", subcmd);
			need_send = 1;

			if(vallen != sizeof(Mp_Info)) {
				PRINTF("WARNING:vallen != sizeof(MSG_GET_MP_INFO)!\n");
				goto EXIT;
			}

			Mp_Info mp_info;
			memset(&mp_info, 0, sizeof(Mp_Info));
			app_web_get_mp_info(&mp_info);
			memcpy(out, &mp_info, sizeof(Mp_Info));
			PRINTF("mp_info layout=%d,win1=%d,win2=%d,mp_status=%d\n", mp_info.layout,
			       mp_info.win1, mp_info.win2, mp_info.mp_status);

			break;
		}

		case MSG_SET_MP_INFO: {
			PRINTF("cmd = 0x%04x is MSG_SET_MP_INFO.\n", subcmd);
			need_send = 1;

			if(vallen != sizeof(Mp_Info)) {
				PRINTF("WARNING:vallen != sizeof(MSG_SET_MP_INFO)!\n");
				goto EXIT;
			}

			Mp_Info mp_info;
			memcpy(&mp_info, actualdata, sizeof(Mp_Info));
			app_set_mp_info(&mp_info);

			PRINTF("MSG_SET_MP_INFO set done!.....\n\n\n\n\n");
			break;
		}

		default:

			PRINTF("channel = %d,UNKOWN  cmd %d\n", channel, subcmd);
			break;

	}

	if(ret < 0) {
		need_send = 1;
		web_ret = SERVER_RET_INVAID_PARM_VALUE;
	}

EXIT:

	if(need_send == 1) {
		totallen = MSGINFOHEAD + sizeof(tcmd) + sizeof(web_ret) + vallen;
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, tcmd, web_ret);
		memcpy(senddata + (totallen - vallen), out, vallen);
		PRINTF("the cmd =%04x,,the ret=%04x,out[0]=%d,out[1]=%d,vallen=%d\n", tcmd,  web_ret,
		       out[0], out[1], vallen);
		send(fd, senddata, totallen, 0);

		if(web_ret != SERVER_RET_OK) {
			PRINTF("the cmd =0x%x,ret= 0x%x\n", subcmd, web_ret);
		}

	}

	//	free(actualdata);
	return status;
}


#ifdef HAVE_RTSP_MODULE
static int app_web_get_stream_protocol(int identifier, int fd, char *data, int valen)
{
	Protocol temp;
	Protocol info;
	memset(&temp, 0, sizeof(Protocol));
	app_protocol_get(&info);

	if(info.status == 1) {
		memcpy(&temp, &info, sizeof(Protocol));
	}

	memcpy(data, &temp, sizeof(Protocol));
	PRINTF("get protocol:ts:%d,rtp:%d,rtsp:%d\n", info.TS, info.RTP, info.RTSP);
	return 0;
}

static int app_web_stream_output_process_int(int channel, int cmd, int in, int *out)
{
	int ret = 0;

	switch(cmd) {
		case MSG_RTSP_GET_USED:
			PRINTF("MSG_RTSP_GET_USED\n");
			ret = app_rtsp_server_used();

			if(ret < 0) {
				ret = -1;
			}

			*out = ret;
			break;

		case MSG_RTSP_DEL_SERVER:
			PRINTF("MSG_RTSP_DEL_SERVER\n");
			app_rtsp_server_delete();
			*out = 0;
			break;

		case MSG_MULT_GET_NUM:
			PRINTF("MSG_MULT_GET_NUM\n");
			ret = app_multicast_get_total_num();

			if(ret < 0) {
				ret = -1;
			}

			*out = ret;
			break;

		case MSG_MULT_GET_TS_RATE:
		case MSG_MULT_GET_RTP_RATE:
		case MSG_MULT_GET_RTPTS_RATE:
			ret = stream_get_rate(cmd, channel);

			if(ret < 0) {
				ret = -1;
			}

			*out = ret;
			break;

		default:
			break;
	}

	return ret;

}
int app_web_get_sdpinfo(RTP_SDP_INFO *in, RTP_SDP_INFO *out)
{
	if(in ==  NULL || out == NULL) {
		ERR_PRN("\n");
		return -1;
	}

	char sdp[2048] = {0};
	int ret = 0;
	ret = rtsp_porting_get_sdp_describe(sdp, sizeof(sdp), in->channel, in->ip, in->vport, in->aport);

	if(ret == -1) {
		return -1;
	}

	snprintf(out->buff, sizeof(out->buff), "%s", sdp);
	PRINTF("sdp:\n%s\n", out->buff);
	return 0;
}
static int app_web_stream_output_process_struct(int identifier, int fd, int cmd, char *data, int valen)
{
	int ret = 0;
	char out[2048] = {0};
	char *temp = NULL;
	//	int outlen = 0;
	int needlen = 0;
	int msgheadlen = 0;
	int totallen = 0;
	stream_output_server_config *config = NULL;
	msgheadlen = MSGINFOHEAD + sizeof(int) + sizeof(ret);
	temp = out + msgheadlen;

	switch(cmd) {
		case MSG_RTSP_GET_GINFO:
			PRINTF("MSG_RTSP_GET_GINFO\n");
			needlen = sizeof(rtsp_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			ret =  app_rtsp_server_get_global_info((rtsp_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		case MSG_RTSP_SET_GINFO:
			PRINTF("MSG_RTSP_SET_GINFO\n");
			needlen = sizeof(rtsp_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			ret =  app_rtsp_server_set_global_info((rtsp_server_config *)data, (rtsp_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		case MSG_RTSP_GET_CINFO:
			PRINTF("MSG_RTSP_GET_CINFO\n");
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			ret = app_rtsp_server_get_common_info((stream_output_server_config *)(temp));
			stream_server_config_printf((stream_output_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		case MSG_RTSP_SET_CINFO:
			PRINTF("MSG_RTSP_SET_CINFO\n");
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)data);
			ret =  app_rtsp_server_set_common_info((stream_output_server_config *)data, (stream_output_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)(temp));
			break;

		case MSG_RTSP_SET_STATUS:
			PRINTF("MSG_RTSP_SET_STATUS\n");
			needlen = sizeof(stream_output_server_config);
			PRINTF("needlen = %d,valen=%d\n", needlen, valen);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)data);
			ret =  app_rtsp_server_set_status((stream_output_server_config *)data, (stream_output_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)(temp));
			break;


		case MSG_RTSP_ADD_SERVER:
			PRINTF("MSG_RTSP_ADD_SERVER\n");
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)data);
			ret =  app_rtsp_server_add((stream_output_server_config *)data, (stream_output_server_config *)temp);

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)(temp));
			break;

		case MSG_MULT_ADD_SERVER:
			PRINTF("MSG_MULT_ADD_SERVER\n");
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)data);
			ret =  app_multicast_add_server((stream_output_server_config *)data, (stream_output_server_config *)temp);

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)(temp));
			break;


		case MSG_MULT_GET_CINFO:
			PRINTF("MSG_MULT_GET_CINFO\n");

			config = (stream_output_server_config *)data;
			int num = config->num;
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			PRINTF("MSG_MULT_GET_CINFO num=%d\n", num);
			ret = app_multicast_get_config(num, (stream_output_server_config *)(temp));
			stream_server_config_printf((stream_output_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		case MSG_MULT_SET_CINFO:
			PRINTF("MSG_MULT_SET_CINFO\n");
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)data);
			ret =  app_multicast_set_config((stream_output_server_config *)data, (stream_output_server_config *)(temp));
			stream_server_config_printf((stream_output_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		case MSG_MULT_SET_STATUS:
			PRINTF("MSG_MULT_SET_STATUS\n");
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)data);
			ret =  app_multicast_set_status((stream_output_server_config *)data, (stream_output_server_config *)(temp));
			stream_server_config_printf((stream_output_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		case MSG_MULT_DEL_SERVER:
			PRINTF("MSG_MULT_DEL_SERVER\n");
			needlen = sizeof(stream_output_server_config);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			stream_server_config_printf((stream_output_server_config *)data);
			ret =  app_multicast_delete_server((stream_output_server_config *)data);
			stream_server_config_printf((stream_output_server_config *)(temp));

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		case MSG_GET_SDP_INFO:
			PRINTF("MSG_GET_SDP_INFO");
			needlen = sizeof(RTP_SDP_INFO);

			if(needlen != valen) {
				PRINTF("needlen = %d,valen=%d\n", needlen, valen);
				ret = SERVER_RET_INVAID_PARM_LEN;
				goto EXIT;
			}

			ret =  app_web_get_sdpinfo((RTP_SDP_INFO *)data, (RTP_SDP_INFO *)temp);

			if(ret < 0) {
				ret = SERVER_INTERNAL_ERROR;
				goto EXIT;
			}

			break;

		default:
			break;

	}

EXIT:
	totallen = msgheadlen  + needlen;
	msgPacket(identifier, (unsigned char *)out, STRUCT_TYPE, totallen, cmd, ret);

	if(ret != SERVER_RET_OK) {
		PRINTF("ERROR,the cmd =0x%x,ret= 0x%x\n", cmd, ret);
	}

	PRINTF("ret = %d,the totallen = %d\n", ret, totallen);
	send(fd, out, totallen, 0);
	return 0;

}
#endif
static int app_web_get_remote_protole(int channel, int *out)
{
	int input, high;
	channel_get_input_info(channel, &input, &high);
	ASSERT_INPUT(input);

	*out = GetRmtProtoleIndex(input);
	PRINTF("protol=%d\n", *out);
	return 0;
}

static int app_web_set_remote_protole(int channel, int index)
{
	int input, high;
	channel_get_input_info(channel, &input, &high);
	ASSERT_INPUT(input);
	PRINTF("input=%d,in=%d\n", input, index);
	SaveRemoteCtrlIndex(input, index);
	return SetRmtProtoleIndex(input, index);
}

static int app_web_PTZ_ctrl(unsigned char *data, int len, int channel)
{

	PRINTF("app_web_PTZ_ctrl...\n");
	int input = SIGNAL_INPUT_1, high = HIGH_STREAM;
	channel_get_input_info(channel, &input, &high);
	PRINTF("channel:[%d] input:[%d] high:[%d]\n", channel, input, high);
	FarCtrlCamera(0, data, len, input_get_remote(input));
	return 0;
}

/*WEB server Thead Function*/
void *weblisten1260ThrFxn()
{
	PRINTF("get pid= %d\n", getpid());
	PRINTF("weblisten2000ThrFxn start\n");
	void                   *status              = 0;
	int 					listenSocket  		= 0 , flags = 0;
	struct sockaddr_in 		addr;
	//struct in_addr          addr_ip;
	int len, client_sock, opt = 1;
	struct sockaddr_in client_addr;
	webMsgInfo_1260		webinfo_1260;
	ssize_t			recvlen;
	//int count = 0;
	char  data_1260[2048] = {0};
	// signal_set();


	//	for(count = TS_STREAM; count < MAX_PROTOCOL; count++) {
	//		WebInitMultAddr(&s_MultAddr[count]);
	//	}

	len = sizeof(struct sockaddr_in);
	memset(&client_addr, 0, len);
	listenSocket =	socket(PF_INET, SOCK_STREAM, 0);

	if(listenSocket < 1)	{
		status  = THREAD_FAILURE;
		return status;
	}

	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family =       AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(ENCODESERVER_1260_PORT);
	PRINTF("\n");
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(listenSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)  	{
		ERR_PRN("[weblistenThrFxn] bind failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}

	PRINTF("\n");

	if(-1 == listen(listenSocket, MAX_LISTEN))     {
		ERR_PRN("listen failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}

	PRINTF("\n");

	if((flags = fcntl(listenSocket, F_GETFL, 0)) == -1)	{
		ERR_PRN("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		//gblSetQuit();
		status  = THREAD_FAILURE;
		return status ;
	}

	if(fcntl(listenSocket, F_SETFL, flags | O_NONBLOCK) == -1) {
		ERR_PRN("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		//gblSetQuit();
		status  = THREAD_FAILURE;
		return status ;
	}

	//ProtocolInit();
	//InitProtocalStatus();




	PRINTF("\n");

	while(1) {
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(listenSocket, &rfds);
		PRINTF("\n");
		//接收recv_buf 复位为空!
		select(FD_SETSIZE, &rfds , NULL , NULL , NULL);
		PRINTF("\n");
		client_sock = accept(listenSocket, (struct sockaddr *)&client_addr, (socklen_t *)&len);

		if(0 > client_sock)     {
			PRINTF("\n");

			if(errno == ECONNABORTED || errno == EAGAIN) 	{
				//usleep(20000);
				continue;
			}

			PRINTF("weblisten thread Function errno  = %d\n", errno);
			status  = THREAD_FAILURE;
			return status;
		}

		memset(&webinfo_1260, 0, sizeof(webinfo_1260));
		recvlen = recv(client_sock, &webinfo_1260, sizeof(webinfo_1260), 0);
		PRINTF("client_sock=%d,recvlen = %d webinfo.len =%d webinfo.type = %d,id=%x\n",
		       client_sock, recvlen, webinfo_1260.len, webinfo_1260.type, webinfo_1260.identifier);

		if(recvlen < 1)	{
			PRINTF("recv failed,errno = %d,error message:%s,client_sock = %d\n", errno, strerror(errno), client_sock);
			status  = THREAD_FAILURE;
			return status;
		}

		if(webinfo_1260.identifier != WEB_IDENTIFIER) {
			PRINTF("id  error,client_sock = %d\n", client_sock);
			status  = THREAD_FAILURE;
			return status;
		}

		len = webinfo_1260.len - sizeof(webinfo_1260);

		PRINTF("web deal begin =%d\n", webinfo_1260.type);

		switch(webinfo_1260.type) {
			case INT_TYPE:
				//PRINTF("web deal begin =%d\n", webinfo_1260.type);
				midParseInt(webinfo_1260.identifier, client_sock, data_1260, len);
				//PRINTF("web deal end =%d\n", webinfo_1260.type);
				break;

			case STRING_TYPE:
				//PRINTF("web deal begin =%d\n", webinfo_1260.type);
				midParseString(webinfo_1260.identifier, client_sock, data_1260, len);
				//PRINTF("web deal end =%d\n", webinfo_1260.type);
				break;

			case STRUCT_TYPE:
				//PRINTF("web deal begin =%d\n", webinfo_1260.type);
				midParseStruct(webinfo_1260.identifier, client_sock, data_1260, len);
				//PRINTF("web deal end =%d\n", webinfo_1260.type);
				break;

			default:
				break;
		}

		PRINTF("web deal end =%d\n", webinfo_1260.type);
		close(client_sock);
	}

	//ProtocolExit();

	close(listenSocket);
	PRINTF("Web listen Thread Function Exit!!\n");
	return status;
}





int app_weblisten_init()
{
	pthread_t           webListen1260Thread;

	if(pthread_create(&webListen1260Thread, NULL, weblisten1260ThrFxn, NULL)) {
		ERR_PRN("Failed to create web listen thread\n");
		return -1;
	}

	PRINTF("app_weblisten_init\n");
	return 0;
}
