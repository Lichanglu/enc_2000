#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "xml/xml_base.h"
#include "reach_enc2000.h"
#include "params.h"
#include "common.h"
#include "encliveplay.h"
#include "capture.h"
#include "log_common.h"
#include "select.h"
#include "rwini.h"




#if 0

params_table *gp_forever_params;
params_table *gp_realtime_params;

extern ENC2000_LINK_STRUCT	gEnc2000;

void printf_params()
{
	static int count = 0;
	PRINTF(" %d paudio = %p, samplebit = %d\n",
	       count++, &(gp_realtime_params->audioPara[0]), (gp_realtime_params->audioPara[0]).SampleBit);
	return ;
}

static void PackHeaderMSG(BYTE *data, BYTE type, WORD len)
{
	MSGHEAD  *p;
	p = (MSGHEAD *)data;
	memset(p, 0, HEAD_LEN);
	p->nLen = htons(len);
	p->nMsg = type;
	return ;
}

static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, s, errno);

			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr, "network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}

			return nRet;
		} else if(nRet == 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, s, errno);
			fprintf(stderr, "Send Net Data Error nRet= %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	return nWriteLen;
}



static void init_system_param(SYSPARAMS *psys)
{
	psys->dwAddr = inet_addr(IP_ADDR_1);
	psys->dwGateWay =  inet_addr("192.168.4.254");
	psys->dwNetMark = inet_addr("255.255.255.0");

	memset(psys->szMacAddr, 0, 8);
	psys->szMacAddr[0] = 0x00;
	psys->szMacAddr[1] = 0x09;
	psys->szMacAddr[2] = 0x32;
	psys->szMacAddr[3] = 0x02;
	psys->szMacAddr[4] = 0x22;
	psys->szMacAddr[5] = 0x00;
	strcpy(psys->strName, "DSS-ENC-MOD");
	strcpy(psys->strVer, "1.0.1");
	psys->unChannel = 2;
	psys->bType = 6;  //0 -------VGABOX  3-------200 4-------110 5-------120 6--------1200  8 --ENC-1100
	bzero(psys->nTemp, sizeof(psys->nTemp));
}

static void init_video_param(VideoParam *pvideo)
{
	bzero(pvideo, sizeof(VideoParam));
	pvideo->nDataCodec = 0x34363248;	//"H264"
	pvideo->nWidth = 1920;			//video width
	pvideo->nHight = 1080;			//video height
	pvideo->nQuality = 45;				//quality (5---90)
	pvideo->sCbr = 1;					// 0---quality  1---bitrate
	pvideo->nFrameRate = 60;			//current framerate
	pvideo->sBitrate = 4096;				//bitrate (128k---4096k)
	pvideo->nColors = 24;			//24
}

static void init_audio_param(AudioParam *paudio)
{
	bzero(paudio, sizeof(AudioParam));
	paudio->Codec = ADTS;		//"ADTS"
	///0---8KHz
	///2---44.1KHz default----44100Hz
	///1---32KHz
	///3---48KHz
	///other-----96KHz
	paudio->SampleRate = 2;
	paudio->Channel = 2;					//audio channel (default 2)
	paudio->SampleBit = 16;				//sample bit(default 16)
	paudio->BitRate = 128000;				//bitrate (default 64000)
	paudio->InputMode = LINE_INPUT;		//input mode (0 ---line 1----mic)
	paudio->RVolume = 23;
	paudio->LVolume = 23;
	PRINTF("paudio =%p,samplebit=%d\n", paudio, paudio->SampleBit);
}

int init_params_table(params_table *ptable)
{
	int index = 0;

	pthread_mutex_init(&ptable->lock, NULL);

	pthread_mutex_lock(&ptable->lock);

	init_system_param(&ptable->sysPara);

	for(index = 0; index < MAX_VIDEO_NUM; index++) {
		init_video_param(&ptable->videoPara[index]);
	}

	PRINTF("\n");

	for(index = 0; index < MAX_AUDIO_NUM; index++) {
		init_audio_param(&ptable->audioPara[index]);
	}

	pthread_mutex_unlock(&ptable->lock);

	return 0;
}

int JoinMacAddr(char *dst, unsigned char *src, int num)
{
	sprintf(dst, "%02x:%02x:%02x:%02x:%02x:%02x", src[0],
	        src[1], src[2], src[3], src[4], src[5]);
	PRINTF("mac addr  = %s \n", dst);

	return 0;
}


