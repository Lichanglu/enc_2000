#include <stdio.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>


#include "reach_enc2000.h"
#include "common.h"
#include "audio.h"
#include "params.h"
#include "encliveplay.h"
#include "encliveplay2.h"
#ifdef HAVE_RTSP_MODULE
#include "app_stream_output.h"
#endif
#include "mid_mutex.h"
#include "mid_timer.h"
#include "log_common.h"
#include "app_protocol.h"
#include "log_server.h"
#include "MMP_control.h"

#include "reach_audio.h"
#include "app_head.h"
//#include "app_audio_control.h "
//extern params_table *gp_realtime_params;

#define  TEST_TWO_AUDIO  1

char audioname[2][AUDIO_DEVICE_NAME_LENGTH] = {"default:CARD=mcasp0", "default:CARD=EVM"};
#define	AUDIO_DEVICE_INDEX_1	0
#define	AUDIO_DEVICE_INDEX_2	1

static int g_audio_update_flag[2] = {0, 0};


int audio_set_update_flag(int audio_input, int flag)
{
	int num = 0;

	if(audio_input == AUDIO_LINEIN_1 || audio_input == AUDIO_LINEIN_2) {
		num = audio_input - 1;

		if(num > 1) {
			num = 0;
		}

		g_audio_update_flag[num] = flag;
		PRINTF("i will set %d input flag =%d\n", audio_input, flag);
	}

	return 0;
}

static int audio_get_update_flag(int audio_input)
{
	if(audio_input == AUDIO_LINEIN_1 || audio_input == AUDIO_LINEIN_2) {
		int num = audio_input - 1;

		if(num > 1) {
			num = 0;
		}

		return g_audio_update_flag[num];
	} else {
		PRINTF("Warnning,audio_input=%d.\n", audio_input);
		return 0;
	}
}

static int write_file(char *file, void *buf, int buflen)
{
	static FILE *fp = NULL;

	if(NULL == fp) {
		fp =	fopen(file, "w");

		if(NULL == fp) {
			printf("fopen  : %s", strerror(errno));
			return -1;
		}
	}

	printf("[write_file] buf : [%p], buflen : [%d] , fp : [%p]\n", buf, buflen, fp);
	fwrite(buf, buflen, 1, fp);
	return 0;
}

static int g_audio_mute[2] = {0};
static int s_mp_audio_mute = 0;
int set_mute_status(int audio_input, int status)
{
	int index = audio_input - 1;

	if(index < 0 || index > 1)	{
		return -1;
	}

	if(IS_MP_STATUS == get_mp_status()) {
		s_mp_audio_mute = status;
		return 0;
	}

	g_audio_mute[index] = status;
	return 0;
}
int set_mp_mute_status(int audio_input, int status, int mp_status)
{
	if(IS_MP_STATUS == mp_status) {
		s_mp_audio_mute = status;
		return 0;
	}

	return 0;
}

int get_mute_status(int audio_input)
{
	int aindex = audio_input - 1;

	if(aindex < 0 || aindex > 1)	{
		return 0;
	}

	if(IS_MP_STATUS == get_mp_status()) {
		return s_mp_audio_mute;
	}

	return g_audio_mute[aindex];
}

