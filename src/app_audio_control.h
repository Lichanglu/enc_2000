
#ifndef _APP_AUDIO_CONTROL_H__
#define _APP_AUDIO_CONTROL_H__

#include "MMP_control.h"
#include "middle_control.h"

int MMP_audio_info_get(int input, MMPAudioParam *aparam);
int mmp_set_audio_info(int input, MMPAudioParam *aparam, int *change);
int web_get_audio_info(int input, AudioEncodeParam *aparam);	

int app_web_set_ainfo(int channel, char *in, char *out);

void printf_audio_info(AudioEncodeParam *info);
int app_web_get_ainfo(int channel, char *out);


#endif



