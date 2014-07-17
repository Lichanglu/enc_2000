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
#endif
#include "mid_mutex.h"
#include "mid_timer.h"
#include "log_common.h"
#include "app_protocol.h"
#include "MMP_control.h"

#include "app_audio_control.h"

#include "xml/xml_base.h"

#include "middle_control.h"


#include "audio.h"
#include "input_to_channel.h"

typedef struct __AudioCommonInfo_T_ {
	AudioEncodeParam 		info[SIGNAL_INPUT_MAX];
	mid_mutex_t  	mutex[SIGNAL_INPUT_MAX];  //互斥锁
} AudioControlInfo;


typedef AudioControlInfo *AudioCommonHandle;
static AudioCommonHandle g_audio_control_handle = NULL;

#define AUDIO_INPUT_CHECK(input) \
	if(input < 0 && input > SIGNAL_INPUT_MP)    \
	{ 							\
		ERR_PRN("input = %d is invaild\n",input);    \
		return -1;        \
	}

static int mmp_check_audio_param(AudioEncodeParam *oldinfo, MMPAudioParam *newinfo, int *change);
//static int web_audio_check_param(int, AudioEncodeParam *oldinfo, AudioEncodeParam *newinfo, int *change);
static int read_audio_encode_param(xmlDocPtr pdoc, xmlNodePtr pnode, AudioEncodeParam *paudio);
static int add_audio_encode_param(xmlNodePtr pnode, AudioEncodeParam *paudio);

static int read_audio_control_file();
static int write_audio_control_file();
static void create_audio_control_handle();
static void detele_audio_control_handle(AudioCommonHandle *handle);


void printf_audio_info(AudioEncodeParam *info)
{
	PRINTF("audio info: \n");
	PRINTF("SampleRate:%d,BitRate:%d,Channel:%d,SampleBit:%d,LVolume:%d,RVolume:%d,InputMode:%d.\n", info->SampleRateIndex, info->BitRate, info->Channel, info->SampleBit, info->LVolume, info->RVolume, info->InputMode);
	PRINTF("mp_input:%d,mute:%d\n", info->mp_input, info->Mute);
	return ;
}


int MMP_audio_info_get(int input, MMPAudioParam *aparam)
{
	AUDIO_INPUT_CHECK(input);
	AudioEncodeParam *info = NULL;
	mid_mutex_lock(g_audio_control_handle->mutex[input]);
	info = &(g_audio_control_handle->info[input]);
	aparam->Codec = info->Codec;
	aparam->SampleRateIndex = info->SampleRateIndex;
	aparam->BitRate = info->BitRate;
	aparam->Channel = info->Channel;
	aparam->SampleBit = info->SampleBit;
	aparam->LVolume = info->LVolume;
	aparam->RVolume = info->RVolume;
	aparam->InputMode = info->InputMode;
	mid_mutex_unlock(g_audio_control_handle->mutex[input]);
	return 0;
}


int MMP_audio_set_mute(int input, unsigned char *data)
{
	AudioEncodeParam info;

	AudioEncodeParam out_info;
	int channel = 0 ;
	int mp_status = get_mp_status();

	if(IS_MP_STATUS == mp_status) {
		input = SIGNAL_INPUT_MP;
	}

	channel = input_get_high_channel(input);
	app_web_get_ainfo(channel, &info);

	if(NULL == data) {
		PRINTF("error,data=NULL\!");
		return -1;
	}

	PRINTF("input=%d mp_status=%d,data[0]=%d\n", input, mp_status, data[0]);

	if(1 == data[0]) {
		info.Mute = 1;
	} else {
		info.Mute = 0;
	}

	app_web_set_ainfo(channel, &info, &out_info);
	return 0;
}

