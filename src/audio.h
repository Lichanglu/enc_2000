#ifndef _AUDIO_H_
#define _AUDIO_H_






Int32 reach_audio_enc_process(ENC2000_LINK_STRUCT *pstruct);
int audio_update_config(int input,int audio_input);
int audio_set_update_flag(int audio_input, int flag);

int set_mute_status(int audio_input, int status);
int set_mp_mute_status(int audio_input, int status, int mp_status);

#endif

