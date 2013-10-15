#ifndef _WEBLISTEN_H
#define _WEBLISTEN_H



extern int midParseInt(int identifier, int fd, char *data, int len);
extern int midParseString(int identifier, int fd, char *data, int len);
extern int midParseStruct(int identifier, int fd, char *data, int len);

int app_weblisten_init();

//extern int app_web_stream_output_process_int(int cmd, int in, int *out);
//extern int app_web_stream_output_process_struct(int identifier,int fd, int cmd, char *data, int valen);
#endif
