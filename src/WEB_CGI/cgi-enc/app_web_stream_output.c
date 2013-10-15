#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "weblib.h"
#include "app_web_stream_output.h"
#include "webTcpCom.h"


/*get sdp info*/
int webGetSdpInfo(char *buf,int sizelen,int channel,char *ip,int vport,int aport)
{
	RTP_SDP_INFO info;
	RTP_SDP_INFO out;
	memset(&info,0,sizeof(RTP_SDP_INFO));
	memset(&out,0,sizeof(RTP_SDP_INFO));

	info.channel = channel;
	snprintf(info.ip,sizeof(info.ip),"%s",ip);
	info.vport = vport;
	info.aport = aport;
	
	int outlen,ret=0;
	int cmd = MSG_GET_SDP_INFO;

	ret = appCmdStructParse(cmd, &info, sizeof(RTP_SDP_INFO), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	snprintf(buf,sizelen,"%s",out.buff);

	return ret;
}



int app_rtsp_get_used()
{
	int ret = 0;
	int value = 0;
	int out = 0;
	int outlen = 0;
	ret =  appCmdIntParse(MSG_RTSP_GET_USED,value,sizeof(value),&out,&outlen);
	if(ret < 0)
	{
		return ret;
	}
	return out;
}

int app_rtsp_get_ginfo(rtsp_server_config *config)
{
	rtsp_server_config in;
	rtsp_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&in,0,sizeof(rtsp_server_config));
	memset(&out,0,sizeof(rtsp_server_config));
	ret = appCmdStructParse(MSG_RTSP_GET_GINFO, &in, sizeof(rtsp_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(config,&out,sizeof(rtsp_server_config));
	return 0;
}


int app_rtsp_set_ginfo(rtsp_server_config *config,rtsp_server_config *newconfig)
{

	rtsp_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(rtsp_server_config));
	ret = appCmdStructParse(MSG_RTSP_SET_GINFO, config, sizeof(rtsp_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(newconfig,&out,sizeof(rtsp_server_config));
	return 0;
}

int app_rtsp_get_cinfo(stream_output_server_config *config)
{
	stream_output_server_config in;
	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&in,0,sizeof(stream_output_server_config));
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_RTSP_GET_CINFO, &in, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(config,&out,sizeof(stream_output_server_config));
	return 0;
}

int app_rtsp_set_cinfo(stream_output_server_config *config,stream_output_server_config *newconfig)
{

	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_RTSP_SET_CINFO, config, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(newconfig,&out,sizeof(stream_output_server_config));
	return 0;
}

int app_rtsp_set_status(stream_output_server_config *config,stream_output_server_config *newconfig)
{

	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_RTSP_SET_STATUS, config, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(newconfig,&out,sizeof(stream_output_server_config));
	return 0;
}

int app_rtsp_delete()
{
	int ret = 0;
	int value = 0;
	int out = 0;
	int outlen = 0;
	ret =  appCmdIntParse(MSG_RTSP_DEL_SERVER,value,sizeof(value),&out,&outlen);
	if(ret < 0)
	{
		return ret;
	}
	return out;	
}

int app_rtsp_add_server(stream_output_server_config *config,stream_output_server_config *newconfig)
{
	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_RTSP_ADD_SERVER, config, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(newconfig,&out,sizeof(stream_output_server_config));
	return 0;
}


int app_mult_get_total_num()
{
	int ret = 0;
	int value = 0;
	int out = 0;
	int outlen = 0;
	ret =  appCmdIntParse(MSG_MULT_GET_NUM,value,sizeof(value),&out,&outlen);
	if(ret < 0)
	{
		return ret;
	}
	return out;	
}

int app_mult_add_server(stream_output_server_config *config,stream_output_server_config *newconfig )
{
	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_MULT_ADD_SERVER, config, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(newconfig,&out,sizeof(stream_output_server_config));
	return 0;
}

int app_mult_del_server(stream_output_server_config *config)
{
	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_MULT_DEL_SERVER, config, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	return 0;
}

int app_mult_get_server_config(int num,stream_output_server_config *config)
{
	stream_output_server_config out;
	stream_output_server_config in;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	memset(&in,0,sizeof(stream_output_server_config));
	in.num = num;

	ret = appCmdStructParse(MSG_MULT_GET_CINFO, &in, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}

	memcpy(config,&out,sizeof(stream_output_server_config));
	return 0;
	
}

int app_mult_set_server_config(stream_output_server_config *config,stream_output_server_config *newconfig)
{
	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_MULT_SET_CINFO, config, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(newconfig,&out,sizeof(stream_output_server_config));
	return 0;

}

int app_mult_set_server_status(stream_output_server_config *config,stream_output_server_config *newconfig)
{
	stream_output_server_config out;
	int outlen = 0;
	int ret = 0;
	memset(&out,0,sizeof(stream_output_server_config));
	ret = appCmdStructParse(MSG_MULT_SET_STATUS, config, sizeof(stream_output_server_config), &out,&outlen);
	if(ret <0)
	{
		return ret;
	}
	memcpy(newconfig,&out,sizeof(stream_output_server_config));
	return 0;

}




int app_mult_get_rate(int type,int channel)
{
	int ret = 0;
	int value = 0;
	int out = 0;
	int outlen = 0;
	int cmd = 0;

	if(type == TYPE_TS)
		cmd = MSG_MULT_GET_TS_RATE;
	else if(type == TYPE_RTP)
		cmd = MSG_MULT_GET_RTP_RATE;
	else if(type == TYPE_TS_OVER_RTP)
		cmd = MSG_MULT_GET_RTPTS_RATE;
	else
		cmd = MSG_MULT_GET_RTP_RATE;


	cmd = cmd | (((channel+1)&0xf)<<16) ;	
	ret =  appCmdIntParse(cmd,value,sizeof(value),&out,&outlen);
	if(ret < 0)
	{
		return ret;
	}
	return out;			
}
