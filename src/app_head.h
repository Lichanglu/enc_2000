#ifndef APP_HEAD_H
#define APP_HEAD_H



int writeWatchDog(void);

int capture_get_input_type(int input);
int app_get_gpio_fd();
int app_mult_set_all_tc(int channel);
int stream_rtsp_set_all_tc(int channel);

unsigned long long getCurrentTime(void);

int set_encode_time(char *data, int vallen);
int get_encode_time(char *data, int vallen);
int capture_get_input_type(int input);

int initWatchDog(void);
int writeWatchDog(void);


int mid_time_adjust_init();
int capture_get_input_hw(int input, int *width, int *height);


int JoinMacAddr(char *dst, unsigned char *src, int num);
int SplitMacAddr(char *src, unsigned char *dst, int num);

unsigned int GetIPaddr(char *interface_name);
unsigned int GetNetmask(char *interface_name);
unsigned int GetBroadcast(char *interface_name);

int app_web_rebootSys(int data, int *out);













#endif