static void *AudioPcmDataInProcess(audio_struct *audioinst)
{
	audio_buf       *pa_inbuf;
	audio_buf       *pa_inbuf2;
	Int32			ret = 0;
	int audio_begin_set = 1;

	Void			*aread_handle;
	Void			*aread_handle2;


	if(audioinst == NULL) {
		PRINTF("AudioPcmDataInProcess: param is NULL!\n");
		return NULL;
	}

	aread_handle = (Void *)audioinst[0].pacaphandle;
	aread_handle2 = (Void *)audioinst[1].pacaphandle;


	while(TRUE) {
		//	writeWatchDog();
		//		sleep(10);
		checkAudioCapParam(audioinst);//检测音频参数有没有变化
		ret = audio_indata_get_empty_buf(&(audioinst[0].paenchandle->indata_user_handle), &pa_inbuf);

		if(ret < 0) {
			PRINTF("audio get empty buf error!\n");
			break;
		}

		ret = audio_read_frame(aread_handle, (Int8 *)(pa_inbuf->addr));

		if(ret < 0) {
			PRINTF("audio read frame error!\n");
			break;
		}

		if(1 == get_mute_status(AUDIO_LINEIN_1)) {
			memset(pa_inbuf->addr, 0, pa_inbuf->buf_size);
		}

		ret = audio_indata_get_empty_buf(&(audioinst[1].paenchandle->indata_user_handle), &pa_inbuf2);

		if(ret < 0) {
			PRINTF("audio get empty buf1 error!\n");
			break;
		}

		ret = audio_read_frame(aread_handle2, (Int8 *)(pa_inbuf2->addr));

		if(ret < 0) {
			PRINTF("audio read frame error!\n");
			break;
		}

		if(1 == get_mute_status(AUDIO_LINEIN_2)) {
			memset(pa_inbuf2->addr, 0, pa_inbuf2->buf_size);
		}

		//write_file("./pcm_001.pcm", pa_inbuf->addr, pa_inbuf->buf_size);

#if 0  //混音
		int nSample = 4096;
		short *pD = (short *)(pa_inbuf->addr);
		short sample1, sample2;
		double nTemp = 0;
		nSample /= 2;
		int i;

		for(i = 0; i < nSample; i++) {
			sample1 = (*((short *)(pa_inbuf->addr) + i));
			sample2 = (*((short *)(pa_inbuf2->addr) + i));

			if((sample1 < 0) && (sample2 < 0)) {
				nTemp = sample1 + sample2 - (sample1 * sample2 / -32767);
			} else {

				nTemp = sample1 + sample2 - (sample1 * sample2 / 32767);
			}

			//nTemp=(sample1 + sample2)*2/3;
			//*pD = (short)nTemp;
			pD++;
		}

#endif
		//usleep(100000);
		//memcpy((Int8 *)(pa_inbuf->addr),tmpbuf,1024);
		ret = audio_indata_put_full_buf(&(audioinst[0].paenchandle->indata_user_handle), pa_inbuf);

		if(ret < 0) {
			PRINTF("audio put full buf error!\n");
			break;
		}

		ret = audio_indata_put_full_buf(&(audioinst[1].paenchandle->indata_user_handle), pa_inbuf2);

		if(ret < 0) {
			PRINTF("audio put full buf error!\n");
			break;
		}

		if(1 == audio_begin_set) {
			audio_begin_set = 0;
			//app_audio_control_set();
		}
	}

	return (void *)0;
}

