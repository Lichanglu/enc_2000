#include "osd.c"
#include "common.h"
#include "middle_control.h"
#include "input_to_channel.h"
#include "app_video_control.h"
#include "app_text_logo.h"

unsigned int  osd_max_x[SIGNAL_INPUT_MAX] = {0};
unsigned int  osd_max_y[SIGNAL_INPUT_MAX] = {0};
//type != 0
static int postype_2_pixel(int channel, int type, int *x, int *y)
{
	int input = 0, high = 0;
	unsigned int temp = 0;
	channel_get_input_info(channel, &input, &high);

	if(osd_max_x[input] == 0 || osd_max_y[input]) {
		app_get_osd_max_xypos(channel, &temp);
	}

	PRINTF("input=%d,osd_max_x=%d,osd_max_y=%d\n", input, osd_max_x[input], osd_max_y[input]);

	if(type == ABSOLUTE2) {
		return -1;
	} else if(type == TOP_LEFT) {
		*x = 0;
		*y = 0;
	} else if(type == TOP_RIGHT) {
		*x = osd_max_x[input];
		*y = 0;
	} else if(type == BOTTOM_LEFT) {
		*x = 0;
		*y = osd_max_y[input];
	} else if(type == BOTTOM_RIGHT) {
		*x = osd_max_x[input];
		*y = osd_max_y[input];
	} else if(type == CENTERED) {
		*x = (osd_max_x[input]) / 2 - 4;
		*y = osd_max_y[input] / 2 - 4;
	} else {
		*x = 0;
		*y = 0;
	}

	return 0;
}


int app_get_logo(int channel, char *data, int vallen, char *outdata)
{
	int ret = 0;
	LogoInfo logo;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	channel_get_input_info(channel, &input, &high);
	get_mp_input(&input);

	if(high != HIGH_STREAM) {
		PRINTF("warnning,not support low stream set logo\n");
		return -1;
	}

	PRINTF("------Input=%d------\n", input);

	ret = get_logo_info(input, &logo);
	//	ret = read_logo_info(id, logo);

	if(ret != 0) {
		PRINTF("get_logo_info logoinfo ret=%d\n", ret);
		memcpy(outdata, &logo, sizeof(LogoInfo));
		return ret;
	}

	memcpy(outdata, &logo, sizeof(LogoInfo));
	PRINTF("LOGO:x=%d,y=%d,enable=%d,alpha=%d\n", logo.x, logo.y, logo.enable, logo.alpha);
	return ret;
}

int app_set_logo(int channel , LogoInfo *data, int vallen, char *outdata)
{
	LogoInfo *logo = (LogoInfo *)data;
	LogoInfo old_logo;
	//	FILE *fd;

	//	LogoInfo *handle = NULL;
	int ret = 0;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	channel_get_input_info(channel, &input, &high);
	get_mp_input(&input);

	if(high != HIGH_STREAM) {
		PRINTF("warnning,not support low stream set logo\n");
		return -1;
	}

	get_logo_info(input, &old_logo);

	if(0 == logo->enable) {
		PRINTF("logo->enable != old_logo.enable.tinfo->enable=%d\n", logo->enable);
		old_logo.enable = logo->enable ;
		set_logo_info(input, &old_logo);
		hide_logo_view(input);
		write_logo_info(input);
		return -1;
	}

	PRINTF("------Input=%d------\n", input);

	if(vallen != sizeof(LogoInfo)) {
		ret = -1;
		PRINTF("WRONG:vallen < sizeof(LogoInfo) \n");
		return ret;
	}


	PRINTF("LOGO:x=%d,y=%d,enable=%d,alpha=%d,name=%s\n", logo->x, logo->y, logo->enable, logo->alpha, logo->filename);

	if(logo->alpha < 0 || logo->x < 0 || logo->x > 1920 || logo->y < 0 || logo->y > 1080) {
		ret = -2;
		return ret;
	}

	if(logo->alpha > 0x80) {
		logo->alpha = 0x80;
	}

	if(input == SIGNAL_INPUT_1) {
		strcpy(logo->filename, "logo_0.png");
	} else if(input == SIGNAL_INPUT_2) {
		strcpy(logo->filename, "logo_1.png");
	} else if(SIGNAL_INPUT_MP == input) {
		strcpy(logo->filename, "logo_2.png");
	}

	postype_2_pixel(channel, logo->postype, &(logo->x), &(logo->y));

	PRINTF("LOGO:x=%d,y=%d,enable=%d,alpha=%d,name=%s\n",
	       logo->x, logo->y, logo->enable, logo->alpha, logo->filename);

	add_logo_osd(input, logo);
	set_logo_info(input, logo);
	ret = write_logo_info(input);

	if(ret == 0) {
		PRINTF("save logoinfo to file failed ret =%d\n", ret);
		return ret;
	}

	return ret;
}


