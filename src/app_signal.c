
#include <stdio.h>//DEVICEDRV

#include "log.h"
#include "common.h"
#include "input_to_channel.h"
#include "log_common.h"
#include "ti_media_std.h"
#include "app_signal.h"
#include "rwini.h"
#include "reach_enc2000.h"
//#include "adv7441_priv.h"
#include <mcfw/src_linux/devices/inc/device.h>
#include "mcfw/src_linux/devices/inc/device_videoDecoder.h"
#include <mcfw/src_linux/devices/adv7441/src/adv7441_priv.h>
#include <mcfw/src_linux/devices/adv7441/inc/adv7441.h>

#include <mcfw/src_linux/devices/adv7442/src/adv7442_priv.h>
#include <mcfw/src_linux/devices/adv7442/inc/adv7442.h>

#include <mcfw/src_linux/devices/gs2971/src/gs2971_priv.h>
#include <mcfw/src_linux/devices/gs2971/inc/gs2971.h>
#include <mcfw/src_linux/devices/gs2972/src/gs2972_priv.h>
#include <mcfw/src_linux/devices/gs2972/inc/gs2972.h>

#include "mid_mutex.h"
//#include <gs2972_priv.h>
#include "app_head.h"


#define H_OFFSET_MAX  (100)
#define H_OFFSET_MIN  (-100)
#define V_OFFSET_MAX  (10)
#define V_OFFSET_MIN  (-10)



typedef struct _Signal_Table_ {
	Vps_Adv7441_hpv_vps signal_info;
	unsigned short res;
	unsigned short digital_val;
	unsigned short analog_val;
} Signal_Table;

typedef struct _HV_TABEL_ {
	char name[48];
	unsigned int h_pos;
	unsigned int width;
	unsigned int hight;
	unsigned int v_pos;
	int id;
} HV_Refer_Tabel;

static Signal_Info g_signal_info[SIGNAL_INPUT_MAX];


static HVTable g_HV_table[SIGNAL_INPUT_MAX][DEVICE_STD_REACH_LAST] = {0};