static int add_system_param(xmlNodePtr pnode, SYSPARAMS *psys)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	char temp2[XML_TEXT_MAX_LENGTH] = {0};

	struct in_addr		addr;

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	memcpy(&addr, &psys->dwAddr, 4);
	strcpy(temp, inet_ntoa(addr));
	xmlNewTextChild(pnode, NULL, BAD_CAST"dwAddr", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	memcpy(&addr, &psys->dwGateWay, 4);
	strcpy(temp, inet_ntoa(addr));
	xmlNewTextChild(pnode, NULL, BAD_CAST"dwGateWay", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	memcpy(&addr, &psys->dwNetMark, 4);
	strcpy(temp, inet_ntoa(addr));
	xmlNewTextChild(pnode, NULL, BAD_CAST"dwNetMark", BAD_CAST(temp));


	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	JoinMacAddr(temp, psys->szMacAddr, 6);
	xmlNewTextChild(pnode, NULL, BAD_CAST"szMacAddr", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%s", psys->strName);
	xmlNewTextChild(pnode, NULL, BAD_CAST"strName", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%s", psys->strVer);
	xmlNewTextChild(pnode, NULL, BAD_CAST"strVer", BAD_CAST(temp));


	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", psys->unChannel);
	xmlNewTextChild(pnode, NULL, BAD_CAST"unChannel", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%c", psys->bType + 48);
	xmlNewTextChild(pnode, NULL, BAD_CAST"bType", BAD_CAST(temp));

	int index = 0;

	for(index = 0; index < 7; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		memset(temp2, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "%d", psys->nTemp[index]);
		sprintf(temp2, "nTemp_%d", index);
		xmlNewTextChild(pnode, NULL, BAD_CAST(temp2), BAD_CAST(temp));
	}

	return 0;
}

static int add_video_param(xmlNodePtr pnode, VideoParam *pvideo)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->nDataCodec);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nDataCodec", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->nWidth);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nWidth", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->nHight);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nHight", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->nQuality);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nQuality", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->sCbr);
	xmlNewTextChild(pnode, NULL, BAD_CAST"sCbr", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->nFrameRate);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nFrameRate", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->sBitrate);
	xmlNewTextChild(pnode, NULL, BAD_CAST"sBitrate", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", pvideo->nColors);
	xmlNewTextChild(pnode, NULL, BAD_CAST"nColors", BAD_CAST(temp));

	return 0;
}


static int add_audio_param(xmlNodePtr pnode, AudioParam *paudio)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->Codec);
	xmlNewTextChild(pnode, NULL, BAD_CAST"Codec", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->SampleRate);
	xmlNewTextChild(pnode, NULL, BAD_CAST"SampleRate", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->Channel);
	xmlNewTextChild(pnode, NULL, BAD_CAST"Channel", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->SampleBit);
	xmlNewTextChild(pnode, NULL, BAD_CAST"SampleBit", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->BitRate);
	xmlNewTextChild(pnode, NULL, BAD_CAST"BitRate", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->InputMode);
	xmlNewTextChild(pnode, NULL, BAD_CAST"InputMode", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->RVolume);
	xmlNewTextChild(pnode, NULL, BAD_CAST"RVolume", BAD_CAST(temp));

	memset(temp, 0, XML_TEXT_MAX_LENGTH);
	sprintf(temp, "%d", paudio->LVolume);
	xmlNewTextChild(pnode, NULL, BAD_CAST"LVolume", BAD_CAST(temp));

	return 0;
}

