

#include "osd.h"
//#include "ti_media_std.h"
#include "dm6467_struct.h"
#include "input_to_channel.h"
#include "Convert.h"
#include "middle_control.h"
#include "Font.h"
#include "screen.h"
#include "sd_demo_osd.h"
#include "log_common.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "png_2_yuv.h"
#include "iconv.h"

#include "app_text_logo.h"
#include "mid_mutex.h"
#include "app_mp_control.h"
#include "app_head.h"

typedef struct Png_Object {
	Int        w;
	Int        h;
	Int8      *rp;
	Int8      *gp;
	Int8      *bp;
	Int        lineLength;
} Png_Object;

typedef struct Png_Object *Png_Handle;

static mid_mutex_t mutex_osd = NULL;

static TextOsd_Info g_text_info[INPUT_TEXT_MAX_NUM];
static int g_display_time[INPUT_TEXT_MAX_NUM];
static LogoInfo gLogoinfo[INPUT_TEXT_MAX_NUM];

//static Png_Handle ghPng[INPUT_TEXT_MAX_NUM];
static Buffer_Handle t_hBuf = NULL;
static TextOsd_Info	g_timeosd_info[INPUT_TEXT_MAX_NUM];
static Font_Handle		g_chnfont 			= NULL;


void *
mymtom_malloc(unsigned int  size, const char *file, int line, const char *func)
{
	void *p;
	p = malloc(size);
	printf("%s:%d:%s:malloc(%ld): p=0x%lx\n",
	       file, line, func, size, (unsigned long)p);
	return p;
}

#define   MALLOC(s)     mymtom_malloc(s,__FILE__,__LINE__,__FUNCTION__)

#define  FREE(p)   do{\
		printf("%s:%d:%s:free(0x%lx)\n",__FILE__,__LINE__,\
		       __FUNCTION__,(unsigned long)p);\
		free(p);\
	}while(0)

static void add_time_osd(int input, int hide);
static int copy_text_global(Font_Handle hFont, Char *txt, Int fontHeight, Int x, Int y,
                            Buffer_Handle hBuf);

static int get_text_size(char *Data);


int get_logo_info(int input, LogoInfo *logo)
{
	if(logo == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		PRINTF("ERROR:newLogoInfo = %p,input=%d\n", logo, input);
		return -1;
	}

	// check
	mid_mutex_lock(mutex_osd);
	memcpy(logo, &(gLogoinfo[input]),  sizeof(LogoInfo));
	mid_mutex_unlock(mutex_osd);
	return 0;
}

int set_logo_info(int input, LogoInfo *newLogoInfo)
{
	if(newLogoInfo == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		PRINTF("ERROR:newLogoInfo = %p\n", newLogoInfo);
		return -1;
	}

	// check
	mid_mutex_lock(mutex_osd);
	memcpy(&(gLogoinfo[input]), newLogoInfo, sizeof(LogoInfo));
	mid_mutex_unlock(mutex_osd);
	return 0;
}


int get_time_display(int input)
{
	if((input > INPUT_TEXT_MAX_NUM || input < 0)) {
		PRINTF("ERROR:input = %d\n", input);
		return -1;
	}

	return g_display_time[input];
}

void set_time_display(int input, int enable, int alpha)
{
	if((input > INPUT_TEXT_MAX_NUM || input < 0)) {
		PRINTF("ERROR:input = %d\n", input);
		return ;
	}

	PRINTF("display the time enable=%d,alpha=%d\n", enable, alpha);
	g_display_time[input] = enable;
	g_timeosd_info[input].text.alpha = alpha ;

	if(!enable) {
		hide_time_osd(input);
	}

	return ;
}


int get_text_info(int input, TextInfo *text)
{
	if(text == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		PRINTF("ERROR:newtextInfo = %p,input=%d\n", text, input);
		return -1;
	}

	mid_mutex_lock(mutex_osd);
	memcpy(text, &(g_text_info[input].text),  sizeof(TextInfo));
	mid_mutex_unlock(mutex_osd);
	return 0;
}

int set_text_info(int input, TextInfo *newtextInfo)
{
	if(newtextInfo == NULL || (input > INPUT_TEXT_MAX_NUM || input < 0)) {
		PRINTF("ERROR:newtextInfo = %p,input=%d\n", newtextInfo, input);
		return -1;
	}

	mid_mutex_lock(mutex_osd);
	memcpy(&(g_text_info[input].text), newtextInfo, sizeof(TextInfo));
	mid_mutex_unlock(mutex_osd);
	return 0;
}

/*
*“ÚŒ™OSDº”…œÀı∑≈«∞√Ê
*∫œ≥… «‘⁄1920*1080…œ◊ˆµƒÀı∑≈
*∑«∫œ≥… «‘⁄‘¥µƒ∑÷±Ê¬ …œ◊ˆµƒÀı∑≈
*/
static void  get_osd_width_height(int input, int *width, int *height)
{
	if(input != SIGNAL_INPUT_MP) {
		capture_get_input_hw(input, width, height);
	} else {
		*width = 1920;
		*height = 1080;
	}
}
static int check_CHN(char *text, int size)
{
	int i;

	for(i = 0; i < size; i++)
		if(text[i] > 127) {
			return 1;
		}

	return 0;
}
//◊Ó¥Û 23∏ˆ∫∫◊÷£¨46∏ˆ◊÷Ω⁄
//”¢Œƒ
#define CHINA_MAX_LEN    (46)
#define ENG_MAX_LEN      (46)
int check_text_size(char *text_buf)
{
	int charlen = strlen(text_buf);
	char temp[64] = {0};

	if(check_CHN(text_buf, charlen)) {
		PRINTF("Text is Chinese!\n");

		if(get_text_size(text_buf) > CHINA_MAX_LEN) {
			memcpy(temp, text_buf, CHINA_MAX_LEN);
			memset(text_buf, 0, charlen);
			memcpy(text_buf, temp, CHINA_MAX_LEN);
		}
	} else {
		PRINTF("Text isn't Chinese!\n");

		if(charlen > ENG_MAX_LEN) {
			memcpy(temp, text_buf, ENG_MAX_LEN);
			memset(text_buf, 0, charlen);
			memcpy(text_buf, temp, ENG_MAX_LEN);
		}
	}

	return 0;
}
static int get_text_size(char *Data)
{
	int n_Len = strlen(Data);
	unsigned char *lpData = (unsigned char *)Data;
	int i = 0;
	int Sum = 0;

	for(i = 0; /**pStr != '\0'*/ i < n_Len ;) {
		if(*lpData <= 127) {
			i++;
			lpData++;
		} else {
			i += 2;
			lpData += 2;
		}

		Sum++;
	}

	return i;
}