const static const Signal_Table s_signal_table[DEVICE_STD_REACH_LAST + 1] = {
	{{"480I",		263, 60, 1587, 720, 240}, DEVICE_STD_NTSC, 1, 1}, // 0-480ix60
	{{"576I",		313, 50, 1600, 720, 288}, DEVICE_STD_PAL, 1, 1}, // 1-576ix50
	{{"480I",		263, 60, 1587, 720, 240}, DEVICE_STD_480I, 1, 1}, // 2-480ix60
	{{"576I",		313, 50, 1600, 720, 288}, DEVICE_STD_576I, 1, 1}, // 3-576ix50
	{{"Revs",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_CIF, 1, 1}, // FVID2_STD_CIF, /**< Interlaced, 360x120 per field NTSC, 360x144 per field PAL. */
	{{"Revs",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_HALF_D1, 1, 1}, // FVID2_STD_HALF_D1, /**< Interlaced, 360x240 per field NTSC, 360x288 per field PAL. */
	{{"Revs",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_D1, 1, 1}, // FVID2_STD_D1, /**< Interlaced, 720x240 per field NTSC, 720x288 per field PAL. */
	{{"480P",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_480P, 1, 1}, // 7-480px60
	{{"576P",		0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_576P, 1, 1}, // 8-576px50

	{{"720P60",	750, 60, 1111, 1280, 720}, DEVICE_STD_720P_60, 1, 1}, // 9-1280x720x60
	{{"720P50",	750, 50, 1333, 1280, 720}, DEVICE_STD_720P_50, 1, 1}, // 10-1280x720x50
	{{"1080I60",	563, 60, 1481, 1920, 540}, DEVICE_STD_1080I_60, 1, 1}, // 11-1920x1080x60i
	{{"1080I50",	562, 50, 1777, 1920, 540}, DEVICE_STD_1080I_50, 1, 1}, // 12-1920x1080x50i
	{{"1080P60",	1125, 60, 740, 1920, 1080}, DEVICE_STD_1080P_60, 1, 1}, // 13-1920x1080x60p
	{{"1080P50",	1125, 50, 888, 1920, 1080}, DEVICE_STD_1080P_50, 1, 1}, // 14-1920x1080x50p
	{{"1080P25",	1125, 25, 1777, 1920, 1080}, DEVICE_STD_1080P_24, 1, 1}, // 15-1920x1080x25p
	{{"1080P30",	1125, 30, 1481, 1920, 1080}, DEVICE_STD_1080P_30, 1, 1}, // 16-1920x1080x30p

	{{"640x480@60",	525, 60, 1588, 640, 480}, DEVICE_STD_VGA_640X480X60, 1, 1}, // 17-640x480x60
	{{"640x480@72",	520, 72, 1320, 640, 480}, DEVICE_STD_VGA_640X480X72, 1, 1}, // 18-640x480x72
	{{"640x480@75",	500, 75, 1333, 640, 480}, DEVICE_STD_VGA_640X480X75, 1, 1}, // 19-640x480x75
	{{"640x480@85",	509, 85, 1155, 640, 480}, DEVICE_STD_VGA_640X480X85, 1, 1}, // 20-640x480x85
	{{"800x600@60",	628, 60, 1320, 800, 600}, DEVICE_STD_VGA_800X600X60, 1, 1}, // 21-800x600x60
	{{"800x600@72",	666, 72, 1040, 800, 600}, DEVICE_STD_VGA_800X600X72, 1, 1}, // 22-800x600x72
	{{"800x600@75",	625, 75, 1066, 800, 600}, DEVICE_STD_VGA_800X600X75, 1, 1}, // 23-800x600x75
	{{"800x600@85",	631, 85, 931, 800, 600}, DEVICE_STD_VGA_800X600X85, 1, 1}, // 24-800x600x85
	{{"1024x768@60",	806, 60, 1033, 1024, 768}, DEVICE_STD_VGA_1024X768X60, 1, 1}, // 25-1024x768x60
	{{"1024x768@70",	806, 70, 885, 1024, 768}, DEVICE_STD_VGA_1024X768X70, 1, 1}, // 26-1024x768x70
	{{"1024x768@75",	800, 75, 833, 1024, 768}, DEVICE_STD_VGA_1024X768X75, 1, 1}, // 27-1024x768x75
	{{"1024x768@85",	808, 85, 728, 1024, 768}, DEVICE_STD_VGA_1024X768X85, 1, 1}, // 28-1024x768x85
	{{"1280x720@60",	750, 60, 1111, 1280, 720}, DEVICE_STD_VGA_1280X720X60, 1, 1}, // 29-1280x720x60
	{{"1280x768@60",	798, 60, 1054, 1280, 768}, DEVICE_STD_VGA_1280X768X60, 1, 1}, // 30-1280x768x60
	{{"1280x768@75",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1280X768X75, 1, 1}, // 31-1280x768x75
	{{"1280x768@85",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1280X768X85, 1, 1}, // 32-1280x768x85
	{{"1280x800@60",	828, 60, 1006, 1280, 800}, DEVICE_STD_VGA_1280X800X60, 1, 1}, // 33-1280x800x60
	{{"1280x960@60",	1000, 60, 833, 1280, 960}, DEVICE_STD_VGA_1280X960X60, 1, 1}, // 34-1280x960x60
	{{"1280x1024@60",	1066, 60, 781, 1280, 1024}, DEVICE_STD_VGA_1280X1024X60, 1, 1}, // 35-1280x1024x60
	{{"1280x1024@75",	1066, 75, 625, 1280, 1024}, DEVICE_STD_VGA_1280X1024X75, 1, 1}, // 36-1280x1024x75
	{{"1280x1024@85",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1280X1024X85, 1, 1}, // 37-1280x1024x85
	{{"1366x768@60",	795, 60, 1047, 1366, 768}, DEVICE_STD_VGA_1366X768X60, 1, 1}, // 38-1366x768x60
	{{"1440x900@60",	934, 60, 901, 1440, 900}, DEVICE_STD_VGA_1440X900X60, 1, 1}, // 39-1440x900x60
	{{"1400x1050@60",	1089, 60, 765, 1400, 1050}, DEVICE_STD_VGA_1400X1050X60, 1, 1}, // 40-1400x1050x60
	{{"1400x1050@75",	0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_VGA_1400X1050X75, 1, 1}, // 41-1400x1050x75
	{{"1600x1200@60",	1250, 60, 666, 1600, 1200}, DEVICE_STD_VGA_1600X1200X60, 1, 1}, // 42-1600x1200x60
	{{"1920x1080@60_DMT", 1125, 60, 740, 1920, 1080}, DEVICE_STD_VGA_1920X1080X60_DMT, 1, 1}, // 43-1920x1080x60-DMT
	{{"1920x1080@60_GTF", 1125, 60, 740, 1920, 1080}, DEVICE_STD_VGA_1920X1080X60_GTF, 1, 1}, // 44-1920x1080x60-GTF
	{{"1920x1200@60",	1244, 60, 675, 1920, 1200}, DEVICE_STD_VGA_1920X1200X60, 1, 1}, // 45-1920x1200x60
	{{"2560x1440@60",	1481, 60, 0xFFFF, 2560, 1440}, DEVICE_STD_VGA_2560X1440X60, 1, 1}, // 46-2560x1440x60

	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_2CH_D1, 1, 1}, // FVID2_STD_MUX_2CH_D1,/**< Interlaced, 2Ch D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_2CH_HALF_D1, 1, 1}, // FVID2_STD_MUX_2CH_HALF_D1, /**< Interlaced, 2ch half D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_2CH_CIF, 1, 1}, // FVID2_STD_MUX_2CH_CIF, /**< Interlaced, 2ch CIF, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_4CH_D1, 1, 1}, // FVID2_STD_MUX_4CH_D1, /**< Interlaced, 4Ch D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_4CH_CIF, 1, 1}, // FVID2_STD_MUX_4CH_CIF, /**< Interlaced, 4Ch CIF, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_4CH_HALF_D1, 1, 1}, // FVID2_STD_MUX_4CH_HALF_D1, /**< Interlaced, 4Ch Half-D1, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_8CH_CIF, 1, 1}, // FVID2_STD_MUX_8CH_CIF, /**< Interlaced, 8Ch CIF, NTSC or PAL. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_MUX_8CH_HALF_D1, 1, 1}, // FVID2_STD_MUX_8CH_HALF_D1, /**< Interlaced, 8Ch Half-D1, NTSC or PAL. */

	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_AUTO_DETECT, 1, 1}, // FVID2_STD_AUTO_DETECT, /**< Auto-detect standard. Used in capture mode. */
	{{"Revs", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_CUSTOM, 1, 1}, // FVID2_STD_CUSTOM, /**< Custom standard used when connecting to external LCD etc...
	//The video timing is provided by the application.
	//Used in display mode. */

	{{"Max", 0xFFFF, 0xFFFF, 0xFFFF, 0, 0}, DEVICE_STD_REACH_LAST, 0, 0} // FVID2_STD_MAX
};

//模拟+分量 参照表
static const HV_Refer_Tabel const hv_refer_dvi_table[DEVICE_STD_REACH_LAST] = {
	{"480I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_NTSC},  			// 0-480ix60
	{"576I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_PAL},  			// 1-576ix50
	{"480I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_480I},  			// 2-480ix60
	{"576I",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_576I},  			// 3-576ix50
	{"Revs",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_CIF},				// FVID2_STD_CIF, /**< Interlaced, 360x120 per field NTSC, 360x144 per field PAL. */
	{"Revs",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_HALF_D1},  			// FVID2_STD_HALF_D1, /**< Interlaced, 360x240 per field NTSC, 360x288 per field PAL. */
	{"Revs",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_D1},  			// FVID2_STD_D1, /**< Interlaced, 720x240 per field NTSC, 720x288 per field PAL. */
	{"480P",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_480P},  			// 7-480px60
	{"576P",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_576P},				// 8-576px50
	//YPbPr
	{"720P60",	255, 1541, 748, 28},				// 9-1280x720x60
	{"720P50",	251, 1537, 746, 26},  			// 10-1280x720x50
	{"1080I60",	185, 2111, 560, 20},				// 11-1920x1080x60i
	{"1080I50",	186, 2112, 562, 22},				// 12-1920x1080x50i
	{"1080P60",	180, 2106, 1120, 40},				// 13-1920x1080x60p
	{"1080P50",	180, 2106, 1120, 40},				// 14-1920x1080x50p
	{"1080P25",	200, 2126, 1122, 42},				// 15-1920x1080x25p
	{"1080P30",	236, 2162, 1120, 42},				// 16-1920x1080x30p

	{"640x480@60",	135, 781, 525, 45, DEVICE_STD_VGA_640X480X60},				// 17 -640x480x60
	{"640x480@72",	160, 806, 508, 28, DEVICE_STD_VGA_640X480X72},				// 18 -640x480x72
	{"640x480@75",	164, 810, 502, 22, DEVICE_STD_VGA_640X480X75},				// 19 -640x480x75
	{"640x480@85",	126, 772, 500, 20, DEVICE_STD_VGA_640X480X85},				// 20 -640x480x85
	{"800x600@60",	202, 1008, 628, 28, DEVICE_STD_VGA_800X600X60}, 				// 21 -800x600x60
	{"800x600@72",	178, 984, 626, 26, DEVICE_STD_VGA_800X600X72},	 			// 22 -800x600x72
	{"800x600@75",	225, 1031, 625, 25, DEVICE_STD_VGA_800X600X75},	 			// 23 -800x600x75
	{"800x600@85",	200, 1006, 631, 31, DEVICE_STD_VGA_800X600X85},	 			// 24 -800x600x85
	{"1024x768@60",	280, 1310, 806, 38, DEVICE_STD_VGA_1024X768X60},				 // 25 -1024x768x60
	{"1024x768@70",	264, 1294, 806, 38, DEVICE_STD_VGA_1024X768X70},				 // 26 -1024x768x70
	{"1024x768@75",	260, 1290, 800, 32, DEVICE_STD_VGA_1024X768X75},				 // 27-1024x768x75
	{"1024x768@85",	292, 1322, 808, 40, DEVICE_STD_VGA_1024X768X85},		 		// 28-1024x768x85
	{"1280x720@60",	320, 1606, 750, 30, DEVICE_STD_VGA_1280X720X60},				 // 29-1280x720x60
	{"1280x768@60",	320, 1606, 798, 30, DEVICE_STD_VGA_1280X768X60},				 // 30-1280x768x60
	{"1280x768@75",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1280X768X75},			 // 31-1280x768x75
	{"1280x768@85",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1280X768X85},	 		// 32-1280x768x85
	{"1280x800@60",	330, 1616, 830, 30, DEVICE_STD_VGA_1280X800X60},	 			// 33-1280x800x60
	{"1280x960@60",	340, 1626, 990, 30, DEVICE_STD_VGA_1280X960X60},	 			// 34-1280x960x60

	{"1280x1024@60",	350, 1636, 1066, 42, DEVICE_STD_VGA_1280X1024X60},	 		// 35-1280x1024x60
	{"1280x1024@75",	376, 1662, 1066, 42, DEVICE_STD_VGA_1280X1024X75},	 		// 36-1280x1024x75
	{"1280x1024@85",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1280X1024X85},	 		// 37-1280x1024x85
	{"1366x768@60",	266, 1638, 788, 20, DEVICE_STD_VGA_1366X768X60},	 		   // 38-1366x768x60
	{"1440x900@60",	350, 1796, 930, 30, DEVICE_STD_VGA_1440X900X60},	 			// 39-1440x900x60
	{"1400x1050@60",	359, 1765, 1087, 37, DEVICE_STD_VGA_1400X1050X60},	 		// 40-1400x1050x60
	{"1400x1050@75",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_1400X1050X75},	 		// 41-1400x1050x75
	{"1600x1200@60",	480, 2086, 1250, 50, DEVICE_STD_VGA_1600X1200X60},  			 // 42-1600x1200x60
	{"1920x1080@60_DMT", 300, 2326, 1100, 20, DEVICE_STD_VGA_1920X1080X60_DMT},		// 43-1920x1080x60-DMT
	{"1920x1080@60_GTF", 100, 2026, 1090, 10, DEVICE_STD_VGA_1920X1080X60_GTF},		// 44-1920x1080x60-GTF
	{"1920x1200@60",	536, 2462, 1242, 38, DEVICE_STD_VGA_1920X1200X60},  			 // 45-1920x1200x60  XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	{"2560x1440@60",	0x00, 0x00, 0x00, 0x00, DEVICE_STD_VGA_2560X1440X60},			// 46-2560x1440x60

	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_2CH_D1}, // FVID2_STD_MUX_2CH_D1,/**< Interlaced, 2Ch D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_2CH_HALF_D1}, // FVID2_STD_MUX_2CH_HALF_D1, /**< Interlaced, 2ch half D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_2CH_CIF}, // FVID2_STD_MUX_2CH_CIF, /**< Interlaced, 2ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_4CH_D1}, // FVID2_STD_MUX_4CH_D1, /**< Interlaced, 4Ch D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_4CH_CIF}, // FVID2_STD_MUX_4CH_CIF, /**< Interlaced, 4Ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_4CH_HALF_D1}, // FVID2_STD_MUX_4CH_HALF_D1, /**< Interlaced, 4Ch Half-D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_8CH_CIF}, // FVID2_STD_MUX_8CH_CIF, /**< Interlaced, 8Ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_MUX_8CH_HALF_D1}, // FVID2_STD_MUX_8CH_HALF_D1, /**< Interlaced, 8Ch Half-D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_AUTO_DETECT}, // FVID2_STD_AUTO_DETECT, /**< Auto-detect standard. Used in capture mode. */
	{"Revs", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_CUSTOM}, // FVID2_STD_CUSTOM, /**< Custom standard used when connecting to external LCD etc...
	//The video timing is provided by the application.
	//Used in display mode. */
	{"Max", 0x00, 0x00, 0x00, 0x00, DEVICE_STD_REACH_LAST}, // FVID2_STD_MAX
};

#ifdef HAVE_ADJUST_SDI_HV
static HVTable g_sdi_HV[SIGNAL_INPUT_MAX][DEVICE_STD_REACH_LAST] = {0};
//SDI信号表
static Vps_Gs2972_sav_eav_vbi  hv_refer_sdi_table[DEVICE_STD_REACH_LAST] = {
	{"480I",		0x00, 0x00, 0x00, 0x00},  			// 0-480ix60
	{"576I",		0x00, 0x00, 0x00, 0x00},  			// 1-576ix50
	{"480I",		0x00, 0x00, 0x00, 0x00},  			// 2-480ix60
	{"576I",		0x00, 0x00, 0x00, 0x00},  			// 3-576ix50
	{"Revs",		0x00, 0x00, 0x00, 0x00},			// FVID2_STD_CIF, /**< Interlaced, 360x120 per field NTSC, 360x144 per field PAL. */
	{"Revs",		0x00, 0x00, 0x00, 0x00},  			// FVID2_STD_HALF_D1, /**< Interlaced, 360x240 per field NTSC, 360x288 per field PAL. */
	{"Revs",		0x00, 0x00, 0x00, 0x00},  			// FVID2_STD_D1, /**< Interlaced, 720x240 per field NTSC, 720x288 per field PAL. */
	{"480P",		0x00, 0x00, 0x00, 0x00},  			// 7-480px60
	{"576P",		0x00, 0x00, 0x00, 0x00},			// 8-576px50

	//sdi
	{"720P60",	365, 0x00, 0x00, 29},		// 9-1280x720x60
	{"720P50",	695, 0x00, 0x00, 29},  		// 10-1280x720x50
	{"1080I60",	275, 0x00, 0x00, 22},		// 11-1920x1080x60i
	{"1080I50",	715, 0x00, 0x00, 22},		// 12-1920x1080x50i
	{"1080P60",	274, 0x00, 0x00, 44},		// 13-1920x1080x60p
	{"1080P50",	714, 0x00, 0x00, 44},		// 14-1920x1080x50p
	{"1080P25",	715, 0x00, 0x00, 44},		// 15-1920x1080x25p
	{"1080P30",	275, 0x00, 0x00, 44},		// 16-1920x1080x30p

	{"640x480@60",	100, 746, 525, 45},				// 17 -640x480x60
	{"640x480@72",	100, 746, 520, 40},				// 18 -640x480x72
	{"640x480@75",	100, 746, 500, 20},				// 19 -640x480x75
	{"640x480@85",	100, 746, 509, 29},				// 20 -640x480x85
	{"800x600@60",	100, 906, 628, 28}, 				// 21 -800x600x60
	{"800x600@72",	100, 906, 666, 66},	 			// 22 -800x600x72
	{"800x600@75",	100, 906, 625, 25},	 			// 23 -800x600x75
	{"800x600@85",	100, 906, 631, 31},	 			// 24 -800x600x85
	{"1024x768@60",	100, 1130, 806, 38},				 // 25 -1024x768x60
	{"1024x768@70",	100, 1130, 806, 38},				 // 26 -1024x768x70
	{"1024x768@75",	100, 1130, 800, 32},				 // 27-1024x768x75
	{"1024x768@85",	100, 1130, 808, 40},		 		// 28-1024x768x85
	{"1280x768@60",	100, 1386, 798, 30},				 // 29-1280x768x60
	{"1280x768@75",	0x00, 0x00, 0x00, 0x00},			 // 30-1280x768x75
	{"1280x768@85",	0x00, 0x00, 0x00, 0x00},	 		// 31-1280x768x85
	{"1280x800@60",	200, 1486, 830, 30},	 			// 32-1280x800x60
	{"1280x960@60",	200, 1486, 990, 30},	 			// 33-1280x960x60
	{"1280x1024@60",	360, 1646, 1066, 42},	 		// 34-1280x1024x60
	{"1280x1024@75",	100, 1386, 1066, 42},	 		// 35-1280x1024x75
	{"1280x1024@85",	100, 1386, 1072, 48},	 		// 36-1280x1024x85
	{"1440x900@60",	100, 1506, 930, 30},	 			// 37-1440x900x60
	{"1400x1050@60",	100, 1506, 1087, 37},	 		// 38-1400x1050x60
	{"1400x1050@75",	100, 1506, 1096, 46},	 		// 39-1400x1050x75
	{"1600x1200@60",	480, 2086, 1250, 50},  			 // 40-1600x1200x60
	{"1920x1200@60",	536, 2462, 1242, 38},  			 // 41-1920x1200x60
	{"2560x1440@60",	0x00, 0x00, 0x00, 0x00},			// 42-2560x1440x60

	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_2CH_D1,/**< Interlaced, 2Ch D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_2CH_HALF_D1, /**< Interlaced, 2ch half D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_2CH_CIF, /**< Interlaced, 2ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_4CH_D1, /**< Interlaced, 4Ch D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_4CH_CIF, /**< Interlaced, 4Ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_4CH_HALF_D1, /**< Interlaced, 4Ch Half-D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_8CH_CIF, /**< Interlaced, 8Ch CIF, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_MUX_8CH_HALF_D1, /**< Interlaced, 8Ch Half-D1, NTSC or PAL. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_AUTO_DETECT, /**< Auto-detect standard. Used in capture mode. */
	{"Revs", 0x00, 0x00, 0x00, 0x00}, // FVID2_STD_CUSTOM, /**< Custom standard used when connecting to external LCD etc...
	//The video timing is provided by the application.
	//Used in display mode. */
	{"Max", 0x00, 0x00, 0x00, 0x00} // FVID2_STD_MAX
};
#endif

static mid_mutex_t mutex_signal;
static int set_HV_table(HVTable hv_table, int input, int id);
static int get_HV_table(HVTable *hv_table, int input, int id);
extern int capture_set_input_type(int input, int mode);
static int detect_signal_HDCP(int input, int *hdcp);

#ifdef  HAVE_ADJUST_SDI_HV
static int self_adjust_SDI_hv(int input);
#endif

int write_input_info(int input1_mode, int input2_mode)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	int 			rst = -1;
	char    		title[24];
	const char config_file[64] = INPUT_CONFIG;
	sprintf(title, "input");



	sprintf(temp, "%d", input1_mode);
	ret =  ConfigSetKey((char *)config_file, title, "Vp0", temp);

	if(ret != 0) {
		PRINTF("Failed to Get input \n");
		goto EXIT;
	}

	sprintf(temp, "%d", input2_mode);
	ret =  ConfigSetKey((char *)config_file, title, "Vp1", temp);

	if(ret != 0) {
		PRINTF("Failed to Get input \n");
		goto EXIT;
	}

	sprintf(temp, "%d", app_get_color_space(CHANNEL_INPUT_1));
	ret =  ConfigSetKey((char *)config_file, title, "color1", temp);

	if(ret != 0) {
		PRINTF("Failed to Get input \n");
		goto EXIT;
	}

	sprintf(temp, "%d", app_get_color_space(CHANNEL_INPUT_2));
	ret =  ConfigSetKey((char *)config_file, title, "color2", temp);

	if(ret != 0) {
		PRINTF("Failed to Get input \n");
		goto EXIT;
	}

	rst = 1;
	PRINTF("write finished!\n");
EXIT:
	return rst;
}

int read_input_info(int *input1_mode, int *input2_mode, int *color1, int *color2)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	int 			rst = -1;
	const char config_file[64] = INPUT_CONFIG;
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char    title[24];
	sprintf(title, "input");

	ret =  ConfigGetKey((char *)config_file, title, "Vp0", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get input \n");
		goto EXIT;
	}

	*input1_mode = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "Vp1", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get input \n");
		goto EXIT;
	}

	*input2_mode = atoi(temp);


	ret =  ConfigGetKey((char *)config_file, title, "color1", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get input \n");
		goto EXIT;
	}

	*color1 = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "color2", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get input \n");
		goto EXIT;
	}

	*color2 = atoi(temp);


	rst = 1;

	PRINTF("read finished!\n-----input1_have_signal=%d,input2_have_signal=%d\n",
	       *input1_mode, *input2_mode);
EXIT:
	return rst;
}

void printf_signal_info(int input, Signal_Info *info)
{
	PRINTF("\t***Signal Info %d***\n", input);
	PRINTF("\t===Device=%s,name=%s,ID=%d,Digtail=%d,fpga=0x%x,YPbPr=%d\n",
	       info->DeviceName, info->name, info->ModeID, info->digital, info->HsfqFpga,
	       info->YPbPr);
}

static int g_fpga_version = 0;
void init_fpga_version()
{
	if(g_fpga_version == 0) {	//check fpga version
		Device_Gs2971Obj gs1_obj;
		Device_VideoDecoderExternInforms signal_info;
		int cmd = IOCTL_DEVICE_VIDEO_DECODER_EXTERN_INFORM;
		memset(&signal_info, 0, sizeof(Device_VideoDecoderExternInforms));
		Device_gs2971Control(&gs1_obj, cmd, &signal_info, NULL);
		g_fpga_version = Device_gs2971GetFpagVersion();
		PRINTF("the fpag version is %d\n", g_fpga_version);
	}
}
int app_get_fpga_version()
{
	return g_fpga_version;
}

int app_get_signal_info(int input, Signal_Info *info)
{
	//	int i = 0;

	if(input < 0 || input > SIGNAL_INPUT_2) {
		PRINTF("WARNING: input =%d\n", input);
		return -1;
	}

	if(!info) {
		PRINTF("WARNING: info =%p\n", info);
		return -1;
	}

	mid_mutex_lock(mutex_signal);
	memcpy(info, &(g_signal_info[input]), sizeof(Signal_Info));
	mid_mutex_unlock(mutex_signal);

	return 0;
}

int no_signal_clean_signal_info(int input)
{
	mid_mutex_lock(mutex_signal);
	memset(&(g_signal_info[input]), 0, sizeof(Signal_Info));
	memcpy((g_signal_info[input].name), "NO Signal", sizeof(g_signal_info[input].name));
	g_signal_info[input].ModeID = -1;
	mid_mutex_unlock(mutex_signal);
	return 0;
}

static int app_set_signal_info(int input, int hdcp, UInt isInterlaced, char *name,
                               Device_VideoDecoderExternInforms *info)
{
	if(input < 0 || input > SIGNAL_INPUT_2) {
		PRINTF("WARNING: input =%d\n", input);
		return -1;
	}

	if(!info) {
		PRINTF("WARNING: info =%p\n", info);
		return -1;
	}

	mid_mutex_lock(mutex_signal);
	memcpy((g_signal_info[input].name), name, sizeof(g_signal_info[input].name));
	memcpy((g_signal_info[input].DeviceName), info->DeviceName, sizeof(g_signal_info[input].DeviceName));
	g_signal_info[input].freq = info->SignalFreq;
	g_signal_info[input].digital = info->SignalTmds;
	g_signal_info[input].hpv = info->SignalHpv;
	g_signal_info[input].Fpga_version = info->SignalHsfqFpga ;
	g_signal_info[input].YPbPr = info->SignalYPbPr;
	g_signal_info[input].HsfqFpga = info->SignalLinenumFpga;
	g_signal_info[input].ModeID = info->ModeID;
	g_signal_info[input].hdcp = hdcp;
	g_signal_info[input].isInterlaced = isInterlaced;
	mid_mutex_unlock(mutex_signal);

	return 0;
}

int detect_signal_info(int input, int isInterlaced)
{
	//	VCAP_VIDEO_SOURCE_CH_STATUS_S info;
	int cmd = IOCTL_DEVICE_VIDEO_DECODER_GET_VIDEO_STATUS;
	int ret = 0;
	Device_Adv7441Obj adv1_Obj ;
	Device_Adv7442Obj adv2_Obj ;
	Device_Gs2971Obj gs1_obj;
	Device_Gs2972Obj gs2_obj;
	char out[32];
	int input_tpye = capture_get_input_type(input);

	Device_VideoDecoderExternInforms signal_info;
	cmd = IOCTL_DEVICE_VIDEO_DECODER_EXTERN_INFORM;

	if(input_tpye == INPUT_DVI) {
		if(SIGNAL_INPUT_2 == input) {
			ret = Device_adv7442Control(&adv2_Obj, cmd, &signal_info, NULL);
		} else if(SIGNAL_INPUT_1 == input) {
			ret = Device_adv7441Control(&adv1_Obj, cmd, &signal_info, NULL);
		}

	} else if(INPUT_SDI == input_tpye) {
		if(SIGNAL_INPUT_2 == input) {
			ret = Device_gs2972Control(&gs2_obj, cmd, &signal_info, NULL);
		} else if(SIGNAL_INPUT_1 == input) {
			ret = Device_gs2971Control(&gs1_obj, cmd, &signal_info, NULL);
		}
	}

	if(signal_info.ModeID == -1) {
		//PRINTF("----signal_info.ModeID=%d\n", signal_info.ModeID);
		memcpy(out, "No Signal", 16);
	} else {
		//PRINTF("----signal_info.ModeID=%d\n", signal_info.ModeID);
		sprintf(out, "%s", s_signal_table[signal_info.ModeID].signal_info.name);
	}

	int hdcp = 0 ;
	detect_signal_HDCP(input, &hdcp);
	app_set_signal_info(input, hdcp, isInterlaced, out, &signal_info);

	//	PRINTF("%s\n", out);
	return signal_info.ModeID;
}
//SET DVI/SDI
int app_web_set_input_type(int type, int *out, int channel)
{
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	int type1 = 0, type2 = 0;
	int ret = 0;
	channel_get_input_info(channel, &input , &high);
	PRINTF("inputtype=%d\n", type);
	ret = capture_set_input_type(input, type);

	type1 = capture_get_input_type(SIGNAL_INPUT_1);
	type2 = capture_get_input_type(SIGNAL_INPUT_2);

	if(ret == 0) {
		write_input_info(type1, type2);
	}

	return 0;
}


//GET dvi /sdi
int get_input_type(int *out, int channel)
{
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	int type = 0;
	channel_get_input_info(channel, &input , &high);

	type = capture_get_input_type(input);

	PRINTF("input type = %d\n", type);
	*out = type;
	return 0;
}

int app_get_hdcp(int input)
{
	Signal_Info info;
	int ret = app_get_signal_info(input, &info);

	if(ret < 0) {
		return -1;
	}

	return info.hdcp;
}

static int detect_signal_HDCP(int input, int *hdcp)
{

	int ret = 0;
	int cmd = IOCTL_DEVICE_VIDEO_DECODER_GET_HDCP;
	Device_Adv7441Obj adv1_obj;
	Device_Adv7442Obj adv2_obj;
	//	Device_Gs2971Obj gs1_obj;
	//	Device_Gs2972Obj gs2_obj;
	int input_type = capture_get_input_type(input);


	if(input_type == INPUT_DVI) {
		if(SIGNAL_INPUT_1 == input) {
			ret = Device_adv7441Control(&adv1_obj, cmd, hdcp, NULL);
		} else if(SIGNAL_INPUT_2 == input) {
			ret = Device_adv7442Control(&adv2_obj, cmd, hdcp, NULL);
		}
	} else if(INPUT_SDI == input_type) {
		*hdcp = 0 ; // sdi的hdcp无效
		/*if(SIGNAL_INPUT_2 == input) {
			ret = Device_gs2972Control(&gs2_obj, cmd, hdcp, NULL);
		} else if(SIGNAL_INPUT_1 == input) {
			ret = Device_gs2971Control(&gs1_obj, cmd, hdcp, NULL);
		}
		*/
	}

	if(ret <  0) {
		//PRINTF("WARNING,get hdcp failed!\n");
		return -1;
	}

	//	PRINTF("get hdcp is %d \n", *hdcp);
	return 0;
}


int app_get_signal_resolution(int channel, char *out, int *outlen)
{
	int input = SIGNAL_INPUT_1, high = 0;
	Signal_Info info;

	if(out == NULL) {
		PRINTF("WARNING:out is NULL\n");
		return -1;
	}

	channel_get_input_info(channel, &input, &high);

	if(input >= SIGNAL_INPUT_MAX || high != HIGH_STREAM) {
		PRINTF("WARNING:input=%d \n", input);
		return -1;
	}

	app_get_signal_info(input, &info);
	memcpy(out, info.name, 32);
	PRINTF("%s\n", out);
	*outlen = strlen(out);
	return 0;
}

int app_web_get_signal_detail_info(int channel, char *out, int vallen)
{
#if 1
	int input = 0, high = HIGH_STREAM;
	//	int ret = 0;
	//	int hdcp = 0;
	char tmpbuf[128] = {0};
	channel_get_input_info(channel, &input, &high);

	if(input >= SIGNAL_INPUT_MAX || high != HIGH_STREAM) {
		PRINTF("WARNING:input=%d \n", input);
		return -1;
	}

	Signal_Info  info;

	app_get_signal_info(input, &info);
	printf_signal_info(input, &info);

	if(-1 ==  info.ModeID) {
		PRINTF("WARNING,get video signal_info failed!ModeID=%d\n", info.ModeID);
		sprintf(out, "Signal:\\t No Signal!\\n");
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "HPV:\\t  0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "TMDS:\\t  0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "VsyncF:\\t 0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "HDCP:\\t 0\\n");
		strcat(out, tmpbuf);
		memset(tmpbuf, 0, strlen(tmpbuf));
		sprintf(tmpbuf, "RGB_YPRPR: 0\\n");
		strcat(out, tmpbuf);

		vallen = strlen(out);
		return -1;
	}

	printf_signal_info(input, &info);

	sprintf(out, "Signal:\\t  %s\\n", info.name);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "HPV:\\t  %d\\n", info.hpv);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "TMDS:\\t  %d\\n", info.digital);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "VsyncF:\\t  %d\\n", s_signal_table[info.ModeID].signal_info.vps);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "HDCP:\\t  %d\\n", info.hdcp);
	strcat(out, tmpbuf);
	memset(tmpbuf, 0, strlen(tmpbuf));
	sprintf(tmpbuf, "RGB_YPRPR: %d\\n", info.YPbPr);
	strcat(out, tmpbuf);

	vallen = strlen(out);

	return 0;
#endif
}



/////HV value

//=============

/*read Hportch Vportch Table*/
static int read_HV_table(int input)
{
	short ret = 0, val = 0;
	char temp[512];
	HVTable pHV ;
	int i = 0;
	char config_file[128];

	if(SIGNAL_INPUT_1 == input) {
		memcpy(config_file, HV_TABLE_A_NAME_1, 128);
	} else if(SIGNAL_INPUT_2 == input) {
		memcpy(config_file, HV_TABLE_A_NAME_2, 128);
	}

	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		ret = val = 0 ;
		pHV.vporch = 0;
		pHV.hporch = 0;
		ret = ConfigGetKey(config_file, hv_refer_dvi_table[i].name, "hporch", temp); // 0

		if(ret == 0) {
			val = atoi(temp);
			pHV.hporch = val;
		}

		ret = val = 0 ;
		ret = ConfigGetKey(config_file, hv_refer_dvi_table[i].name, "vporch", temp);

		if(ret == 0) {
			val = atoi(temp);
			pHV.vporch = val;
		}

		if(pHV.vporch != 0 || pHV.hporch != 0) {
			PRINTF("id=%d,h=%d,v=%d\n", i, pHV.hporch, pHV.vporch);
			set_HV_table(pHV, input, i);
		}
	}

	return 0;
}

static int get_HV_table(HVTable *hv_table, int input, int id)
{
	if(hv_table == NULL) {
		PRINTF("WRANG:hv_table is NULL!");
		return -1;
	}

	hv_table->hporch = g_HV_table[input][id].hporch;
	hv_table->vporch = g_HV_table[input][id].vporch;

	return 0;
}


static int set_HV_table(HVTable hv_table, int input, int id)
{
	g_HV_table[input][id].hporch = hv_table.hporch;
	g_HV_table[input][id].vporch = hv_table.vporch;
	return 0;
}



/*save HV  Table*/
static int write_HV_table(int input)
{
	char  temp[512];
	HVTable pHV ;
	int ret;
	int i = 0;
	char config_file[128];

	if(SIGNAL_INPUT_1 == input) {
		memcpy(config_file, HV_TABLE_A_NAME_1, 128);
	} else if(SIGNAL_INPUT_2 == input) {
		memcpy(config_file, HV_TABLE_A_NAME_2, 128);
	}


	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		PRINTF("--%d--\n", i);
		get_HV_table(&pHV, input, i);

		if(pHV.hporch != 0 || pHV.vporch != 0) {
			PRINTF("pHV.hporch =%d\n", pHV.hporch);
			sprintf(temp, "%d", pHV.hporch);
			ret = ConfigSetKey(config_file, hv_refer_dvi_table[i].name, "hporch", temp); // 0

			if(ret != 0) {
				DEBUG(DL_ERROR, "%s!!\n", hv_refer_dvi_table[i].name);
				return -1;
			}

			PRINTF("pHV.vporch =%d\n", pHV.vporch);
			sprintf(temp, "%d", pHV.vporch);
			ret = ConfigSetKey(config_file, hv_refer_dvi_table[i].name, "vporch", temp);

			if(ret != 0) {
				DEBUG(DL_ERROR, "%s!!\n", hv_refer_dvi_table[i].name);
				return -1;
			}
		}
	}

	return 0;

}
static int check_HV_valid(int res, int h_pos, int v_pos)
{
	int base_h = 0, base_v = 0, offset = 0;
	base_h = hv_refer_dvi_table[res].h_pos;
	base_v = hv_refer_dvi_table[res].v_pos;

	if(h_pos != 0) {
		offset = h_pos - base_h;

		if(offset > H_OFFSET_MAX || offset < H_OFFSET_MIN) {
			PRINTF("WARNING:H value Too big! base_h= %d,h_pos=%d\n", base_h, h_pos);
			return -1;
		}

		PRINTF("h_pos - base_h=%d\n", offset);
	}

	if(v_pos != 0) {
		offset = v_pos - base_v;

		if(offset > V_OFFSET_MAX || offset < V_OFFSET_MIN) {
			PRINTF("WARNING:V value Too big! base_v= %d,v_pos=%d\n", base_v, v_pos);
			return -1;
		}

		PRINTF("v_pos - base_v=%d\n", offset);
	}

	return 0;
}