//*change = 0,notingh change,  1,mmp change, 2, only other change
static int mmp_check_audio_param(AudioEncodeParam *oldinfo, MMPAudioParam *newinfo, int *change)
{
	if(oldinfo == NULL || newinfo == NULL) {
		ERR_PRN("NULL ptr\n");
		return -1;
	}

	int mmp_change = 0;

	// 不支持44100采样率
	if((/*newinfo->SampleRateIndex == 2 ||*/ newinfo->SampleRateIndex == 3)
	   && (newinfo->SampleRateIndex != oldinfo->SampleRateIndex)) {
		mmp_change = 1;
		oldinfo->SampleRateIndex = newinfo->SampleRateIndex;
	}

	if((newinfo->BitRate == MIN_AAC_LC_BITRATE || newinfo->BitRate == 96000 || newinfo->BitRate == 128000)
	   && (newinfo->BitRate != oldinfo->BitRate)) {
		mmp_change = 1;
		oldinfo->BitRate = newinfo->BitRate;
	}

	if((newinfo->LVolume >= 0 && newinfo->LVolume <= 100)
	   && (newinfo->LVolume != oldinfo->LVolume)) {
		mmp_change = 1;
		oldinfo->LVolume = newinfo->LVolume;
	}


	if((newinfo->RVolume >= 0 && newinfo->RVolume <= 100)
	   && (newinfo->RVolume != oldinfo->RVolume)) {
		mmp_change = 1;
		oldinfo->RVolume = newinfo->RVolume;
	}

	/*
		if((newinfo->mp_input == AUDIO_LINEIN_1 || newinfo->mp_input == AUDIO_LINEIN_2)
		   && (newinfo->mp_input != oldinfo->mp_input)) {
			mmp_change = 1;
			oldinfo->mp_input = newinfo->mp_input;
		}
	*/
	if(mmp_change == 1) {
		*change = 1;
	} else {
		*change = 0;
	}

	return 0;
}


int mmp_set_audio_info(int input, MMPAudioParam *aparam, int *change)
{
	AUDIO_INPUT_CHECK(input);
	AudioEncodeParam *info = NULL;
	int ret = 0;
	int audio_input = AUDIO_LINEIN_1;
	int mp_status = get_mp_status();

	if(SIGNAL_INPUT_1 == input) {
		audio_input = AUDIO_LINEIN_1;
	} else if(SIGNAL_INPUT_2 == input) {
		audio_input = AUDIO_LINEIN_2;
	} else if(SIGNAL_INPUT_MP == input && IS_MP_STATUS == mp_status) {
		input_get_audio_input(input, &audio_input);
	}

	mid_mutex_lock(g_audio_control_handle->mutex[input]);
	info = &(g_audio_control_handle->info[input]);
	ret = mmp_check_audio_param(info, aparam, change);

	mid_mutex_unlock(g_audio_control_handle->mutex[input]);

	if(*change != 0) {
		audio_update_config(input, audio_input);
		audio_set_update_flag(audio_input, 1);
		//may be need mid_timer
		write_audio_control_file();
	}

	return 0;
}