static int read_system_param(xmlDocPtr pdoc, xmlNodePtr pnode, SYSPARAMS *psys)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	char temp2[XML_TEXT_MAX_LENGTH] = {0};

	struct in_addr		addr;
	xmlNodePtr		node;
	int				ret = 0;

	if(NULL == pdoc || NULL == pnode || NULL == psys) {
		fprintf(stderr, "read_system_param error, params invalid!\n");
		return -1;
	}

	node = get_children_node(pnode, BAD_CAST "dwAddr");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			inet_aton(temp, &addr);
			memcpy(&psys->dwAddr, &addr, 4);
			PRINTF("ipaddr: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "dwGateWay");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			inet_aton(temp, &addr);
			memcpy(&psys->dwGateWay, &addr, 4);
			PRINTF("gateway: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "dwNetMark");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			inet_aton(temp, &addr);
			memcpy(&psys->dwNetMark, &addr, 4);
			PRINTF("netmask: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "szMacAddr");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);
		PRINTF("mac: %s\n", temp);

		if(-1 != ret) {
			SplitMacAddr(temp, psys->szMacAddr, 6);
		}
	}

	node = get_children_node(pnode, BAD_CAST "strName");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			strcpy(psys->strName, temp);
			PRINTF("name: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "strVer");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			strcpy(psys->strVer, temp);
			PRINTF("version: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "unChannel");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			psys->unChannel = atoi(temp);
			PRINTF("channel: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "bType");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			psys->bType = atoi(temp);
			PRINTF("box type: %s\n", temp);
		}
	}

	int index = 0;

	for(index = 0; index < 7; index++) {
		memset(temp2, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp2, "nTemp_%d", index);
		node = get_children_node(pnode, BAD_CAST(temp2));

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			ret = get_current_node_value(temp, pdoc, node);

			if(-1 != ret) {
				psys->nTemp[index] = atoi(temp);
			}
		}
	}

	return 0;
}

static int read_video_param(xmlDocPtr pdoc, xmlNodePtr pnode, VideoParam *pvideo)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;
	int				ret = 0;

	if(NULL == pdoc || NULL == pnode || NULL == pvideo) {
		fprintf(stderr, "read_video_param error, params invalid!\n");
		return -1;
	}

	node = get_children_node(pnode, BAD_CAST "nDataCodec");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->nDataCodec = atoi(temp);

			PRINTF("\nencode mode: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nWidth");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->nWidth = atoi(temp);
			PRINTF("width: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nHight");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->nHight = atoi(temp);
			PRINTF("height: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nQuality");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->nQuality = atoi(temp);
			PRINTF("quality: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "sCbr");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->sCbr = atoi(temp);
			PRINTF("cbr: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nFrameRate");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->nFrameRate = atoi(temp);
			PRINTF("framerate: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "sBitrate");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->sBitrate = atoi(temp);
			PRINTF("bitrate: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "nColors");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			pvideo->nColors = atoi(temp);
			PRINTF("colors: %s\n", temp);
		}
	}

	return 0;
}


static int read_audio_param(xmlDocPtr pdoc, xmlNodePtr pnode, AudioParam *paudio)
{
	char				temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;
	int				ret = 0;

	if(NULL == pdoc || NULL == pnode || NULL == paudio) {
		fprintf(stderr, "read_audio_param error, params invalid!\n");
		return -1;
	}

	node = get_children_node(pnode, BAD_CAST "Codec");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->Codec = atoi(temp);
			PRINTF("\ncodec: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "SampleRate");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->SampleRate = atoi(temp);
			PRINTF("samplerate: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "Channel");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->Channel = atoi(temp);
			PRINTF("channel: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "SampleBit");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->SampleBit = atoi(temp);
			PRINTF("samplebit: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "BitRate");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->BitRate = atoi(temp);
			PRINTF("bitrate: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "InputMode");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->InputMode = atoi(temp);
			PRINTF("inputmode: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "RVolume");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->RVolume = atoi(temp);
			PRINTF("rvolume: %s\n", temp);
		}
	}

	node = get_children_node(pnode, BAD_CAST "LVolume");

	if(node) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		ret = get_current_node_value(temp, pdoc, node);

		if(-1 != ret) {
			paudio->LVolume = atoi(temp);
			PRINTF("lvolume: %s\n", temp);
		}
	}

	return 0;
}

static int modify_system_param(xmlNodePtr pnode, SYSPARAMS *pold, SYSPARAMS *pnew)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	char temp2[XML_TEXT_MAX_LENGTH] = {0};

	struct in_addr		addr;
	xmlNodePtr		node;

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		fprintf(stderr, "modify_system_param error, params invalid!\n");
		return -1;
	}

	fprintf(stderr, "%d, %d\n\n\n", pnew->dwAddr, pold->dwAddr);

	if(pnew->dwAddr != pold->dwAddr) {
		node = get_children_node(pnode, BAD_CAST "dwAddr");
		fprintf(stderr, "%p\n", node);

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			memcpy(&addr, &pnew->dwAddr, 4);
			strcpy(temp, inet_ntoa(addr));
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->dwGateWay != pold->dwGateWay) {
		node = get_children_node(pnode, BAD_CAST "dwGateWay");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			memcpy(&addr, &pnew->dwGateWay, 4);
			strcpy(temp, inet_ntoa(addr));
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->dwNetMark != pold->dwNetMark) {
		node = get_children_node(pnode, BAD_CAST "dwNetMark");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			memcpy(&addr, &pnew->dwNetMark, 4);
			strcpy(temp, inet_ntoa(addr));
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	JoinMacAddr(temp, pnew->szMacAddr, 6);
	JoinMacAddr(temp2, pold->szMacAddr, 6);

	if(strcmp((const char *)temp, temp2) != 0) {
		node = get_children_node(pnode, BAD_CAST "szMacAddr");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			JoinMacAddr(temp, pnew->szMacAddr, 6);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(strcmp((const char *)pnew->strName, (const char *)pold->strName) != 0) {
		node = get_children_node(pnode, BAD_CAST "strName");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%s", pnew->strName);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(strcmp((const char *)pnew->strVer, (const char *)pold->strVer) != 0) {
		node = get_children_node(pnode, BAD_CAST "strVer");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%s", pnew->strVer);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->unChannel != pold->unChannel) {
		node = get_children_node(pnode, BAD_CAST "unChannel");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->unChannel);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->bType != pold->bType) {
		node = get_children_node(pnode, BAD_CAST "bType");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%c", pnew->bType + 48);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	int index = 0;

	for(index = 0; index < 7; index++) {
		if(pnew->nTemp[index] != pold->nTemp[index]) {
			memset(temp2, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp2, "nTemp_%d", index);
			node = get_children_node(pnode, BAD_CAST(temp2));

			if(node) {
				memset(temp, 0, XML_TEXT_MAX_LENGTH);
				sprintf(temp, "%d", pnew->nTemp[index]);
				modify_node_value(node, BAD_CAST(temp));
			}
		}
	}

	system("sync");

	return 0;
}

static int modify_video_param(xmlNodePtr pnode, VideoParam *pold, VideoParam *pnew)
{
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		fprintf(stderr, "modify_video_param error, params invalid!\n");
		return -1;
	}

	if(pnew->nDataCodec != pold->nDataCodec) {
		node = get_children_node(pnode, BAD_CAST "nDataCodec");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->nDataCodec);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->nWidth != pold->nWidth) {
		node = get_children_node(pnode, BAD_CAST "nWidth");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->nWidth);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->nHight != pold->nHight) {
		node = get_children_node(pnode, BAD_CAST "nHight");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->nHight);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->nQuality != pold->nQuality) {
		node = get_children_node(pnode, BAD_CAST "nQuality");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->nQuality);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->sCbr != pold->sCbr) {
		node = get_children_node(pnode, BAD_CAST "sCbr");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->sCbr);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->nFrameRate != pold->nFrameRate) {
		node = get_children_node(pnode, BAD_CAST "nFrameRate");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->nFrameRate);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->sBitrate != pold->sBitrate) {
		node = get_children_node(pnode, BAD_CAST "sBitrate");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->sBitrate);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->nColors != pold->nColors) {
		node = get_children_node(pnode, BAD_CAST "nColors");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->nColors);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	system("sync");

	return 0;
}


static int modify_audio_param(xmlNodePtr pnode, AudioParam *pold, AudioParam *pnew)
{
	char				temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr		node;

	if(NULL == pnode || NULL == pold || NULL == pnew) {
		fprintf(stderr, "modify_audio_param error, params invalid!\n");
		return -1;
	}

	if(pnew->Codec != pold->Codec) {
		node = get_children_node(pnode, BAD_CAST "Codec");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->Codec);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->SampleRate != pold->SampleRate) {
		node = get_children_node(pnode, BAD_CAST "SampleRate");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->SampleRate);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->Channel != pold->Channel) {
		node = get_children_node(pnode, BAD_CAST "Channel");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->Channel);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->SampleBit != pold->SampleBit) {
		node = get_children_node(pnode, BAD_CAST "SampleBit");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->SampleBit);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->BitRate != pold->BitRate) {
		node = get_children_node(pnode, BAD_CAST "BitRate");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->BitRate);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->InputMode != pold->InputMode) {
		node = get_children_node(pnode, BAD_CAST "InputMode");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->InputMode);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->RVolume != pold->RVolume) {
		node = get_children_node(pnode, BAD_CAST "RVolume");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->RVolume);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	if(pnew->LVolume != pold->LVolume) {
		node = get_children_node(pnode, BAD_CAST "LVolume");

		if(node) {
			memset(temp, 0, XML_TEXT_MAX_LENGTH);
			sprintf(temp, "%d", pnew->LVolume);
			modify_node_value(node, BAD_CAST(temp));
		}
	}

	system("sync");

	return 0;
}