static int g_CbCr_status[2][DEVICE_STD_REACH_LAST] = {0};

static int get_CbCr_status(int input, int std)
{
	if(std < 0 || std >= DEVICE_STD_REACH_LAST) {
		return -1;
	}

	return g_CbCr_status[input][std];
}

static int set_CbCr_status(int input, int std, int status)
{
	if(std < 0 || std >= DEVICE_STD_REACH_LAST || (status != 0 && status != 1)) {
		return -1;
	}

	g_CbCr_status[input][std] = status;
	return 0;
}

static int write_CbCr_table(int input, int std)
{
	const char *config_file = CbCr_TABLE_CFG;
	int CbCr = 0;
	char  temp[8] = {0};
	char key[16] = {0};
	int ret = -1;

	if(std < 0 || std >= DEVICE_STD_REACH_LAST) {
		return 0;
	}

	CbCr = get_CbCr_status(input, std);
	sprintf(temp, "%d", CbCr);
	sprintf(key, "CbCr[%d]", input);
	ret = ConfigSetKey(config_file, s_signal_table[std].signal_info.name, (void *)key, (void *)temp); // 0

	if(ret != 0) {
		DEBUG(DL_ERROR, "[%s]\n", s_signal_table[std].signal_info.name);
		return -1;
	}

	return 0;
}