static int web_check_audio_param(AudioEncodeParam *oldinfo, AudioEncodeParam *newinfo, int *change)
{
	if(oldinfo == NULL || newinfo == NULL) {
		ERR_PRN("NULL ptr\n");
		return -1;
	}

	PRINTF("printf new audio\n");
	printf_audio_info(newinfo);
	int web_change = 0;

	if((newinfo->SampleRateIndex == 2 || newinfo->SampleRateIndex == 3)
	   && (newinfo->SampleRateIndex != oldinfo->SampleRateIndex)) {
		web_change = 1;
		oldinfo->SampleRateIndex = newinfo->SampleRateIndex;
	}

	if((newinfo->BitRate == MIN_AAC_LC_BITRATE || newinfo->BitRate == 96000 || newinfo->BitRate == 128000)
	   && (newinfo->BitRate != oldinfo->BitRate)) {
		web_change = 1;
		oldinfo->BitRate = newinfo->BitRate;
	}

	if((newinfo->LVolume >= 0 && newinfo->LVolume <= 100)
	   && (newinfo->LVolume != oldinfo->LVolume)) {
		web_change = 1;
		oldinfo->LVolume = newinfo->LVolume;
	}


	if((newinfo->RVolume >= 0 && newinfo->RVolume <= 100)
	   && (newinfo->RVolume != oldinfo->RVolume)) {
		web_change = 1;
		oldinfo->RVolume = newinfo->RVolume;
	}


	if((0 == newinfo->Mute || 1 == newinfo->Mute)
	   && (newinfo->Mute != oldinfo->Mute)) {
		web_change = 1;
		oldinfo->Mute = newinfo->Mute;
	}

	if((newinfo->mp_input == AUDIO_LINEIN_1 || newinfo->mp_input == AUDIO_LINEIN_2)
	   && (newinfo->mp_input != oldinfo->mp_input)) {
		web_change = 1;
		oldinfo->mp_input = newinfo->mp_input;
		input_set_audio_input(newinfo->mp_input);
	}

	PRINTF("web_change=%d\n", web_change);

	if(web_change == 1) {
		*change = 1;
	} else {
		*change = 0;
	}

	return 0;
}


int web_get_audio_info(int input, AudioEncodeParam *aparam)
{
	AUDIO_INPUT_CHECK(input);
	AudioEncodeParam *info = NULL;
	mid_mutex_lock(g_audio_control_handle->mutex[input]);
	info = &(g_audio_control_handle->info[input]);
	memcpy(aparam, info, sizeof(AudioEncodeParam));
	mid_mutex_unlock(g_audio_control_handle->mutex[input]);

	return 0;
}

int app_web_get_ainfo(int channel, char *out)
{
	AudioEncodeParam sinfo;
	int ret = -1;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	memset(&sinfo, 0, sizeof(AudioEncodeParam));

	channel_get_input_info(channel, &input , &high);
	PRINTF("--INPUT=%d--\n", input);

	ret = web_get_audio_info(input, &sinfo);
	printf_audio_info(&sinfo);
	memcpy(out, &sinfo, sizeof(AudioEncodeParam));
	return ret;
}

int app_web_set_ainfo(int channel, char *in, char *out)
{
	int change = 0;
	int ret = 0;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	AudioEncodeParam *info = NULL;

	AudioEncodeParam *oldinfo = (AudioEncodeParam *)in;
	AudioEncodeParam *newinfo = (AudioEncodeParam *)out;

	channel_get_input_info(channel, &input , &high);

	//web_set_audio_info(input, oldinfo, &change);

	int audio_input =  oldinfo->mp_input;
	PRINTF("input=%d,audio_input=%d\n", input, audio_input);

	mid_mutex_lock(g_audio_control_handle->mutex[input]);
	info = &(g_audio_control_handle->info[input]);
	ret = web_check_audio_param(info, oldinfo, &change);
	mid_mutex_unlock(g_audio_control_handle->mutex[input]);

	if(change != 0) {
		audio_update_config(input, audio_input);
		audio_set_update_flag(audio_input, 1);
		//may be need mid_timer
		write_audio_control_file();
	}

	web_get_audio_info(input, newinfo);

	if(change == 1) {
		MMPAudioParam temp ;
		memset(&temp, 0, sizeof(MMPAudioParam));
		MMP_audio_info_get(input, &temp);

		if(input == SIGNAL_INPUT_2) {
			MMP_config_update2(high, 1 , (unsigned char *)&temp, sizeof(MMPAudioParam));
		} else if(input == SIGNAL_INPUT_1) {
			MMP_config_update1(high, 1 , (unsigned char *)&temp, sizeof(MMPAudioParam));
		} else if(SIGNAL_INPUT_MP == input) {
			MMP_config_update1(high, 1 , (unsigned char *)&temp, sizeof(MMPAudioParam));
			MMP_config_update2(high, 1 , (unsigned char *)&temp, sizeof(MMPAudioParam));
		}
	}

	return 0;
}