static int check_text_pos(int input, TextInfo *text, int text_width)
{
	int new_xpos = 0, new_ypos = 0;
	int width = 0 , height = 0;
	//	int temp = 0;
	get_osd_width_height(input, &width, &height);

	PRINTF("width*height:%dX%d,text_width=%d,old_xpos=%d\n", width, height, text_width);

	if(text->xpos + text_width > width) {
		new_xpos = width - text_width;
		PRINTF(">width,new_xpos=%d\n", new_xpos);
	} else {
		new_xpos = text->xpos;
		PRINTF("<width,new_xpos=%d\n", new_xpos);
	}

	text->xpos = new_xpos;

	if(height == 540) {
		height = height * 2;
	}

	if(text->ypos + TEXT_BITMAP_HEIGHT + 1 > height) {
		new_ypos = height - TEXT_BITMAP_HEIGHT;
		PRINTF(">height,new_ypos=%d\n", new_ypos);
	} else {
		new_ypos =  text->ypos;
		PRINTF("<width,new_ypos=%d\n", new_ypos);
	}

	text->ypos = new_ypos;
	return 0;
}




/*¥˙¬Î◊™ªª:¥”“ª÷÷±‡¬Î◊™Œ™¡Ì“ª÷÷±‡¬Î*/
static int code_convert(char *from_charset, char *to_charset, char *inbuf, int inlen, char *outbuf, int outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;
	cd = iconv_open(to_charset, from_charset);

	if((iconv_t) - 1 == cd) {
		PRINTF("open the char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	memset(outbuf, 0, outlen);

	if(iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)&outlen) == (size_t) - 1) {
		PRINTF("conver char set failed,errno = %d,strerror(errno) = %s \n", errno, strerror(errno));
		return -1;
	}

	iconv_close(cd);
	return 0;
}
extern void get_rtc_clock(struct rtc_time *rtc_tm);
/*µ√µΩœµÕ≥ ±º‰*/
static void get_sys_time(DATE_TIME_INFO *dtinfo)
{
	long ts;
	struct tm *ptm = NULL;
#if 1
	ts = time(NULL);
	ptm = localtime((const time_t *)&ts);
	dtinfo->year = ptm->tm_year + 1900;
	dtinfo->month = ptm->tm_mon + 1;
	dtinfo->mday = ptm->tm_mday;
	dtinfo->hours = ptm->tm_hour;
	dtinfo->min = ptm->tm_min;
	dtinfo->sec = ptm->tm_sec;
#else
	struct rtc_time rtc_tm;
	get_rtc_clock(&rtc_tm);
	dtinfo->year = rtc_tm.tm_year + 1900;
	dtinfo->month = rtc_tm.tm_mon + 1;
	dtinfo->mday = rtc_tm.tm_mday;
	dtinfo->hours = rtc_tm.tm_hour;
	dtinfo->min = rtc_tm.tm_min;
	dtinfo->sec = rtc_tm.tm_sec;
#endif

}

static unsigned char *time_yuyv_buf = NULL;
static void add_time_osd(int input, int hide)
{
	char  text[128] = {0};
	DATE_TIME_INFO info;
	TextInfo  *time = &(g_timeosd_info[input].text);
	Buffer_Handle hbuf = t_hBuf ;//g_timeosd_info[0].hBuf;//π≤”√“ª∏ˆhBuf
	//	unsigned char *yuyv_buf ;
	get_sys_time(&info);
	sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", info.year, info.month, info.mday, info.hours, info.min, info.sec);

	Screen_clear(hbuf, 0, 0, TIME_BITMAP_WIDTH, TIME_BITMAP_HEIGHT);
	copy_text_global(g_chnfont, text, OSD_FONT_SIZE, 40, 40, hbuf);

	time->xpos = 200;
	time->ypos = 300;

	if(hide == OSD_HIDE_VIEW) {
		time->alpha = hide;
	}

	if(IS_MP_STATUS == get_mp_status()) {
		BufferGfx_Dimensions dim;
		BufferGfx_getDimensions(hbuf, &dim);
		/*		yuyv_buf = (unsigned char *)malloc(dim.width * dim.height * 2);

				if(yuyv_buf == NULL) {
					ERR_PRN("yuyv_buf is NULL\n");
					return ;
				}
		*/
		time->width = dim.width;
		time->height = dim.height;
		yuvuv420_2_yuyv422(time_yuyv_buf, hbuf->buf, dim.width , dim.height);

		hbuf->bufsize = dim.width * dim.height * 2;
		memcpy(hbuf->buf, time_yuyv_buf, hbuf->bufsize);
		SD_subtitle_osdUpdate(input, WINDOW_TIME_OSD, hbuf->buf, hbuf->bufsize, time);

		return ;
	}

	SD_subtitle_osdUpdate(input, WINDOW_TIME_OSD, hbuf->buf, hbuf->bufsize, time);
	return ;
}