int read_CbCr_table(void)
{
	const char *config_file = CbCr_TABLE_CFG;
	//	int CbCr = 0;
	char  temp[8] = {0};
	char key[16] = {0};
	int ret = -1;
	int i = 0;
	int input = 0;

	if(access(CbCr_TABLE_CFG, F_OK) < 0) {
		PRINTF("%s is not existence\n", CbCr_TABLE_CFG);
		return -1;
	}

	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		for(input = 0; input < 2; input ++) {
			memset(temp, 0, 8);
			sprintf(key, "CbCr[%d]", input);
			ret = ConfigGetKey(config_file, s_signal_table[i].signal_info.name, key, (void *)temp); // 0

			if(ret != 0) {
				DEBUG(DL_ERROR, "[%s]\n", s_signal_table[i].signal_info.name);
				continue;
			}

			set_CbCr_status(input, i, atoi(temp));
		}
	}

	return 0;
}

int adjust_signal_CbCr(int input)
{
	PRINTF("input:[%d]\n", input);
	int input_type = capture_get_input_type(input);

	//only dvi need cbcr change
	if(input_type == INPUT_SDI) {
		return -1;
	}


	Signal_Info info;
	int id = 0;
	int status = 0;
	app_get_signal_info(input, &info);
	printf_signal_info(input, &info);
	id = info.ModeID;
	status = get_CbCr_status(input, id);

	if(status < 0) {
		return -1;
	}

	if(status) {
		if(SIGNAL_INPUT_1 == input) {
			cap_invert_cbcr_adv7441_HV(&gEnc2000.capLink.adv7441_phandle, 1);
		} else if(SIGNAL_INPUT_2 == input) {
			cap_invert_cbcr_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, 1);
		}
	}

	return 0;
}