int create_params_table_file(char *xml_file, params_table *ptable)
{
	int index = 0;
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlDocPtr doc;
	xmlNodePtr root_node;
	xmlNodePtr sys_param_node;
	xmlNodePtr video_param_node[MAX_VIDEO_NUM];
	xmlNodePtr audio_param_node[MAX_AUDIO_NUM];

	pthread_mutex_lock(&ptable->lock);

	doc = xmlNewDoc(BAD_CAST"1.0"); 			//定义文档和节点指针

	if(NULL == doc) {
		fprintf(stderr, "xmlNewDoc failed, file: %s\n", xml_file);
		pthread_mutex_unlock(&ptable->lock);
		return -1;
	}

	root_node = xmlNewNode(NULL, BAD_CAST"params_table");
	xmlDocSetRootElement(doc, root_node);		//设置根节点

	sys_param_node = xmlNewNode(NULL, BAD_CAST "sys_param");
	xmlAddChild(root_node, sys_param_node);
	add_system_param(sys_param_node, &ptable->sysPara);

	for(index = 0; index < MAX_VIDEO_NUM; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "video_param_%d", index);
		video_param_node[index] = xmlNewNode(NULL, BAD_CAST(temp));
		xmlAddChild(root_node, video_param_node[index]);
		add_video_param(video_param_node[index], &ptable->videoPara[index]);
	}

	for(index = 0; index < MAX_AUDIO_NUM; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "audio_param_%d", index);
		audio_param_node[index] = xmlNewNode(NULL, BAD_CAST(temp));
		xmlAddChild(root_node, audio_param_node[index]);
		add_audio_param(audio_param_node[index], &ptable->audioPara[index]);
	}

	/*保存新的xml文件*/
	int ret = xmlSaveFormatFileEnc(xml_file, doc, "UTF-8", 1);

	if(-1 == ret) {
		fprintf(stderr, "xml save params table failed !!!\n");
		ret = -1;
		goto cleanup;
	}

#if 0
	xmlChar *xml_buf;
	int size;
	//xmlDocDumpFormatMemory(doc, &xml_buf, &size, 1);
	xmlDocDumpFormatMemoryEnc(doc, &xml_buf, &size,  "UTF-8", 1);
	PRINTF("[create_xml_file]\n");
	PRINTF("%s\n", (char *)xml_buf);

	xmlFree(xml_buf);
#endif
	//释放文档内节点动态申请的内存
cleanup:
	xmlFreeDoc(doc);

	system("sync");

	pthread_mutex_unlock(&ptable->lock);

	return ret;
}

int read_params_table_file(char *xml_file, params_table *ptable)
{
	int index = 0;
	int ret = 0;
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr sys_param_node;
	xmlNodePtr video_param_node[MAX_VIDEO_NUM];
	xmlNodePtr audio_param_node[MAX_AUDIO_NUM];

	parse_xml_t px;

	pthread_mutex_lock(&ptable->lock);

	init_file_dom_tree(&px, xml_file);

	if(NULL == px.pdoc || NULL == px.proot) {
		fprintf(stderr, "init_file_dom_tree failed, xml file: %s\n", xml_file);
		ret = -1;
		goto cleanup;
	}

	sys_param_node = get_children_node(px.proot, BAD_CAST"sys_param");

	if(NULL == sys_param_node) {
		fprintf(stderr, "get sys_param node failed, xml file: %s\n", xml_file);
		ret = -1;
		goto cleanup;
	}

	read_system_param(px.pdoc, sys_param_node, &ptable->sysPara);

	for(index = 0; index < MAX_VIDEO_NUM; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "video_param_%d", index);
		video_param_node[index] = get_children_node(px.proot, BAD_CAST(temp));

		if(video_param_node[index]) {
			read_video_param(px.pdoc, video_param_node[index], &ptable->videoPara[index]);
		}
	}

	for(index = 0; index < MAX_AUDIO_NUM; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "audio_param_%d", index);
		audio_param_node[index] = get_children_node(px.proot, BAD_CAST(temp));

		if(audio_param_node[index]) {
			read_audio_param(px.pdoc, audio_param_node[index], &ptable->audioPara[index]);
		}
	}

cleanup:
	release_dom_tree(px.pdoc);

	pthread_mutex_unlock(&ptable->lock);

	return ret;
}

int modify_params_table_file(char *xml_file, params_table *pold_table, params_table *pnew_table)
{
	int index = 0;
	char temp[XML_TEXT_MAX_LENGTH] = {0};
	xmlNodePtr sys_param_node;
	xmlNodePtr video_param_node[MAX_VIDEO_NUM];
	xmlNodePtr audio_param_node[MAX_AUDIO_NUM];

	parse_xml_t px;

	pthread_mutex_lock(&pold_table->lock);
	pthread_mutex_lock(&pnew_table->lock);

	init_file_dom_tree(&px, xml_file);

	if(NULL == px.pdoc || NULL == px.proot) {
		fprintf(stderr, "init_file_dom_tree failed, xml file: %s\n", xml_file);
		goto cleanup;
	}

	sys_param_node = get_children_node(px.proot, BAD_CAST"sys_param");

	if(sys_param_node) {
		modify_system_param(sys_param_node, &pold_table->sysPara, &pnew_table->sysPara);
	}


	for(index = 0; index < MAX_VIDEO_NUM; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "video_param_%d", index);
		video_param_node[index] = get_children_node(px.proot, BAD_CAST(temp));

		if(video_param_node[index]) {
			modify_video_param(video_param_node[index],
			                   &pold_table->videoPara[index], &pnew_table->videoPara[index]);
		}
	}

	for(index = 0; index < MAX_AUDIO_NUM; index++) {
		memset(temp, 0, XML_TEXT_MAX_LENGTH);
		sprintf(temp, "audio_param_%d", index);
		audio_param_node[index] = get_children_node(px.proot, BAD_CAST(temp));

		if(audio_param_node[index]) {
			modify_audio_param(audio_param_node[index],
			                   &pold_table->audioPara[index], &pnew_table->audioPara[index]);
		}
	}