void hide_time_osd(int input)
{
	char  text[128] = {0};
	DATE_TIME_INFO info;
	TextInfo  time;
	Buffer_Handle hbuf = t_hBuf;
	memcpy(&time, &(g_timeosd_info[input].text), sizeof(TextInfo));
	//	unsigned char *yuyv_buf ;
	get_sys_time(&info);
	sprintf(text, "%04d/%02d/%02d %02d:%02d:%02d", info.year, info.month, info.mday, info.hours, info.min, info.sec);

	Screen_clear(hbuf, 0, 0, TIME_BITMAP_WIDTH, TIME_BITMAP_HEIGHT);
	copy_text_global(g_chnfont, text, OSD_FONT_SIZE, 40, 40, hbuf);

	time.xpos = 200;
	time.ypos = 300;

	time.alpha = 0;

	if(IS_MP_STATUS == get_mp_status()) {
		BufferGfx_Dimensions dim;
		BufferGfx_getDimensions(hbuf, &dim);

		time.width = dim.width;
		time.height = dim.height;
		yuvuv420_2_yuyv422(time_yuyv_buf, hbuf->buf, dim.width , dim.height);

		hbuf->bufsize = dim.width * dim.height * 2;
		memcpy(hbuf->buf, time_yuyv_buf, hbuf->bufsize);
		SD_subtitle_osdUpdate(input, WINDOW_TIME_OSD, hbuf->buf, hbuf->bufsize, &time);

		return ;
	}

	SD_subtitle_osdUpdate(input, WINDOW_TIME_OSD, hbuf->buf, hbuf->bufsize, &time);
	return ;

}

/*ÃÌº” ±º‰◊÷ƒªœﬂ≥Ã*/
static void time_osd_thread(void *pParam)
{
	int i = 0;

	while(1) {
		for(i = 0; i < INPUT_TEXT_MAX_NUM; i++) {
			while(SWITCH_STATUS == get_mp_status()) {
				sleep(1);
			}

			if(g_display_time[i] != 0) {
				add_time_osd(i, OSD_DISPLAY_VIEW);
			}
		}

		sleep(1);
	}

}

void hide_text_view(int input)
{
	TextInfo text;
	int ret = 0;
	Buffer_Handle hbuf = g_text_info[input].hBuf;
	ret = get_text_info(input, &text);

	if(ret < 0) {
		PRINTF("get text failed!\n");
		return ;
	}

	text.alpha = 0;
	PRINTF("hbuf->bufsize=%d,x/y =%d/%d\n", hbuf->bufsize, text.xpos, text.ypos);
	SD_subtitle_osdUpdate(input, WINDOW_TEXT_OSD, hbuf->buf, hbuf->bufsize, &text);
}

static unsigned char *text_yuyv_buf = NULL;
static void display_text_view(int input, const char *text_buff, int len, TextInfo *text)
{
	BufferGfx_Dimensions dim;
	Buffer_Handle hbuf = g_text_info[input].hBuf;
	PRINTF("input=%d\n", input);

	Screen_clear(hbuf, 0, 0, TEXT_BITMAP_WIDTH, TEXT_BITMAP_HEIGHT);
	copy_text_global(g_chnfont, (char *)text_buff, OSD_FONT_SIZE, 32, 32, hbuf);
	BufferGfx_getDimensions(hbuf, &dim);

	if(SIGNAL_INPUT_MP == input) {
		/*
				unsigned char *yuyv_buf = (unsigned char *)malloc(dim.width * dim.height * 2);

				if(yuyv_buf == NULL) {
					ERR_PRN("yuyv_buf is NULL\n");
					return ;
				}
		*/
		yuvuv420_2_yuyv422(text_yuyv_buf, hbuf->buf, dim.width, dim.height);

		if(len != 0) {
			text->width  =  16 * len;

			if(text->width > TEXT_BITMAP_WIDTH) {
				text->width  = TEXT_BITMAP_WIDTH;
			}
		}

		text->height = dim.height;
		hbuf->bufsize = dim.width * dim.height * 2;
		memcpy(hbuf->buf, text_yuyv_buf, hbuf->bufsize);
		SD_subtitle_osdUpdate(input, WINDOW_TEXT_OSD, hbuf->buf, hbuf->bufsize , text);

	} else {
		if(len != 0) {
			text->width   =  16 * (len);
		}

		if(text->width > TEXT_BITMAP_WIDTH) {
			text->width  = TEXT_BITMAP_WIDTH;
		}

		text->height = dim.height;
		SD_subtitle_osdUpdate(input, WINDOW_TEXT_OSD, hbuf->buf, hbuf->bufsize , text);
	}
}


/*
* ‘⁄∫œ≥…1ƒ£ Ωœ¬Œ¥ƒ‹…Ë÷√layout≥ˆœ÷corrupted double linked list¥ÌŒÛ
* “Ú¥À‘⁄µ⁄“ª¥Œ≤ª◊ˆ…Ë÷√layout
*/
static int update_mp_layout_1(int *start_flag)
{
	int mp_status = get_mp_status();
	Mp_Info info;
	sleep(1);

	if(IS_MP_STATUS == mp_status &&
	   1 == *start_flag &&
	   MP_LAYOUT_1 == get_mp_layout()) {
		app_web_get_mp_info(&info);
		app_web_set_mp_layout(info.layout, info.win1, info.win2);
	}

	if(IS_MP_STATUS == mp_status &&
	   MP_LAYOUT_1 == get_mp_layout()) {
		*start_flag = 1;
	}

	PRINTF("start_flag=%d\n", *start_flag);

	return 0;
}