int adjust_analog_signal_hv(int input)
{
	//	static char begin_id_set_cbcr[DEVICE_STD_REACH_LAST] = {0};
	HVTable hv_table;
	int id = 0;
	int ret = 0;
	//	char buf[64] = {0};
	int digital = 0 ;
	Signal_Info info;
	int h = 0, v = 0;
	int cbcr = 0; //偶
	int input_tpye = capture_get_input_type(input);
	app_get_signal_info(input, &info);
	printf_signal_info(input, &info);
	digital = info.digital;
	id = info.ModeID;

#ifdef HAVE_ADJUST_SDI_HV

	if(input_tpye == INPUT_SDI) {
		self_adjust_SDI_hv(input);
		return 0;
	}

#endif

	if(AD7441_YPbPr_INPUT == info.YPbPr) {
		PRINTF("NO define HAVE_ADJUST_YPBPR_HV!\n");

		if(DEVICE_STD_1080I_60 == info.ModeID || DEVICE_STD_1080I_50 == info.ModeID) {
			PRINTF("I signal!\n");
			return 0;
		}

#ifndef HAVE_ADJUST_YPBPR_HV
		return 0;
#endif
	}

	//数字信号不用调整 ，同时分量信号不让调整
	if(0 != digital || input_tpye != INPUT_DVI) {
		PRINTF("It's digital\n");
		return 0;
	}

	get_HV_table(&hv_table, input, id);
	h = hv_table.hporch - hv_refer_dvi_table[id].h_pos;
	v = hv_table.vporch - hv_refer_dvi_table[id].v_pos;
	PRINTF("id=%d,hporch=%d,vporch=%d\n", id, hv_table.hporch, hv_table.vporch);

	if(hv_table.hporch == 0 &&  hv_table.vporch == 0) {
		return -1;
	}

	cbcr = h % 2 ;

#if 0

	if(0 != cbcr && 0 == begin_id_set_cbcr[id]) {
		if(SIGNAL_INPUT_1 == input) {
			cap_invert_cbcr_adv7441_HV(&gEnc2000.capLink.adv7441_phandle, 1);
		} else if(SIGNAL_INPUT_2 == input) {
			cap_invert_cbcr_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, 1);
		}

		begin_id_set_cbcr[id] = 1;
	}

