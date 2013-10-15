
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "lcd.h"
#include "app_audio_control.h"
#include "app_video_control.h"
#include "input_to_channel.h"
#include "../version.h"
#include "encliveplay.h"
#include "encliveplay2.h"
#include "sysparams.h"
#include "lcdshow.h"
typedef struct lcd_show_ {
	char ipaddr[16];
	char netmask[16];
	char gateway[16];
	int clients;
	int width;
	int height;
	int VideoBitRate;
	int SampleRate;
	int AudioBitRate;
} lcd_show_t;

#define	SHOW_TIME	3
int lcd_show_string(int fd, lcd_show_t *v)
{
	char pCh[8] = {0};
	char pclients[16] = {0};
	char pwidth[16] = {0};
	char pheight[16] = {0};
	char pVideoBitRate[32] = {0};
	char pSampleRate[32] = {0};
	char pAudioBitRate[32] = {0};
	char dev_id[32] = {0};
	char serialno[32] = {0};
	char serialno_view[48] = {0};
	char soft_version[32] = {0};
	int clients = 0;
	int index = 0;
	int index_sum = 1;

	if(ENC2000 == sys_get_product_type()) {
		index_sum = 2;
	}

	for(index = 0; index < index_sum; index ++) {
		memset(pCh, 0, sizeof(pCh));
		memset(pclients, 0, sizeof(pclients));
		memset(pwidth, 0, sizeof(pwidth));
		memset(pheight, 0, sizeof(pheight));
		memset(pVideoBitRate, 0, sizeof(pVideoBitRate));
		memset(pSampleRate, 0, sizeof(pSampleRate));
		memset(pAudioBitRate, 0, sizeof(pAudioBitRate));
		ClearScreen(fd);

		if(ENC2000 == sys_get_product_type()) {
			snprintf(dev_id, 31, "ID:ENC2000");
		} else if(ENC1600 == sys_get_product_type()) {
			snprintf(dev_id, 31, "ID:ENC1600");
		} else {
			snprintf(dev_id, 31, "ID:UNKNOW");
		}

		ShowStringLine(fd, dev_id, LCD_SECOND_LINE);
		snprintf(soft_version, 31, "Version:%s", SOFT_VERSION);
		ShowStringLine(fd, soft_version, LCD_THIRD_LINE);
		app_get_serialno(serialno);
		snprintf(serialno_view, 48, "Serialno:%s", serialno);
		ShowStringLine(fd, serialno_view, LCD_FOURTH_LINE);
		sleep(SHOW_TIME);
		ClearScreen(fd);
		snprintf(pCh, 7, "CH%d:", index + 1);
		ShowStringLine(fd, pCh, LCD_FIRST_LINE);
		ShowStringLine(fd, v[index].ipaddr, LCD_SECOND_LINE);
		ShowStringLine(fd, v[index].netmask, LCD_THIRD_LINE);
		ShowStringLine(fd, v[index].gateway, LCD_FOURTH_LINE);
		sleep(SHOW_TIME);
		ClearScreen(fd);
		snprintf(pCh, 7, "CH%d:", index + 1);
		ShowStringLine(fd, pCh, LCD_FIRST_LINE);

		if(0 == index) {
			clients = get_MMP1_connent();
		} else {
			clients = get_MMP2_connent();
		}

		snprintf(pclients, 15, "Clients:%d", clients);
		ShowStringLine(fd, pclients, LCD_SECOND_LINE);
		snprintf(pwidth, 15, "Width:%d", v[index].width);
		ShowStringLine(fd, pwidth, LCD_THIRD_LINE);
		snprintf(pheight, 15, "Height:%d", v[index].height);
		ShowStringLine(fd, pheight, LCD_FOURTH_LINE);
		sleep(SHOW_TIME);
		ClearScreen(fd);
		snprintf(pCh, 7, "CH%d:", index + 1);
		ShowStringLine(fd, pCh, LCD_FIRST_LINE);
		snprintf(pVideoBitRate, 31, "VideoBitRate:%dK", v[index].VideoBitRate);
		ShowStringLine(fd, pVideoBitRate, LCD_SECOND_LINE);
		snprintf(pSampleRate, 31, "SampleRate:%dK", v[index].SampleRate);
		ShowStringLine(fd, pSampleRate, LCD_THIRD_LINE);
		snprintf(pAudioBitRate, 31, "AudioBitRate:%dK", v[index].AudioBitRate / 1000);
		ShowStringLine(fd, pAudioBitRate, LCD_FOURTH_LINE);
		sleep(SHOW_TIME);
	}

	return 0;
}