cleanup:
	xmlSaveFormatFile(xml_file, px.pdoc, 1);
	release_dom_tree(px.pdoc);

	system("sync");

	pthread_mutex_unlock(&pnew_table->lock);
	pthread_mutex_unlock(&pold_table->lock);

	return 0;
}

int get_system_param(params_table *ptable, SYSPARAMS *psys)
{
	if(NULL == ptable || NULL == psys) {
		fprintf(stderr, "get_system_param failed, params error\n");
		return -1;
	}

	pthread_mutex_lock(&ptable->lock);

	//FIX ME ，数组大小隐患
	strncpy((char *)psys->szMacAddr, (char *)ptable->sysPara.szMacAddr, 8);
	strncpy(psys->strName, ptable->sysPara.strName, 16);
	strncpy(psys->strVer, ptable->sysPara.strVer, 10);
	strncpy(psys->bTemp, ptable->sysPara.bTemp, 3);
	psys->dwAddr = ptable->sysPara.dwAddr;
	psys->dwGateWay = ptable->sysPara.dwGateWay;
	psys->dwNetMark = ptable->sysPara.dwNetMark;
	psys->unChannel = ptable->sysPara.unChannel;
	psys->bType = ptable->sysPara.bType;
	psys->unChannel = ptable->sysPara.unChannel;
	memcpy(psys->nTemp, ptable->sysPara.nTemp, sizeof(int) * 7);

	pthread_mutex_unlock(&ptable->lock);

	return 0;
}


int get_video_param(params_table *ptable, int chid, VideoParam *pvideo)
{
	if(NULL == ptable || NULL == pvideo) {
		fprintf(stderr, "get_video_param failed, params error\n");
		return -1;
	}

	if(chid < 0 || chid > MAX_VIDEO_NUM) {
		fprintf(stderr, "get_video_param failed, chid error, %d\n", chid);
		return -1;
	}

	pthread_mutex_lock(&ptable->lock);

	pvideo->nDataCodec	= ptable->videoPara[chid].nDataCodec;
	pvideo->nFrameRate	= ptable->videoPara[chid].nFrameRate;
	pvideo->nWidth		= ptable->videoPara[chid].nWidth;
	pvideo->nHight		= ptable->videoPara[chid].nHight;
	pvideo->nColors		= ptable->videoPara[chid].nColors;
	pvideo->nQuality	= ptable->videoPara[chid].nQuality;
	pvideo->sCbr		= ptable->videoPara[chid].sCbr;
	pvideo->sBitrate	= ptable->videoPara[chid].sBitrate;

	pthread_mutex_unlock(&ptable->lock);

	return 0;
}

int get_audio_param(params_table *ptable, int chid, AudioParam *paudio)
{
	if(NULL == ptable || NULL == paudio) {
		fprintf(stderr, "get_audio_param failed, params error\n");
		return -1;
	}

	if(chid < 0 || chid > MAX_AUDIO_NUM) {
		fprintf(stderr, "get_audio_param failed, chid error, %d\n", chid);
		return -1;
	}

	pthread_mutex_lock(&ptable->lock);

	//PRINTF("id =%d,paudio =%p,samplebit=%d\n",chid,&(ptable->audio_param[chid]),ptable->audio_param[chid].SampleBit);
	paudio->Codec		= ptable->audioPara[chid].Codec;
	paudio->SampleRate	= ptable->audioPara[chid].SampleRate;
	paudio->BitRate		= ptable->audioPara[chid].BitRate;
	paudio->Channel		= ptable->audioPara[chid].Channel;
	paudio->SampleBit	= ptable->audioPara[chid].SampleBit;
	paudio->LVolume		= ptable->audioPara[chid].LVolume;
	paudio->RVolume		= ptable->audioPara[chid].RVolume;
	paudio->InputMode	= ptable->audioPara[chid].InputMode;

	//PRINTF("paudio->SampleBit= %d\n",paudio->SampleBit);

	pthread_mutex_unlock(&ptable->lock);

	return 0;
}

int get_realtime_param(params_table *p_rt_table, params_table *p_out_table)
{
	if(NULL == p_rt_table || NULL == p_out_table) {
		fprintf(stderr, "get_realtime_param failed, params error\n");
		return -1;
	}

	// 注: out_table的lock成员不可用

	pthread_mutex_lock(&p_rt_table->lock);

	memcpy(p_out_table, p_rt_table, sizeof(params_table));

	pthread_mutex_unlock(&p_rt_table->lock);

	return 0;
}

