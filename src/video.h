#ifndef _VIDEO_H_
#define _VIDEO_H_

#include "reach_enc2000.h"


Int32 reach_host_bits_process(ENC2000_LINK_STRUCT *pstruct);

Int32 reach_venc_process(ENC2000_LINK_STRUCT *pstruct, int num,int mp_status);

int get_enc_state(void);

void set_enc_state(int state);
int get_encoding_mode(void);
void set_encoding_mode(int state);

int app_video_request_iframe(int channel);



#endif
