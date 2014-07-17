#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "xml_base.h"
#include "process_xml_cmd.h"
#include "input_to_channel.h"
#include "middle_control.h"
#include "MMP_control.h"
#include "sysparams.h"
#include "app_signal.h"
#include "log.h"


#ifdef SUPPORT_XML_PROTOCOL
#ifndef XML_CHECK_INPUT
#define XML_CHECK_INPUT(index) {PRINTF("--input=%d--\n",index);\
		if(index < 0 || index > SIGNAL_INPUT_MP){\
			ERR_PRN("input =%d\n",index);\
			return -1;}\
	}
#endif


extern REMOTE_INFO   g_remote_info[2];

//static unsigned char		gszSendBuf[MAX_PACKET] = {0};
//static unsigned int		gnSentCount  = 0;

extern int MMP_get_sysparam(int input, MMPSysParam *info, int mmp);

static int GetMuteStatus(int index);

/*未实现函数接口*/
int get_system_info(int index, SYSTEMINFO *info)
{
	XML_CHECK_INPUT(index)
	MMPSysParam sys;
	int mmp = 1;
	char seriano[32] = {0};

	if(index == SIGNAL_INPUT_1) {
		mmp = 1;
	} else if(index == SIGNAL_INPUT_2) {
		mmp = 2;
	}

	MMP_get_sysparam(index, &sys, mmp);

	app_get_serialno(seriano);

	strcpy(info->deviceType, "ENC1200");
	memcpy(info->strName, seriano, strlen(seriano));
	memcpy(info->szMacAddr, sys.szMacAddr, sizeof(sys.szMacAddr));
	info->dwAddr = sys.dwAddr;
	info->dwGateWay = sys.dwGateWay;
	info->dwNetMark = sys.dwNetMask;


	snprintf(info->strVer, sizeof(info->strVer), "%s", sys.strVer);
	info->channelNum = sys.unChannel;
	info->encodeType = 0;
	return 0;
}

int get_sys_remote_info(int index, REMOTE_INFO *info)
{
	/*
		XML_CHECK_INPUT(index)
		int protocol=0;
		int channel= input_get_high_channel( index);
		app_web_get_remote_protole(channel,&protocol);
		//info->type = protocol;
		*/
	info->type = g_remote_info[index].type;
	info->speed = g_remote_info[index].speed;
	info->caddr = g_remote_info[index].caddr;
	info->prezition = g_remote_info[index].prezition;
	info->reservation = g_remote_info[index].reservation;
	PRINTF("WARNING:This function is NULL\n");
	return 0;
}


static int Request_Rate(int index, MMPLowVideoParam *low_video_info, int len, int nPos)
{
	int rate = 0;
	int scbr = 0;
	app_video_get_rate(index, &rate, &scbr);

	PRINTF("rate=%d\n", rate);
	return rate;

}

//low
static int get_Bitrate(int input)
{
	XML_CHECK_INPUT(input);
	MMPVideoParam video;
	int bitrate = 0;
	int channel = 0;

	if(input == SIGNAL_INPUT_1) {
		channel = CHANNEL_INPUT_1_LOW;
	} else if(input == SIGNAL_INPUT_2) {
		channel = CHANNEL_INPUT_2_LOW;
	}

	MMP_video_info_get(channel, &video);
	bitrate = video.sBitrate;
	return bitrate;
}

static int get_Frame(int input)
{
	XML_CHECK_INPUT(input);
	MMPVideoParam video;
	int famre = 0;
	int channel = 0;

	if(input == SIGNAL_INPUT_1) {
		channel = CHANNEL_INPUT_1_LOW;
	} else if(input == SIGNAL_INPUT_2) {
		channel = CHANNEL_INPUT_2_LOW;
	}

	MMP_video_info_get(channel, &video);
	famre = video.nFrameRate;
	return 0;
}
static int SetEncodeVideoParams(int index, MMPVideoParam *vparam, int flag)
{
	XML_CHECK_INPUT(index);
	int channel = input_get_high_channel(index);
	MMP_video_info_set(channel, vparam, &flag);
	return 0;
}

/*Force I frame */
static int ForceIframe(int index, int start)
{
	XML_CHECK_INPUT(index);
	int channel = input_get_high_channel(index);
	app_video_request_iframe(channel);
	return 0;
}


static int process_low_quality(int index, MMPLowVideoParam *low)
{
	PRINTF("WARNING:This function is NULL\n");
	return 0;
}

static  void get_VideoParam(int index, MMPVideoParam *video)
{
	int channel = input_get_high_channel(index);
	MMP_video_info_get(channel, video);
}

static int SaveParamsToFlash(int index)
{
	PRINTF("WARNING:This function is NULL\n");
	return 0;
}

//no need
static int SaveLowRateParamsToFlash(int index)
{
	PRINTF("WARNING:This function is NULL\n");
	return 0;
}

static int get_AudioEncodeParam(int index, AudioEncodeParam  *audio)
{
	XML_CHECK_INPUT(index);
	int channel = input_get_high_channel(index);
	app_web_get_ainfo(channel, audio);

	return 0;
}

static int SetEncodeAudioParams(int index, AudioEncodeParam *pnew_param)
{
	int ret = 0;
	int channel = 0;
	AudioEncodeParam out;
	XML_CHECK_INPUT(index);
	channel = input_get_high_channel(index);
	app_web_set_ainfo(channel, pnew_param, &out);
}

static int get_mode_flag(int index, int flag)
{
	XML_CHECK_INPUT(index);
	Signal_Info info;
	app_get_signal_info(index, &info);

	if(info.ModeID > 0) {
		return 1;
	}

	return 0;
}

static int get_audio_mute_flag(int index)
{

	return GetMuteStatus(index);
}

void GetDeviceType(char *dtype)
{
	MMPSysParam info;
	MMP_get_sysparam(SIGNAL_INPUT_1, &info, 1);
	memcpy(dtype, info.strName, 16);
	PRINTF("%s\n", dtype);
}