void *lcd_show_process(void *arg)
{
	//	printf("[lcd_show_process] start...\n");
	lcd_show_t video_info[2];
	MMPVideoParam vp0, vp1;
	MMPAudioParam ap0, ap1;
	int width = 0;
	int height = 0;
	int sample = 0;
	int fd;
	struct in_addr addr;
	unsigned int ip = 0;
	unsigned int gateway = 0;
	unsigned int netmask = 0;
	int input = 0;
	int channel = 0;
	fd = InitLed();

	if(fd < 0) {
		printf("InitLed is fail !!\n");
		return NULL;
	}

	while(1) {
		input = SIGNAL_INPUT_1;
		channel = CHANNEL_INPUT_1;

		if(IS_MP_STATUS == get_mp_status()) {
			input = SIGNAL_INPUT_MP;
			channel = CHANNEL_INPUT_MP;
		}

		memset(video_info, 0, sizeof(lcd_show_t) * 2);
		memset(&vp0, 0, sizeof(MMPVideoParam));
		memset(&vp1, 0, sizeof(MMPVideoParam));
		sys_get_network(0, &ip, &gateway, &netmask);
		memcpy(&addr, &ip, 4);

		strcpy(video_info[0].ipaddr, inet_ntoa(addr));
		memcpy(&addr, &gateway, 4);
		strcpy(video_info[0].gateway, inet_ntoa(addr));
		memcpy(&addr, &netmask, 4);
		strcpy(video_info[0].netmask, inet_ntoa(addr));

		MMP_video_info_get(channel, &vp0);
		app_get_media_info(&width, &height, &sample, channel);
		capture_get_input_hw(SIGNAL_INPUT_1, &width, &height);
		video_info[0].width = width;
		video_info[0].height = height;
		video_info[0].VideoBitRate = vp0.sBitrate;

		MMP_audio_info_get(input, &ap0);
		video_info[0].SampleRate = ChangeSampleIndex(ap0.SampleRateIndex) / 1000;
		video_info[0].AudioBitRate = ap0.BitRate;

		if(ENC2000 == sys_get_product_type()) {
			if(IS_IND_STATUS == get_mp_status()) {
				input = SIGNAL_INPUT_2;
				channel = CHANNEL_INPUT_2;
			}

			sys_get_network(1, &ip, &gateway, &netmask);
			memcpy(&addr, &ip, 4);
			strcpy(video_info[1].ipaddr, inet_ntoa(addr));
			memcpy(&addr, &gateway, 4);
			strcpy(video_info[1].gateway, inet_ntoa(addr));
			memcpy(&addr, &netmask, 4);
			strcpy(video_info[1].netmask, inet_ntoa(addr));

			MMP_video_info_get(channel, &vp1);
			app_get_media_info(&width, &height, &sample, channel);
			capture_get_input_hw(SIGNAL_INPUT_2, &width, &height);
			video_info[1].width = width;
			video_info[1].height = height;
			video_info[1].VideoBitRate = vp1.sBitrate;

			MMP_audio_info_get(input, &ap1);
			video_info[1].SampleRate = ChangeSampleIndex(ap1.SampleRateIndex) / 1000;
			video_info[1].AudioBitRate = ap1.BitRate;
		}

		lcd_show_string(fd, video_info);
	}

}

int lcd_show(void)
{
	//	PRINTF("[lcd_show] start ...\n");
	pthread_t p;

	if(pthread_create(&p, NULL, lcd_show_process, NULL)) {
		//		PRINTF( "r_pthread_create failed !!!\n");
		return -1;
	}

	return 0;
}