#ifdef HAVE_RTSP_MODULE
static unsigned long long last_time = 0;
#endif
static void *AudioBitsDataOutProcess(audio_struct *audioinst)
{
	Int32			ret = 0;
	AudioEncodeParam		audioparam1;
	AudioEncodeParam		audioparam2;
	AudioEncodeParam		audioparam_mp;
	int 					audio_begin1 = 1;
	int 					audio_begin2 = 1;
	int 					audio_begin_mp = 1;
	int 					mp_audio_input = 0;
	int 					audio_change = 0;
	audio_buf       *pa_outbuf1  = NULL;
	audio_buf       *pa_outbuf2  = NULL;

	unsigned char *send_buf1 = NULL;
	unsigned char *send_buf2 = NULL;
	int send_len1 = 0;
	int send_len2 = 0;
	AudioEncodeParam		temp_param;

	if(audioinst == NULL) {
		PRINTF("AudioBitsDataOutProcess: audioinst is NULL!\n");
		return NULL;
	}

	memset(&audioparam1, 0, sizeof(AudioEncodeParam));
	memset(&audioparam2, 0, sizeof(AudioEncodeParam));
	memset(&audioparam_mp, 0, sizeof(AudioEncodeParam));

#ifdef HAVE_RTSP_MODULE
	unsigned int sound_time = 0;
	unsigned long long rtp_time = 0;
	unsigned long long start_time = 0;

	unsigned long long temp_time = 0;
	APP_AUDIO_DATA_INFO info;
	int config_change1 = 1;
	int config_change2 = 1;
#endif
	PRINTF(" mute=%d,%d\n", get_mute_status(AUDIO_DEVICE_INDEX_1), get_mute_status(AUDIO_DEVICE_INDEX_2));


	//fp = fopen("audio.aac","w+");

	while(TRUE) {
		while(SWITCH_STATUS == get_mp_status()) {
			sleep(1);
			audio_change = 1;
		}

		if(1 == audio_change) {
			if(IS_MP_STATUS == get_mp_status()) {
				web_get_audio_info(SIGNAL_INPUT_MP, &temp_param);
				input_set_audio_input(temp_param.mp_input);
				//				PRINTF("mp_input=%d\n",temp_param.mp_input);
				//				input_get_audio_input(SIGNAL_INPUT_MP, &mp_audio_input);
				audio_update_config(SIGNAL_INPUT_MP, temp_param.mp_input);
			} else if(IS_IND_STATUS == get_mp_status()) {
				audio_update_config(SIGNAL_INPUT_1, AUDIO_LINEIN_1);
				//sleep(1);
				audio_update_config(SIGNAL_INPUT_2, AUDIO_LINEIN_2);
			}

			audio_change = 0;
		}

		send_len1 = 0;
		send_len2 = 0;
		writeWatchDog();

		ret = audio_outdata_get_full_buf(&(audioinst[0].paenchandle->outdata_user_handle), &pa_outbuf1);

		if(ret < 0) {
			PRINTF("audio get full buf error!\n");
			break;
		}


#if TEST_TWO_AUDIO

		ret = audio_outdata_get_full_buf(&(audioinst[1].paenchandle->outdata_user_handle), &pa_outbuf2);

		if(ret < 0) {
			PRINTF("audio get full buf error!\n");
			break;
		}

#endif

		send_buf1 = pa_outbuf1->addr;
		send_len1 = pa_outbuf1->buf_size;

		//test
		send_buf2 = pa_outbuf2->addr;
		send_len2 = pa_outbuf2->buf_size;

		//mp模式下，只能出一个音频
		if(get_mp_status() == IS_MP_STATUS) {
			int audio_input = AUDIO_LINEIN_1;
			input_get_audio_input(SIGNAL_INPUT_MP, &audio_input);

			if(audio_input == AUDIO_LINEIN_2) {
				send_buf1 = pa_outbuf2->addr;
				send_len1 = pa_outbuf2->buf_size;
				send_buf2 = pa_outbuf2->addr;
				send_len2 = pa_outbuf2->buf_size;
			} else if(audio_input == AUDIO_LINEIN_1) {
				send_buf1 = pa_outbuf1->addr;
				send_len1 = pa_outbuf1->buf_size;
				send_buf2 = pa_outbuf1->addr;
				send_len2 = pa_outbuf1->buf_size;
			}

			//check if audio config is change
			if((1 == audio_begin_mp) || audio_get_update_flag(audio_input) == 1) {
				audio_begin_mp = 0;
				memset(&temp_param, 0, sizeof(AudioEncodeParam));
				web_get_audio_info(SIGNAL_INPUT_MP,  &temp_param);

				if((audioparam_mp.SampleRateIndex != temp_param.SampleRateIndex)
				   || (audioparam_mp.BitRate != temp_param.BitRate)) {
					config_change1 = 1;
					memcpy(&audioparam_mp, &temp_param, sizeof(AudioEncodeParam));
				}

				PRINTF("input 1 ,audio SampleRateIndex = %d,bitrate=%d\n", audioparam_mp.SampleRateIndex, audioparam_mp.BitRate);
				audio_set_update_flag(audio_input, 0);
			}
		} else {
			//check if audio config is change
			if((1 == audio_begin1) || audio_get_update_flag(AUDIO_LINEIN_1) == 1) {
				audio_begin1 = 0;
				memset(&temp_param, 0, sizeof(AudioEncodeParam));
				web_get_audio_info(SIGNAL_INPUT_1,  &temp_param);


				if((audioparam1.SampleRateIndex != temp_param.SampleRateIndex)
				   || (audioparam1.BitRate != temp_param.BitRate)) {
					config_change1 = 1;
					memcpy(&audioparam1, &temp_param, sizeof(AudioEncodeParam));
				}

				PRINTF("input 1 ,audio SampleRateIndex = %d,bitrate=%d\n", audioparam1.SampleRateIndex, audioparam1.BitRate);
				audio_set_update_flag(AUDIO_LINEIN_1, 0);
			}

			//	PRINTF("123\n");
			//check if audio config is change
			if((1 == audio_begin2) || audio_get_update_flag(AUDIO_LINEIN_2) == 1) {
				audio_begin2 = 0;
				memset(&temp_param, 0, sizeof(AudioEncodeParam));
				web_get_audio_info(SIGNAL_INPUT_2,  &temp_param);


				if((audioparam2.SampleRateIndex != temp_param.SampleRateIndex)
				   || (audioparam2.BitRate != temp_param.BitRate)) {
					config_change2 = 1;
					memcpy(&audioparam2, &temp_param, sizeof(AudioEncodeParam));
				}

				PRINTF("input 2 ,audio SampleRateIndex = %d,bitrate=%d\n", audioparam2.SampleRateIndex, audioparam2.BitRate);
				audio_set_update_flag(AUDIO_LINEIN_2, 0);
			}
		}

#ifdef HAVE_RTSP_MODULE
		sound_time = get_run_time();
		start_time =  app_get_start_time();
		temp_time = rtp_time = getCurrentTime();

		if(rtp_time < start_time) {
			PRINTF("warnning,the rtp_time=%lld,the start time = %lld\n", rtp_time, start_time);
			app_set_start_time();
			start_time = app_get_start_time();
			rtp_time = 0;
		} else {
			rtp_time = rtp_time - start_time;
		}

		if(last_time > rtp_time) {
			PRINTF("Warnning,last_time > rtp_time\n");
			rtp_time = last_time + 1;
			//last_time = last_time;
		}

#endif

#if 0
		int i = 0;
		unsigned char *p_temp = send_outbuf1->addr;

		for(i = 0; i < 7; i++) {
			PRINTF("%2x", p_temp[i]);
		}

#endif

		//	PRINTF("audio len =%d", pa_outbuf->buf_size);
		SendAudioToClient1(send_len1, send_buf1, 0, 0, 16);
		LowSendAudioToClient1(send_len1, send_buf1, 0, 0, 16);

#if TEST_TWO_AUDIO
		//PRINTF("audio len =%d\n", send_len2);
		SendAudioToClient2(send_len2, send_buf2, 0, 0, 16);
		LowSendAudioToClient2(send_len2, send_buf2, 0, 0, 16);
#endif

#ifdef HAVE_RTSP_MODULE
		memset(&info, 0, sizeof(info));

		info.stream_channel = CHANNEL_INPUT_1;
		info.channel = audioparam1.Channel ;

		static int g_test = 0;
		g_test++;
		//info.timestamp = g_test *23; //sound_time;
		info.timestamp = sound_time; //sound_time;
		info.rtp_time = rtp_time & 0xffffffff;

		if(g_test % 300 == 0) {
			PRINTF("timestamp=%d,rtp_time=%d==%lld = %lld==%d\n", info.timestamp, info.rtp_time, start_time, temp_time, get_run_time());
		}

		last_time = rtp_time;

		if(app_get_have_stream() == 1 && (send_len1 != 4096)) {
			info.samplerate = ChangeSampleIndex(audioparam1.SampleRateIndex);
			info.recreate = config_change1;

			//channel 1 high
			info.stream_channel = CHANNEL_INPUT_1;
			app_set_media_info(-1, -1, info.samplerate , info.stream_channel);
			app_stream_audio_output_channel(send_buf1, send_len1, &info);

			//channel 1 low
			info.recreate = config_change1;
			info.stream_channel = CHANNEL_INPUT_1_LOW;
			app_set_media_info(-1, -1, info.samplerate , info.stream_channel);
			app_stream_audio_output_channel(send_buf1, send_len1, &info);
			config_change1 = 0;
		}

#if TEST_TWO_AUDIO


		if(app_get_have_stream() == 1 && (send_len2 != 4096)) {
			info.samplerate = ChangeSampleIndex(audioparam2.SampleRateIndex);
			info.recreate = config_change2;

			//channel 2 high
			info.stream_channel = CHANNEL_INPUT_2;
			app_set_media_info(-1, -1, info.samplerate , info.stream_channel);
			app_stream_audio_output_channel(send_buf2, send_len2, &info);

			//channel 2 low
			info.stream_channel = CHANNEL_INPUT_2_LOW;
			app_set_media_info(-1, -1, info.samplerate , info.stream_channel);
			app_stream_audio_output_channel(send_buf2, send_len2, &info);
			config_change2 = 0;

		}

#endif


#endif
		ret = audio_outdata_put_empty_buf(&(audioinst[0].paenchandle->outdata_user_handle), pa_outbuf1);

		if(ret < 0) {
			PRINTF("audio put empty buf error!\n");
			break;
		}

		//第二路音频输出
#if TEST_TWO_AUDIO

		ret = audio_outdata_put_empty_buf(&(audioinst[1].paenchandle->outdata_user_handle), pa_outbuf2);

		if(ret < 0) {
			PRINTF("audio put empty buf error!\n");
			break;
		}

#endif
	}

	return 0;
}