int process_get_room_info_cmd(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	XML_CHECK_INPUT(index);
	int sockfd  = GETSOCK_NEW(index, pos);
	AUDIO_PARAM audio;
	RATE_INFO high_rate;
	RATE_INFO low_rate;
	int room_id = *roomid;
	AudioEncodeParam ainfo;
	MMPVideoParam vinfo;
	int channel1 = 0, channel2 = 0;
	XML_CHECK_INPUT(index);
	MMPVideoParam video1;
	MMPVideoParam video2;

	int channel = input_get_high_channel(index);
	app_web_get_ainfo(channel, &ainfo);

	if(index == SIGNAL_INPUT_1) {
		channel1 = CHANNEL_INPUT_1_LOW;
		channel2 = CHANNEL_INPUT_1;
	} else if(index == SIGNAL_INPUT_2) {
		channel1 = CHANNEL_INPUT_2_LOW;
		channel2 = CHANNEL_INPUT_2;
	}

	MMP_video_info_get(channel1, &video1);
	MMP_video_info_get(channel2, &video2);
	memset(&audio, 0, sizeof(AUDIO_PARAM));
	memset(&high_rate, 0, sizeof(RATE_INFO));
	memset(&low_rate, 0, sizeof(RATE_INFO));
	memset(send_buf, 0, sizeof(send_buf));

	audio.InputMode = ainfo.InputMode;
	audio.SampleRate = ainfo.SampleBit;
	audio.BitRate = ainfo.BitRate;
	audio.LVolume = ainfo.LVolume;
	audio.RVolume = ainfo.RVolume;


	high_rate.rateType = 0;
	high_rate.nWidth = video2.nWidth;
	high_rate.nHeight = video2.nHight;
	high_rate.nFrame = video2.nFrameRate;
	high_rate.nBitrate = video2.sBitrate;

	low_rate.rateType = 1;
	low_rate.nWidth = video1.nWidth;
	low_rate.nHeight = video1.nHight;
	low_rate.nFrame = video1.nFrameRate;
	low_rate.nBitrate = video1.sBitrate;

	package_room_info_msg(send_buf, &audio, &high_rate, &low_rate, msgCode, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


static int get_volume_value(int index)
{
	XML_CHECK_INPUT(index);
	int value = 0;
	AudioEncodeParam  audio;
	get_AudioEncodeParam(index, &audio);
	value = (audio.LVolume + audio.RVolume) / 2;
	return 0;
}

/*调节边框*/
static int revise_picture(int index, short h, short v)
{
	XML_CHECK_INPUT(index);

	int ret = app_set_hv(index, h, v, 1);
	return ret;
}
/*green adjust*/
static int GreenScreenAjust(int index)
{
	XML_CHECK_INPUT(index);
	int h = 1;
	int ret = app_set_hv(index, h, 0, 1);
	return ret;
}

static int GetMuteStatus(int index)
{
	XML_CHECK_INPUT(index);
	int audio_input = AUDIO_LINEIN_1;
	input_get_audio_input(index, &audio_input);
	return get_mute_status(audio_input);
}
/////////////////////////////////////////////////////////////////////////
#ifdef SUPPORT_IP_MATRIX

#define  MAX_RESOLUTION_WIDTH				1920
#define  MAX_RESOLUTION_HEIGHT				1080
#define  MAX_VIDEO_BATE						20000
#define  MAX_AUDIO_BATE						128000
#endif

#define ROOM_INFO_XML_NO_USEID   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<PassKey>%s</PassKey>\n<MsgCode>%d</MsgCode>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomInfo>\n<RoomID>%d</RoomID>\n<ConnStatus>%d</ConnStatus>\n<RecStatus>%d</RecStatus>\n<RecName>%s</RecName>\n<AudioInfo>\n<InputMode>%d</InputMode>\n<SampleRate>%d</SampleRate>\n<Bitrate>%d</Bitrate>\n<Lvolume>%d</Lvolume>\n<Rvolume>%d</Rvolume>\n</AudioInfo>\n<EncInfo>\n<ID>%d</ID>\n<EncIP>%s</EncIP>\n<Status>%d</Status>\n<QualityInfo>\n<RateType>%d</RateType>\n<EncBitrate>%d</EncBitrate>\n<EncWidth>%d</EncWidth>\n<EncHeight>%d</EncHeight>\n<EncFrameRate>%d</EncFrameRate>\n</QualityInfo>\n<QualityInfo>\n<RateType>%d</RateType>\n<EncBitrate>%d</EncBitrate>\n<EncWidth>%d</EncWidth>\n<EncHeight>%d</EncHeight>\n<EncFrameRate>%d</EncFrameRate>\n</QualityInfo>\n</EncInfo>\n</RoomInfo>\n</MsgBody>\n</ResponseMsg>"

#define PACK_GET_VOL_XML_NO_USEID "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n<VoiceInfo>\n<Volume>%d</Volume>\n</VoiceInfo>\n</MsgBody>\n</ResponseMsg>"

#define PACK_MUTE_XML_NO_USEID 	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n</ResponseMsg>"

#define PACK_REQUEST_IFRAME_XML_NO_USEID	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n</ResponseMsg>"

#define PACK_PIC_ADJUST_XML_NO_USEID		"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n</ResponseMsg>"

#define PACK_FIX_RESOLUTION_XML_NO_USEID   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RecCtrlResp>\n<RoomID>%d</RoomID>\n<Result>%d</Result>\n</RecCtrlResp>\n</MsgBody>\n</ResponseMsg>"


#define ROOM_INFO_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<PassKey>%s</PassKey>\n<MsgCode>%d</MsgCode>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomInfo>\n<RoomID>%d</RoomID>\n<ConnStatus>%d</ConnStatus>\n<RecStatus>%d</RecStatus>\n<RecName>%s</RecName>\n<AudioInfo>\n<InputMode>%d</InputMode>\n<SampleRate>%d</SampleRate>\n<Bitrate>%d</Bitrate>\n<Lvolume>%d</Lvolume>\n<Rvolume>%d</Rvolume>\n</AudioInfo>\n<EncInfo>\n<ID>%d</ID>\n<EncIP>%s</EncIP>\n<Status>%d</Status>\n<QualityInfo>\n<RateType>%d</RateType>\n<EncBitrate>%d</EncBitrate>\n<EncWidth>%d</EncWidth>\n<EncHeight>%d</EncHeight>\n<EncFrameRate>%d</EncFrameRate>\n</QualityInfo>\n<QualityInfo>\n<RateType>%d</RateType>\n<EncBitrate>%d</EncBitrate>\n<EncWidth>%d</EncWidth>\n<EncHeight>%d</EncHeight>\n<EncFrameRate>%d</EncFrameRate>\n</QualityInfo>\n</EncInfo>\n</RoomInfo>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_GET_VOL_XML "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n<VoiceInfo>\n<Volume>%d</Volume>\n</VoiceInfo>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_MUTE_XML 	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_REQUEST_IFRAME_XML	"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_PIC_ADJUST_XML		"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RoomID>%d</RoomID>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"

#define PACK_REPORT_XML		"<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><RequestMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n</MsgHead>\n<MsgBody>\n<EncInfo>\n<Status>%d</Status>\n<Mute>%d</Mute>\n</EncInfo>\n</MsgBody>\n</RequestMsg>"

#define PACK_FIX_RESOLUTION_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<RecCtrlResp>\n<RoomID>%d</RoomID>\n<Result>%d</Result>\n</RecCtrlResp>\n</MsgBody>\n<UserID>%d</UserID>\n</ResponseMsg>"


#ifdef SUPPORT_IP_MATRIX
#define PACK_ENCODE_ENABLE_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<EnableInfo>\n<MaxWidth>%d</MaxWidth>\n<MaxHeight>%d</MaxHeight>\n<MaxVideoBite>%d</MaxVideoBite>\n<MaxVideoFrame>%d</MaxVideoFrame>\n<MaxAudioBite>%d</MaxAudioBite>\n</EnableInfo>\n</MsgBody>\n</ResponseMsg>"

#define PACK_GET_SYSTEM_XML   "<\?xml version=\"1.0\" encoding=\"UTF-8\"\?><ResponseMsg>\n<MsgHead>\n<MsgCode>%d</MsgCode>\n<PassKey>%s</PassKey>\n<ReturnCode>%d</ReturnCode>\n</MsgHead>\n<MsgBody>\n<Name>%s</Name>\n<SerialNum>%s</SerialNum>\n<MacAddr>%s</MacAddr>\n<IPAddr>%s</IPAddr>\n<GateWay>%s</GateWay>\n<NetMask>%s</NetMask>\n<DeviceVersion>%s</DeviceVersion>\n<VideoRate >%d</VideoRate >\n<DeviceType >%d</DeviceType >\n</MsgBody>\n</ResponseMsg>"
#endif


static int g_audio_param_change[2] = {0};
static int g_video_param_change[2] = {0};

static pthread_mutex_t   g_video_param_mutex[2];
static pthread_mutex_t   g_audio_param_mutex[2];

#define RESP_ROOT_KEY (BAD_CAST "ResponseMsg")
#define REQ_ROOT_KEY (BAD_CAST "RequestMsg")


int init_av_change_param_mutex(int index)
{
	pthread_mutex_init(&g_audio_param_mutex[index], NULL);
	pthread_mutex_init(&g_video_param_mutex[index], NULL);
	return 0;
}

void set_audio_param_change_flag(int index, int flag)
{
	pthread_mutex_lock(&g_audio_param_mutex[index]);
	g_audio_param_change[index] = flag;
	pthread_mutex_unlock(&g_audio_param_mutex[index]);
}

int get_audio_param_change_flag(int index)
{
	int flag  =   -1;
	pthread_mutex_lock(&g_audio_param_mutex[index]);
	flag  = g_audio_param_change[index];
	pthread_mutex_unlock(&g_audio_param_mutex[index]);
	return flag;
}

void set_video_param_change_flag(int index, int flag)
{
	pthread_mutex_lock(&g_video_param_mutex[index]);
	g_video_param_change[index] = flag;
	pthread_mutex_unlock(&g_video_param_mutex[index]);
}

int get_video_param_change_flag(int index)
{
	int flag  =   -1;
	pthread_mutex_lock(&g_video_param_mutex[index]);
	flag  = g_video_param_change[index];
	pthread_mutex_unlock(&g_video_param_mutex[index]);
	return flag;
}


static  int  g_low_rate_flag[2] = {0};

void set_low_rate_flag(int index, int flag)
{
	g_low_rate_flag[index] = flag;
}

int get_low_rate_flag(int index)
{
	return g_low_rate_flag[index];
}

int package_login_add_xml_leaf(char *login_val, xmlNodePtr child_node, xmlNodePtr far_node, char *key_name)
{
	char tmpbuf[64] = {0};
	memset(tmpbuf, 0, sizeof(tmpbuf));
	strcpy(tmpbuf, login_val);
	package_add_xml_leaf(child_node, far_node, (char *)key_name, (char *)tmpbuf);
	return 0;
}
int package_login_response_msg(int index, char *send_buf, int msg_code_val, char *passkey, char *return_val, char *user_id)
{

	char msgCode[8] = {0};
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ResponseMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr code_node 			= NULL;
	xmlNodePtr return_node 			= NULL;
	xmlNodePtr name_node 			= NULL;
	xmlNodePtr serialnum_node 			= NULL;
	xmlNodePtr mac_node 			= NULL;
	xmlNodePtr ip_node 			= NULL;
	xmlNodePtr gateway_node 			= NULL;
	xmlNodePtr netmask_node 			= NULL;
	xmlNodePtr device_ver_node 			= NULL;
	xmlNodePtr channel_node 			= NULL;
	xmlNodePtr ppt_index_node 			= NULL;
	xmlNodePtr lowrate_node 			= NULL;
	xmlNodePtr encodetype_node 			= NULL;
	xmlNodePtr passekey_node 			= NULL;
#ifdef SUPPORT_IP_MATRIX
	xmlNodePtr videorate_node 			= NULL;
	xmlNodePtr devicetype_node 			= NULL;
#endif
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	char tmpbuf[64] = {0};
	char *src = NULL;
	struct in_addr ipaddr;
	SYSTEMINFO   *info = malloc(sizeof(SYSTEMINFO));
	memset(info, 0, sizeof(SYSTEMINFO));
	get_system_info(index, info);
	sprintf(msgCode, "%d", msg_code_val);
	package_add_xml_leaf(code_node, head_node, (char *)"MsgCode", (char *)msgCode);
	package_add_xml_leaf(passekey_node, head_node, (char *)"PassKey", (char *)passkey);
	package_add_xml_leaf(return_node, head_node, (char *)"ReturnCode", (char *)return_val);
	body_node = xmlNewNode(NULL, BAD_CAST "MsgBody");
	xmlAddChild(root_node, body_node);
	package_add_xml_leaf(name_node, body_node, (char *)"Name", (char *)info->deviceType);
	PRINTF("info->strName=[%s]\n", info->strName);
	package_login_add_xml_leaf(info->strName, serialnum_node, body_node, (char *)"SerialNum");
	src = info->szMacAddr;
	sprintf(tmpbuf, "%02x:%02x:%02x:%02x:%02x:%02x", src[0],
	        src[1], src[2], src[3], src[4], src[5]);
	PRINTF("mac addr  = %s \n", tmpbuf);
	package_login_add_xml_leaf(tmpbuf, mac_node, body_node, (char *)"MacAddr");
	memcpy(&ipaddr, &(info->dwAddr), 4);
	package_login_add_xml_leaf(inet_ntoa(ipaddr), ip_node, body_node, (char *)"IPAddr");
	memcpy(&ipaddr, &(info->dwGateWay), 4);
	package_login_add_xml_leaf(inet_ntoa(ipaddr), gateway_node, body_node, (char *)"GateWay");
	memcpy(&ipaddr, &(info->dwNetMark), 4);
	package_login_add_xml_leaf(inet_ntoa(ipaddr), netmask_node, body_node, (char *)"NetMask");
	package_login_add_xml_leaf(info->strVer, device_ver_node, body_node, (char *)"DeviceVersion");
	sprintf(tmpbuf, "%d", info->channelNum);
	package_login_add_xml_leaf(tmpbuf, channel_node, body_node, (char *)"ChannelNum");
	sprintf(tmpbuf, "%d", info->pptIndex);
	package_login_add_xml_leaf(tmpbuf, ppt_index_node, body_node, (char *)"PPTIndex");
	sprintf(tmpbuf, "%d", info->lowRate);
	package_login_add_xml_leaf(tmpbuf, lowrate_node, body_node, (char *)"LowRate");
	sprintf(tmpbuf, "%d", info->encodeType);
	package_login_add_xml_leaf(tmpbuf, encodetype_node, body_node, (char *)"EncodeType");
#ifdef SUPPORT_IP_MATRIX
	sprintf(tmpbuf, "%d", 20000);
	package_login_add_xml_leaf(tmpbuf, videorate_node, body_node, (char *)"VideoRate");
	memset(tmpbuf, 0, 64);
	sprintf(tmpbuf, "%d", 101);
	package_login_add_xml_leaf(tmpbuf, devicetype_node, body_node, (char *)"DeviceType");
#endif


	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	printf("%s\n", (char *)temp_xml_buf);
	memcpy(send_buf, temp_xml_buf, size);
	//printf("%s\n",send_buf);
	xmlFree(temp_xml_buf);
	free(info);
	release_dom_tree(doc);
	return 0;
}

int package_room_info_msg(char *send_buf, AUDIO_PARAM *audio, RATE_INFO *high_rate, RATE_INFO *low_rate, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, ROOM_INFO_XML_NO_USEID, passkey, msgcode, retcode, roomid, 1, 1, "xxx", audio->InputMode, audio->SampleRate,
		        audio->BitRate, audio->LVolume, audio->RVolume, 1, "x.x.x.x", 1, high_rate->rateType, high_rate->nBitrate,
		        high_rate->nWidth, high_rate->nHeight, high_rate->nFrame, low_rate->rateType, low_rate->nBitrate,
		        low_rate->nWidth, low_rate->nHeight, low_rate->nFrame);
	} else {
		sprintf(send_buf, ROOM_INFO_XML, passkey, msgcode, retcode, roomid, 1, 1, "xxx", audio->InputMode, audio->SampleRate,
		        audio->BitRate, audio->LVolume, audio->RVolume, 1, "x.x.x.x", 1, high_rate->rateType, high_rate->nBitrate,
		        high_rate->nWidth, high_rate->nHeight, high_rate->nFrame, low_rate->rateType, low_rate->nBitrate,
		        low_rate->nWidth, low_rate->nHeight, low_rate->nFrame, atoi(user_id));
	}

	return 0;
}
int package_get_volume(char *send_buf, int msgcode, char *passkey, int retcode, int vol, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_GET_VOL_XML_NO_USEID, msgcode, passkey, retcode, roomid, vol);
	} else {
		sprintf(send_buf, PACK_GET_VOL_XML, msgcode, passkey, retcode, roomid, vol, atoi(user_id));
	}

	return 0;
}
int package_xml_mute_msg(char *send_buf, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_MUTE_XML_NO_USEID, msgcode, passkey, retcode, roomid);
	} else {
		sprintf(send_buf, PACK_MUTE_XML, msgcode, passkey, retcode, roomid, atoi(user_id));
	}

	return 0;
}
int package_xml_request_I_frame_msg(char *send_buf, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_REQUEST_IFRAME_XML_NO_USEID, msgcode, passkey, retcode, roomid);
	} else {
		sprintf(send_buf, PACK_REQUEST_IFRAME_XML, msgcode, passkey, retcode, roomid, atoi(user_id));
	}

	return 0;
}
int package_xml_pic_adjust_msg(char *send_buf, int msgcode, char *passkey, int retcode, char *user_id, int roomid)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_PIC_ADJUST_XML_NO_USEID, msgcode, passkey, retcode, roomid);
	} else {
		sprintf(send_buf, PACK_PIC_ADJUST_XML , msgcode, passkey, retcode, roomid, atoi(user_id));
	}

	return 0;
}


