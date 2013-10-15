#ifndef APP_MP_CONTROL_H
#define APP_MP_CONTROL_H


enum{
	MP_LAYOUT_1=1,
	MP_LAYOUT_2,
	MP_LAYOUT_3,
	MP_LAYOUT_4,
	MP_LAYOUT_5,
	MP_LAYOUT_6,
	MP_LAYOUT_MAX,
}MP_MODE;

Int32 mp_set_resolution(Int32 input,Int32 streamId, UInt32 outWidth, UInt32 outHeight, UInt32 inWidth, UInt32 inHeight);
void app_mp_video_control();
int app_mp_set_scale(int mode);
void init_MP_info(int *mp_status);
int write_mp_config();

Void app_web_get_mp_info( Mp_Info* mp_mode);
Void app_web_set_mp_layout( UInt32 layout,UInt32 win1,UInt32 win2);

int reach_begin(void);
int reach_stop(void);
int mp_reach_begin(void);
int mp_reach_stop(void);

int set_mp_layout(int mode);
int get_mp_layout();

int app_get_original_resolution(int *width, int *height);

void init_MP_info(int *mp_status);
int app_set_mp_info(Mp_Info *mp_info);



#endif