int app_update_logo(int channel, char *indata, char *outdata)
{
	int ret = 0;
	char com[256] = {0};
	char filename[256] = {0};

	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	channel_get_input_info(channel, &input, &high);
	get_mp_input(&input);

	if(high != HIGH_STREAM) {
		PRINTF("warnning,not support low stream set logo\n");
		return -1;
	}

	system("sync");

	strcpy(filename, indata);

	strcpy(outdata, indata);


	// check the logo file
	ret =  check_logo_png(input, filename);

	if(ret == 0) {
		snprintf(com, sizeof(com), "mv %s %s_%d.png", filename, LOGOFILE, input);
		system(com);
		PRINTF("success change logo file. com=%s\n", com);
	} else {
		PRINTF("ret = %d!\n", ret);
		return ret;
	}

	PRINTF("check logo ret = %d\n", ret);
	return ret;
}




int read_text_info(int id, TextInfo *text)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//int 			enable = 0;
	const char config_file[64] = TEXT_CONFIG;
	int 			rst = -1;
	PRINTF("LOGO=%p\n", text);
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char	title[24] = {0};
	sprintf(title, "text_%d", id);

	ret =  ConfigGetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		PRINTF("Failed to Get text x\n");
		goto EXIT;
	}

	text->xpos = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		PRINTF("Failed to Get text y\n");
		goto EXIT;
	}

	text->ypos = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		PRINTF("Failed to Get text enable\n");
		goto EXIT;
	}

	text->enable = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		PRINTF("Failed to Get text alpha\n");
		goto EXIT;
	}

	text->alpha = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "showtime", temp);

	if(ret != 0) {
		PRINTF("Failed to Get text showtime\n");
		goto EXIT;
	}

	text->showtime = atoi(temp);

	//pos type
	ret =  ConfigGetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		PRINTF("Failed to Get text postype\n");
		goto EXIT;
	}

	text->postype = atoi(temp);


	ret =  ConfigGetKey((char *)config_file, title, "content", temp);

	if(ret != 0) {
		PRINTF("Failed to Get text content\n");
		goto EXIT;
	}

	PRINTF("\n");
	//temp[15] = 0;
	strcpy(text->msgtext, temp);
	PRINTF("\n");
	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);
	PRINTF("\n");
	return rst;
}