int app_audio_get_rate(int channel)
{
	int rate = 0;
	//	int ret = -1;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;

	channel_get_input_info(channel, &input , &high);

	AUDIO_INPUT_CHECK(input);
	AudioEncodeParam *info = NULL;
	mid_mutex_lock(g_audio_control_handle->mutex[input]);
	info = &(g_audio_control_handle->info[input]);
	rate = info->BitRate;
	mid_mutex_unlock(g_audio_control_handle->mutex[input]);
	return rate;
}

#if 1
//------------------middle
#endif


static int read_audio_encode_param(xmlDocPtr pdoc, xmlNodePtr pnode, AudioEncodeParam *paudio)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;
	int				ret = 0;
	int value = 0;

	if(NULL == pdoc || NULL == pnode || NULL == paudio) {
		fprintf(stderr, "read_video_param error, params invalid!\n");
		return -1;
	}

	node = get_children_node(pnode, BAD_CAST "Codec");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			paudio->Codec = ADTS;//atoi(temp);

			PRINTF("\nCodec: %s\n", temp);
		}
	}

	///0---8KHz
	///2---44.1KHz default----44100Hz
	///1---32KHz
	///3---48KHz
	///other-----96KHz
	node = get_children_node(pnode, BAD_CAST "SampleRateIndex");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == 2 || value == 3) {
				paudio->SampleRateIndex = value;
			}

			PRINTF("SampleRateIndex: %d\n", value);
		}
	}

	node = get_children_node(pnode, BAD_CAST "BitRate");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == 96000 || value == MIN_AAC_LC_BITRATE || value == 12800) {
				paudio->BitRate = value;
			}

			PRINTF("BitRate: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "Channel");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);
			paudio->Channel = 2;  //channel only value = 2
			PRINTF("Channel: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "SampleBit");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			//	value = atoi(temp);
			paudio->SampleBit = 16;//value;
			PRINTF("SampleBit: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "LVolume");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value >= 0 && value <= 100) {
				paudio->LVolume = value;
			}

			PRINTF("LVolume: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "RVolume");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value >= 0 && value <= 100) {
				paudio->RVolume = value;
			}

			PRINTF("RVolume: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "InputMode");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == LINE_INPUT || value == MIC_INPUT) {
				paudio->InputMode = atoi(temp);
			}

			PRINTF("InputMode: %s\n", temp);
		}
	}

	// 合成的音频输入  非分成则无需改变
	node = get_children_node(pnode, BAD_CAST "mpInput");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == 1 || value == 2) {
				paudio->mp_input = value;
			}

			PRINTF("mpInput: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "Mute");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, sizeof(temp), pdoc, node);

		if(-1 != ret) {
			value = atoi(temp);

			if(value == 0 || value == 1) {
				paudio->Mute = value;
			}

			PRINTF("Mute: %s\n", temp);
		}
	}


	return 0;
}


static int add_audio_encode_param(xmlNodePtr pnode, AudioEncodeParam *paudio)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->Codec);
	xmlNewTextChild(pnode, NULL, BAD_CAST"Codec", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->SampleRateIndex);
	xmlNewTextChild(pnode, NULL, BAD_CAST"SampleRateIndex", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->BitRate);
	xmlNewTextChild(pnode, NULL, BAD_CAST"BitRate", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->Channel);
	xmlNewTextChild(pnode, NULL, BAD_CAST"Channel", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->SampleBit);
	xmlNewTextChild(pnode, NULL, BAD_CAST"SampleBit", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->LVolume);
	xmlNewTextChild(pnode, NULL, BAD_CAST"LVolume", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->RVolume);
	xmlNewTextChild(pnode, NULL, BAD_CAST"RVolume", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->InputMode);
	xmlNewTextChild(pnode, NULL, BAD_CAST"InputMode", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->mp_input);
	xmlNewTextChild(pnode, NULL, BAD_CAST"mpInput", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->Mute);
	xmlNewTextChild(pnode, NULL, BAD_CAST"Mute", BAD_CAST(temp));
	return 0;
}