#endif

	if(input == SIGNAL_INPUT_2) { //adv7442 input1
		int new_pos = ((hv_table.hporch) << 16) | (hv_table.vporch);
		//must get signal is digtal or ananlog
		cap_set_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, new_pos);
	} else if(input == SIGNAL_INPUT_1) { //input1  7441
		int new_pos = ((hv_table.hporch) << 16) | (hv_table.vporch);
		cap_set_adv7441_HV(&gEnc2000.capLink.adv7441_phandle, new_pos);
	}

	return ret;
}

#ifdef  HAVE_ADJUST_SDI_HV
static int g_SDI_adjust_flag [SIGNAL_INPUT_MAX] = {0, 0, 0};
int app_enable_SDI_adjust_flag(int channel)
{
	int input = 0, high = 0;
	channel_get_input_info(channel, & input, &high);
	g_SDI_adjust_flag[input] = 1;
	PRINTF("g_SDI_adjust_flag[%d]=%d\n", input, g_SDI_adjust_flag[input]);
	return 0;
}

static int reset_SDI_adjust_flag(int input)
{
	int flag = 0;
	flag = g_SDI_adjust_flag[input];
	g_SDI_adjust_flag[input] = 0;
	return flag ;
}



static int get_SDI_HV(HVTable *hv_table, int id, int input)
{
	if(hv_table == NULL) {
		PRINTF("WRANG:hv_table is NULL!");
		return -1;
	}

	hv_table->hporch = g_sdi_HV[input][id].hporch;
	hv_table->vporch = g_sdi_HV[input][id].vporch;

	return 0;
}


static int set_SDI_HV(HVTable hv_table, int id, int input)
{
	g_sdi_HV[input][id].hporch = hv_table.hporch;
	g_sdi_HV[input][id].vporch = hv_table.vporch;
	return 0;
}

static int read_SDI_HV_table(int input)
{
	short ret = 0, val = 0;
	char temp[512];
	HVTable pHV ;
	int i = 0;
	char config_file[64] ;

	if(SIGNAL_INPUT_1 == input) {
		memcpy(config_file, HV_TABLE_SDI_NAME_1, 64);
	} else if(SIGNAL_INPUT_2 == input) {
		memcpy(config_file, HV_TABLE_SDI_NAME_2, 64);
	}

	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		ret = val = 0 ;
		pHV.vporch = 0;
		pHV.hporch = 0;
		ret = ConfigGetKey(config_file, hv_refer_sdi_table[i].name, "hporch", temp); // 0

		if(ret == 0) {
			val = atoi(temp);
			pHV.hporch = val;
		}

		ret = val = 0 ;
		ret = ConfigGetKey(config_file, hv_refer_sdi_table[i].name, "vporch", temp);

		if(ret == 0) {
			val = atoi(temp);
			pHV.vporch = val;
		}

		if(pHV.vporch != 0 || pHV.hporch != 0) {
			PRINTF("id=%d,h=%d,v=%d\n", i, pHV.hporch, pHV.vporch);
			set_SDI_HV(pHV, i, input);
		}
	}

	return 0;
}