/*ÃÌº”◊÷ƒª*/
int add_text_info(int input, TextInfo *info)
{
	static int mp_layout_1_flag = 0;
	update_mp_layout_1(&mp_layout_1_flag);
	int charlen = strlen(info->msgtext);
	PRINTF("x:%d,y:%d,enable:%d,showtime:%d,text=%s\n",
	       info->xpos, info->ypos, info->enable, info->showtime, info->msgtext);

	if(charlen != 0 && info->enable != 0) {
		PRINTF("display text\n");
		int size_len = get_text_size(info->msgtext);

		if(check_CHN(info->msgtext, charlen)) {
			char temp[512] = {0};
			PRINTF("check_CHN is CHN!\n");
			code_convert("gb2312", "utf-32", info->msgtext, charlen, temp, charlen * 4);

			if(info->postype == BOTTOM_RIGHT || info->postype == TOP_RIGHT
			   || info->postype == BOTTOM_LEFT) {
				check_text_pos(input, info, size_len * 16);
			}

			PRINTF("text charlen=%d,size_len=%d\n", charlen, size_len);
			display_text_view(input, temp, size_len, info);
		} else {
			if(info->postype == BOTTOM_RIGHT || info->postype == TOP_RIGHT) {
				check_text_pos(input, info, charlen * 16);
			}

			PRINTF("text charlen=%d,size_len=%d\n", charlen, size_len);
			display_text_view(input, info->msgtext, charlen, info);
		}
	}

	return 0;
}


static int copy_text_global(Font_Handle hFont, Char *txt, Int fontHeight, Int x, Int y,
                            Buffer_Handle hBuf)
{
	BufferGfx_Dimensions dim;
	FT_Error error;
	Int index;
	Int width, height;
	Int i, j, p, q, x_max, y_max, sp;
	UInt32 srcValue, dstValue;
	UInt8 *src;
	FT_Vector pen;
	FT_Face face = (FT_Face) hFont->face;
	Convert_Mode cMode;
	Int inc, start;
	UInt8 *ptr = NULL, *rowPtr;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	UInt32 TestCharSet = 0;
	UInt32 nTestFF = 0;
	UInt32 nTestDD = 0;
	wchar_t *wcTxt = 0;

	//int *wcTxt = 0;
	memcpy((Char *)&TestCharSet, txt, 4);
	nTestFF = TestCharSet & 0xFFFF0000;
	nTestDD = TestCharSet & 0x0000FFFF;

	if(nTestFF == 0 && nTestDD > 0) {
		memcpy((Char *)&TestCharSet, txt + 4, 4);
		nTestFF = TestCharSet & 0xFFFF0000;
		nTestDD = TestCharSet & 0x0000FFFF;

		if(nTestFF == 0 && nTestDD > 0) {
			wcTxt = (wchar_t *)txt + 1;
		}
	}

	BufferGfx_getDimensions(hBuf, &dim);
	//face->glyph->bitmap.num_grays =100;
	FT_Set_Char_Size(face,
	                 fontHeight << 6,
	                 fontHeight << 6,
	                 100,
	                 100);

	Convert_init();

	//	PRINTF("copy_text_global() =%d, not supporting image color format=%p=%d\n", sizeof(wchar_t),hBuf, BufferGfx_getColorSpace(hBuf));



	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_2BIT) {
		cMode = Convert_Mode_RGB888_2BIT;
		start = 65;
		inc =  80;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
		start = 80;
		inc = 80;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
		start = 0;
		inc = 0;
	} else {
		//PRINTF("copy_text_global() not supporting image color format=%d\n", BufferGfx_getColorSpace(hBuf));
		return SW_EFAIL;
	}

	pen.x = x << 6;
	pen.y = (dim.height - y + start) << 6;
	pen.x = 0;


	while(wcTxt ? *wcTxt : *txt) {
		FT_Set_Transform(face, NULL, &pen);

		index = FT_Get_Char_Index(face, wcTxt ? *wcTxt : *txt);

		error = FT_Load_Glyph(face, index, 0);

		if(error) {
			PRINTF("Failed to load glyph for character %c\n", wcTxt ? *wcTxt : *txt);
			break;
		}

		if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);

			if(error) {
				PRINTF("Failed to render glyph for character %c\n", wcTxt ? *wcTxt : *txt);
				break;
			}
		}

		width = face->glyph->bitmap.width;
		height = face->glyph->bitmap.rows;

		x = face->glyph->bitmap_left;
		y = dim.height + inc - face->glyph->bitmap_top;
		x_max = x + width;
		y_max = y + height;


		src = face->glyph->bitmap.buffer;

		//PRINTF("ptr=%p=width=%d,height=%d\n", ptr, width, height);
		ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;

		for(j = y, q = 0; j < y_max; j++, q++) {
			rowPtr = ptr + Screen_getInc(colorSpace, x);

			for(i = x, p = 0; i < x_max; i++, p++) {

				if(i >= dim.width || j >= dim.height) {
					continue;
				}

				sp = q * width + p;
				//if(src[sp]!=0)
				//src[sp]=~src[sp];
				//if(src[sp] ==255)
				//src[sp]=src[sp] &30;
				//PRINTF("src[sp]=%d\n",src[sp]);
				srcValue = src[sp] << 16 | src[sp] << 8 | src[sp];
				dstValue = Convert_execute(cMode, srcValue, i & 1);

				Screen_setPixelPtr(rowPtr, bufSize, i, dstValue,
				                   TRUE, colorSpace);
				rowPtr += Screen_getStep(colorSpace, i);
			}

			ptr += dim.lineLength;
		}

		wcTxt ? wcTxt++ : txt++;
		// txt++;

		pen.x += face->glyph->advance.x;
		pen.y += face->glyph->advance.y;

	}

	return SW_EOK;
}