static int read_audio_control_file()
{
	char xml_file[256] = AUDIO_CONFIG;
	int index = 0;
	int ret = 0;
	char temp[XML_TEXT_MAX_LENGTH] = {0};

	xmlNodePtr audio_config_node[SIGNAL_INPUT_MAX];


	parse_xml_t px;


	init_file_dom_tree(&px, xml_file);

	if(NULL == px.pdoc || NULL == px.proot) {
		PRINTF("init_file_dom_tree failed, xml file: %s\n", xml_file);
		ret = -1;
		goto cleanup;
	}

	for(index = 0; index < SIGNAL_INPUT_MAX; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "audio_encode_%d", index);
		audio_config_node[index] = get_children_node(px.proot, BAD_CAST(temp));
		PRINTF(" TEMP = %s \n", temp);
		PRINTF(" (audio_config_node[index] = 0x%p \n", audio_config_node[index]);

		if(audio_config_node[index]) {
			mid_mutex_lock(g_audio_control_handle->mutex[index]);
			read_audio_encode_param(px.pdoc, audio_config_node[index], &(g_audio_control_handle->info[index]));
			mid_mutex_unlock(g_audio_control_handle->mutex[index]);
		}

		printf_audio_info(&(g_audio_control_handle->info[index]));
	}


cleanup:
	release_dom_tree(px.pdoc);


	return ret;

}
static int write_audio_control_file()
{
	char xml_file[256] = AUDIO_CONFIG;

	int index = 0;
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlDocPtr doc;
	xmlNodePtr root_node;

	xmlNodePtr audio_config_node[SIGNAL_INPUT_MAX];


	//	pthread_mutex_lock(&ptable->lock);
	PRINTF("7\n");
	doc = xmlNewDoc(BAD_CAST"1.0"); 			//定义文档和节点指针

	if(NULL == doc) {
		PRINTF("xmlNewDoc failed, file: %s\n", xml_file);
		//pthread_mutex_unlock(&ptable->lock);
		return -1;
	}

	PRINTF("6\n");
	root_node = xmlNewNode(NULL, BAD_CAST"audio_control");
	xmlDocSetRootElement(doc, root_node);		//设置根节点
	PRINTF("5\n");

	for(index = 0; index < SIGNAL_INPUT_MAX; index++) {
		PRINTF("index=%d\n", index);
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "audio_encode_%d", index);
		audio_config_node[index] = xmlNewNode(NULL, BAD_CAST(temp));
		xmlAddChild(root_node, audio_config_node[index]);
		mid_mutex_lock(g_audio_control_handle->mutex[index]);
		add_audio_encode_param(audio_config_node[index], &(g_audio_control_handle->info[index]));
		mid_mutex_unlock(g_audio_control_handle->mutex[index]);
	}

	PRINTF("4\n");
	/*保存新的xml文件*/
	int ret = xmlSaveFormatFileEnc(xml_file, doc, "UTF-8", 1);

	if(-1 == ret) {
		PRINTF("xml save params table failed !!!\n");
		ret = -1;
		goto cleanup;
	}

	PRINTF("3\n");
	//释放文档内节点动态申请的内存
cleanup:
	xmlFreeDoc(doc);
	PRINTF("1\n");
	//usleep(100000);
	system("sync");

	//	FILE *fp =NULL;
	//	fp = popen("sync","r");
	//	pclose(fp);

	PRINTF("2\n");
	return ret;

}


static void detele_audio_control_handle(AudioCommonHandle *handle)
{
	int i = 0;

	if(*handle != NULL) {
		for(i = 0; i < SIGNAL_INPUT_MAX; i++) {
			mid_mutex_destroy(&((*handle)->mutex[i]));
		}

		free(*handle);
	}

	*handle = NULL;
	return ;
}