#define	A_CAPTURE_BUF_SIZE	4096
static Int32 set_audio_capture_params(audio_capture_params *paread_params, AudioEncodeParam *audio_param, char *audio_name)
{
	paread_params->bufsize = A_CAPTURE_BUF_SIZE;
	paread_params->channel = audio_param->Channel;

	strcpy(paread_params->device_name, audio_name);

	paread_params->samplebit = audio_param->SampleBit;
	PRINTF("audio_param.SampleBit = %d\n", audio_param->SampleBit);

	assert(audio_param->SampleBit == 8 || audio_param->SampleBit == 16);
	paread_params->samplerate = ChangeSampleIndex(audio_param->SampleRateIndex);
	paread_params->mode = AUDIO_INPUT_MODE;
	paread_params->inputmode = audio_param->InputMode;
	paread_params->mictype = 0;//mic类型
	return 0;
}

static Int32 set_audio_encdec_params(audio_encdec_params *paparams, AudioEncodeParam *audio_param)
{
	paparams->inst_type = AAC_ENC;
	paparams->aenc_create_param.encoderType = AUDIO_CODEC_TYPE_AAC_LC;
	paparams->aenc_create_param.sampleRate = ChangeSampleIndex(audio_param->SampleRateIndex);
	paparams->aenc_create_param.bitRate = audio_param->BitRate;
	paparams->aenc_create_param.numberOfChannels = audio_param->Channel;
	return 0;
}