int BeyondAudioParam(AudioParam *pNew, AudioParam *pOld)
{
	int ret = 0;

	if(pNew->BitRate != pOld->BitRate) {
		pOld->BitRate = pNew->BitRate;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam BitRate change:%d\n", pNew->BitRate);
	}

	if(pNew->BitRate > 128000 || pNew->BitRate <= 0) {
		pOld->BitRate = pNew->BitRate = 128000;
	}

	if(pNew->BitRate != pOld->BitRate) {
		pOld->BitRate = pNew->BitRate ;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam BitRate change:%d\n", pNew->BitRate);
	}

	if(pNew->Channel != pOld->Channel || pNew->Channel != 2) {
		pOld->Channel = 2;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam Channel change:%d\n", pNew->Channel);
	}

	if(pNew->SampleBit != pOld->SampleBit || pNew->SampleBit != 16) {
		pOld->SampleBit = 16 ;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam SampleBit change:%d\n", pNew->SampleBit);
	}

	if(pNew->SampleRate != pOld->SampleRate) {
		pOld->SampleRate = pNew->SampleRate ;

		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam SampleRate change:%d\n", pNew->SampleRate);
	}

	if(pNew->SampleRate != pOld->SampleRate) {
		pOld->SampleRate = pNew->SampleRate;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam SampleRate change:%d\n", pNew->SampleRate);
	}

	if(pNew->LVolume != pOld->LVolume) {
		pOld->LVolume = pNew->LVolume ;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam LVolume change:%d\n", pNew->LVolume);
	}

	if(pNew->RVolume != pOld->RVolume) {
		pOld->RVolume = pNew->RVolume ;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam RVolume change:%d\n", pNew->RVolume);
	}

	if(pNew->InputMode != pOld->InputMode) {
		pOld->InputMode = pNew->InputMode ;
		ret = 1;
		DEBUG(DL_DEBUG, "AudioParam InputMode change:%d\n", pNew->InputMode);
	}

	return (ret);
}


#if 0
int msg_get_video_param(int socket, int chid, unsigned char *data, params_table *ptable)
{
	int length = 0, ret;
	VideoParam video_param;

	if(chid < 0 || chid > MAX_VIDEO_NUM) {
		fprintf(stderr, "msg_get_video_param failed, chid error, %d\n", chid);
		return -1;
	}

	length = HEAD_LEN + sizeof(VideoParam);
	PackHeaderMSG(data, MSG_GET_VIDEOPARAM, length);

	ret = get_video_param(ptable, chid, &video_param);

	if(ret < 0) {
		PRINTF("get_video_param error!\n");
		return -1;
	}

	memcpy(data + HEAD_LEN, &video_param, sizeof(VideoParam));
	ret = WriteData(socket, data, length);

	if(ret < 0) {
		PRINTF("msg_get_video_param error %d socket:%d\n", ret, socket);
		return -1;
	}

	return 0;
}
#endif

int msg_get_audio_param(int socket, int chid, unsigned char *data, params_table *ptable)
{
	int length = 0, ret;
	AudioParam audio_param;

	if(chid < 0 || chid > MAX_AUDIO_NUM) {
		fprintf(stderr, "msg_get_audio_param failed, chid error, %d\n", chid);
		return -1;
	}

	length = HEAD_LEN + sizeof(AudioParam);
	PackHeaderMSG(data, MSG_GET_AUDIOPARAM, length);

	ret = get_audio_param(ptable, chid, &audio_param);

	if(ret < 0) {
		PRINTF("get_audio_param error!\n");
		return -1;
	}

	memcpy(data + HEAD_LEN, &audio_param, sizeof(AudioParam));
	ret = WriteData(socket, data, length);

	if(ret < 0) {
		PRINTF("msg_get_audio_param error %d socket:%d\n", ret, socket);
		return -1;
	}

	return 0;
}

