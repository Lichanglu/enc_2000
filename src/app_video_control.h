#ifndef _APP_VIDEO_CONTROL_H__
#define _APP_VIDEO_CONTROL_H__

#include "mid_mutex.h"
#include "MMP_control.h"
#include "middle_control.h"
#include "input_to_channel.h"
//编码的设置参数(2high,2 low)
typedef struct __VideoCommonInfo {

	//video: mmioFOURCC('H','2','6','4');
	//unsigned int is_hp;   // 1 :HP  ,0 :BP
	unsigned int ndatacodec;
	unsigned int ncolors ;
	unsigned int width;       					//video: width
	unsigned int hight;       					//video: height
	unsigned short scbr;								//video: 0--Qaulity/ 1--BitRate  
	unsigned int quality;							//video: quality
	unsigned short bitrate;							//video: bitrate
	unsigned int framerate;   					//vdieo: FrameRate
	unsigned int gop;                      //gop
	SceneConfiguration   	preset ;			//场景配置选项。
} VideoEnodeInfo;

/****************************************************************************
*  主要描述与缩放相关的参数。
*         a) 录制锁定分辨率。出入0/1，锁定到当前编码的宽高
*         b) 高中低质量以及电影模式。 传入需要锁定的宽高，码率，需要互斥
*		  c) 低质量码流。出入宽，高，帧率，码率。(保存到FLASH)
*         d) 页面的锁定分辨率，包含全屏缩放以及按比例缩放和宽高 (保存到FLASH)
*
*   优先级: 录制 > 高中低 > 页面 ，其中低质量是额外的
*
*****************************************************************************/

//scale 参数
typedef struct __VIDEOSCALEINFO {
	int quality_lock_flag ;  		// 0 不处于质量锁定模式 1 处于质量锁定模式
	int record_lock_flag;      //  0 不处于录制锁定模式  1 处于录制锁定模式
	int web_lock_flag ;        // 0  不处于web锁定模式  1 处于录制锁定模式
	int low_lock_flag ;        // 0 不处于小流缩放模式  1 处于小流缩放模式
#ifdef HAVE_IP_XML
	int ip_xml_lock_flag;     // 0  不锁定，1锁定
#endif
	int outwidth ;     			//输出宽
	int outheight ;     			//输出高
	int scalemode ;   			 //0 -- 1缩放模式
} VideoScaleInfo;




typedef struct __VideoCommonHandle {
	unsigned int profile; //encode profile ,hp,mp,bp
	VideoEnodeInfo info[MAX_CHANNEL];
	VideoScaleInfo sinfo[MAX_CHANNEL];
	mid_mutex_t  mutex[MAX_CHANNEL];  //互斥锁
	mid_mutex_t  smutex[MAX_CHANNEL];  //互斥锁
} VideoCommonInfo;

typedef VideoCommonInfo *VideoCommonHandle;


extern int set_scale_mode(int link_id, int mode);
int MMP_video_info_get(int channel, MMPVideoParam *vparam);
int MMP_video_info_set(int channel, MMPVideoParam *vparam, int *change);
int web_video_info_get(int channel, WebVideoEncodeInfo *vparam);
int web_video_info_set(int channel, WebVideoEncodeInfo *vparam, int *change);
int app_video_get_profile();
int app_video_set_profile(int profile);
void app_video_control_set(int);
int app_get_scale_status(int channel, int *scale, int *width, int *height);
void app_scale_control_set();
int MMP_recode_lock_set(int channel, int islock);
int MMP_quailty_lock_set(int channel, int qulity, int outwidth, int outheight);
int MMP_lowstream_lock_set(int channel, int outwidth, int outheight);
int web_scale_lock_set(int channel, int outwidth, int outheight, int scalemode);
int web_scale_lock_get(int channel, int *width, int *height, int *scalemode);
int write_encoding_mode(int enclevel, int save);

int update_video_fps(int input);
int MMP_scale_lock_get(int channel, int *width, int *height);

int app_init_no_signal_res(void);

int app_init_video_control();
void app_set_scale_mode(int state);
int get_scale_status(void);




#endif