int package_xml_report_msg(char *send_buf, int msgcode, char *passkey, int status, int mute, char *user_id)
{
	sprintf(send_buf, PACK_REPORT_XML , msgcode, passkey, status, mute);
	return 0;
}

int package_xml_fix_resolution_msg(char *send_buf, int msgcode, char *passkey, int retcode, int roomid, int result, char *user_id)
{
	if(atoi(user_id) == -1) {
		sprintf(send_buf, PACK_FIX_RESOLUTION_XML_NO_USEID, msgcode, passkey, retcode, roomid, result);
	} else {
		sprintf(send_buf, PACK_FIX_RESOLUTION_XML , msgcode, passkey, retcode, roomid, result, atoi(user_id));
	}

	return 0;
}

#ifdef SUPPORT_IP_MATRIX
int package_xml_encode_enable_msg(char *send_buf, int msgcode, char *passkey, int retcode, int roomid, char *user_id)
{
	int max_frame = 60;
	sprintf(send_buf, PACK_ENCODE_ENABLE_XML, msgcode, passkey, retcode, MAX_RESOLUTION_WIDTH, MAX_RESOLUTION_HEIGHT, MAX_VIDEO_BATE, max_frame, MAX_AUDIO_BATE);

	return 0;
}


int package_xml_get_system_msg(int index, char *send_buf, int msgcode, char *passkey, int retcode)
{
	SYSTEMINFO   *info = malloc(sizeof(SYSTEMINFO));
	char tmpbuf[64] = {0};
	char *src = NULL;
	struct in_addr ipaddr, gateway, netmask;
	get_system_info(index, info);
	src = info->szMacAddr;
	sprintf(tmpbuf, "%02x:%02x:%02x:%02x:%02x:%02x", src[0],
	        src[1], src[2], src[3], src[4], src[5]);
	PRINTF("mac addr  = %s \n", tmpbuf);
	memcpy(&ipaddr, &(info->dwAddr), 4);
	memcpy(&gateway, &(info->dwGateWay), 4);
	memcpy(&netmask, &(info->dwNetMark), 4);
	sprintf(send_buf, PACK_GET_SYSTEM_XML, msgcode, passkey, retcode, info->deviceType, info->strName, tmpbuf, inet_ntoa(ipaddr), inet_ntoa(gateway), inet_ntoa(netmask),
	        info->strVer, 20000, 101);

	PRINTF("send_xml=\n%s\n\n", send_buf);
	free(info);
	return 0;
}

