#ifndef OSD_H
#define OSD_H

#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <linux/watchdog.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <iconv.h>
#include <png.h>


#include "ti_media_std.h"
#include "dm6467_struct.h"

#include "middle_control.h"

#include "Font.h"
#include "screen.h"


#include "sd_demo_osd.h"

//#include "Text.h"
//#include "png.h"
//#include "logo.h"
#include "log_common.h"
#include "input_to_channel.h"

#define HAVE_OSD_MODULE    1


#define CHN_FONT_NAME        "data/fonts/simkai.ttf"

#define MSG_ADD_TEXT    33  //加入字幕
#define TEXT_BUFTYPE_NUMTYPES  2

//#define MAX_OSD_WIDTH           (16*2*32)


#define TEXT_BITMAP_WIDTH		OSD_MAX_WIDTH    //	(16*2*32)   //(16*2 *24) //8 * 4 * 50
#define TEXT_BITMAP_HEIGHT		64  //8 * 4 * 2

#define TIME_BITMAP_WIDTH		8 * 4 * 10    //8 * 4 * 12
#define TIME_BITMAP_HEIGHT		64//8 * 4 * 2

#define	PNG_MAX_WIDTH		64
#define	PNG_MAX_HEIGHT		64


#define	PNG_DF_WIDTH		64
#define	PNG_DF_HEIGHT		64



#define TIME_XPOS		(32*2)
#define TIME_YPOS		(32*2)
#define OSD_FONT_SIZE		23
#define OSD_TRANS_VALUE   0xFF

#define OSD_DISPLAY_VIEW   0x80
#define OSD_HIDE_VIEW      0



// 2lu video +1 lu sw video     
#define INPUT_TEXT_MAX_NUM     SIGNAL_INPUT_MAX


/*加字幕结构标题*/
typedef struct __RecAVTitle {
	int  x;         		//x pos
	int  y;        			//y pos
	int len;   					//Text实际长度
	char Text[128];			//text
} RecAVTitle;


#define LOGOFILE "/opt/dvr_rdk/ti816x_2.8/logo"



typedef enum __DisplayLogo_Text {
    NOdisplay = 0,
    OnlyShowText,
    OnlyShowLogo,
    BothShow,
} DisplayLogo_Text;


typedef struct _TEXTOSD_INFO_T_ {
	Buffer_Handle hBuf;
	TextInfo text;
} TextOsd_Info;


#if defined (__cplusplus)
extern "C" {
#endif

int create_TextTime_info(void);
int init_osd_info(void);
void osd_deinit();
 
void hide_text_view(int input);
int check_logo_png(int input, char *filename);
int add_logo_osd(int input, LogoInfo* lh);
int update_osd_view(int input,int have_signal);

int check_text_size(char* text_buf);


int add_text_info(int input, TextInfo *info);

int set_text_info(int input, TextInfo *newtextInfo);
int get_text_info(int input, TextInfo *text);
void set_time_display(int input, int enable,int alpha);
int get_time_display(int input);
int set_logo_info(int input, LogoInfo *newLogoInfo);
int get_logo_info(int input, LogoInfo *logo);
void hide_time_osd(int input);



#if defined (__cplusplus)
}
#endif




#endif
