#ifndef	_REACH_H_
#define	_REACH_H_

#include <osa.h>

#include <link_api/system.h>
#include <link_api/captureLink.h>
#include <link_api/displayLink.h>

#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>

#include <osa_que.h>
#include <osa_mutex.h>
#include <osa_thr.h>
#include <osa_sem.h>

#include "input_to_channel.h"
#include "rwini.h"



#define  IP_ADDR_1 "192.168.4.163"
#define  IP_ADDR_2 "192.168.4.167"

#define 	ETH0 0
#define 	ETH0_1  1

#define HAVE_MP_MODULE  1
//#define HAVE_IP_XML  



#define MAX_ALG_LINK        (2)
#define MAX_IPC_FRAMES_LINK (2)

/**********************************	  控制台  ************************************/

#define WRITE_YUV_TASK							(0)		// 写YUV图任务
#define PRINTF_BITSTREAM_INFO					(1)		// 打印编码后的比特流信息
#define DEFAULT_ENCODE_MODE						(0)		// 0--单画面 1--多画面

#define VIP0_DEFAULT_CAPTURE_PORT				(0)		// 默认采集口 0 -- DVI
#define VIP1_DEFAULT_CAPTURE_PORT				(0)		//            1 -- SDI




/*********************************************************************************/


typedef struct _dsp_alg_struct_ {
	Uint32							link_id;
	AlgLink_CreateParams	create_params;
}dspAlg_struct;


typedef struct _ipcFrames_indsp_struct_ {
	Uint32							link_id;
	IpcFramesInLinkRTOS_CreateParams	create_params;
}ipcFrames_indsp_struct;





typedef struct _ENC2000_LINK_STRUCT_ {
	Int32			start_runing;
	int gpiofd;
	audio_struct	audioencLink[2];
	cap_struct		capLink;
	nullsrc_struct	nullSrcLink;
	dei_struct      deiLink[2];
	merge_struct	mergeLink[5];
	enc_struct		encLink;
	
	select_struct	selectLink[4];
	swms_struct	 	swmsLink[2];
	nsf_struct      nsfLink[2];
	dup_struct	    dupLink[2];
	sclr_struct     sclrLink[2];
	ipcoutm3_struct	ipc_outvpss_Link;
	ipcinm3_struct	ipc_invideo_Link;

	ipcbit_outvideo_struct	ipcbit_outvideo_Link;
	ipcbit_inhost_struct	ipcbit_inhost_Link;

	IpcFramesOutLinkRTOS_CreateParams	ipcFramesOutVpssToHostPrm;
    IpcFramesInLinkHLOS_CreateParams	ipcFramesInHostPrm;

	Uint32 	ipcFramesOutVpssToHostId;
	Uint32 	ipcFramesInHostId;

	Bool 			enableOsdAlgLink;
	dspAlg_struct 		osd_dspAlg_Link[MAX_ALG_LINK];
	ipcFrames_indsp_struct     ipcFrames_indsp_link[MAX_ALG_LINK];
	
    	UInt32                         ipcFramesOutVpssId[MAX_ALG_LINK];
	IpcFramesOutLinkRTOS_CreateParams  ipcFramesOutVpssPrm[MAX_ALG_LINK];		
	
	
	
	
}ENC2000_LINK_STRUCT;

typedef enum{
	MAIN_MMP = 1,
	SECOND_MMP =2
}SYSINFO_MMP;

#if 0	
typedef enum{
	NO_MP_STATUS = -2,
	CREATE_NO_MP= -1,
	DEFAULT_STATUS = 0,
	CREATE_MP  = 1,
	MP_STATUS = 2
}Mp_Status;
#endif


extern ENC2000_LINK_STRUCT	gEnc2000;

//###############################主要流程#################################################

#if 0
//最底层的设置获取信号相关设置
typedef struct __SigalInputInfo{
	int is_dvi;  // 1 dvi ,0 sdi
	int mode;
	int inputwidth;  // 信号源的宽 ，蓝屏为19201*1080
	int inputheight;
}SigalInputInfo;

//信号的各种详细信息
typedef struct __InputCommonInfo {
	char inputSignal[128];
	unsigned int 	hpv;
	unsigned int  	tmds;
	unsigned int 	vsyncF;
	unsigned int 	hdcp;
	int  color_space;
} InputCommonInfo;

#endif






//##################################################################################################

#endif