#endif


int process_xml_login_msg(int input, int pos, char *send_buf, user_login_info *login_info, int msgCode, char *passkey, char *user_id)
{

	int sockfd = GETSOCK_NEW(input, pos);
	memset(send_buf, 0, sizeof(send_buf));

	if((strcmp(login_info->username, "admin") == 0) &&
	   (strcmp(login_info->password, "123") == 0)) {
		PRINTF("user admin login success\n");
	} else {
		PRINTF("user admin login failed\n");
		package_head_msg(send_buf, msgCode, passkey, "0", user_id);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send login err failed\n");
			return -1;
		}

		return -1;
	}

	package_login_response_msg(input, send_buf, msgCode, passkey, "1", user_id);
	PRINTF("send before\n");

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	PRINTF("send after\n");
	SETCLIUSED_NEW(input, pos, TRUE);
	SETCLILOGIN_NEW(input, pos, TRUE);
	PRINTF("index:[%d] used=%d,login=%d,flag=%d\n",
	       input,
	       ISUSED_NEW(input, pos),
	       ISLOGIN_NEW(input, pos),
	       GETLOWRATEFLAG_NEW(input, pos));

	return 0;
}

int process_xml_request_rate_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd  = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	PRINTF("index:[%d] pos=%d,enter request rate cmd\n", index, pos);
	SET_SEND_AUDIO_NEW(index, pos, TRUE);
	SET_SEND_VIDEO_NEW(index, pos, TRUE);