int write_text_info(int input)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	const char config_file[64] = TEXT_CONFIG;
	int 			rst = 0;
	TextInfo 		text ;
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char    title[24];
	sprintf(title, "text_%d", input);

	ret = get_text_info(input, & text);

	if(ret < 0) {
		PRINTF("get text info failed!\n");
		return ret;
	}

	sprintf(temp, "%d", text.xpos);
	ret =  ConfigSetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		PRINTF("Failed to Get logo x\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.ypos);
	ret =  ConfigSetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		PRINTF("Failed to Get logo y\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.enable);
	ret =  ConfigSetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		PRINTF("Failed to Get logo enable\n");
		goto EXIT;
	}

	strcpy(temp, text.msgtext);
	temp[strlen(text.msgtext)] = '\0';
	ret =  ConfigSetKey((char *)config_file, title, "content", temp);

	if(ret != 0) {
		PRINTF("Failed to Get logo filename\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.alpha);
	ret =  ConfigSetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		PRINTF("Failed to Get logo alpha\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.showtime);
	ret =  ConfigSetKey((char *)config_file, title, "showtime", temp);

	if(ret != 0) {
		PRINTF("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	sprintf(temp, "%d", text.postype);
	ret =  ConfigSetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		PRINTF("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	/*
		sprintf(temp, "%d", text->show);
		ret =  ConfigSetKey((char *)config_file, title, "show", temp);

		if(ret != 0) {
			PRINTF("Failed to Get logo show\n");
			goto EXIT;
		}
	*/
	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);

	return rst;
}


int read_logo_info(int id,  LogoInfo *logo)
{
	PRINTF("logo =%p\n", logo);
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	int 			rst = -1;
	const char config_file[64] = LOGO_CONFIG;
	char    title[24];
	sprintf(title, "logo_%d", id);

	ret =  ConfigGetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get logo x\n");
		goto EXIT;
	}

	logo->x = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get logo y\n");
		goto EXIT;
	}

	logo->y = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get logo enable\n");
		goto EXIT;
	}

	logo->enable = atoi(temp);
	ret =  ConfigGetKey((char *)config_file, title, "filename", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get logo filename\n");
		goto EXIT;
	}

	temp[15] = 0;
	strcpy(logo->filename, temp);

	ret =  ConfigGetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get logo alpha\n");
		goto EXIT;
	}

	logo->alpha = atoi(temp);

	/*
		ret =  ConfigGetKey((char *)config_file, title, "show", temp);

		if(ret != 0) {
			ERR_PRN("Failed to Get logo show\n");
			goto EXIT;
		}

		logo->show = atoi(temp);
	*/

	ret =  ConfigGetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Get logo isThrough\n");
		goto EXIT;
	}

	logo->postype = atoi(temp);


	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);

	return rst;
}

int write_logo_info(int input)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	int				rst = 0;
	const char config_file[64] = LOGO_CONFIG ;
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char    title[24];
	LogoInfo logo;
	sprintf(title, "logo_%d", input);


	ret = get_logo_info(input, &logo);

	if(ret < 0) {
		PRINTF("get text info failed!\n");
		return ret;
	}

	sprintf(temp, "%d", logo.x);
	ret =  ConfigSetKey((char *)config_file, title, "x", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set logo x\n");
		goto EXIT;
	}

	sprintf(temp, "%d", logo.y);
	ret =  ConfigSetKey((char *)config_file, title, "y", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set logo y\n");
		goto EXIT;
	}

	PRINTF("write:logo.enable=%d\n", logo.enable);
	sprintf(temp, "%d", logo.enable);
	ret =  ConfigSetKey((char *)config_file, title, "enable", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set logo enable\n");
		goto EXIT;
	}


	ret =  ConfigSetKey((char *)config_file, title, "filename", logo.filename);

	if(ret != 0) {
		ERR_PRN("Failed to Set filename\n");
		goto EXIT;
	}

	sprintf(temp, "%d", logo.alpha);
	ret =  ConfigSetKey((char *)config_file, title, "alpha", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set alpha \n");
		goto EXIT;
	}

	/*
		sprintf(temp, "%d", logo->show);
		ret =  ConfigSetKey((char *)config_file, title, "show", temp);

		if(ret != 0) {
			ERR_PRN("Failed to Set show \n");
			goto EXIT;
		}
	*/
	sprintf(temp, "%d", logo.postype);
	ret =  ConfigSetKey((char *)config_file, title, "postype", temp);

	if(ret != 0) {
		ERR_PRN("Failed to Set isThrough \n");
		goto EXIT;
	}


	rst = 1;
EXIT:
	//pthread_mutex_unlock(&gSetP_m.save_sys_m);
	return rst;
}





int app_get_text(int channel, char *data, int vallen, char *outdata)
{
	int ret = 0;

	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	TextInfo text  ;

	ASSERT_INPUT(input);
	channel_get_input_info(channel, &input, &high);
	get_mp_input(&input);
	PRINTF("------Input=%d------\n", input);

	ret = get_text_info(input, &text);

	if(high != HIGH_STREAM || ret < 0) {
		PRINTF("warnning,not support low stream set logo\n");
		return -1;
	}

	memcpy(outdata, &text, sizeof(TextInfo));
	PRINTF("id=%d:text:x=%d,y=%d,enable=%d,alpha=%d\n", input, text.xpos, text.ypos, text.enable, text.alpha);
	return ret;
}

