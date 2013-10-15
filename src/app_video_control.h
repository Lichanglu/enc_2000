#ifndef _APP_VIDEO_CONTROL_H__
#define _APP_VIDEO_CONTROL_H__

#include "mid_mutex.h"
#include "MMP_control.h"
#include "middle_control.h"
#include "input_to_channel.h"
//��������ò���(2high,2 low)
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
	SceneConfiguration   	preset ;			//��������ѡ�
} VideoEnodeInfo;

/****************************************************************************
*  ��Ҫ������������صĲ�����
*         a) ¼�������ֱ��ʡ�����0/1����������ǰ����Ŀ��
*         b) ���е������Լ���Ӱģʽ�� ������Ҫ�����Ŀ�ߣ����ʣ���Ҫ����
*		  c) ������������������ߣ�֡�ʣ����ʡ�(���浽FLASH)
*         d) ҳ��������ֱ��ʣ�����ȫ�������Լ����������źͿ�� (���浽FLASH)
*
*   ���ȼ�: ¼�� > ���е� > ҳ�� �����е������Ƕ����
*
*****************************************************************************/

//scale ����
typedef struct __VIDEOSCALEINFO {
	int quality_lock_flag ;  		// 0 ��������������ģʽ 1 ������������ģʽ
	int record_lock_flag;      //  0 ������¼������ģʽ  1 ����¼������ģʽ
	int web_lock_flag ;        // 0  ������web����ģʽ  1 ����¼������ģʽ
	int low_lock_flag ;        // 0 ������С������ģʽ  1 ����С������ģʽ
#ifdef HAVE_IP_XML
	int ip_xml_lock_flag;     // 0  ��������1����
#endif
	int outwidth ;     			//�����
	int outheight ;     			//�����
	int scalemode ;   			 //0 -- 1����ģʽ
} VideoScaleInfo;




typedef struct __VideoCommonHandle {
	unsigned int profile; //encode profile ,hp,mp,bp
	VideoEnodeInfo info[MAX_CHANNEL];
	VideoScaleInfo sinfo[MAX_CHANNEL];
	mid_mutex_t  mutex[MAX_CHANNEL];  //������
	mid_mutex_t  smutex[MAX_CHANNEL];  //������
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