static Int32 adjust_volume(int num, int LVolume, int RVolume)
{
	int l_value = 0;
	int r_value = 0;

	if(0 == LVolume) {
		l_value = 0;
	} else {
		l_value = 100 + 10 * (LVolume / 3);
	}

	if(0 == RVolume) {
		r_value = 0;
	} else {
		r_value = 100 + 10 * (RVolume / 3);
	}

	PRINTF("num:[%d] LVolume:[%d] RVolume:[%d]\n", num, LVolume, RVolume);
	int card = num;

	if(0 == num) {
		card = 1;
	} else if(1 == num) {
		card = 0;
	}

	char cmd[256] = {0};
	snprintf(cmd, sizeof(cmd), "amixer -c %d cset numid=5,iface=MIXER,name='PCM Volume' %d ; amixer -c %d cset numid=6,iface=MIXER,name='L ADC VOLUME' %d", card, r_value, card, l_value);
	system(cmd);

	//	memset(cmd, 0, 128);
	//	sprintf(cmd, "amixer -c %d cset numid=6,iface=MIXER,name='L ADC VOLUME' %d", card, l_value);
	//	system(cmd);

	PRINTF("num=%d,card=%d,RVolume=%d,LVolume=%d\n", num, card, r_value, l_value);
	return 0;
}