int app_set_text(int channel, char *data, int vallen, char *outdata)
{
	if(vallen != sizeof(TextInfo)) {
		PRINTF("vallen=%d \n", vallen);
		PRINTF("WRONG:vallen < sizeof(LogoInfo) \n");
		return -1;
	}

	PRINTF("display text enter ....\n");
	TextInfo *tinfo = (TextInfo *)(data);
	int ret = 0;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	TextInfo text;
	int msg_len  = strlen(tinfo->msgtext);

	ASSERT_INPUT(input);

	channel_get_input_info(channel, &input, &high);
	get_mp_input(&input);
	ret = get_text_info(input, &text);

	if(high != HIGH_STREAM || ret < 0) {
		PRINTF("warnning,not support low stream set logo\n");
		return -1;
	}

	if(0 == tinfo->enable) {
		PRINTF("tinfo->enable != text.enable.tinfo->enable=%d\n", tinfo->enable);
		text.enable = tinfo->enable ;
		set_time_display(input, text.enable, text.alpha);
		hide_text_view(input);
		set_text_info(input, &text);
		write_text_info(input);
		return -1;
	}

	PRINTF("------Input=%d------\n", input);

	if(vallen < sizeof(TextInfo)) {
		ret = -1;
		PRINTF("warnning,vallen < sizeof(TextInfo)\n");
		return ret;
	}

	if(tinfo->xpos < 0 || tinfo->xpos > 1920 || tinfo->ypos < 0 || tinfo->ypos > 1080) {
		ret = -2;
		return ret;
	}

	if(tinfo->alpha > 0x80) {
		tinfo->alpha = 0x80;
	}

	postype_2_pixel(channel, tinfo->postype, &(tinfo->xpos), &(tinfo->ypos));

	if(0 != msg_len) {
		check_text_size(tinfo->msgtext);
		ret = add_text_info(input, tinfo);
	} else {
		hide_text_view(input);
	}

	set_text_info(input, tinfo);
	set_time_display(input, tinfo->showtime, tinfo->alpha);

	write_text_info(input);

	memcpy(outdata, tinfo, sizeof(TextInfo));

	return 0;
}


/*获得文本长度*/
int get_text_len(char *Data)
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

	return Sum;
}


int MMP_set_osd_text(int input, unsigned char *data, int len)
{
	RecAVTitle recavtitle;
	int textXpos = 0;
	int textYpos = 0;
	int high = HIGH_STREAM;
	TextInfo text;
	int ret = 0;

	ASSERT_INPUT(input);

	/*	channel_get_input_info(channel, &input , &high);

		if(IS_MP_STATUS == mp_status) { // MP_STATUS
			input = SIGNAL_INPUT_1;
		}
	*/
	ret = get_text_info(input, &text);
	//	text.show = 1;
	//	text.enable = 1;

	PRINTF("input=%d,high=%d\n",  input, high);

	if(high != HIGH_STREAM || ret < 0) {
		PRINTF("warnning,not support high stream\n");
		return -1;
	}

	//	id = input;
	memset(&recavtitle, 0x00000, sizeof(RecAVTitle));
	memcpy(&recavtitle, data, len);
	PRINTF("display the text =%s =recavtitle.len=%d\n", recavtitle.Text, recavtitle.len);

	if(recavtitle.len > 0) {

		int len = get_text_len(recavtitle.Text);
		int charlen = strlen(recavtitle.Text);

		memset(text.msgtext, 0, sizeof(text.msgtext));
		memcpy(text.msgtext, recavtitle.Text, charlen);
		PRINTF("len = %d charlen=%d\n", len, charlen);

		textXpos = recavtitle.x;
		textYpos = recavtitle.y;

		if(textXpos % 32) {
			textXpos -= (textXpos % 32);
		}

		if(textYpos % 32) {
			textYpos -= (textYpos % 32);
		}

		PRINTF("textXpos = %d  textYpos = %d\n\n", textXpos, textYpos);

		text.enable = 1;
		text.xpos = textXpos;
		text.ypos = textYpos;
		text.alpha = 0x80;

		if(len != recavtitle.len) {
			text.showtime = 1;
		} else {
			text.showtime = 0;
		}

		if(charlen != 0) {
			check_text_size(text.msgtext);
			add_text_info(input, &text);
		} else {
			hide_text_view(input);
		}

		set_time_display(input, text.showtime, text.alpha);
	} else {
		set_time_display(input, 0, 0x80);
		hide_text_view(input);
	}

	set_text_info(input, &text);
	write_text_info(input);
	return 0;
}

