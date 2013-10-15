#ifndef _COMMON_H_
#define _COMMON_H_

//#include <mcfw/src_linux/osa/inc/osa_thr.h>

#include <assert.h>

#include "log.h"
#include "log_common.h"


#define 	ENCODE_TYPE					6

#define MAX_CLIENT				6
#define DSP_NUM				    4




typedef int SOCKET;
typedef int DSPFD;
typedef unsigned short WORD;
typedef unsigned int   DWORD;
typedef unsigned char  BYTE;


#if 1
typedef enum {
    DL_NONE,
    DL_ERROR,
    DL_WARNING,
    DL_FLOW,
    DL_DEBUG,
    DL_ALL,
} EDebugLevle;

#define DEBUG_LEVEL   	(DL_ALL)


#define DEBUG(x,fmt,arg...) \
	do {\
		if ((x)<DEBUG_LEVEL){\
			fprintf(stderr, fmt, ##arg);\
		}\
	}while (0)


#endif
#if 0
typedef struct _R_GPIO_data_ {
	unsigned int gpio_num;
	unsigned int gpio_value;
} R_GPIO_data;
#endif


/**/

#define NETWORK_CONFIG				(".config/network.dat")

#define PRODUCT_CONFIG  			("product.ini")
#define VERSION_CONFIG  			("version.dat")


#define LOGO_CONFIG 			(".config/logo_1260.ini")
#define TEXT_CONFIG				(".config/text.ini")

#define HV_TABLE_D_NAME         (".config/digital.ini")
#define HV_TABLE_A_NAME_1         (".config/analog.ini")
#define HV_TABLE_A_NAME_2         (".config/analog2.ini")
#define CbCr_TABLE_CFG		(".config/CbCrTable.ini")
#define HV_TABLE_SDI_NAME_1         ("sdi_config.ini")
#define HV_TABLE_SDI_NAME_2        ("sdi_config2.ini")
#define INPUT_CONFIG   			(".config/input.dat")
#define VIDEO_CONFIG            (".config/video.dat")
#define AUDIO_CONFIG            (".config/audio.dat")
#define MP_CONFIG               (".config/composite.dat")
#define ENCODING_CONFIG         (".config/encoding.dat")


#define  MIN_BITRATE_VALUE      (128)
#define  MIN_AAC_LC_BITRATE     (80000)

int ParseIDRHeader(unsigned char *pdata);
		
int getostime();

int ChangeSampleIndex(int index);

void SetEthConfig(unsigned int ipaddr, unsigned netmask, unsigned int gateway);

int CheckIPNetmask(int ipaddr, int netmask, int gw);

int set_gpio_value(int fd,unsigned int gnum, int gvalue);

int write_yuv_420(unsigned char *py, unsigned char *pu, unsigned char *pv, 
								int width, int height, int chid);

int write_yuv_422(unsigned char *py, unsigned char *pu, unsigned char *pv, 
								int width, int height, int chid);






#endif
