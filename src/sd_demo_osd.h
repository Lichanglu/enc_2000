#ifndef _SD_DEMO_OSD_H__
#define _SD_DEMO_OSD_H__


#define SD_DEMO_OSD_NUM_WINDOWS      (4)

#define SD_DEMO_OSD_WIN0_STARTX      (16)
#define SD_DEMO_OSD_WIN0_STARTY      (16)
#define SD_DEMO_OSD_GLOBAL_ALPHA    (0x80)
#define SD_DEMO_OSD_MAX_FILE_NAME_SIZE (128)

#define OSD_MAX_WIDTH  720
#define OSD_MAX_HEIGHT 1080


enum{
	WINDOW_TEXT_OSD,
	WINDOW_TIME_OSD,
	WINDOW_LOGO_OSD,
	WINDOW_MAX_OSD
};

 int SD_Demo_osdInit(int algid,int);
 void SD_Demo_osdDeinit(void);

int SD_logo_osdUpdate(int input,int osdtype,unsigned char *osdbuff,int osdlen,LogoInfo *info);
int SD_subtitle_osdUpdate(int input, int osdtype, unsigned char *osdbuff, int osdlen,TextInfo *info);


#endif