int msg_get_system_param(int socket, unsigned char *data, params_table *ptable)
{
	int length = 0, ret;
	SYSPARAMS system_param;

	fprintf(stderr, "001--msg_get_system_param!\n");

	length = HEAD_LEN + sizeof(SYSPARAMS);
	PackHeaderMSG(data, MSG_SYSPARAMS, length);

	fprintf(stderr, "002--msg_get_system_param!\n");

	ret = get_system_param(ptable, &system_param);

	if(ret < 0) {
		PRINTF("get_system_param error!\n");
		return -1;
	}

	fprintf(stderr, "003--msg_get_system_param!\n");

	memcpy(data + HEAD_LEN, &system_param, sizeof(SYSPARAMS));
	ret = WriteData(socket, data, length);

	if(ret < 0) {
		PRINTF("msg_get_system_param error %d socket:%d\n", ret, socket);
		return -1;
	}

	fprintf(stderr, "004--msg_get_system_param!\n");

	return 0;
}
#if 0
int msg_set_video_param(int chid, unsigned char *data, params_table *ptable)
{
	VideoParam *pnew_param;
	VideoParam old_param;
	int ret = 0;


	if(chid < 0 || chid > MAX_VIDEO_NUM) {
		fprintf(stderr, "msg_set_video_param failed, chid error, %d\n", chid);
		return -1;
	}

	if(NULL == data || NULL == ptable) {
		fprintf(stderr, "msg_set_video_param, params error!\n");
		return -1;
	}

	get_video_param(ptable, chid, &old_param);
	pnew_param = (VideoParam *)data;

	if(pnew_param->nFrameRate == 38) {
		if(chid == 0) {
			g_vp0_capture_mode = 0;
			set_gpio_value(gEnc2000.gpiofd, 39, 0);
		} else {
			g_vp1_capture_mode = 0;
			set_gpio_value(gEnc2000.gpiofd, 40, 0);
		}
	}

	if(pnew_param->nFrameRate == 39) {
		if(chid == 0) {
			g_vp0_capture_mode = 1;
			set_gpio_value(gEnc2000.gpiofd, 39, 1);
		} else {
			g_vp1_capture_mode = 1;
			set_gpio_value(gEnc2000.gpiofd, 40, 1);
		}
	}

	if(pnew_param->nFrameRate == 42) {
		g_vp0_writeyuv_flag = 1;
	}

	if(pnew_param->nFrameRate == 43) {
		g_vp1_writeyuv_flag = 1;
	}

	if(pnew_param->nFrameRate == 44) {
		g_mp_writeyuv_flag = 1;
	}

	ret = (pnew_param->nFrameRate != old_param.nFrameRate)
	      || (pnew_param->sBitrate != old_param.sBitrate);

	if(ret) {
		if(pnew_param->sCbr == 0) {
			enc_set_fps(gEnc2000.encLink.link_id, chid,
			            (pnew_param->nFrameRate) * 1000, (pnew_param->nQuality) * 100 * 1100);
		} else {
			enc_set_fps(gEnc2000.encLink.link_id, chid,
			            (pnew_param->nFrameRate) * 1000, (pnew_param->sBitrate) * 1100);
		}

	}

	PRINTF("set video param  chid = %d, old nFrameRate : [%d], new nFrameRate : [%d] bitrate : [%d]\n\n",
	       chid, old_param.nFrameRate, pnew_param->nFrameRate, pnew_param->sBitrate);

	return 0;
}
#endif
int msg_set_system_param(unsigned char *data, params_table *ptable)
{
	SYSPARAMS *pnew_param;
	SYSPARAMS old_param;
	int ret = 0;
	int  chl = 0;
	int status = 0, flag = 0;

	if(NULL == data || NULL == ptable) {
		fprintf(stderr, "msg_set_system_param, params error!\n");
		return -1;
	}

	get_system_param(ptable, &old_param);
	pnew_param = (SYSPARAMS *)data;

	ret = CheckIPNetmask(pnew_param->dwAddr, pnew_param->dwNetMark, pnew_param->dwGateWay);

	if(ret == 0) {
		DEBUG(DL_ERROR, "Set IP addr error!!!!\n");
		return -1;
	}

#if 0

	for(i = 0; i < 8; i++) {
		if(pold->szMacAddr[i] == 0) {
			count++;
		}
	}

	if(count != 8) {
		for(i = 0; i < 8; i++) {
			if(pold->szMacAddr[i] != pnew->szMacAddr[i]) {
				pold->szMacAddr[i] = pnew->szMacAddr[i];
				status = 1;
			}
		}
	}

	if(status) {
		DEBUG(DL_DEBUG, "The User Has Changed The MAC Addr!\n");
	}

	status = 0;
#endif

	if(old_param.dwAddr != 0 && old_param.dwAddr != pnew_param->dwAddr)	{
		old_param.dwAddr = pnew_param->dwAddr;
		DEBUG(DL_DEBUG, "The User Has Changed The IP Addr!\n");
		status = 1;
	}

	if(old_param.dwGateWay != 0 && old_param.dwGateWay != pnew_param->dwGateWay)	{
		old_param.dwGateWay = pnew_param->dwGateWay;
		DEBUG(DL_DEBUG, "The User Has Changed The GateWay!\n");
		flag = 1;
	}

	if(old_param.dwNetMark != 0 && old_param.dwNetMark != pnew_param->dwNetMark)	{
		old_param.dwNetMark = pnew_param->dwNetMark;
		DEBUG(DL_DEBUG, "The User Has Changed The NetMark!\n");
		status = 1;
	}

	if(old_param.nTemp[0] != pnew_param->nTemp[0])	{
		old_param.nTemp[0]	= pnew_param->nTemp[0] ;
		DEBUG(DL_DEBUG, "The dhcp flag Has Changed	\n");
		status = 1;
	}

	if(strlen(old_param.strName) != 0) {
		strcpy(old_param.strName, pnew_param->strName);
	}

	if(strlen(old_param.strVer) != 0) {
		strcpy(old_param.strVer, pnew_param->strVer);
	}

	if(old_param.unChannel != pnew_param->unChannel)	{
		DEBUG(DL_DEBUG, "The Channel numbers has changed !\n");
		old_param.unChannel = pnew_param->unChannel;
		old_param.unChannel = 1;
		chl = 1;
	}

#if 0

	if(old_param.nTemp != pnew_param->nTemp) {
		old_param.nTemp = pnew_param->nTemp;
		DEBUG(DL_DEBUG, "The User Has Changed The sTemp!\n");
	}

#endif

	if(chl || status || flag) {
		//		SendBroadCastMsg(PORT_ONE, MSG_SYSPARAMS, (unsigned char *)pnew_param, sizeof(SYSPARAMS));
	}

	if(status || flag) {
		return 1;
	} else {
		return 0;
	}

	return 0;
}


int msg_set_audio_param(int chid, unsigned char *data, params_table *ptable)
{
	AudioParam *pnew_param, old_param;
	int ret = 0;

	if(chid < 0 || chid > MAX_AUDIO_NUM) {
		fprintf(stderr, "msg_set_audio_param failed, chid error, %d\n", chid);
		return -1;
	}

	get_audio_param(ptable, chid, &old_param);
	pnew_param = (AudioParam *)data;

	ret = BeyondAudioParam(pnew_param, &old_param);

	if(ret) {
		ptable->audioPara[chid].SampleRate = pnew_param->SampleRate;
		ptable->audioPara[chid].SampleBit = pnew_param->SampleBit;
		ptable->audioPara[chid].RVolume = pnew_param->RVolume;
		ptable->audioPara[chid].LVolume = pnew_param->LVolume;
		ptable->audioPara[chid].InputMode = pnew_param->InputMode;
		audio_setCapParamInput(0, pnew_param->InputMode);
		audio_setCapParamSampleRate(gEnc2000.audioencLink[0].pacaphandle, ChangeSampleIndex(pnew_param->SampleRate));
		audio_setCapParamFlag(gEnc2000.audioencLink[0].pacaphandle);
		audio_setEncParam(gEnc2000.audioencLink[0].paenchandle, ChangeSampleIndex(pnew_param->SampleRate), pnew_param->BitRate);
		audio_setEncParamFlag(gEnc2000.audioencLink[0].paenchandle);

	}

	return 0;
}