#if 0
static Int text_string_len(Font_Handle hFont, Char *txt, Int fontHeight,
                           Int *stringWidth, Int *stringHeight)
{
	Int index;
	Int width = 0;
	Int height = 0;
	FT_Vector pen;
	FT_Face face = (FT_Face) hFont->face;
	UInt32 TestCharSet = 0;
	UInt32 nTestFF = 0;
	UInt32 nTestDD = 0;
	wchar_t *wcTxt = 0;
	//int *wcTxt = 0;
	FT_Set_Char_Size(face,
	                 fontHeight << 6,
	                 fontHeight << 6,
	                 100,
	                 100);

	pen.x = 0;
	pen.y = 0;
	memcpy((Char *)&TestCharSet, txt, 4);
	nTestFF = TestCharSet & 0xFFFF0000;
	nTestDD = TestCharSet & 0x0000FFFF;

	if(nTestFF == 0 && nTestDD > 0) {
		memcpy((Char *)&TestCharSet, txt + 4, 4);
		nTestFF = TestCharSet & 0xFFFF0000;
		nTestDD = TestCharSet & 0x0000FFFF;

		if(nTestFF == 0 && nTestDD > 0) {
			wcTxt = (wchar_t *)txt;
		}
	}

	while(wcTxt ? *wcTxt : *txt) {
		FT_Set_Transform(face, NULL, &pen);
		index = FT_Get_Char_Index(face, wcTxt ? *wcTxt : *txt);

		if(FT_Load_Glyph(face, index, 0)) {
			PRINTF("Failed to load glyph for character %c\n", wcTxt ? *wcTxt : *txt);
			break;
		}

		if(face->glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			if(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
				PRINTF("Failed to render glyph for character %c\n", wcTxt ? *wcTxt : *txt);
				break;
			}
		}

		width += face->glyph->bitmap.width + 1;

		if(face->glyph->bitmap.rows > height) {
			height = face->glyph->bitmap.rows;
		}

		pen.x += face->glyph->advance.x;
		pen.y += face->glyph->advance.y;
		(wcTxt != NULL) ? wcTxt++ : txt++;
	}

	*stringWidth = width;
	*stringHeight = height;

	return SW_EOK;
}
#endif
//============