int MMP_set_logo_info(int input, void *buf)
{
	LogoInfo *logo = (LogoInfo *)buf;
	int ret = 0 ;
	//	char filename[64];
	//	memcpy(&logo, buf, sizeof(LogoInfo));

	if(logo->alpha > 0x80 || logo->alpha < 0 || logo->x < 0 || logo->x > 1920
	   || logo->y < 0 || logo->y > 1080) {
		return -1;
	}

	if(input < 0 || input >= SIGNAL_INPUT_MAX) {
		ERR_PRN("input error!\n");
		return -1;
	}

	PRINTF("alpha=%d\n", logo->alpha);
	/**/
	sprintf(logo->filename, "logo_%d.png", input);
	logo->enable = 1;
	logo->alpha = 0x80 * (1 - logo->alpha / 100.0);

	PRINTF("input =%d,filename=%s,enable=%d,alpha=%d\n", input, logo->filename,
	       logo->enable, logo->alpha);
	ret = add_logo_osd(input, logo);

	if(ret == 0) {
		set_logo_info(input, logo);
	}

	write_logo_info(input);

	return ret;
}

extern VideoCommonHandle g_video_control_handle ;

int app_get_osd_max_xypos(int channel, int *out)
{
	unsigned short x_pos = 0, y_pos = 0;
	int input = SIGNAL_INPUT_1;
	int high = HIGH_STREAM;
	int width = 0, height = 0;
	int mp_status = get_mp_status();

	if(mp_status == IS_MP_STATUS) {
		channel = CHANNEL_INPUT_MP;
	}

	channel_get_input_info(channel, &input, &high);

	if(input != SIGNAL_INPUT_MP) {
		capture_get_input_hw(input, &width, &height);
	} else {
		width = 1920;
		height = 1080;
	}

	//	get_source_width_height(channel,&width,&height);

	if(height == 540  && width == 1920) {
		height = height * 2;
	}

	PRINTF("input=%d,width*height=%d*%d\n", input, width, height);
	x_pos = width - 64;
	y_pos = height - 64;
	*out = (x_pos << 16) | y_pos;
	PRINTF("max_x_pos=%d,max_y_pos=%d\n", x_pos, y_pos);
	osd_max_x[input] = x_pos;
	osd_max_y[input] = y_pos;
	return 0;
}

void update_osd_change_mp()
{
	int input = SIGNAL_INPUT_1;
	TextInfo text;
	LogoInfo logo;
	int i = 0;

	if(IS_MP_STATUS == get_mp_status()) {
		input = SIGNAL_INPUT_MP;
		get_logo_info(input, &logo);

		if(logo.enable) {
			add_logo_osd(input, &logo);
		}

		get_text_info(input, &text);

		if(text.enable) {
			add_text_info(input, &text);
		}
	} else if(IS_IND_STATUS == get_mp_status()) {
		for(i = SIGNAL_INPUT_1 ; i < SIGNAL_INPUT_MP; i++) {
			memset(&text, 0, sizeof(TextInfo));
			memset(&logo, 0, sizeof(LogoInfo));
			get_logo_info(i, &logo);

			if(logo.enable) {
				add_logo_osd(i, &logo);
			}

			get_text_info(i, &text);

			if(text.enable) {
				add_text_info(i, &text);
			}
		}
	}
}
