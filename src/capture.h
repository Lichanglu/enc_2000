#ifndef _CAPTURE_H_
#define _CAPTURE_H_


//#define MAX_VIDEO_CHUNNAL	2





//extern int g_vp0_capture_mode;
//extern int g_vp1_capture_mode;

//extern int g_vp0_status;
//extern int g_vp1_status;


Int32 reach_vcapture_process(ENC2000_LINK_STRUCT *pstruct);
Int32 reach_MP_vcapture_process(ENC2000_LINK_STRUCT *pstruct);
Int32 reach_video_detect_task_process(ENC2000_LINK_STRUCT *pstruct);
Int32 reach_config_cap_device(ENC2000_LINK_STRUCT *pstruct);

int capture_get_input_hw(int VP, int *width, int *height);
int capture_set_input_hw(Uint32 VP, Uint32 width, Uint32 height);


int capture_get_input_type(int input);
int  capture_set_input_type(int input, int mode);

int get_capture_state(SIGNAL_INPUT input);

#endif