static int Png_checkHead(unsigned char *head, int length)
{
	//int ret = 0;
	int i;
	unsigned char fixhead[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

	if(length < 33) {
		return 1;
	}

	for(i = 0; i < 8; i++) {

		if(fixhead[i] != head[i]) {
			return 1;
		}
	}

	if(head[25] == 3) {
		return 1;
	}

	return 0;

}

/*
static unsigned char *yuv422_buf ;
static unsigned char  *yuv420_buf;
*/
static unsigned char  *logo_yuv_buf = NULL;

static int Png_create(int input, char *fileName, int show, LogoInfo *logo)
{
	//	Png_Handle hPng;
	unsigned char *row_pointers[720]; // TODO large enough?
	unsigned char head[33]; //pngÕ∑
	//    Int r, g, b;
	Int  y;//Int x;
	png_structp png_ptr;
	png_infop info_ptr;
	png_uint_32 width, height;
	Int bit_depth, color_type, interlace_type;
	FILE *infile;
	int len = 0;
	int ret = 0;

	unsigned char *yuv422_buf = NULL ;
	unsigned char  *yuv420_buf = NULL;
	//	unsigned char  *yuv420sp_buf;

	PRINTF("fileName=%s\n", fileName);

	Convert_init();
	infile = fopen(fileName, "rb");

	if(infile == NULL) {
		ERR_PRN("Failed to open image file [%s]\n", fileName);
		ret = -1;
		return ret;
	}

	if(fread(head, 1, sizeof(head), infile) < sizeof(head)) {
		ERR_PRN("Failed fread png head=%d\n", sizeof(head));
		fclose(infile);
		ret = -1;
		return ret;
	}

	fseek(infile, 0L, SEEK_SET);

	if(Png_checkHead(head, sizeof(head))) {
		ERR_PRN("Png head error\n");
		fclose(infile);
		ret = -3;
		return ret;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if(png_ptr == NULL) {
		ERR_PRN("Failed to create read struct\n");
		fclose(infile);
		ret = -1;
		return ret;
	}

	info_ptr = png_create_info_struct(png_ptr);

	if(info_ptr == NULL) {
		ERR_PRN("Failed to create info struct\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(infile);
		//free(hPng);
		ret = -1;
		return ret;
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		ERR_PRN("Failed png_jmpbuf\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
		fclose(infile);
		//free(hPng);
		ret = -1;
		return ret;
	}

	png_init_io(png_ptr, infile);

	png_read_info(png_ptr, info_ptr);
	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
	             &interlace_type, int_p_NULL, int_p_NULL);

	png_set_strip_16(png_ptr);
	png_set_strip_alpha(png_ptr);
	png_set_packing(png_ptr);
	png_set_packswap(png_ptr);

	if(color_type == PNG_COLOR_TYPE_PALETTE) {
		png_set_palette_to_rgb(png_ptr);
	}

	if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
		png_set_gray_1_2_4_to_8(png_ptr);
	}

	for(y = 0; y < height; y++) {
		row_pointers[y] = png_malloc(png_ptr,
		                             png_get_rowbytes(png_ptr, info_ptr));
	}

	png_read_image(png_ptr, row_pointers);
	png_read_end(png_ptr, info_ptr);

	len = width * height;

	if(width > PNG_MAX_WIDTH || height > PNG_MAX_HEIGHT) {
		PRINTF("png Too big!\n");
		ret = -4;
		goto DELETE_PNG;
	}

	if(width % 8 != 0) {
		PRINTF("png is width%8!=0 width=%d!\n", width);
		ret = -5;
		goto DELETE_PNG;
	}

	PRINTF("logo size=%dx%d\n", width , height);

	if(show == 1) {
		char res[32] = {0};
		//		char yuv_name[32] = {0};
		sprintf(res, "%dx%d", width, height);

		if(logo->x % 2 != 0) {
			logo->x -= 1;
		}

		if(SIGNAL_INPUT_MP == input) {
			rgb_2_yuyv422(logo_yuv_buf, row_pointers, width, height);
			PRINTF("-------\n");
			//	snprintf(yuv_name,32,"%s_yuyv422.yuv",res);
			//	write_yuv4xx_file(logo_yuv_buf, height* width*2,yuv_name);
			int png_buf_len = width * height * 2;

			logo->height = height ;
			logo->width = width;

			ret = SD_logo_osdUpdate(input, 2, logo_yuv_buf, png_buf_len, logo);
			goto DELETE_PNG;
		}

		PRINTF("higth=%d,width=%d\n", width, height);
		yuv422_buf = (unsigned char *)MALLOC(len * 2);

		if(yuv422_buf == NULL) {
			PRINTF("Error =%d\n", len * 2);
			ret = -1;
			goto DELETE_PNG;
		}

		PRINTF("yuv422_buf..\n");
		rgb_2_yuv422(yuv422_buf, row_pointers, width, height);
		//snprintf(yuv_name,32,"%s_yuv422.yuv",res);
		//write_yuv4xx_file(yuv422_buf, height* width*2,yuv_name);

		PRINTF("yuv420_buf malloc---\n");
		yuv420_buf = (unsigned char *)MALLOC(len * 2);

		if(!yuv420_buf) {
			PRINTF("yuv420_buf:MALLOC failed!\n");
			ret = -1;
			goto DELETE_PNG;
		}

		PRINTF("yuv422_2_yuv420----\n");
		yuv422_2_yuv420(yuv420_buf, yuv422_buf, width, height);
		//snprintf(yuv_name,32,"%s_yuv420.yuv",res);
		//write_yuv4xx_file(yuv420_buf, height* width*3/2,yuv_name);

		PRINTF("yuv420p_2_yuv420sp----\n");
		yuv420p_2_yuv420sp(logo_yuv_buf, yuv420_buf, width, height);
		//		snprintf(yuv_name,32,"%s_yuv420sp.yuv",res);
		//		write_yuv420sp_file(logo_yuv_buf, width, height,yuv_name);

		int png_buf_len = len * 3 / 2;

		logo->height = height;
		logo->width = width;

		ret = SD_logo_osdUpdate(input, 2, logo_yuv_buf, png_buf_len, logo);

		PRINTF("....free yuv422_buf=%p buff!\n", yuv422_buf);

		if(yuv422_buf) {
			FREE(yuv422_buf);
		}

		PRINTF("....FREE yuv420_buf=%p buff!\n", yuv420_buf);

		if(yuv420_buf) {
			FREE(yuv420_buf);
		}

		PRINTF("....FREE yuv buff is NULL!\n");
		yuv422_buf = NULL;
		yuv420_buf = NULL;
		//		yuv420sp_buf = NULL;

	}

DELETE_PNG:

	for(y = 0; y < height; y++) {
		png_free(png_ptr, row_pointers[y]);
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

	fclose(infile);

	PRINTF("Png create success\n");
	return ret;
}


#if 0
static Void Png_show(Png_Handle hPng, Int x, Int y, Buffer_Handle hBuf)
{
	Int   minWidth;
	Int   minHeight;
	Int32 srcValue, dstValue;
	Int   i, j, pos;
	Convert_Mode cMode;
	UInt8 *ptr, *rowPtr;
	BufferGfx_Dimensions dim;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	BufferGfx_getDimensions(hBuf, &dim);
	ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;
	ptr += Screen_getInc(colorSpace, x);

	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
	} else {
		ERR_PRN("Png_show() not supporting image color format\n");
		return;
	}

	minWidth = dim.width < hPng->w ? dim.width : hPng->w;
	minHeight = dim.height < hPng->h ? dim.height : hPng->h;

	for(j = 0; j < minHeight; j++) {
		rowPtr = ptr;

		for(i = 0; i < minWidth; i++) {
			pos = j * hPng->w + i;
			srcValue = ((hPng->rp[pos] & 0xff) << 16) |
			           ((hPng->gp[pos] & 0xff) << 8) |
			           (hPng->bp[pos] & 0xff);
			dstValue = Convert_execute(cMode, srcValue, (i + x) & 1);
			Screen_setPixelPtr(rowPtr, bufSize, i + x, dstValue,
			                   FALSE, colorSpace);
			rowPtr += Screen_getStep(colorSpace, i + x);
		}

		ptr += dim.lineLength;
	}
}

static Void Png_showFilt(Png_Handle hPng, Int x, Int y, Buffer_Handle hBuf, int Filt)
{
	Int   minWidth;
	Int   minHeight;
	Int32 srcValue, dstValue;
	Int   i, j, pos;
	Convert_Mode cMode;
	UInt8 *ptr, *rowPtr;
	BufferGfx_Dimensions dim;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	BufferGfx_getDimensions(hBuf, &dim);
	ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;
	ptr += Screen_getInc(colorSpace, x);

	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
	} else {
		ERR_PRN("Png_show() not supporting image color format\n");
		return;
	}

	minWidth = dim.width < hPng->w ? dim.width : hPng->w;
	minHeight = dim.height < hPng->h ? dim.height : hPng->h;

	for(j = 0; j < minHeight; j++) {
		rowPtr = ptr;

		for(i = 0; i < minWidth; i++) {
			pos = j * hPng->w + i;
			srcValue = ((hPng->rp[pos] & 0xff) << 16) |
			           ((hPng->gp[pos] & 0xff) << 8) |
			           (hPng->bp[pos] & 0xff);

			if(srcValue == Filt) {
				rowPtr += Screen_getStep(colorSpace, i + x);
				continue;
			}

			dstValue = Convert_execute(cMode, srcValue, (i + x) & 1);
			Screen_setPixelPtr(rowPtr, bufSize, i + x, dstValue,
			                   FALSE, colorSpace);
			rowPtr += Screen_getStep(colorSpace, i + x);
		}

		ptr += dim.lineLength;
	}
}

static void Png_getWH(Png_Handle hPng, int *w, int *h)
{
	*w = hPng->w;
	*h = hPng->h;
	return;
}

static Void Png_showTransparency(Png_Handle hPng, Int x, Int y, Buffer_Handle hBuf, int isThrough, int Filt, int bl)
{
	Int   minWidth;
	Int   minHeight;
	Int32 srcValue, dstValue;
	Int   i, j, pos;
	Convert_Mode cMode;
	UInt8 *ptr, *rowPtr;
	UInt8 by, bcbcr, fy, fcbcr, ry, rcbcr;
	by = bcbcr = fy = fcbcr = ry = rcbcr = 0;
	float alpha = (float)bl / (float)100;
	BufferGfx_Dimensions dim;
	Int32 bufSize = Buffer_getSize(hBuf);
	ColorSpace_Type colorSpace = BufferGfx_getColorSpace(hBuf);

	BufferGfx_getDimensions(hBuf, &dim);
	ptr = (UInt8 *) Buffer_getUserPtr(hBuf) + y * dim.lineLength;
	ptr += Screen_getInc(colorSpace, x);

	if(BufferGfx_getColorSpace(hBuf) == ColorSpace_YUV422PSEMI) {
		cMode = Convert_Mode_RGB888_YUV422SEMI;
	} else if(BufferGfx_getColorSpace(hBuf) == ColorSpace_RGB565) {
		cMode = Convert_Mode_RGB888_RGB565;
	} else {
		ERR_PRN("Png_show() not supporting image color format\n");
		return;
	}

	minWidth = dim.width < hPng->w ? dim.width : hPng->w;
	minHeight = dim.height < hPng->h ? dim.height : hPng->h;

	for(j = 0; j < minHeight; j++) {
		rowPtr = ptr;

		for(i = 0; i < minWidth; i++) {
			pos = j * hPng->w + i;
			srcValue = ((hPng->rp[pos] & 0xff) << 16) |
			           ((hPng->gp[pos] & 0xff) << 8) |
			           (hPng->bp[pos] & 0xff);

			if((srcValue == Filt) && (isThrough == 1)) {
				rowPtr += Screen_getStep(colorSpace, i + x);
				continue;
			}

			dstValue = Convert_execute(cMode, srcValue, (i + x) & 1);
			fy = (dstValue >> 8) & 0xff;
			fcbcr = dstValue & 0xff;
			Screen_getPixelPtr(rowPtr, bufSize, i + x, &by, &bcbcr, FALSE, colorSpace);
			ry = (fy - 16) * (1 - alpha) + (by - 16) * (alpha) + 16;
			rcbcr = (fcbcr - 128) * (1 - alpha) + (bcbcr - 128) * (alpha) + 128; //¬ªÏªè
			dstValue = ry;
			dstValue <<= 8;
			dstValue |= rcbcr;
			Screen_setPixelPtr(rowPtr, bufSize, i + x, dstValue, FALSE, colorSpace);
			rowPtr += Screen_getStep(colorSpace, i + x);
		}

		ptr += dim.lineLength;
	}
}



static Int Png_delete(Png_Handle hPng)
{
	if(hPng) {
		if(hPng->rp) {
			FREE(hPng->rp);
		}

		if(hPng->gp) {
			FREE(hPng->gp);
		}

		if(hPng->bp) {
			FREE(hPng->bp);
		}

		FREE(hPng);
	}

	return SW_EOK;
}

#endif


//error code -4 invaild file -3 size to big
int check_logo_png(int input, char *filename)
{
	LogoInfo logo;
	int ret = 0;
	PRINTF("filename=%s\n", filename);
	ret = Png_create(input, filename, 0, &logo);

	if(ret < 0) {
		PRINTF("Png create failed!\n");
		return ret ;//-3 IS no png -4 too big
	}

	return ret;
}



void hide_logo_view(int input)
{
	LogoInfo logo ;
	int ret = get_logo_info(input, &logo);
	char filename[64] = {0};
	sprintf(filename, "%s", logo.filename);
	PRINTF("failename=%s,input=%d\n", filename, input);
	logo.alpha = 0;
	ret  = Png_create(input, filename, 1, &logo);
}

int  add_logo_osd(int input, LogoInfo *logo)
{
	int ret = 0;
	char filename[64] = {0};
	static int mp_layout_1_flag = 0;
	update_mp_layout_1(&mp_layout_1_flag);

	PRINTF("logo->enable=%d!\n", logo->enable);
	sprintf(filename, "%s", logo->filename);
	ret  = Png_create(input, filename, logo->enable, logo);
	return ret;
}


int update_osd_view(int input, int have_signal)
{
	//	int input = SIGNAL_INPUT_1, high = HIGH_STREAM;
	//	channel_get_input_info(VP, &input, &high);
	TextInfo text;
	LogoInfo logo ;
	int ret = 0;
	PRINTF("update input=%d,have_signal=%d\n", input, have_signal);

	ret = get_text_info(input, &text);

	if(ret < 0) {
		PRINTF("get text failed!\n");
		return -1;
	}

	ret = get_logo_info(input, &logo);

	if(ret < 0) {
		PRINTF("get logo failed!\n");
		return -1;
	}

	if(1 == have_signal) {
		if(text.enable) {
			add_text_info(input, &text);
		}

		if(logo.enable) {
			add_logo_osd(input, &logo);
		}
	} else {
		if(logo.enable) {
			hide_logo_view(input);
		}

		if(text.enable) {
			hide_text_view(input);
		}

		if(text.showtime) {
			hide_time_osd(input);
		}

	}
}


void osd_deinit()
{
	if(g_chnfont) {
		Font_delete(g_chnfont);
	}

	g_chnfont = NULL;
}

int create_TextTime_info(void)
{
	Buffer_Handle hbuf;
	Int                 bufSize;
	BufferGfx_Attrs     gfxAttrs  ;
	pthread_t		drawtimethread ;
	int i = 0;

	memset(&gfxAttrs, 0, sizeof(BufferGfx_Attrs));
	gfxAttrs.colorSpace = ColorSpace_NOTSET;

	g_chnfont = Font_create(CHN_FONT_NAME);

	if(g_chnfont == NULL) {
		PRINTF("Failed to create font %s\n", CHN_FONT_NAME);
		return -1;
	}

	/*¥¥Ω®textµƒBuffer*/
	gfxAttrs.dim.width      = TEXT_BITMAP_WIDTH;
	gfxAttrs.dim.height     = TEXT_BITMAP_HEIGHT;
	gfxAttrs.dim.x 					= 0 ;
	gfxAttrs.dim.y 					= 0;
	gfxAttrs.dim.lineLength = ColorSpace_getBpp(ColorSpace_YUV422PSEMI) * TEXT_BITMAP_WIDTH / 8;
	gfxAttrs.colorSpace     = ColorSpace_YUV422PSEMI;
	bufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 2;
	PRINTF("bufsize =%d\n", bufSize);

	for(i = 0; i < INPUT_TEXT_MAX_NUM; i++) {
		memset(&g_text_info[i], 0, sizeof(TextOsd_Info));
		hbuf = Buffer_create(bufSize, &gfxAttrs);

		if(hbuf == NULL) {
			PRINTF("Failed to allocate bitmap buffer of size %d\n", bufSize);
			return -1;
		}

		g_display_time[i] = 0;

		g_text_info[i].hBuf = hbuf;
		g_text_info[i].text.xpos = 0;
		g_text_info[i].text.ypos = 0;
		g_text_info[i].text.width = TEXT_BITMAP_WIDTH;
		g_text_info[i].text.height = TEXT_BITMAP_HEIGHT;
		//	g_text_info[i].text.show = 0;

		Screen_clear(hbuf, 0, 0, TEXT_BITMAP_WIDTH, TEXT_BITMAP_HEIGHT);
		hbuf = NULL;

		g_timeosd_info[i].text.xpos = TIME_XPOS;
		g_timeosd_info[i].text.ypos = TIME_YPOS;
		g_timeosd_info[i].text.width = TIME_BITMAP_WIDTH;
		g_timeosd_info[i].text.height = TIME_BITMAP_HEIGHT;
		g_timeosd_info[i].hBuf = NULL;
	}

	/*¥¥Ω®timerµƒBuffer*/
	gfxAttrs.dim.width      = TIME_BITMAP_WIDTH;
	gfxAttrs.dim.height     = TIME_BITMAP_HEIGHT;
	gfxAttrs.dim.lineLength = ColorSpace_getBpp(ColorSpace_YUV422PSEMI) * TIME_BITMAP_WIDTH / 8;
	gfxAttrs.colorSpace     = ColorSpace_YUV422PSEMI;
	bufSize = gfxAttrs.dim.lineLength * gfxAttrs.dim.height * 2;
	PRINTF("12 bufsize =%d\n", bufSize);
	t_hBuf = Buffer_create(bufSize, &gfxAttrs);

	if(t_hBuf == NULL) {
		PRINTF("Failed to allocate bitmap buffer of size %d\n", bufSize);
		return -1;
	}


	//	g_timeosd_info.text.show = 0;
	Screen_clear(t_hBuf, 0, 0, TIME_BITMAP_WIDTH, TIME_BITMAP_HEIGHT);

	pthread_create(&drawtimethread, NULL, (void *)time_osd_thread, (void *)NULL);
	return 0;
}



int init_osd_info()
{
	int i = 0;
	int ret = 0;
	TextInfo	text;
	LogoInfo    logo;
	mutex_osd = mid_mutex_create();
	PRINTF("logo_yuv_buf MALLOC \n");
	logo_yuv_buf = (unsigned char *)MALLOC(PNG_MAX_WIDTH * PNG_MAX_HEIGHT * 2);

	if(!logo_yuv_buf) {
		ERR_PRN("yuv420sp_buf:MALLOC failed!\n");
		return -1;
	}

	text_yuyv_buf = (unsigned char *)MALLOC(TEXT_BITMAP_WIDTH * TEXT_BITMAP_HEIGHT * 2);

	if(text_yuyv_buf == NULL) {
		ERR_PRN("yuyv_buf is NULL\n");
		return -1;
	}

	time_yuyv_buf = (unsigned char *)MALLOC(TIME_BITMAP_WIDTH * TIME_BITMAP_HEIGHT * 2);

	if(time_yuyv_buf == NULL) {
		ERR_PRN("time_yuyv_buf is NULL\n");
		return -1;
	}

	for(i = 0 ; i < INPUT_TEXT_MAX_NUM; i++) {
		memset(&text, 0, sizeof(TextInfo));
		memset(&logo, 0, sizeof(LogoInfo));
		PRINTF("input=%d\n", i);

		ret = read_logo_info(i, &logo);

		if(ret < 0) {
			logo.alpha = 0x80;
			logo.enable = 0 ;
			memset(logo.filename, 0, sizeof(logo.filename));
		}

		if(logo.enable) {
			add_logo_osd(i, &logo);
		}

		set_logo_info(i, &logo);

		ret = read_text_info(i,  &(text));

		if(ret < 0) {
			PRINTF("WARNING:read_text_info failed!input=%d\n", i);
			text.alpha = 0x80;
			text.enable = 0;
		}

		set_text_info(i, &text);

		if(text.enable) {
			add_text_info(i, &(text));
		}

		g_timeosd_info[i].text.alpha = text.alpha;
		g_display_time[i] = text.showtime;
	}

	PRINTF("init_osd_info finished!\n");
	return 0;
}



