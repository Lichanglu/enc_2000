#ifndef _APP_TEXT_LOGO_H
#define _APP_TEXT_LOGO_H
 int get_logo_info(int ,LogoInfo* );
 int set_logo_info(int ,LogoInfo* );
 int get_text_info(int , TextInfo* );
 int set_text_info(int , TextInfo* );
 int MMP_set_logo_info(int input,void* buf);
 int read_logo_info(int id,  LogoInfo *logo);
 int write_logo_info(int input);
 int read_text_info(int id, TextInfo *text);
 int write_text_info(int input);
int MMP_set_osd_text(int input, unsigned char *data, int len);
void update_osd_change_mp();
int app_get_osd_max_xypos(int channel, int *out);
int app_update_logo(int channel, char *indata, char *outdata);
int MMP_set_logo_info(int input, void *buf);
int app_set_text(int channel, char *data, int vallen, char *outdata);
int app_get_text(int channel, char *data, int vallen, char *outdata);
int app_set_logo(int channel , LogoInfo *data, int vallen, char *outdata);
int app_get_logo(int channel, char *data, int vallen, char *outdata);




#endif
