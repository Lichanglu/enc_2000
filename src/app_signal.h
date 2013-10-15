#ifndef APP_SIGNAL_H
#define APP_SIGNAL_H
//#include <mcfw/src_linux/devices/inc/device.h>
#include "reach_system.h"


#define HAVE_ADJUST_YPBPR_HV
#define HAVE_ADJUST_SDI_HV


/*偏移量*/
typedef struct __HV__ {
	short	hporch;
	short	vporch;
} HVTable;

typedef struct __SIGNAL_INFO__
{
	char	name[32];
	UInt8	DeviceName[16];
	UInt32	ModeID;
	UInt32	digital; // tmds --> 1.digital   0 为模拟
	UInt32	hpv;
	UInt32	freq;
	UInt32	YPbPr;
	UInt32	HsfqFpga;
	UInt32	Fpga_version;
	UInt32  hdcp;
	UInt32  isInterlaced; // 1 为I   0 为P
}Signal_Info;

void init_HV_table(void);
int app_get_signal_info(int input, Signal_Info *info);
int app_web_get_signal_detail_info(int channel, char *out, int vallen);
int app_get_signal_resolution(int channel, char *out, int *outlen);

int app_get_hdcp(int input);
int get_input_type(int *out, int channel);

int app_web_set_input_type(int type, int *out, int channel);

int read_input_info(int *input1_mode, int *input2_mode,int* color1,int* color2);

int app_set_hv(int input, int h, int v,int save);

#ifdef  HAVE_ADJUST_SDI_HV
int app_enable_SDI_adjust_flag(int channel);
#endif

int read_CbCr_table(void);
int app_set_CbCr(int input);
int adjust_signal_CbCr(int input);

int detect_signal_info(int input,int isInterlaced);

int app_set_color_space(int channel,int value,int *out);
int app_get_color_space(int channel);

int set_CSC_value(int input,int value);


int  check_video_fps(int input,int fps);


int adjust_analog_signal_hv(int input);

int no_signal_clean_signal_info(int input);

void init_fpga_version();

void init_HV_table(void);

int read_CbCr_table(void);

int app_get_fpga_version();

void printf_signal_info(int input, Signal_Info *info);




#endif

