#ifndef _RTSP_PORTING_COMMON_H__
#define _RTSP_PORTING_COMMON_H__


/*
* RTSP使用前需要强制I帧，用于VLC/QT快速播放
*/
void rtsp_porting_force_Iframe();
int rtsp_porting_server_need_stop();
#endif