int msg_save_param_table(params_table *pold_table, params_table *pnew_table)
{
	return modify_params_table_file(CONFIG_TABLE_FILE, pold_table, pnew_table);
}

int init_all_params()
{
	int ret = 0;
	SYSPARAMS sysparam;
	PRINTF("\n");


	gp_forever_params = (params_table *)malloc(sizeof(params_table));

	if(gp_forever_params == NULL) {
		gp_forever_params = (params_table *)malloc(sizeof(params_table));

		if(gp_forever_params == NULL) {
			goto cleanup;
		}
	}

	gp_realtime_params = (params_table *)malloc(sizeof(params_table));

	if(gp_realtime_params == NULL) {
		gp_realtime_params = (params_table *)malloc(sizeof(params_table));

		if(gp_realtime_params == NULL) {
			goto cleanup;
		}
	}

	init_params_table(gp_forever_params);
	init_params_table(gp_realtime_params);

	if(access(CONFIG_TABLE_FILE, F_OK) != 0) {
		ret = create_params_table_file(CONFIG_TABLE_FILE, gp_forever_params);

		if(ret == -1) {
			fprintf(stderr, "warming: create config file %s failed!\n", CONFIG_TABLE_FILE);
		}
	} else {
		read_params_table_file(CONFIG_TABLE_FILE, gp_forever_params);
		read_params_table_file(CONFIG_TABLE_FILE, gp_realtime_params);
	}

	get_system_param(gp_realtime_params, &sysparam);

	PRINTF("paudio =%p,samplebit=%d\n", &(gp_realtime_params->audioPara[0]), (gp_realtime_params->audioPara[0]).SampleBit);

	//SetEthConfig(sysparam.dwAddr, sysparam.dwNetMark, sysparam.dwGateWay);

	return 0;
cleanup:

	if(gp_forever_params) {
		free(gp_forever_params);
	}

	if(gp_realtime_params) {
		free(gp_realtime_params);
	}

	return -1;
}
int audio_setCapvol(int ch, int mode)
{
	char cmd[200] = {0};
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' %d", ch, mode);
	system(cmd);
	return 1;
}
int audio_setCapParamInput(int ch, int mode)
{
	char cmd[200] = {0};
	memset(cmd, 0, sizeof(cmd));

	if(mode == 1) { //非平衡mic  in
		sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=93,iface=MIXER,name='Left PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=94,iface=MIXER,name='Left PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=95,iface=MIXER,name='Left PGA Mixer Line2L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=87,iface=MIXER,name='Right PGA Mixer Line2R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=96,iface=MIXER,name='Left PGA Mixer Mic3L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=97,iface=MIXER,name='Left PGA Mixer Mic3R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=88,iface=MIXER,name='Right PGA Mixer Mic3L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=89,iface=MIXER,name='Right PGA Mixer Mic3R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=85,iface=MIXER,name='Right PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=86,iface=MIXER,name='Right PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=39,iface=MIXER,name='ADC HPF Cut-off' 1", ch);
		system(cmd);
	} else if(mode == 0) {
		//line in
		sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=93,iface=MIXER,name='Left PGA Mixer Line1L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=94,iface=MIXER,name='Left PGA Mixer Line1R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=95,iface=MIXER,name='Left PGA Mixer Line2L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=87,iface=MIXER,name='Right PGA Mixer Line2R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=96,iface=MIXER,name='Left PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=97,iface=MIXER,name='Left PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=88,iface=MIXER,name='Right PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=89,iface=MIXER,name='Right PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=85,iface=MIXER,name='Right PGA Mixer Line1R Switch' 1 ", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=86,iface=MIXER,name='Right PGA Mixer Line1L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=39,iface=MIXER,name='ADC HPF Cut-off' 0", ch);
		system(cmd);
	} else {
		//平衡mic
		sprintf(cmd, "amixer cset -c %d numid=36,iface=MIXER,name='AGC Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=93,iface=MIXER,name='Left PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=94,iface=MIXER,name='Left PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=95,iface=MIXER,name='Left PGA Mixer Line2L Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=87,iface=MIXER,name='Right PGA Mixer Line2R Switch' 1", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=96,iface=MIXER,name='Left PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=97,iface=MIXER,name='Left PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=88,iface=MIXER,name='Right PGA Mixer Mic3L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=89,iface=MIXER,name='Right PGA Mixer Mic3R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=85,iface=MIXER,name='Right PGA Mixer Line1R Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=86,iface=MIXER,name='Right PGA Mixer Line1L Switch' 0", ch);
		system(cmd);
		sprintf(cmd, "amixer cset -c %d numid=39,iface=MIXER,name='ADC HPF Cut-off' 1", ch);
		system(cmd);
	}

	return 1;

}

#endif

/*Read Remote Protocol*/
int ReadRemoteCtrlIndex(int remote)
{
	char 			temp[8] = {0};
	int 			ret  = -1 ;
	char rkey[16] = {0};
	sprintf(rkey, "remote%d", remote);
	/*remote control index*/
	ret =  ConfigGetKey(REMOTE_NAME, rkey, "index", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get remote index\n");
		return 4;
	}

	return atoi(temp);
}


/*save remote control index*/
int SaveRemoteCtrlIndex(int remote, int ProtoleIndex)
{
	unsigned char temp[8] = {0};
	char rkey[16] = {0};
	int ret = 0;
	/*index*/
	sprintf((char *)temp, "%d", ProtoleIndex);
	sprintf(rkey, "remote%d", remote);
	ret =  ConfigSetKey(REMOTE_NAME, rkey, "index", temp);

	if(ret != 0) {
		ERR_PRN("set remote index failed\n");
		return ret;
	}

	return ret;
}