/*save HV  Table*/
static int write_SDI_HV_table(int input)
{
	char  temp[512];
	HVTable pHV ;
	int ret;
	int i = 0;
	char config_file[64] ;

	if(SIGNAL_INPUT_1 == input) {
		memcpy(config_file, HV_TABLE_SDI_NAME_1, 64);
	} else if(SIGNAL_INPUT_2 == input) {
		memcpy(config_file, HV_TABLE_SDI_NAME_2, 64);
	}

	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		PRINTF("--%d--\n", i);
		get_SDI_HV(&pHV, i, input);

		if(pHV.hporch != 0 || pHV.vporch != 0) {
			PRINTF("pHV.hporch =%d\n", pHV.hporch);
			sprintf(temp, "%d", pHV.hporch);
			ret = ConfigSetKey(config_file, hv_refer_sdi_table[i].name, "hporch", temp); // 0

			if(ret != 0) {
				DEBUG(DL_ERROR, "%s!!\n", hv_refer_sdi_table[i].name);
				return -1;
			}

			PRINTF("pHV.vporch =%d\n", pHV.vporch);
			sprintf(temp, "%d", pHV.vporch);
			ret = ConfigSetKey(config_file, hv_refer_sdi_table[i].name, "vporch", temp);

			if(ret != 0) {
				DEBUG(DL_ERROR, "%s!!\n", hv_refer_sdi_table[i].name);
				return -1;
			}
		}
	}

	return 0;

}

int adjust_SDI_hv(int input, int h, int v)
{
	short  h_pos = 0, v_pos = 0;
	//	char buf[64];
	int pos = 0, new_pos = 0;
	int id = 0;
	int cmd = 0, ret = 0;
	//	int r1 = 0, r2 = 0;
	Signal_Info info;
	int save = 1;
	HVTable hv_table;
	int input_tpye = capture_get_input_type(input);

	if(input_tpye != INPUT_SDI) {
		PRINTF("Input is Not SDI!\n");
		return 0;
	}

	if(1 !=  reset_SDI_adjust_flag(input)) {
		PRINTF("It is not ReachWebAdmin!\n");
		return 0;
	}

	PRINTF("input=%d,h=%d,v=%d\n", input, h, v);
	app_get_signal_info(input, &info);
	printf_signal_info(input, &info);
	id = info.ModeID ;

	get_SDI_HV(&hv_table, id, input);

	if(SIGNAL_INPUT_1 == input) {
		cmd = IOCTL_DEVICE_VIDEO_DECODER_GET_DIRECTION;
		ret = Device_gs2971Control(gEnc2000.capLink.gs2971_phandle, cmd, &pos, NULL);

		if(ret < 0) {
			ERR_PRN(" Device_gs2971Control return error!\n");
			return -1;
		}

		h_pos = ((pos & 0xFFFF0000) >> 16) + h;
		v_pos = (pos & 0xFFFF) + v;

		PRINTF("old h =%d,v=%d,new:h_pos=%d,v_pos=%d\n", ((pos & 0xFFFF0000) >> 16), pos & 0xFFFF,  h_pos, v_pos);

		new_pos = ((h_pos) << 16) | v_pos;
		cmd = IOCTL_DEVICE_VIDEO_DECODER_SET_DIRECTION;
		ret = Device_gs2971Control(gEnc2000.capLink.gs2971_phandle, cmd, &new_pos, NULL);

		if(ret < 0) {
			ERR_PRN(" Device_gs2971Control return error!\n");
			return -1;
		}
	} else if(SIGNAL_INPUT_2 == input) {
		cmd = IOCTL_DEVICE_VIDEO_DECODER_GET_DIRECTION;
		ret = Device_gs2972Control(gEnc2000.capLink.gs2972_phandle, cmd, &pos, NULL);

		if(ret < 0) {
			ERR_PRN(" Device_gs2972Control return error!\n");
			return -1;
		}

		h_pos = ((pos & 0xFFFF0000) >> 16) + h;
		v_pos = (pos & 0xFFFF) + v;

		PRINTF("old:h =%d,v=%d, new:h_pos=%d,v_pos=%d\n", ((pos & 0xFFFF0000) >> 16), pos & 0xFFFF,  h_pos, v_pos);

		new_pos = ((h_pos) << 16) | v_pos;
		cmd = IOCTL_DEVICE_VIDEO_DECODER_SET_DIRECTION;
		ret = Device_gs2972Control(gEnc2000.capLink.gs2972_phandle, cmd, &new_pos, NULL);

		if(ret < 0) {
			ERR_PRN(" Device_gs2972Control return error!\n");
			return -1;
		}
	}

	if(id > 0 && save == 1) {
		//	get_SDI_HV(&hv_table, id);
		hv_table.hporch = h_pos ;
		hv_table.vporch = v_pos;
		set_SDI_HV(hv_table, id, input);
		write_SDI_HV_table(input);
	}

	PRINTF("OK!\n");
	return 0;

}


static int self_adjust_SDI_hv(int input)
{
	//	static char begin_id_set_cbcr[DEVICE_STD_REACH_LAST]={0};
	HVTable hv_table;
	int id = 0;
	int ret = 0;
	//	char buf[64] = {0};
	Signal_Info info;
	int h = 0, v = 0;
	int cbcr = 0; //偶
	int cmd = IOCTL_DEVICE_VIDEO_DECODER_SET_DIRECTION;
	int input_tpye = capture_get_input_type(input);
	app_get_signal_info(input, &info);
	printf_signal_info(input, &info);
	id = info.ModeID;

	if(input_tpye != INPUT_SDI) {
		PRINTF("It's INPUT_SDI\n");
		return 0;
	}

	get_SDI_HV(&hv_table, id, input);
	h = hv_table.hporch - hv_refer_sdi_table[id].sav;//h
	v = hv_table.vporch - hv_refer_sdi_table[id].e_vbi;//v
	PRINTF("id=%d,h=%d,v=%d\n", id, h, v);

	if((hv_table.vporch ==  hv_refer_sdi_table[id].e_vbi &&
	    hv_table.hporch ==  hv_refer_sdi_table[id].sav)
	   || (0 == hv_table.vporch &&  0 == hv_table.hporch)) {
		return -1;
	}

	if(input == SIGNAL_INPUT_2) { //adv7442 input1
		int new_pos = ((hv_table.hporch) << 16) | (hv_table.vporch);
		ret = Device_gs2972Control(gEnc2000.capLink.gs2972_phandle, cmd, &new_pos, NULL);
	} else if(input == SIGNAL_INPUT_1) { //input1  7441
		int new_pos = ((hv_table.hporch) << 16) | (hv_table.vporch);
		ret = Device_gs2971Control(gEnc2000.capLink.gs2971_phandle, cmd, &new_pos, NULL);
	}

	return ret;
}

#endif

int app_set_CbCr(int input)
{
	PRINTF("input:[%d]\n", input);
	int input_type = capture_get_input_type(input);

	//only dvi need cbcr change
	if(input_type == INPUT_SDI) {
		return -1;
	}

	Signal_Info info;
	int id = 0;
	int status = 0;
	app_get_signal_info(input, &info);
	printf_signal_info(input, &info);
	id = info.ModeID;
	status = get_CbCr_status(input, id);

	if(status < 0) {
		return -1;
	}

	if(SIGNAL_INPUT_1 == input) {
		cap_invert_cbcr_adv7441_HV(&gEnc2000.capLink.adv7441_phandle, 1);
	} else if(SIGNAL_INPUT_2 == input) {
		cap_invert_cbcr_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, 1);
	}

	set_CbCr_status(input, id, status ? 0 : 1);
	write_CbCr_table(input, id);
	return 0;
}