Int32 reach_audio_enc_process(ENC2000_LINK_STRUCT *pstruct)
{
	audio_encdec_params		paparams[2];
	AudioEncodeParam	 	audio_param[SIGNAL_INPUT_MAX];
	audio_capture_params	paread_params[2];
	Int32 ret = 0;
	int i;
	int input = SIGNAL_INPUT_1;
	int mp_status = get_mp_status();

	if(NULL == pstruct) {
		fprintf(stderr, "reach_audio_enc_process failed, params error!\n");
		return -1;
	}

	memset(&(audio_param[SIGNAL_INPUT_1]), 0, sizeof(AudioEncodeParam));
	memset(&(audio_param[SIGNAL_INPUT_2]), 0, sizeof(AudioEncodeParam));
	memset(&(audio_param[SIGNAL_INPUT_MP]), 0, sizeof(AudioEncodeParam));
	web_get_audio_info(SIGNAL_INPUT_1,  &audio_param[SIGNAL_INPUT_1]);
	web_get_audio_info(SIGNAL_INPUT_2,  &audio_param[SIGNAL_INPUT_2]);
	web_get_audio_info(SIGNAL_INPUT_MP,  &audio_param[SIGNAL_INPUT_MP]);

	for(i = 0; i < 2; i++) { //创建2个音频设备
		//get app audio config
		//	get_audio_param(gp_realtime_params, i, &audio_param);
		if(IS_MP_STATUS == mp_status) {
			input = SIGNAL_INPUT_MP;
		} else {
			if(i == 0) {
				input = SIGNAL_INPUT_1;
			} else if(i == 1) {
				input = SIGNAL_INPUT_2;
			}
		}

		printf_audio_info(&(audio_param[input]));
		set_audio_capture_params(&paread_params[i], &(audio_param[input]), audioname[i]);

		//		PRINTF("audio_param.SampleBit = %d=sample rate=%d\n", audio_param.SampleBit, paread_params[i].samplerate);
		//create pcm read
		ret = audio_device_create_inst((void *) & (pstruct->audioencLink[i]), &paread_params[i]);

		if(ret < 0) {
			PRINTF("audio_capture_create_inst failed!\n");
			return -1;
		}

		set_mute_status(audio_param[input].mp_input, audio_param[input].Mute);
	}

	for(i = 0; i < 2; i++) {
		int num = 0;

		if(IS_MP_STATUS == mp_status) {
			int audio_input = 0;
			input = SIGNAL_INPUT_MP;
			input_get_audio_input(input, &audio_input);
			num = i; //合成使用audio_input
		} else {
			if(i == 0) {
				input = SIGNAL_INPUT_1;
				num = 0;
			} else if(i == 1) {
				num = 1;
				input = SIGNAL_INPUT_2;
			}
		}

		set_audio_encdec_params(&paparams[i], &(audio_param[input]));
		pstruct->audioencLink[i].paparam = paparams[i];
		adjust_volume(num, audio_param[input].LVolume, audio_param[input].RVolume);
	}

	ret = audio_encdec_create_inst(pstruct->audioencLink, AudioPcmDataInProcess, AudioBitsDataOutProcess, paparams, NULL);

	if(ret < 0) {
		PRINTF("audio_encdec_create_inst failed!\n");
		return -1;
	}

	return 0;
}

int audio_update_config(int input, int audio_input)
{
	int ret = 0;
	int num = 0;
	AudioEncodeParam info;
	AudioCommonInfo cinfo;

	if(audio_input < AUDIO_LINEIN_1  || audio_input > AUDIO_LINEIN_2) {
		ERR_PRN("audio_input=%d\n", audio_input);
		return -1;
	}

	memset(&info, 0, sizeof(info));
	memset(&cinfo, 0, sizeof(cinfo));

	web_get_audio_info(input, &info);

	num = audio_input - 1;

	if(num < 0) {
		ERR_PRN("num=%d\n", num);
		return -1;
	}

	cinfo.input_num = num ;//只支持  0--1
	cinfo.samplerate = ChangeSampleIndex(info.SampleRateIndex);
	cinfo.bitrate = info.BitRate;
	cinfo.channel = info.Channel;
	cinfo.samplebit = info.SampleBit;
	cinfo.lvolume = info.LVolume;
	cinfo.rvolume = info.RVolume;
	cinfo.inputmode = info.InputMode;
	cinfo.mictype = 0;
	cinfo.is_mute = info.Mute;

	PRINTF("cinfo=%p\n", &cinfo);
	ret = audio_setParam(gEnc2000.audioencLink, cinfo);

	if(ret < 0) {
		ERR_PRN("audio_setParam failed!\n");
		return -1;
	}

	PRINTF("num=%d,audio_input=%d,mute=%d,Lvolume=%d,Rvolume=%d\n", num, audio_input, cinfo.is_mute, cinfo.lvolume, cinfo.rvolume);

	//	audio_setCapParamInput(audio_input, info.InputMode);
	audio_setCapParamSampleRate(gEnc2000.audioencLink[num].pacaphandle, ChangeSampleIndex(info.SampleRateIndex));
	//	audio_setCapParamFlag(gEnc2000.audioencLink[num].pacaphandle);//不是每次都需要重建PCM

	adjust_volume(num, cinfo.lvolume, cinfo.rvolume);
	set_mute_status(audio_input, cinfo.is_mute);

	//	audio_setEncParam(gEnc2000.audioencLink[i].paenchandle, ChangeSampleIndex(info.SampleRate), info.BitRate);
	//	audio_setEncParamFlag(gEnc2000.audioencLink[i].paenchandle);

	return 0;
}
