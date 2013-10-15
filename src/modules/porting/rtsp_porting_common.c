/*******************************************************************
*  rtsp porting  common for rtsp .
*  主要用于封装RTSP需要用到部分APP的函数的封装
*                               add by zm
* 												2011-5-31
********************************************************************/

#ifdef HAVE_RTSP_MODULE
/*
* RTSP使用前需要强制I帧，用于VLC/QT快速播放
*/
#include <stdio.h>


#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/types.h>
#include <pthread.h>
#include <time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

#include "stream_output_struct.h"
#include "rtsp_output_server.h"
#include "log_common.h"



//extern int ForceIframe(int start);

int rtsp_porting_server_need_stop()
{
	if(app_rtsp_get_active() == 0) {
		return 1;
	} else {
		return 0;
	}
}


int rtsp_porting_force_Iframe(int roomid)
{
	//	ForceIframe(0);
	return 0;
}


int rtsp_porting_close_client(int pos, void *client)
{
	return stream_close_rtsp_client(pos, client);
}

void *rtsp_porting_create_client(int pos, struct sockaddr_in *video_addr, struct sockaddr_in *audio_addr, int stream_channel)
{
	return stream_create_rtsp_client(pos, video_addr, audio_addr, stream_channel);
}

int rtsp_porting_get_ginfo(int *mult, char *ip, unsigned short *vport, unsigned short *aport, int stream_channel)
{
	return stream_get_rtsp_ginfo(mult, ip, vport, aport, stream_channel);
}


int rtsp_porting_parse_url(char *url, char *localip, int *roomid)
{
#if 1
	char url2[512] = {0};
	char *temp = NULL;

	sprintf(url2, "rtsp://%s", localip);

	if(strncmp(url, url2, strlen(url2)) != 0) {
		PRINTF("ERROR!!!rtsp_describe_authentication 4.\n");
		return -1;
	}

	temp = strstr(url + strlen(url2), "/");

	if(temp == NULL || strlen(temp) == 1) {
		PRINTF("roomid will set to 0\n");
		*roomid = 0;
		return 0;
	}

	if(strncmp(temp + 1, "stream", strlen("stream")) == 0) {
		if(strncmp(temp + 1, "stream0", strlen("stream0")) == 0) {
			if(strncmp(temp + 1, "stream0/high", strlen("stream0/high")) == 0) {
				*roomid = 0;
			} else if(strncmp(temp + 1, "stream0/low", strlen("stream0/low")) == 0) {
				*roomid = 1;
			} else if(strcmp(temp + 1, "stream0") == 0) {
				*roomid = 0;
			}
		} else if(strncmp(temp + 1, "stream1", strlen("stream1")) == 0) {
			if(strncmp(temp + 1, "stream1/high", strlen("stream1/high")) == 0) {
				*roomid = 2;
			} else if(strncmp(temp + 1, "stream1/low", strlen("stream1/low")) == 0) {
				*roomid = 3;
			} else if(strcmp(temp + 1, "stream1") == 0) {
				*roomid = 2;
			}
		}

		return 0;
	} else {
		PRINTF("ERROR!!!rtsp_describe_authentication 4.\n");
		return -1;
	}

	return -1;
#else  //if no stream ,will ok
	*roomid = 0;
	return 0;
#endif
}






#endif

