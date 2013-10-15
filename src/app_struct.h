

#include "MMP_control.h"

#if 0



//in audio.c
//定义handle = info+mutex;
// audio_handle
typedef struct __AudioCommonHandle{
	AudioCommonInfo info;
	mid_mutex_t  mutex;  //互斥锁
}*AudioCommonHanle;

static AudioCommonHanle g_audio_handle[MAX_INPUT];

#define LOCK(id) (mid_mutex_lock(g_audio_handle[id].mutex))
#define UNLOCK(id) (mid_mutex_unlock(g_audio_handle[id].mutex))

//encodemange设置audio info到g_audio_handle
int reach_audio_info_set(ReachAudioParam *info,int channel);
//encodemange获取audio info从 g_audio_handle
int reach_audio_info_get(ReachAudioParam *info,int channel);


int web_audio_info_set(WebAudioParam *info,int channel);
int web_get_audio_info(WebAudioParam *info,int channel);



//sysparm.dat里面有,换成xml方式
/*
[sysParam]
type=6
channels=1
version=1.0.7HP
name=DSS-ENC-MOD
macaddr=28:5a:b3:be:00:00
[network_0]
netmask=255.255.255.0
gateway=192.168.4.254
ipaddr=192.168.4.97
dhcp=0
[network_1]
netmask=255.255.255.0
gateway=192.168.4.254
ipaddr=192.168.4.98
dhcp=0
*/

//encode.dat
/*
[videoParam_high0]
//height=1080
//width=1920
bitrate=3000
cbr=1
quality=45
colors=24
framerate=30
hp=1
gop=150
[videoParam_low0]
//height=1080
//width=1920
bitrate=3000
cbr=1
quality=45
colors=24
framerate=30
hp=1
gop=150
[audioParam_0]
inputMode=0
mictype=0
input_num= 0
Rvolume=30
Lvolume=30
samplebit=16
channel=2
bitrate=128000
samplerate=3
[videoParam_high1]
//height=1080
//width=1920
bitrate=3000
cbr=1
quality=45
colors=24
framerate=30
hp=1
gop=150
[videoParam_low1]
//height=1080
//width=1920
bitrate=3000
cbr=1
quality=45
colors=24
framerate=30
hp=1
gop=150
[audioParam_1]
inputMode=0
mictype=0
input_num= 0
Rvolume=30
Lvolume=30
samplebit=16
channel=2
bitrate=128000
samplerate=3
*/

//output.dat
/*
[videoParam_high0]
needscale = 0 
outwidth=1920
outheight=1080
israte=1 //等比,全屏
[videoParam_low0]
needscale = 0 
outwidth=704
outheight=576
israte=1 //等比,全屏
[videoParam_high1]
needscale = 0 
outwidth=1920
outheight=1080
israte=1 //等比,全屏
[videoParam_low1]
needscale = 0 
outwidth=704
outheight=576
israte=1 //等比,全屏
*/


//TextInfo.dat 放置logo,text相关的设置


#endif