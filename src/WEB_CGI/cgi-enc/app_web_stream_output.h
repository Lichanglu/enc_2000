#ifndef _APP_WEB_STREAM_OUTPU_H__
#define _APP_WEB_sTREAM_OUTPUT_H__

#include "../../stream_output/stream_output_struct.h"


int app_rtsp_get_used();
int app_rtsp_get_ginfo(rtsp_server_config *config);
int app_rtsp_set_ginfo(rtsp_server_config *config,rtsp_server_config *newconfig);

int app_rtsp_get_cinfo(stream_output_server_config *config);
int app_rtsp_set_cinfo(stream_output_server_config *config,stream_output_server_config *newconfig);
int app_rtsp_delete();
int app_rtsp_add_server(stream_output_server_config *config,stream_output_server_config *newconfig);
int app_mult_get_total_num();
int app_mult_add_server(stream_output_server_config *config,stream_output_server_config *newconfig);
int app_mult_del_server(stream_output_server_config *config);
int app_mult_get_server_config(int num,stream_output_server_config *config);
int app_mult_set_server_config(stream_output_server_config *config,stream_output_server_config *newconfig);
int webGetSdpInfo(char *buf,int sizelen,int channel,char *ip,int vport,int aport);
int app_rtsp_set_status(stream_output_server_config *config,stream_output_server_config *newconfig);
int app_mult_set_server_status(stream_output_server_config *config,stream_output_server_config *newconfig);
int app_mult_get_rate(int type,int channel);


#endif