int app_set_hv(int input, int h, int v, int save)
{
	short  h_pos = 0, v_pos = 0;
	int id = 0;
	int ret = 0;
	//	char buf[64];
	int digital = 0  ;
	Signal_Info info;
	int input_tpye = capture_get_input_type(input);
	int pos = 0, new_pos = 0;
	PRINTF("move H=%d,V=%d \n", h, v);

	PRINTF("-input=%d-\n", input);
	app_get_signal_info(input, &info);
	printf_signal_info(input, &info);
	digital = info.digital;
	/*
	if(input == SIGNAL_INPUT_2) { //adv7442 input1

		PRINTF(".........\n");
		int temp_cmd = IOCTL_DEVICE_VIDEO_DECODER_INVERT_CBCR;
		cap_invert_cbcr_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, 1);

	}
	*/

#ifdef HAVE_ADJUST_SDI_HV

	if(input_tpye == INPUT_SDI) {
		PRINTF("signal is SDI!\n");
		adjust_SDI_hv(input, h, v);
		return 0;
	}

#endif

	//sdI 不让调整
	if(digital != 0) {
		PRINTF("signal is digital!\n");
		return 0;
	}

	if(AD7441_YPbPr_INPUT == info.YPbPr) {
		PRINTF("************Signal is AD7441_YPbPr_INPUT!\n");

		if(DEVICE_STD_1080I_60 == info.ModeID || DEVICE_STD_1080I_50 == info.ModeID) {
			PRINTF("I signal!\n");

			if(v != 0) {
				return 0;
			}
		}

#ifndef HAVE_ADJUST_YPBPR_HV
		return 0;
#endif
	}

	id = info.ModeID;

	if(input_tpye != INPUT_DVI) {
		PRINTF("signal is SDI!\n");
		return 0;
	}

#if 0

	//数字、分量、SDI不让调CBCR
	if(1 == h || -1 == h) {
		if(SIGNAL_INPUT_1 == input) {
			cap_invert_cbcr_adv7441_HV(&gEnc2000.capLink.adv7441_phandle, 1);
		} else if(SIGNAL_INPUT_2 == input) {
			cap_invert_cbcr_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, 1);
		}
	}

#endif
	PRINTF("input =%d digital=%d\n", input, digital);
	PRINTF("h=%d,v=%d\n", h, v);

	if(input == SIGNAL_INPUT_2) { //adv7442 input1
		cap_get_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, &pos);
		h_pos = (pos & 0xFFFF0000) >> 16;
		v_pos = pos & 0xFFFF;
		PRINTF("old:h_pos=%d,v_pos=%d,pos=%d\n", h_pos, v_pos, pos);
		new_pos = ((h_pos + h) << 16) | (v_pos + v);

		if(h == 0) {
			ret = check_HV_valid(id, 0, v_pos + v);
		} else if(v == 0) {
			ret = check_HV_valid(id, h_pos + h, 0);
		}

		if(ret < 0) {
			PRINTF("check_HV_valid failed!\n");
			return -1;
		}

		PRINTF("new:h_pos =%d,v_pos=%d\n", (h_pos + h), (v_pos + v));

		//must get signal is digtal or ananlog
		cap_set_adv7442_HV(&gEnc2000.capLink.adv7442_phandle, new_pos);
	} else if(input == SIGNAL_INPUT_1) { //input1  7441
		cap_get_adv7441_HV(&gEnc2000.capLink.adv7441_phandle, &pos);
		h_pos = (pos & 0xFFFF0000) >> 16;
		v_pos = pos & 0xFFFF;
		PRINTF("old:h_pos=%d,v_pos=%d,pos=%d\n", h_pos, v_pos, pos);
		new_pos = ((h_pos + h) << 16) | (v_pos + v);

		if(h == 0) {
			ret = check_HV_valid(id, 0, v_pos + v);
		} else if(v == 0) {
			ret = check_HV_valid(id, h_pos + h, 0);
		}

		PRINTF("new:h_pos =%d,v_pos=%d\n", (h_pos + h), (v_pos + v));

		if(ret < 0) {
			PRINTF("check_HV_valid failed!\n");
			return -1;
		}

		cap_set_adv7441_HV(&gEnc2000.capLink.adv7441_phandle, new_pos);
	}

	if(id > 0 && save == 1) {
		HVTable hv_table;
		get_HV_table(&hv_table, input, id);

		//模拟才保存
		if(digital == 0) {
			hv_table.hporch = h_pos + h;
			hv_table.vporch = v_pos + v;
			set_HV_table(hv_table, input, id);
			write_HV_table(input);

		}
	} else {
		PRINTF("get signal_info failed! save=%d\n", save);
	}

	sleep(1);
	return 0;
}

void init_HV_table(void)
{
	int i = 0 ;

	for(i = 0; i < SIGNAL_INPUT_MAX; i++) {
		memset(&g_signal_info[i], 0, sizeof(Signal_Info));
	}

	mutex_signal = mid_mutex_create();

	for(i = 0; i < DEVICE_STD_REACH_LAST; i++) {
		g_HV_table[SIGNAL_INPUT_1][i].hporch = 0;
		g_HV_table[SIGNAL_INPUT_1][i].vporch = 0;

		g_HV_table[SIGNAL_INPUT_2][i].hporch = 0;
		g_HV_table[SIGNAL_INPUT_2][i].vporch = 0;

#ifdef HAVE_ADJUST_SDI_HV
		g_sdi_HV[SIGNAL_INPUT_1][i].hporch = 0;
		g_sdi_HV[SIGNAL_INPUT_1][i].vporch = 0;

		g_sdi_HV[SIGNAL_INPUT_2][i].hporch = 0;
		g_sdi_HV[SIGNAL_INPUT_2][i].vporch = 0;
#endif
	}

	read_HV_table(SIGNAL_INPUT_1);
	read_HV_table(SIGNAL_INPUT_2);
#ifdef HAVE_ADJUST_SDI_HV
	read_SDI_HV_table(SIGNAL_INPUT_1);
	read_SDI_HV_table(SIGNAL_INPUT_2);
#endif

}
/*
*	源帧率未达到60，设源帧率
*/
int  check_video_fps(int input, int fps)
{
	int frame = 0;
	Signal_Info info;
	int id = -1;
	int multiple = 1;
	int new_frame = 0;
	PRINTF("fps=%d\n", fps);

	if(SIGNAL_INPUT_MP ==  input) {
		return fps;
	}

	app_get_signal_info(input, &info);
	id = info.ModeID;

	if(id < 0) {
		PRINTF("NO Signal!\n");
		frame = 30; //蓝屏给30
	} else {
		if(1 == info.isInterlaced) {
			multiple = 2;
		}

		frame = s_signal_table[id].signal_info.vps / multiple;
	}

	new_frame = (fps > frame) ? frame : fps;
	PRINTF("isInterlaced=%d,multiple=%d,signal fps=%d,set fps=%d,new_frame=%d\n",
	       info.isInterlaced, multiple, frame, fps, new_frame);
	return new_frame;
}

int set_CSC_value(int input, int value)
{
	Device_Adv7441Obj obj7441;
	Device_Adv7442Obj obj7442;
	int csc_value = 0xf1;
	int pri = 0xf1;
	int cmd = IOCTL_DEVICE_VIDEO_DECODER_CSC;

	if(1 == value) {
		pri = 0x02;
		csc_value = 0xf1; //auto
	} else if(0 == value) {
		pri = 0xf1;
		csc_value = 0x02; //off
	} else {
		pri = 0xf1;
		csc_value = 0x01;	//on
	}

	PRINTF("COLOR=%d\n", csc_value);

	if(input == SIGNAL_INPUT_1) {
		Device_adv7441Control(&obj7441, cmd, &pri, NULL);
		Device_adv7441Control(&obj7441, cmd, &csc_value, NULL);
	} else if(SIGNAL_INPUT_2 == input) {
		Device_adv7442Control(&obj7442, cmd, &pri, NULL);
		Device_adv7442Control(&obj7442, cmd, &csc_value, NULL);
	}

	return 0;
}
/*  set color space*/
int g_color_space[SIGNAL_INPUT_MAX] = {0};
int app_set_color_space(int channel, int value, int *out)
{
	int color = 0;
	int input = SIGNAL_INPUT_1, high = 0;
	channel_get_input_info(channel, &input, &high);

	PRINTF("value[%d]=%d,color=0x%x\n", input, value, color);
	set_CSC_value(input, value);

	if(g_color_space[input] != value) {
		g_color_space[input] = value;
		write_input_info(capture_get_input_type(SIGNAL_INPUT_1), capture_get_input_type(SIGNAL_INPUT_2));
	}

	*out = value;

	return 0;
}
int app_get_color_space(int channel)
{
	int input = SIGNAL_INPUT_1, high = 0;
	channel_get_input_info(channel, &input, &high);

	if(input < 0 || input > SIGNAL_INPUT_2) {
		PRINTF("WARNING:input=%d\n", input);
		return 0;
	}

	PRINTF("g_color_space[%d]=%d\n", input, g_color_space[input]);
	return g_color_space[input];
}