static void create_audio_control_handle()
{
	if(g_audio_control_handle != NULL) {
		ERR_PRN("g_audio_control_handle is already init\n");
		return ;
	}

	int i = 0;
	g_audio_control_handle = (AudioCommonHandle)malloc(sizeof(AudioControlInfo));

	if(g_audio_control_handle == NULL) {
		ERR_PRN("g_audio_control_handle is malloc failed.\n");
		return ;
	}

	memset(g_audio_control_handle, 0, sizeof(AudioControlInfo));

	//init mutex
	for(i = 0; i < SIGNAL_INPUT_MAX; i++) {
		g_audio_control_handle->mutex[i] = mid_mutex_create();

		if(g_audio_control_handle->mutex[i] == NULL) {
			ERR_PRN("create %d mutex  failed\n", i);
			goto CREATE_ERR;
		}

		g_audio_control_handle->info[i].Codec = ADTS;	//"ADTS"
		///0---8KHz
		///2---44.1KHz default----44100Hz
		///1---32KHz
		///3---48KHz
		///other-----96KHz
		g_audio_control_handle->info[i].SampleRateIndex = 3; //3;	//"H264"
		g_audio_control_handle->info[i].BitRate = 128000;	//"音频码率"
		g_audio_control_handle->info[i].Channel = 2 ;	//"2"
		g_audio_control_handle->info[i].SampleBit = 16;	//sample bit(default 16)
		g_audio_control_handle->info[i].LVolume = 23;	//
		g_audio_control_handle->info[i].RVolume = 23;	//
		g_audio_control_handle->info[i].InputMode = LINE_INPUT;	// mic,linein

		g_audio_control_handle->info[i].mp_input = i + 1;

		if(i == SIGNAL_INPUT_MP) {
			g_audio_control_handle->info[i].mp_input = 1;    //mp
		}

		g_audio_control_handle->info[i].Mute = 0;

	}

	return ;
CREATE_ERR:
	detele_audio_control_handle(&g_audio_control_handle);
	return ;


}


int app_init_audio_control(int mp_status)
{
	int i = 0;
	AudioEncodeParam info;
	create_audio_control_handle();
	read_audio_control_file();

	for(i = SIGNAL_INPUT_1 ; i < SIGNAL_INPUT_MAX; i++) {
		web_get_audio_info(i, &info);
		set_mute_status(info.mp_input, info.Mute);
		set_mp_mute_status(info.mp_input, info.Mute, mp_status);
		mp_set_audio_input(info.mp_input, mp_status);
		printf_audio_info(&info);
	}

	//init handle
	//init mutex
	//read from file
	return 0;
}
/*
int audio_lvalue(int audio_input)
{
	adjust_volume(num, cinfo.lvolume, cinfo.rvolume);
	set_mute_status(num, cinfo.is_mute);
}
*/
//初始化之后，才能设置
void app_audio_control_set()
{
	//set audio info
	int input = 0;
	int audio_input = AUDIO_LINEIN_1;
	int mp_status = get_mp_status();

	if(IS_MP_STATUS == mp_status) {
		input = SIGNAL_INPUT_MP;
		input_get_audio_input(input, &audio_input);
		audio_update_config(input, audio_input);
		audio_set_update_flag(AUDIO_LINEIN_1, 1);
		audio_set_update_flag(AUDIO_LINEIN_2, 1);
	} else {
		input = SIGNAL_INPUT_1;
		audio_input = AUDIO_LINEIN_1;
		audio_update_config(input, audio_input);
		audio_set_update_flag(audio_input, 1);

		input = SIGNAL_INPUT_2;
		audio_input = AUDIO_LINEIN_2;
		audio_update_config(input, audio_input);
		audio_set_update_flag(audio_input, 1);
	}

}