#if 1
	MMPLowVideoParam param;
	int     length = 0;
	int 	quality = 0;
	memset(&param, 0, sizeof(MMPLowVideoParam));
	param.nWidth = 352;
	param.nHeight = 288;
	param.nBitrate = get_Bitrate(index);
	param.nFrame = get_Frame(index);
	length = sizeof(MMPLowVideoParam) ;
	quality = GET_PARSE_LOW_RATE_FLAG(index, pos);
	PRINTF("index:[%d] pos=%d,quality=%d,bitrate=%d,frame=%d\n", index, pos, quality, param.nBitrate, param.nFrame);

	if(Request_Rate(index, &param, length, pos) < 0) {
		package_head_msg(send_buf, msgCode, passkey, "0", user_id);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}
	}

#endif

	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}




int process_xml_remote_ctrl_msg(int index, int pos, char *send_buf, REMOTE_INFO *far_ctrl, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	PRINTF("index:[%d], pos=%d,type=%d,speed=%d,caddr=%d,prezition=%d\n",
	       index, pos, far_ctrl->type, far_ctrl->speed, far_ctrl->caddr, far_ctrl->prezition);
	remote_ctrl_camera(index, &far_ctrl[index]);
	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int beyon_high_quality_param(int index, RATE_INFO *high_rate, MMPVideoParam *video_param, int *ret_val)
{
	if((high_rate->nBitrate >= 128 && high_rate->nBitrate <= 20000) && (high_rate->nBitrate != video_param->sBitrate)) {
		video_param->sBitrate = high_rate->nBitrate;
		*ret_val = 2;
	}

	if((high_rate->nFrame >= 0 && high_rate->nFrame <= 60) && (high_rate->nFrame != video_param->nFrameRate)) {
		video_param->nFrameRate = high_rate->nFrame;
		*ret_val = 1;
	}

	return 0;
}
static int process_high_quality(int index, RATE_INFO *rate_info, MMPVideoParam *Oldp)
{
	int save = 0;
	PRINTF("old param!! frame=%d,bitrate=%d,width=%d,height=%d\n"
	       , Oldp->nFrameRate, Oldp->sBitrate, Oldp->nWidth, Oldp->nHight);
	beyon_high_quality_param(index, rate_info, Oldp, &save);

	if(25 != Oldp->nFrameRate  ) {
		Oldp->nFrameRate = 25;
	}

	if(save) {
		set_video_param_change_flag(index, TRUE);

		if(save == 1) {
			SetEncodeVideoParams(index, Oldp, 1);

		} else if(save == 2) {
			SetEncodeVideoParams(index, Oldp, 2);
			ForceIframe(index, 0);//
		}
	}

	return 0;
}
/*检查参数合法性*/
static int CheckLowRateParam(const MMPLowVideoParam *pp)
{
	if(pp->nHeight > LOWRATE_MAX_HEIGHT || pp->nWidth > LOWRATE_MAX_WIDTH
	   || pp->nHeight < LOWRATE_MIN_HEIGHT || pp->nWidth < LOWRATE_MIN_WIDTH) {
		ERR_PRN("CheckLowRateParam() ERROR width = %d height = %d\n", pp->nWidth, pp->nHeight);
		return 0;
	}

	if(pp->nBitrate < 128 || pp->nBitrate > 4096) {
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

int process_set_quality_cmd(int index, int pos, char *send_buf, RATE_INFO *rate_info, int msgCode, char *passkey, char *user_id, int *roomid)
{
	RATE_INFO *high_rate = NULL;
	RATE_INFO *low_rate = NULL;

	int rate_info_num = 0;
	int high_rate_flag = 0;
	int low_rate_flag = 0;
	MMPLowVideoParam low;
	int sockfd = GETSOCK_NEW(index, pos);

	high_rate = &rate_info[0];
	low_rate = &rate_info[1];

#ifdef SUPPORT_IP_MATRIX

	if(get_video_lock_resolution(index) > 0) {
		memset(send_buf, 0, sizeof(send_buf));
		package_head_msg(send_buf, msgCode, passkey, "1", user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}

		return 0;
	}

#endif
	MMPVideoParam Oldp;
	get_VideoParam(index, &Oldp);
	int ret = 0;


	//PRINTF("num =%d,ratetype=%d,width=%d,height=%d,frame=%d,bitrate=%d\n",high_rate->rateInfoNum,
	//	high_rate->rateType,high_rate->nWidth,high_rate->nHeight,high_rate->nFrame,high_rate->nBitrate);
	if(high_rate->rateInfoNum == 0) {
		memset(send_buf, 0, sizeof(send_buf));
		package_head_msg(send_buf, msgCode, passkey, "0", user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}

		return -1;
	}

	rate_info_num = high_rate->rateInfoNum;
	memset(&low, 0, sizeof(MMPLowVideoParam));

	if(rate_info_num == 1) {
		high_rate_flag = high_rate->rateType;
		PRINTF("num =%d,ratetype=%d,width=%d,height=%d,frame=%d,bitrate=%d\n", high_rate->rateInfoNum,
		       high_rate->rateType, high_rate->nWidth, high_rate->nHeight, high_rate->nFrame, high_rate->nBitrate);

		if(high_rate_flag == 0) {
			process_high_quality(index, high_rate, &Oldp);
			SaveParamsToFlash(index);
		} else if(high_rate_flag == 1) {
			memcpy(&low, &(high_rate->nWidth), (sizeof(RATE_INFO) - 8));
			PRINTF("low info ----nBitrate:%d,nFrame:%d,nHeight:%d,nWidth:%d------\n", low.nBitrate, low.nFrame, low.nHeight, low.nWidth);
			ret = CheckLowRateParam(&low);

			if(0 == ret) {
				PRINTF("set low param  error\n");
				memset(send_buf, 0, sizeof(send_buf));
				package_head_msg(send_buf, msgCode, passkey, "0", user_id);
				PRINTF("%s\n", send_buf);

				if(tcp_send_data(sockfd, send_buf) < 0) {
					PRINTF("send tcp data failed\n");
					return -1;
				}

				return -1;
			}

			process_low_quality(index, &low);
			SaveLowRateParamsToFlash(index);
		}

	} else if(rate_info_num == 2) {
		high_rate_flag = high_rate->rateType;
		low_rate_flag = low_rate->rateType;

		if((high_rate_flag  == 0)) {
			process_high_quality(index, high_rate, &Oldp);
			SaveParamsToFlash(index);
		}

		if((low_rate_flag == 1)) {
			memcpy(&low, &(low_rate->nWidth), sizeof(RATE_INFO) - 8);
			ret = CheckLowRateParam(&low);

			if(0 == ret) {
				PRINTF("set low param  error\n");
				memset(send_buf, 0, sizeof(send_buf));
				package_head_msg(send_buf, msgCode, passkey, "0", user_id);
				PRINTF("%s\n", send_buf);

				if(tcp_send_data(sockfd, send_buf) < 0) {
					PRINTF("send tcp data failed\n");
					return -1;
				}

				return -1;
			}

			PRINTF("low info ----nBitrate:%d,nFrame:%d,nHeight:%d,nWidth:%d------\n", low.nBitrate, low.nFrame, low.nHeight, low.nWidth);
			process_low_quality(index, &low);
			SaveLowRateParamsToFlash(index);
		}
	}

	{

		//		PRINTF("deal set quality info cmd len=%d\n",sizeof(send_buf));
		memset(send_buf, 0, sizeof(send_buf));
		package_head_msg(send_buf, msgCode, passkey, "1", user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}
	}

	return 0;
}

/*beyond audio param*/
static int beyond_audio_param(AUDIO_PARAM *newinfo, AudioEncodeParam *oldinfo)
{

	if(oldinfo == NULL || newinfo == NULL) {
		ERR_PRN("NULL ptr\n");
		return -1;
	}

	int mmp_change = 0;


	return (mmp_change);
}

int process_set_audio_cmd(int index, int pos, char *send_buf, AUDIO_PARAM *audio_info, int msgCode, char *passkey, char *user_id, int *roomid)
{
	AudioEncodeParam  Oldp;
	AUDIO_PARAM Newp;
	int ret = 0;
	int sockfd = GETSOCK_NEW(index, pos);
	int len = sizeof(AUDIO_PARAM);
#ifdef SUPPORT_IP_MATRIX

	if((get_fix_resolution() != 0) || get_audio_lock_resolution(index) > 0) {
		package_head_msg(send_buf, msgCode, passkey, "1", user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}

		return 0;
	}

#elif SUPPORT_XML_PROTOCOL

	if((get_fix_resolution() != 0)) {
		package_head_msg(send_buf, msgCode, passkey, "1", user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}

		return 0;
	}

#endif
	memcpy(&Newp, audio_info, len);
	get_AudioEncodeParam(index, &Oldp);

	//PRINTF("Newp->InputMode=%d,Newp->BitRate=%d,Newp->SampleRate=%d,Newp->LVolume=%d,Newp->RVolume=%d\n",
	//	Newp.InputMode,Newp.BitRate,Newp.SampleRate,Newp.LVolume,Newp.RVolume);
	if((Newp.LVolume != Oldp.LVolume) || (Newp.RVolume != Oldp.RVolume)) {
		Oldp.LVolume = Newp.LVolume ;
		Oldp.RVolume = Newp.RVolume ;
		PRINTF("AudioParam LVolume change:%d\n", Newp.LVolume);
	}

	ret = beyond_audio_param(&Newp, &Oldp);

	if(ret) {
		set_audio_param_change_flag(index, TRUE);
		SetEncodeAudioParams(index, &Newp);
		SaveParamsToFlash(index);
	}

	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return ret;
}


int process_xml_get_volume_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	//PRINTF("process NEW_MSG_GET_VOLUME msg\n");
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	memset(send_buf, 0, sizeof(send_buf));
	int vol_value = get_volume_value(index);
	package_get_volume(send_buf, msgCode, passkey, 1, vol_value, user_id, room_id);

	//PRINTF("%s\n",send_buf);
	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


int process_mute_cmd(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	int mute_flag = -1;
	int room_id = *roomid;

	if(msgCode == NEW_MSG_MUTE) {
		mute_flag = GetMuteStatus(index);
		mute_flag ^= 1;
	}

	memset(send_buf, 0, sizeof(send_buf));
	package_xml_mute_msg(send_buf, NEW_MSG_MUTE, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


int process_pic_adjust_cmd(int index, int pos, char *send_buf, PICTURE_ADJUST *info, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int             ret = 0;
	int 			sockfd = GETSOCK_NEW(index, pos);
	short           hporch, vporch;
	short 			color_trans = info->color_trans;
	int room_id = *roomid;
	hporch = info->hporch;
	vporch = info->vporch;
	PRINTF("hporch = %d vporch= %d \n", hporch, vporch);
	ret = revise_picture(index, hporch, vporch);

	if(hporch == 0 && vporch == 0 && color_trans != 0) {
		GreenScreenAjust(index);
	}

	memset(send_buf, 0, sizeof(send_buf));
	package_xml_pic_adjust_msg(send_buf, msgCode, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int process_xml_request_IFrame_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	PRINTF("process request_IFrame msg\n");
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	memset(send_buf, 0, sizeof(send_buf));
	ForceIframe(index, GETLOWRATEFLAG_NEW(index, pos));
	package_xml_request_I_frame_msg(send_buf, msgCode, passkey, 1, user_id, room_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int process_fix_resolution_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	int flag = 0;

#ifdef SUPPORT_IP_MATRIX

	if(get_ipmatrix_lock_resolution(index) > 0) {
		package_xml_fix_resolution_msg(send_buf, msgCode, passkey, 0, room_id, GET_FIX_RESOLUTION_FLAG(index, pos), user_id);
		PRINTF("%s\n", send_buf);

		if(tcp_send_data(sockfd, send_buf) < 0) {
			PRINTF("send tcp data failed\n");
			return -1;
		}

		return 0;
	}

#endif

	if(GET_FIX_RESOLUTION_FLAG(index, pos) == MMP_LOCK_SCREEN_IN) {
		XML_recode_lock_set(index, LOCK_SCALE);
	} else {
		XML_recode_lock_set(index, UNLOCK_SCALE);
	}

	memset(send_buf, 0, sizeof(send_buf));
	package_xml_fix_resolution_msg(send_buf, msgCode, passkey, 1, room_id, GET_FIX_RESOLUTION_FLAG(index, pos), user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

#ifdef SUPPORT_IP_MATRIX

static int g_audio_resolution[2] = {0};   //  1   加锁     0   解锁

int get_audio_lock_resolution(int index)
{
	return g_audio_resolution[index];
}

int add_audio_lock_resolution(int index)
{
	g_audio_resolution[index] += 1;
	return g_audio_resolution[index];
}

int cut_audio_lock_resolution(int index)
{
	g_audio_resolution[index] -= 1;

	if(g_audio_resolution[index] <= 0) {
		g_audio_resolution[index] = 0;
	}

	return g_audio_resolution[index];
}

static int g_video_resolution[2] = {0, 0};   //  1  加锁     0  解锁

int get_video_lock_resolution(int index)
{
	return g_video_resolution[index];
}

int add_video_lock_resolution(int index)
{
	g_video_resolution[index] += 1;
	return g_video_resolution[index];
}

int cut_video_lock_resolution(int index)
{
	g_video_resolution[index] -= 1;

	if(g_video_resolution[index] <= 0) {
		g_video_param_change[index] = 0;
	}

	return g_video_resolution[index];
}
static int g_ipmatrix_lock_resolution[2] = {0};   //  1  加锁     0  解锁

int get_ipmatrix_lock_resolution(int index)
{
	return g_ipmatrix_lock_resolution[index];
}

int add_ipmatrix_lock_resolution(int index)
{
	g_ipmatrix_lock_resolution[index] += 1;
	return g_ipmatrix_lock_resolution[index];
}

int cut_ipmatrix_lock_resolution(int index)
{
	g_ipmatrix_lock_resolution[index] -= 1;

	if(g_ipmatrix_lock_resolution[index] <= 0) {
		g_ipmatrix_lock_resolution[index] = 0;
	}

	return g_ipmatrix_lock_resolution[index];
}


process_sys_restart_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	system("sync");
	sleep(4);
	PRINTF("Restart sys\n");
	system("reboot -f");

	return 0;
}

int process_encode_enable_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	int flag = 0;
	memset(send_buf, 0, sizeof(send_buf));
	package_xml_encode_enable_msg(send_buf, msgCode, passkey, 1, room_id, user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}



int process_xml_stop_rate_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid)
{
	int sockfd  = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	PRINTF("index:[%d] pos=%d,enter request rate cmd\n", index, pos);
	SET_SEND_AUDIO_NEW(index, pos, 0);
	SET_SEND_VIDEO_NEW(index, pos, 0);
	SETLOWRATEFLAG_NEW(index, pos, -1);
	package_head_msg(send_buf, msgCode, passkey, "1", user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}

int process_lock_av_param_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id, int *roomid, LOCK_PARAM *lock, int *lock_count)
{
	int sockfd = GETSOCK_NEW(index, pos);
	int room_id = *roomid;
	int lock_num = *lock_count;
	int flag = 0;

	if(lock->audio_lock == 1 && (lock_num == 1)) {
		PRINTF("lock->audio_lock = %d\n", lock->audio_lock);
		add_audio_lock_resolution(index);
	} else if(lock->audio_lock == 0 && (lock_num == 0)) {
		PRINTF("lock->audio_lock = %d\n", lock->audio_lock);
		cut_audio_lock_resolution(index);
	} else {
		PRINTF("input audio_lock param err\n");
	}

	if(lock->video_lock == 0 && (lock_num == 0)) {
		PRINTF("lock->video_lock = %d\n", lock->video_lock);
		cut_video_lock_resolution(index);
	} else if(lock->video_lock == 1 && (lock_num == 1)) {
		PRINTF("lock->video_lock = %d\n", lock->video_lock);
		add_video_lock_resolution(index);
	} else {
		PRINTF("input audio_lock param err\n");
	}

	memset(send_buf, 0, sizeof(send_buf));

	if(lock->resulotion_lock == 0 && (lock_num == 0)) {
		PRINTF("lock->video_lock = %d\n", lock->resulotion_lock);
		cut_ipmatrix_lock_resolution(index);
	} else if(lock->resulotion_lock == 1 && (lock_num == 1)) {
		PRINTF("lock->video_lock = %d\n", lock->resulotion_lock);
		//=====================
		add_ipmatrix_lock_resolution(index);
	} else {
		PRINTF("input resolution_lock param error\n");
	}

	package_xml_fix_resolution_msg(send_buf, msgCode, passkey, 1, room_id, 1, user_id);
	PRINTF("%s\n", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}


/*请求低码流*/

int process_get_system_msg(int index, int pos, char *send_buf, int msgCode, char *passkey, char *user_id)
{

	int sockfd = GETSOCK_NEW(index, pos);
	memset(send_buf, 0, sizeof(send_buf));
	package_xml_get_system_msg(index, send_buf, msgCode, passkey, 1);
	PRINTF("send before\n %s \n ", send_buf);

	if(tcp_send_data(sockfd, send_buf) < 0) {
		PRINTF("send tcp data failed\n");
		return -1;
	}

	return 0;
}



static int quit_status[2] = {1, 1};
static int get_quit_status(int index)
{
	return quit_status[index];
}

void set_quit_status(int index)
{
	quit_status[index] = 0;
}


void *report_pthread_fxn(void *arg)
{
	PRINTF("[report_pthread_fxn] start ...\n");

	if(NULL == arg) {
		return NULL;
	}

	int mode = 0;
	int audio_flag = 1;
	char send_buf[1024] = {0};
	char device[16] = {0};
	int nPos  = 0;
	int sockfd = -1;
	AUDIO_PARAM Aparam;
	RATE_INFO Rinfo;
	char user_id[16] = {0};
	int i = 0;
	int count = 0;
	int flag = -1;
	int index = *(int *)arg;
	GetDeviceType(device);

	while(get_quit_status(index)) {
		mode = get_mode_flag(index, 0);
		audio_flag = get_audio_mute_flag(index);

		for(i = 0 ; i < MAX_CLIENT_NUM ; i++) {
			if(ISLOGIN_NEW(index, i) && ISUSED_NEW(index, i)) {
				sockfd = GETSOCK_NEW(index, i);

				if(sockfd < 0) {
					printf("sockfd = %d\n", GETSOCK_NEW(index, i));
					sleep(3);
					continue;
				}

				if(get_audio_param_change_flag(index) || get_video_param_change_flag(index)) {
					PRINTF("audio or video param changed\n");
					process_get_room_info_cmd(index, i, send_buf, NEW_MSG_GET_ROOM_INFO, device, user_id, &i);
					set_video_param_change_flag(index, FALSE);
					set_audio_param_change_flag(index, FALSE);
					memset(send_buf, 0, sizeof(send_buf));
				}

				package_xml_report_msg(send_buf, NEW_MSG_STATUS_REPORT, device, mode, audio_flag, user_id);
				if(tcp_send_data(sockfd, send_buf) < 0) {
					PRINTF("send tcp data failed, socket:[%d] errno=%d,%s\n", sockfd, errno, strerror(errno));
					//return -1;
				} else {
					count = add_heart_count(index, i);
				}

				PRINTF("input = %d,client [%d] heart send count = %d\n", index, i, count);

				if(count > 3) {
					PRINTF("input  =  %d [%d]client heart maybe stop , close cli [%d]\n", index, i, i);
					{
						writeWatchDog();
						SETCLIUSED_NEW(index, i, FALSE);
						SET_SEND_AUDIO_NEW(index, i, FALSE);
						SETCLILOGIN_NEW(index, i, FALSE);
						SETLOWRATEFLAG_NEW(index, i, 0);
						SET_SEND_VIDEO_NEW(index, i, FALSE);
						SET_PARSE_LOW_RATE_FLAG(index, i, INVALID_SOCKET);
						SET_FIX_RESOLUTION_FLAG(index, i, FALSE);
						set_heart_count(index, i, FALSE);
						set_fix_resolution(FALSE);
						shutdown(sockfd, SHUT_RDWR);
						close(sockfd);
						SETSOCK_NEW(index, i, INVALID_SOCKET);
						writeWatchDog();
					}

				}
			}
		}

		sleep(3);
	}


}


#endif

static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	//	pthread_mutex_lock(&gSetP_m.send_m);
	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			PRINTF("WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, errno, s);

			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr, "network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}

			//			pthread_mutex_unlock(&gSetP_m.send_m);
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


int  SendVideoDataToIpMatrixClient(int index, void *pData, int nLen)
{

	//printf("[SendDataToIpMatrixClient] index:[%d]\n", index);
	int nRet = -1;
	int cnt = 0;
	SOCKET	sendsocket = 0;

	//send multi client
	for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
		if(ISUSED_NEW(index, cnt)
		   && ISLOGIN_NEW(index, cnt)
		   && GET_SEND_VIDEO_NEW(index, cnt)) {
			sendsocket = GETSOCK_NEW(index, cnt);

			if(sendsocket > 0) {
				nRet = WriteData(sendsocket, pData, nLen);

				if(nRet < 0) {
					SETCLIUSED_NEW(index, cnt, FALSE);
					SETCLILOGIN_NEW(index, cnt, FALSE);
					PRINTF("Error: SOCK = %d count = %d  errno[%d]:[%s]  ret = %d\n", sendsocket, cnt, errno, strerror(errno), nRet);
				}
			}
		}
	}

	return 0;
}

int  SendAudioDataToIpMatrixClient(int index, void *pData, int nLen)
{

	//printf("[SendDataToIpMatrixClient] index:[%d]\n", index);
	int nRet = -1;
	int cnt = 0;
	SOCKET	sendsocket = 0;

	//send multi client
	for(cnt = 0; cnt < MAX_CLIENT; cnt++) {
		if(ISUSED_NEW(index, cnt)
		   && ISLOGIN_NEW(index, cnt)
		   && GET_SEND_AUDIO_NEW(index, cnt)) {
			sendsocket = GETSOCK_NEW(index, cnt);

			//	PRINTF(" sendsocket=%d!\n", sendsocket);
			if(sendsocket > 0) {
				nRet = WriteData(sendsocket, pData, nLen);

				if(nRet < 0) {
					SETCLIUSED_NEW(index, cnt, FALSE);
					SETCLILOGIN_NEW(index, cnt, FALSE);
					PRINTF("Error: SOCK = %d count = %d  errno[%d]:[%s]  ret = %d\n", sendsocket, cnt, errno, strerror(errno), nRet);
				}
			}
		}
	}

	return 0;
}


#endif
