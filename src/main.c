#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <assert.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <sys/types.h>
#include <dirent.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>

#include <mcfw/src_linux/devices/inc/device.h>
#include <mcfw/src_linux/devices/inc/device_videoDecoder.h>
#include <mcfw/src_linux/devices/adv7441/inc/adv7441.h>
#include <mcfw/src_linux/devices/adv7441/src/adv7441_priv.h>

//#include <mcfw/src_linux/devices/adv7442/inc/adv7442.h>
#include <mcfw/src_linux/devices/adv7442/src/adv7442_priv.h>


#include "reach_enc2000.h"

#include "lcd.h"

#include "app_sdk_tcp.h"
#include "capture.h"
#include "select.h"
#include "audio.h"
#include "video.h"
#include "params.h"
#include "common.h"
#include "remotectrl.h"
#include "encliveplay.h"
#ifdef HAVE_RTSP_MODULE
#include "app_stream_output.h"
#endif
#include "log_common.h"
#include "mid_task.h"
#include "mid_timer.h"
#include "log_server.h"
#include "weblisten.h"
#include "ts_build.h"


#include "osd.h"
#include "sd_demo_osd.h"

#include "middle_control.h"

#include "lcdshow.h"
#include "fp_led.h"

#include "new_tcp_com.h"
#include "process_xml_cmd.h"

#include "app_version.h"
#include "sysparams.h"

#include "app_video_control.h"
#include "app_signal.h"

#define	MAX_SOURCE_NUM	2
ENC2000_LINK_STRUCT	gEnc2000;
static int program_begin = 1;
/*
0  -- 默认
1  -- 非合成
2  -- 合成
-1-- 创建非合成ing
-2-- 创建合成ing
*/
static int g_mp_status = 1;

extern Void *g_bit_handle;
extern Bool g_osd_enable;

static void set_mp_status(int status);
extern int get_mp_status();

static SystemVideo_Ivahd2ChMap_Tbl systemVid_encDecIvaChMapTbl = {
	.isPopulated = 1,
	.ivaMap[0] =
	{
		.EncNumCh  = 2,
		.EncChList = {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
		.DecNumCh  = 0,
		.DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	},
	.ivaMap[1] =
	{
		.EncNumCh  = 1,
		.EncChList = {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.DecNumCh  = 0,
		.DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0},
	},
	.ivaMap[2] =
	{
		.EncNumCh  = 1,
		.EncChList = {3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
		.DecNumCh  = 0,
		.DecChList = {0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0},
	},
};

extern int StartEncoderMangerServer2();
extern int ReadRemoteCtrlIndex(char *config_file);

Int32 reach_demo_link_init(ENC2000_LINK_STRUCT *pstruct)
{
	pstruct->start_runing = 1;

	pstruct->enableOsdAlgLink = 1;
	//	pstruct->audioencLink[0].pacaphandle = NULL;
	//	pstruct->audioencLink[0].paenchandle = NULL;
	//	pstruct->audioencLink[1].pacaphandle = NULL;
	//	pstruct->audioencLink[1].paenchandle = NULL;

	pstruct->capLink.link_id = SYSTEM_LINK_ID_CAPTURE;
	pstruct->nullSrcLink.link_id = SYSTEM_VPSS_LINK_ID_NULL_SRC_0;
	pstruct->deiLink[0].link_id  = SYSTEM_LINK_ID_DEI_HQ_0;
	pstruct->deiLink[1].link_id  = SYSTEM_LINK_ID_DEI_HQ_1;

	PRINTF("pstruct->capLink.link_id=0x%x\n", pstruct->capLink.link_id);
	PRINTF("pstruct->nullSrcLink.link_id=0x%x\n", pstruct->nullSrcLink.link_id);
	PRINTF("pstruct->deiLink[0].link_id=0x%x\n", pstruct->deiLink[0].link_id);
	PRINTF("pstruct->deiLink[1].link_id=0x%x\n", pstruct->deiLink[1].link_id);




	pstruct->mergeLink[0].link_id = SYSTEM_VPSS_LINK_ID_MERGE_0;
	pstruct->mergeLink[1].link_id = SYSTEM_VPSS_LINK_ID_MERGE_1;
	pstruct->mergeLink[2].link_id = SYSTEM_VPSS_LINK_ID_MERGE_2;
	pstruct->mergeLink[3].link_id = SYSTEM_VPSS_LINK_ID_MERGE_3;
	pstruct->mergeLink[4].link_id = SYSTEM_VPSS_LINK_ID_MERGE_4;

	PRINTF("pstruct->mergeLink[0].link_id=0x%x\n", pstruct->mergeLink[0].link_id);
	PRINTF("pstruct->mergeLink[1].link_id=0x%x\n", pstruct->mergeLink[1].link_id);
	PRINTF("pstruct->mergeLink[2].link_id=0x%x\n", pstruct->mergeLink[2].link_id);
	PRINTF("pstruct->mergeLink[3].link_id=0x%x\n", pstruct->mergeLink[3].link_id);

	pstruct->selectLink[0].link_id = SYSTEM_VPSS_LINK_ID_SELECT_0;
	pstruct->selectLink[1].link_id = SYSTEM_VPSS_LINK_ID_SELECT_1;
	pstruct->selectLink[2].link_id = SYSTEM_VPSS_LINK_ID_SELECT_2;
	pstruct->selectLink[3].link_id = SYSTEM_VPSS_LINK_ID_SELECT_3;

	PRINTF("pstruct->selectLink[0].link_id=0x%x\n", pstruct->selectLink[0].link_id);
	PRINTF("pstruct->selectLink[1].link_id=0x%x\n", pstruct->selectLink[1].link_id);
	PRINTF("pstruct->selectLink[2].link_id=0x%x\n", pstruct->selectLink[2].link_id);
	PRINTF("pstruct->selectLink[3].link_id=0x%x\n", pstruct->selectLink[3].link_id);


	pstruct->swmsLink[0].link_id = SYSTEM_LINK_ID_SW_MS_MULTI_INST_0;
	pstruct->swmsLink[1].link_id = SYSTEM_LINK_ID_SW_MS_MULTI_INST_1;
	pstruct->nsfLink[0].link_id = SYSTEM_LINK_ID_NSF_0;
	pstruct->nsfLink[1].link_id = SYSTEM_LINK_ID_NSF_1;
	pstruct->dupLink[0].link_id = SYSTEM_VPSS_LINK_ID_DUP_0;
	pstruct->dupLink[1].link_id = SYSTEM_VPSS_LINK_ID_DUP_1;

	PRINTF("pstruct->swmsLink[0].link_id=0x%x\n", pstruct->swmsLink[0].link_id);
	PRINTF("pstruct->swmsLink[1].link_id=0x%x\n", pstruct->swmsLink[1].link_id);
	PRINTF("pstruct->nsfLink[0].link_id=0x%x\n", pstruct->nsfLink[0].link_id);
	PRINTF("pstruct->nsfLink[1].link_id=0x%x\n", pstruct->nsfLink[1].link_id);
	PRINTF("pstruct->dupLink[0].link_id=0x%x\n", pstruct->dupLink[0].link_id);
	PRINTF("pstruct->dupLink[1].link_id=0x%x\n", pstruct->dupLink[1].link_id);


	pstruct->ipcFramesOutVpssToHostId	= SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_0;
	pstruct->ipcFramesInHostId			= SYSTEM_HOST_LINK_ID_IPC_FRAMES_IN_0;

	pstruct->sclrLink[0].link_id = SYSTEM_LINK_ID_SCLR_INST_0;
	pstruct->sclrLink[1].link_id = SYSTEM_LINK_ID_SCLR_INST_1;

	pstruct->encLink.link_id = SYSTEM_LINK_ID_VENC_0;

	pstruct->ipc_outvpss_Link.link_id = SYSTEM_VPSS_LINK_ID_IPC_OUT_M3_0;
	pstruct->ipc_invideo_Link.link_id = SYSTEM_VIDEO_LINK_ID_IPC_IN_M3_0;

	pstruct->ipcbit_outvideo_Link.link_id = SYSTEM_VIDEO_LINK_ID_IPC_BITS_OUT_0;
	pstruct->ipcbit_inhost_Link.link_id = SYSTEM_HOST_LINK_ID_IPC_BITS_IN_0;

	PRINTF("pstruct->ipcFramesOutVpssToHostId=0x%x\n", pstruct->ipcFramesOutVpssToHostId);
	PRINTF("pstruct->ipcFramesInHostId=0x%x\n", pstruct->ipcFramesInHostId);
	PRINTF("pstruct->sclrLink[0].link_id=0x%x\n", pstruct->sclrLink[0].link_id);
	PRINTF("pstruct->sclrLink[1].link_id=0x%x\n", pstruct->sclrLink[1].link_id);
	PRINTF("pstruct->encLink.link_id=0x%x\n", pstruct->encLink.link_id);
	PRINTF("pstruct->ipc_outvpss_Link.link_id=0x%x\n", pstruct->ipc_outvpss_Link.link_id);

	PRINTF("pstruct->ipc_invideo_Link.link_id=0x%x\n", pstruct->ipc_invideo_Link.link_id);
	PRINTF("pstruct->ipcbit_outvideo_Link.link_id=0x%x\n", pstruct->ipcbit_outvideo_Link.link_id);
	PRINTF("pstruct->ipcbit_inhost_Link.link_id=0x%x\n", pstruct->ipcbit_inhost_Link.link_id);



	if(pstruct->enableOsdAlgLink) {
		pstruct->ipcFramesOutVpssId[0]   	= SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_1;
		pstruct->osd_dspAlg_Link[0].link_id = SYSTEM_LINK_ID_ALG_0;
		pstruct->ipcFrames_indsp_link[0].link_id = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_0;
		PRINTF("pstruct->osd_dspAlg_Link[0].link_id = %d\n", pstruct->osd_dspAlg_Link[0].link_id);
		PRINTF("pstruct->ipcFrames_indsp_link[0].link_id = %d\n", pstruct->ipcFrames_indsp_link[0].link_id);
		//MP
		//	pstruct->ipcFramesOutVpssId[1]		= SYSTEM_VPSS_LINK_ID_IPC_FRAMES_OUT_2;
		//	pstruct->osd_dspAlg_Link[1].link_id = SYSTEM_LINK_ID_ALG_1;
		//	pstruct->ipcFrames_indsp_link[1].link_id = SYSTEM_DSP_LINK_ID_IPC_FRAMES_IN_1;
	}

	PRINTF("pstruct->merger[3].link_id = %d\n", pstruct->mergeLink[2].link_id);
	PRINTF("pstruct->merger[3].link_id = %d\n", pstruct->mergeLink[3].link_id);
	PRINTF("pstruct->selectLink[0].link_id = %d\n", pstruct->selectLink[0].link_id);
	PRINTF("pstruct->selectLink[1].link_id = %d\n", pstruct->selectLink[1].link_id);

	cap_init_create_param(&(pstruct->capLink.create_params));
	nullsrc_init_create_param(&(pstruct->nullSrcLink.create_params));
	dei_init_create_param(&(pstruct->deiLink[0].create_params));
	dei_init_create_param(&(pstruct->deiLink[1].create_params));
	merge_init_create_param(&(pstruct->mergeLink[0].create_params));
	merge_init_create_param(&(pstruct->mergeLink[1].create_params));
	merge_init_create_param(&(pstruct->mergeLink[2].create_params));
	merge_init_create_param(&(pstruct->mergeLink[3].create_params));
	merge_init_create_param(&(pstruct->mergeLink[4].create_params));
	select_init_create_param(&(pstruct->selectLink[0].create_params));
	select_init_create_param(&(pstruct->selectLink[1].create_params));
	select_init_create_param(&(pstruct->selectLink[2].create_params));
	select_init_create_param(&(pstruct->selectLink[3].create_params));
	swms_init_create_param(&(pstruct->swmsLink[0].create_params));
	swms_init_create_param(&(pstruct->swmsLink[1].create_params));
	nsf_init_create_param(&(pstruct->nsfLink[0].create_params));
	nsf_init_create_param(&(pstruct->nsfLink[1].create_params));
	dup_init_create_param(&(pstruct->dupLink[0].create_params));
	dup_init_create_param(&(pstruct->dupLink[1].create_params));

	REACH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams, pstruct->ipcFramesOutVpssToHostPrm);
	REACH_INIT_STRUCT(IpcFramesInLinkHLOS_CreateParams, pstruct->ipcFramesInHostPrm);

	ipcoutm3_init_create_param(&(pstruct->ipc_outvpss_Link.create_params));
	ipcinm3_init_create_param(&(pstruct->ipc_invideo_Link.create_params));
	enc_init_create_param(&(pstruct->encLink.create_params));
	sclr_init_create_param(&(pstruct->sclrLink[0].create_params));
	sclr_init_create_param(&(pstruct->sclrLink[1].create_params));
	REACH_INIT_STRUCT(IpcBitsOutLinkRTOS_CreateParams, pstruct->ipcbit_outvideo_Link.create_params);
	REACH_INIT_STRUCT(IpcBitsInLinkHLOS_CreateParams, pstruct->ipcbit_inhost_Link.create_params);


	if(pstruct->enableOsdAlgLink) {
		REACH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams, (pstruct->ipcFramesOutVpssPrm[0]));
		REACH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  , (pstruct->ipcFrames_indsp_link[0].create_params));
		REACH_INIT_STRUCT(AlgLink_CreateParams               , (pstruct->osd_dspAlg_Link[0].create_params));
		// memset(&(pstruct->ipcFramesOutVpssPrm[0]),0,sizeof(IpcFramesOutLinkRTOS_CreateParams));
		//memset(&(pstruct->ipcFramesOutVpssPrm[0].baseCreateParams),0,sizeof(IpcLink_CreateParams));

		//mp
		//REACH_INIT_STRUCT(IpcFramesOutLinkRTOS_CreateParams, (pstruct->ipcFramesOutVpssPrm[1]));
		//	REACH_INIT_STRUCT(IpcFramesInLinkRTOS_CreateParams  , (pstruct->ipcFrames_indsp_link[1].create_params));
		//	REACH_INIT_STRUCT(AlgLink_CreateParams               , (pstruct->osd_dspAlg_Link[1].create_params));
	}

	return 0;
}

Int32 reach_demo_create_and_start(ENC2000_LINK_STRUCT *pstruct)
{
	System_linkCreate(pstruct->capLink.link_id, &(pstruct->capLink.create_params), sizeof(CaptureLink_CreateParams));
	cap_config_videodecoder(pstruct->capLink.link_id);
	System_linkCreate(pstruct->nullSrcLink.link_id, &(pstruct->nullSrcLink.create_params), sizeof(NullSrcLink_CreateParams));
	System_linkCreate(pstruct->mergeLink[0].link_id, &(pstruct->mergeLink[0].create_params), sizeof(MergeLink_CreateParams));
	System_linkCreate(pstruct->selectLink[0].link_id, &(pstruct->selectLink[0].create_params), sizeof(SelectLink_CreateParams));
	System_linkCreate(pstruct->deiLink[0].link_id, &(pstruct->deiLink[0].create_params), sizeof(DeiLink_CreateParams));
	System_linkCreate(pstruct->deiLink[1].link_id, &(pstruct->deiLink[1].create_params), sizeof(DeiLink_CreateParams));
	System_linkCreate(pstruct->mergeLink[1].link_id, &(pstruct->mergeLink[1].create_params), sizeof(MergeLink_CreateParams));

	if(pstruct->enableOsdAlgLink) {
		PRINTF("---------osdlink 1----------------\n");
		System_linkCreate(pstruct->ipcFramesOutVpssId[0], &(pstruct->ipcFramesOutVpssPrm[0]), sizeof(pstruct->ipcFramesOutVpssPrm[0]));

		PRINTF("---------osdlink 1----------------\n");
		System_linkCreate(pstruct->ipcFrames_indsp_link[0].link_id, &(pstruct->ipcFrames_indsp_link[0].create_params), sizeof(IpcFramesInLinkRTOS_CreateParams));
		PRINTF("---------osdlink 3------------%u----\n", pstruct->ipcFrames_indsp_link[0].link_id);
		System_linkCreate(pstruct->osd_dspAlg_Link[0].link_id, &(pstruct->osd_dspAlg_Link[0].create_params), sizeof(AlgLink_CreateParams));
		PRINTF("---------osdlink 4--------%u--------\n", pstruct->osd_dspAlg_Link[0].link_id);
	}



	System_linkCreate(pstruct->selectLink[1].link_id, &(pstruct->selectLink[1].create_params), sizeof(SelectLink_CreateParams));

	System_linkCreate(pstruct->dupLink[0].link_id, &(pstruct->dupLink[0].create_params), sizeof(DupLink_CreateParams));
	System_linkCreate(pstruct->dupLink[1].link_id, &(pstruct->dupLink[1].create_params), sizeof(DupLink_CreateParams));

#if WRITE_YUV_TASK
	System_linkCreate(pstruct->mergeLink[4].link_id, &(pstruct->mergeLink[4].create_params), sizeof(MergeLink_CreateParams));
	System_linkCreate(pstruct->ipcFramesOutVpssToHostId,
	                  &(pstruct->ipcFramesOutVpssToHostPrm), sizeof(IpcFramesOutLinkRTOS_CreateParams));
	System_linkCreate(pstruct->ipcFramesInHostId,
	                  &(pstruct->ipcFramesInHostPrm), sizeof(IpcFramesInLinkHLOS_CreateParams));
#endif

	System_linkCreate(pstruct->mergeLink[2].link_id, &(pstruct->mergeLink[2].create_params), sizeof(MergeLink_CreateParams));
	System_linkCreate(pstruct->selectLink[2].link_id, &(pstruct->selectLink[2].create_params), sizeof(SelectLink_CreateParams));
	System_linkCreate(pstruct->sclrLink[1].link_id, &(pstruct->sclrLink[1].create_params), sizeof(SclrLink_CreateParams));
	System_linkCreate(pstruct->nsfLink[1].link_id, &(pstruct->nsfLink[1].create_params), sizeof(NsfLink_CreateParams));

	System_linkCreate(pstruct->mergeLink[3].link_id, &(pstruct->mergeLink[3].create_params), sizeof(MergeLink_CreateParams));
	System_linkCreate(pstruct->selectLink[3].link_id, &(pstruct->selectLink[3].create_params), sizeof(SelectLink_CreateParams));
	System_linkCreate(pstruct->ipc_outvpss_Link.link_id, &(pstruct->ipc_outvpss_Link.create_params), sizeof(IpcLink_CreateParams));
	System_linkCreate(pstruct->ipc_invideo_Link.link_id, &(pstruct->ipc_invideo_Link.create_params), sizeof(IpcLink_CreateParams));
	System_linkCreate(pstruct->encLink.link_id, &(pstruct->encLink.create_params), sizeof(EncLink_CreateParams));
	System_linkCreate(pstruct->ipcbit_outvideo_Link.link_id, &(pstruct->ipcbit_outvideo_Link.create_params), sizeof(IpcBitsOutLinkRTOS_CreateParams));
	System_linkCreate(pstruct->ipcbit_inhost_Link.link_id, &(pstruct->ipcbit_inhost_Link.create_params), sizeof(IpcBitsInLinkHLOS_CreateParams));


	System_linkStart(pstruct->ipcbit_inhost_Link.link_id);
	System_linkStart(pstruct->ipcbit_outvideo_Link.link_id);
	System_linkStart(pstruct->encLink.link_id);
	System_linkStart(pstruct->ipc_invideo_Link.link_id);
	System_linkStart(pstruct->ipc_outvpss_Link.link_id);
	System_linkStart(pstruct->selectLink[3].link_id);
	System_linkStart(pstruct->mergeLink[3].link_id);

	System_linkStart(pstruct->nsfLink[1].link_id);
	System_linkStart(pstruct->sclrLink[1].link_id);
	System_linkStart(pstruct->selectLink[2].link_id);
	System_linkStart(pstruct->mergeLink[2].link_id);


	System_linkStart(pstruct->dupLink[1].link_id);
	System_linkStart(pstruct->dupLink[0].link_id);

	System_linkStart(pstruct->selectLink[1].link_id);

	System_linkStart(pstruct->mergeLink[1].link_id);

	System_linkStart(pstruct->deiLink[1].link_id);
	System_linkStart(pstruct->deiLink[0].link_id);
	System_linkStart(pstruct->selectLink[0].link_id);


	PRINTF("...........\n");


	if(pstruct->enableOsdAlgLink) {

		/* OSD Related configuraion and DSP Link parameter update*/
		//   memset(osdFormat, SYSTEM_DF_YUV420SP_UV, gSD_Demo_ctrl.numEncChannels);
		//SD_Demo_osdInit(2, &osdFromat, pstruct->osd_dspAlg_Link[0].link_id);
		SD_Demo_osdInit(pstruct->osd_dspAlg_Link[0].link_id, IS_IND_STATUS);
	}

	System_linkStart(pstruct->mergeLink[0].link_id);
	System_linkStart(pstruct->nullSrcLink.link_id);
	System_linkStart(pstruct->capLink.link_id);

	return 0;
}

Int32 reach_MP_create_and_start(ENC2000_LINK_STRUCT *pstruct)
{
	System_linkCreate(pstruct->capLink.link_id, &(pstruct->capLink.create_params), sizeof(CaptureLink_CreateParams));
	cap_config_videodecoder(pstruct->capLink.link_id);
	System_linkCreate(pstruct->nullSrcLink.link_id, &(pstruct->nullSrcLink.create_params), sizeof(NullSrcLink_CreateParams));

	System_linkCreate(pstruct->mergeLink[0].link_id, &(pstruct->mergeLink[0].create_params), sizeof(MergeLink_CreateParams));
	System_linkCreate(pstruct->selectLink[0].link_id, &(pstruct->selectLink[0].create_params), sizeof(SelectLink_CreateParams));
	System_linkCreate(pstruct->deiLink[0].link_id, &(pstruct->deiLink[0].create_params), sizeof(DeiLink_CreateParams));
	System_linkCreate(pstruct->deiLink[1].link_id, &(pstruct->deiLink[1].create_params), sizeof(DeiLink_CreateParams));
	System_linkCreate(pstruct->mergeLink[1].link_id, &(pstruct->mergeLink[1].create_params), sizeof(MergeLink_CreateParams));

	System_linkCreate(pstruct->selectLink[1].link_id, &(pstruct->selectLink[1].create_params), sizeof(SelectLink_CreateParams));

#if WRITE_YUV_TASK
	System_linkCreate(pstruct->dupLink[1].link_id, &(pstruct->dupLink[1].create_params), sizeof(DupLink_CreateParams));
#endif

	System_linkCreate(pstruct->swmsLink[0].link_id, &(pstruct->swmsLink[0].create_params), sizeof(SwMsLink_CreateParams));

	if(pstruct->enableOsdAlgLink) {
		System_linkCreate(pstruct->ipcFramesOutVpssId[0], &(pstruct->ipcFramesOutVpssPrm[0]), sizeof(pstruct->ipcFramesOutVpssPrm[0]));
		PRINTF("ipcFramesOutVpssId...\n");
		System_linkCreate(pstruct->ipcFrames_indsp_link[0].link_id, &(pstruct->ipcFrames_indsp_link[0].create_params), sizeof(IpcFramesInLinkRTOS_CreateParams));
		PRINTF("ipcFrames_indsp_link...\n");
		System_linkCreate(pstruct->osd_dspAlg_Link[0].link_id, &(pstruct->osd_dspAlg_Link[0].create_params), sizeof(AlgLink_CreateParams));
		PRINTF("osd_dspAlg_Link...\n");
	}

	System_linkCreate(pstruct->dupLink[0].link_id, &(pstruct->dupLink[0].create_params), sizeof(DupLink_CreateParams));
	System_linkCreate(pstruct->mergeLink[2].link_id, &(pstruct->mergeLink[2].create_params), sizeof(MergeLink_CreateParams));


#if WRITE_YUV_TASK
	System_linkCreate(pstruct->mergeLink[3].link_id, &(pstruct->mergeLink[3].create_params), sizeof(MergeLink_CreateParams));
	System_linkCreate(pstruct->ipcFramesOutVpssToHostId,
	                  &(pstruct->ipcFramesOutVpssToHostPrm), sizeof(IpcFramesOutLinkRTOS_CreateParams));
	System_linkCreate(pstruct->ipcFramesInHostId,
	                  &(pstruct->ipcFramesInHostPrm), sizeof(IpcFramesInLinkHLOS_CreateParams));
#endif


	System_linkCreate(pstruct->sclrLink[0].link_id, &(pstruct->sclrLink[0].create_params), sizeof(SclrLink_CreateParams));
	System_linkCreate(pstruct->nsfLink[0].link_id, &(pstruct->nsfLink[0].create_params), sizeof(NsfLink_CreateParams));

	System_linkCreate(pstruct->ipc_outvpss_Link.link_id, &(pstruct->ipc_outvpss_Link.create_params), sizeof(IpcLink_CreateParams));
	System_linkCreate(pstruct->ipc_invideo_Link.link_id, &(pstruct->ipc_invideo_Link.create_params), sizeof(IpcLink_CreateParams));
	System_linkCreate(pstruct->encLink.link_id, &(pstruct->encLink.create_params), sizeof(EncLink_CreateParams));
	System_linkCreate(pstruct->ipcbit_outvideo_Link.link_id, &(pstruct->ipcbit_outvideo_Link.create_params), sizeof(IpcBitsOutLinkRTOS_CreateParams));
	System_linkCreate(pstruct->ipcbit_inhost_Link.link_id, &(pstruct->ipcbit_inhost_Link.create_params), sizeof(IpcBitsInLinkHLOS_CreateParams));



	System_linkStart(pstruct->ipcbit_inhost_Link.link_id);
	System_linkStart(pstruct->ipcbit_outvideo_Link.link_id);
	System_linkStart(pstruct->encLink.link_id);
	System_linkStart(pstruct->ipc_invideo_Link.link_id);
	System_linkStart(pstruct->ipc_outvpss_Link.link_id);

	System_linkStart(pstruct->nsfLink[0].link_id);
	System_linkStart(pstruct->sclrLink[0].link_id);

#if WRITE_YUV_TASK
	System_linkStart(pstruct->ipcFramesInHostId);
	System_linkStart(pstruct->ipcFramesOutVpssToHostId);
	System_linkStart(pstruct->mergeLink[3].link_id);
#endif



	System_linkStart(pstruct->mergeLink[2].link_id);
	System_linkStart(pstruct->dupLink[0].link_id);

	if(pstruct->enableOsdAlgLink) {
		SD_Demo_osdInit(pstruct->osd_dspAlg_Link[0].link_id, IS_MP_STATUS);
	}

	System_linkStart(pstruct->swmsLink[0].link_id);

#if WRITE_YUV_TASK
	System_linkStart(pstruct->dupLink[1].link_id);
#endif

	System_linkStart(pstruct->selectLink[1].link_id);

	System_linkStart(pstruct->mergeLink[1].link_id);
	System_linkStart(pstruct->deiLink[1].link_id);
	System_linkStart(pstruct->deiLink[0].link_id);
	System_linkStart(pstruct->selectLink[0].link_id);
	System_linkStart(pstruct->mergeLink[0].link_id);

	System_linkStart(pstruct->nullSrcLink.link_id);
	System_linkStart(pstruct->capLink.link_id);

	return 0;
}

void mp_reach_begin(void)
{
	//	Device_Adv7442Obj adv7442;
	//	Device_Adv7441Obj adv7441;

	PRINTF("\n\n........mp start.....\n\n");
	reach_config_cap_device(&gEnc2000);
	writeWatchDog();
	reach_demo_link_init(&gEnc2000);
	writeWatchDog();
	host_system_init();
	vpss_detect_board();
	writeWatchDog();
	vpss_reset_video_devices();
	writeWatchDog();
	tiler_disable_allocator();
	video_set_ch2ivahd_map_tbl(&systemVid_encDecIvaChMapTbl);
	writeWatchDog();
	//mp
	//	SclrLink_chDynamicSetOutRes sclr_params;
	reach_MP_vcapture_process(&gEnc2000);
	writeWatchDog();
	reach_MP_select_process(&gEnc2000);
	reach_venc_process(&gEnc2000, 2, IS_MP_STATUS);
	reach_host_bits_process(&gEnc2000);
	writeWatchDog();
	reach_MP_create_and_start(&gEnc2000);
	writeWatchDog();
	sleep(2);
	/*
		sclr_params.chId = 0;
		sclr_params.width = 704;
		sclr_params.height = 576;
		sclr_params.pitch[0] = 1408;
		sclr_params.pitch[1] = 0;
		sclr_set_output_resolution(gEnc2000.sclrLink[0].link_id, &sclr_params);
		sclr_set_framerate(gEnc2000.sclrLink[0].link_id, 0, 60, 30);
	*/
	writeWatchDog();
	set_mp_status(IS_MP_STATUS);
	writeWatchDog();
	reach_video_detect_task_process(&gEnc2000);
	writeWatchDog();
	app_mp_video_control();
	writeWatchDog();
	//	Device_adv7442Control(&adv7442, DEVICE_CMD_START, NULL, NULL);
	//	Device_adv7441Control(&adv7441, DEVICE_CMD_START, NULL, NULL);

	if(0 == program_begin) {
		g_osd_enable = TRUE;
		update_osd_change_mp();
	}

	writeWatchDog();
	program_begin = 0;
	app_init_no_signal_res();
	writeWatchDog();

	if(get_mp_status() == IS_MP_STATUS) {
		PRINTF("-------MP --SET I FREAM---\n");
		app_video_request_iframe(CHANNEL_INPUT_MP);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_MP_LOW);
	} else if(get_mp_status == IS_IND_STATUS) {
		PRINTF("-------ind --SET I FREAM---\n");
		app_video_request_iframe(CHANNEL_INPUT_1);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_1_LOW);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_2);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_2_LOW);
	}

	writeWatchDog();
	app_mp_video_control();
	PRINTF("\n\n8888888888____mp start finished!\n !\n");

}

Int mp_reach_stop(void)
{
	//	Device_Adv7442Obj adv7442;
	//	Device_Adv7441Obj adv7441;

	//	int cnt = 0;

	g_osd_enable = FALSE;

	gEnc2000.capLink.isDetect = FALSE;

	set_mp_status(SWITCH_STATUS);
	PRINTF("\n\n........mp stop.....\n\n");

	PRINTF("STOP link Done\n");

	sleep(1);//write thread exit

	//device stop
	//	Device_adv7442Control(&adv7442, DEVICE_CMD_STOP, NULL, NULL);
	//	Device_adv7441Control(&adv7441, DEVICE_CMD_STOP, NULL, NULL);
	PRINTF("STOP adv VP\n");

	//cap stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.capLink.link_id) {
		System_linkStop(gEnc2000.capLink.link_id);
	}

	//NULL stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nullSrcLink.link_id) {
		System_linkStop(gEnc2000.nullSrcLink.link_id);
	}


	//merge0 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[0].link_id) {
		System_linkStop(gEnc2000.mergeLink[0].link_id);
	}

	//select0 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[0].link_id) {
		System_linkStop(gEnc2000.selectLink[0].link_id);
	}

	//dei stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[0].link_id) {
		System_linkStop(gEnc2000.deiLink[0].link_id);
	}


	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[1].link_id) {
		System_linkStop(gEnc2000.deiLink[1].link_id);
	}

	//merge1 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[1].link_id) {
		System_linkStop(gEnc2000.mergeLink[1].link_id);
	}

	//selectLink1 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[1].link_id) {
		System_linkStop(gEnc2000.selectLink[1].link_id);
	}

	if(SYSTEM_LINK_ID_INVALID != gEnc2000.swmsLink[0].link_id) {
		System_linkStop(gEnc2000.swmsLink[0].link_id);
	}

	//osd
	if(gEnc2000.enableOsdAlgLink) {
		System_linkStop(gEnc2000.ipcFramesOutVpssId[0]);
		System_linkStop(gEnc2000.ipcFrames_indsp_link[0].link_id);
		System_linkStop(gEnc2000.osd_dspAlg_Link[0].link_id);
		PRINTF("STOP adv ipcFrames_indsp_link!\n");
		PRINTF("STOP adv osd_dspAlg_Link!\n");
	}

	//dup stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.dupLink[0].link_id) {
		System_linkStop(gEnc2000.dupLink[0].link_id);
	}

	//merge2 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[2].link_id) {
		System_linkStop(gEnc2000.mergeLink[2].link_id);
	}


	//selectLink2 stop
	//	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[2].link_id) {
	//		System_linkStop(gEnc2000.selectLink[2].link_id);
	//	}

	//sclr stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.sclrLink[0].link_id) {
		System_linkStop(gEnc2000.sclrLink[0].link_id);
	}

	//nfs stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nsfLink[0].link_id) {
		System_linkStop(gEnc2000.nsfLink[0].link_id);
	}

#if 0

	//merge3 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[3].link_id) {
		System_linkStop(gEnc2000.mergeLink[3].link_id);
	}

	//selectLink3 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[3].link_id) {
		System_linkStop(gEnc2000.selectLink[3].link_id);
	}

#endif

	//ipc_outvpss_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_outvpss_Link.link_id) {
		System_linkStop(gEnc2000.ipc_outvpss_Link.link_id);
	}

	//ipc_invideo_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_invideo_Link.link_id) {
		System_linkStop(gEnc2000.ipc_invideo_Link.link_id);
	}

	//enc_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.encLink.link_id) {
		System_linkStop(gEnc2000.encLink.link_id);
	}

	//ipcbit_outvideo_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_outvideo_Link.link_id) {
		System_linkStop(gEnc2000.ipcbit_outvideo_Link.link_id);
		PRINTF("STOP adv ipcbit_outvideo_Link!\n");
	}


	//ipcbit_inhost_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_inhost_Link.link_id) {
		System_linkStop(gEnc2000.ipcbit_inhost_Link.link_id);
	}


	ipcbit_delete_bitsprocess_inst(g_bit_handle);
	PRINTF("STOP  bits sprocess!\n");


	//................................................
	// delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.capLink.link_id) {
		System_linkDelete(gEnc2000.capLink.link_id);
	}

	//NULL delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nullSrcLink.link_id) {
		System_linkDelete(gEnc2000.nullSrcLink.link_id);
	}

	//merge0 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[0].link_id) {
		System_linkDelete(gEnc2000.mergeLink[0].link_id);
	}

	//select0 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[0].link_id) {
		System_linkDelete(gEnc2000.selectLink[0].link_id);
		PRINTF("DELETE link selectLink\n");
	}

	//dei delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[0].link_id) {
		System_linkDelete(gEnc2000.deiLink[0].link_id);
	}

	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[1].link_id) {
		System_linkDelete(gEnc2000.deiLink[1].link_id);
	}

	//merge1 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[1].link_id) {
		System_linkDelete(gEnc2000.mergeLink[1].link_id);
		PRINTF("DELETE link mergeLink\n");
	}

	//selectLink1 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[1].link_id) {
		System_linkDelete(gEnc2000.selectLink[1].link_id);
		PRINTF("DELETE link selectLink\n");
	}

	//swms delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.swmsLink[0].link_id) {
		System_linkDelete(gEnc2000.swmsLink[0].link_id);
		PRINTF("DELETE link dupLink\n");
	}

	//osd
	if(gEnc2000.enableOsdAlgLink) {
		System_linkDelete(gEnc2000.ipcFramesOutVpssId[0]);
		System_linkDelete(gEnc2000.ipcFrames_indsp_link[0].link_id);
		System_linkDelete(gEnc2000.osd_dspAlg_Link[0].link_id);
		SD_Demo_osdDeinit();
		PRINTF("--------------osd delete----------1--------\n");
		PRINTF("--------------osd delete-----------2-------\n");
		PRINTF("--------------osd delete-----------3-------\n");
	}

	//dup delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.dupLink[0].link_id) {
		System_linkDelete(gEnc2000.dupLink[0].link_id);
		PRINTF("DELETE link dupLink\n");
	}

	//	if(SYSTEM_LINK_ID_INVALID != gEnc2000.dupLink[1].link_id) {
	//		System_linkDelete(gEnc2000.dupLink[1].link_id);
	//	}

	//merge2 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[2].link_id) {
		System_linkDelete(gEnc2000.mergeLink[2].link_id);
	}


	//selectLink2 delete
	//	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[2].link_id) {
	//		System_linkDelete(gEnc2000.selectLink[2].link_id);
	//	}

	//sclr delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.sclrLink[0].link_id) {
		System_linkDelete(gEnc2000.sclrLink[0].link_id);
	}

	//nfs delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nsfLink[0].link_id) {
		System_linkDelete(gEnc2000.nsfLink[0].link_id);
	}

	//merge3 delete
	//	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[3].link_id) {
	//		System_linkDelete(gEnc2000.mergeLink[3].link_id);
	//	}

	//selectLink3 delete
	//	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[3].link_id) {
	//		System_linkDelete(gEnc2000.selectLink[3].link_id);
	//	}

	//ipc_outvpss_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_outvpss_Link.link_id) {
		System_linkDelete(gEnc2000.ipc_outvpss_Link .link_id);
	}

	//ipc_invideo_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_invideo_Link.link_id) {
		System_linkDelete(gEnc2000.ipc_invideo_Link.link_id);
	}

	//enc_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.encLink.link_id) {
		System_linkDelete(gEnc2000.encLink.link_id);
	}


	//ipcbit_outvideo_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_outvideo_Link.link_id) {
		System_linkDelete(gEnc2000.ipcbit_outvideo_Link.link_id);
	}

	//ipcbit_inhost_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_inhost_Link.link_id) {
		System_linkDelete(gEnc2000.ipcbit_inhost_Link.link_id);
	}

	PRINTF("DELETE link Done\n");

	tiler_enable_allocator();

	host_system_deinit();

	//Device_DeInit();

	PRINTF("Deinit Done! \n");

	PRINTF("\n\n........mp stop done .....\n\n");
	return 0;
}


Int32 reach_print_current_status(ENC2000_LINK_STRUCT *pstruct, Int32 time)
{
	dsp_calc_cpuload_start();
	//	char ch = 0 ;
	int mp_status = get_mp_status();
	int i = 0;
	EncLink_GetDynParams video_params;

	while(1) {

		sleep(5);
		dsp_print_cpuload();

		dsp_calc_cpuload_reset();
		continue;

		if(0) {
			mp_status = get_mp_status();

			if(mp_status == IS_IND_STATUS) {
				for(i = 0; i < 4; i++) {
					memset(&video_params, 0, sizeof(video_params));
					video_params.chId = i;
					enc_get_dynparams(pstruct->encLink.link_id, &video_params);
					PRINTF("chId=%d,inputwidth:%d,inputheight:%d,bitrate:%d,fps:%d,gop:%d\n",
					       video_params.chId, video_params.inputWidth, video_params.inputHeight,
					       video_params.targetBitRate, video_params.targetFps, video_params.intraFrameInterval);
				}
			} else if(mp_status == IS_MP_STATUS) {

				for(i = 0; i < 2; i++) {
					memset(&video_params, 0, sizeof(video_params));
					video_params.chId = i;
					enc_get_dynparams(pstruct->encLink.link_id, &video_params);
					PRINTF("chId=%d,inputwidth:%d,inputheight:%d,bitrate:%d,fps:%d,gop:%d\n",
					       video_params.chId, video_params.inputWidth, video_params.inputHeight,
					       video_params.targetBitRate, video_params.targetFps, video_params.intraFrameInterval);

				}
			}

			continue;
		}

#define KEL_PRINTF_FLAG  1
#if KEL_PRINTF_FLAG
		PRINTF("demo is running........\n");

		cap_print_advstatistics(pstruct->capLink.link_id);
		sleep(1);
		dei_print_statics(pstruct->deiLink[0].link_id);
		sleep(1);
		dei_print_statics(pstruct->deiLink[1].link_id);
		sleep(1);

		if(mp_status == IS_MP_STATUS) {
			swms_print_statics(pstruct->swmsLink[0].link_id);
			sleep(1);
		}

		sclr_print_statics(pstruct->sclrLink[1].link_id);
		sleep(1);


		//if(mp_status == IS_IND_STATUS) {
		//	sclr_print_statics(pstruct->sclrLink[1].link_id);
		//	sleep(1);
		//}

		nsf_print_static(pstruct->nsfLink[0].link_id);
		sleep(1);

		enc_print_statistics(pstruct->encLink.link_id);
		sleep(3);
		enc_print_ivahd_statistics(pstruct->encLink.link_id);
		PRINTF("-----------------\n");
		//dsp_print_cpuload();

		//	dsp_calc_cpuload_reset();
		sleep(3);
#else
		sleep(10);

#endif
	}

	return 0;
}

void  app_module_init()
{
	//add some app code by zm ,2012/9/1
	char ts_version[64] = {0};




#ifdef HAVE_RTSP_MODULE
	unsigned int dwAddr;
	ts_build_get_version(ts_version, sizeof(ts_version));

	//if(gProtocol.status == 0) {
	//	PRINTF("I will not init the stream out modules\n");
	//	} else
	{
		unsigned int ip = 0, ip1 = 0;
		unsigned int gateway = 0, gateway1 = 0;
		unsigned int netmask = 0, netmask1 = 0;
		int ret = 0;
		//		struct in_addr addr1;

		ret = sys_get_network(ETH0, &ip, &gateway, &netmask);
		ret = sys_get_network(ETH0_1, &ip1, &gateway1, &netmask1);

		if(ret == -1) {
			PRINTF("Error,get ip is error\n");

		} else {
			dwAddr =  ip;
			PRINTF("I will init the stream out modules\n");
			app_stream_output_init(ip, ip1);
		}
	}

#endif

	PRINTF("ts version is %s.\n", ts_version);
	return ;
}

typedef	void Sigfunc(int);

/*信号包裹函数*/
Sigfunc *Signal(int signo, Sigfunc *func)
{
	Sigfunc	*sigfunc;

	if((sigfunc = signal(signo, func)) == SIG_ERR) {
		ERR_PRN("ERROR:  signal error \n");
	}

	return(sigfunc);
}

static void set_mp_status(int status)
{
	g_mp_status = status;
}
int get_mp_status()
{
	return g_mp_status;
}
int test(int *p)
{
	*p = 0;
	return 0;
}

/*
* 对config文件夹操作*
*/
static int init_config_file(void)
{
	if(NULL == opendir(".config")) {
		system("mkdir .config");
		PRINTF("mkdir .config\n");
	}

	return 0;
}

//
int reach_stop(int done)
{
	//	Device_Adv7442Obj adv7442;
	//	Device_Adv7441Obj adv7441;
	//	int cnt = 0;

	g_osd_enable = FALSE;

	gEnc2000.capLink.isDetect = FALSE;

	PRINTF("\n........... separate stop ..........\n\n");

	PRINTF("STOP link Done\n");
	set_mp_status(SWITCH_STATUS);

	sleep(1);//write thread exit

	//device stop
	//	Device_adv7442Control(&adv7442, DEVICE_CMD_STOP, NULL, NULL);
	//	Device_adv7441Control(&adv7441, DEVICE_CMD_STOP, NULL, NULL);

	//cap stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.capLink.link_id) {
		System_linkStop(gEnc2000.capLink.link_id);
	}

	//NULL stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nullSrcLink.link_id) {
		System_linkStop(gEnc2000.nullSrcLink.link_id);
	}

	//merge0 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[0].link_id) {
		System_linkStop(gEnc2000.mergeLink[0].link_id);
	}

	//select0 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[0].link_id) {
		System_linkStop(gEnc2000.selectLink[0].link_id);
	}

	//dei stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[0].link_id) {
		System_linkStop(gEnc2000.deiLink[0].link_id);
	}


	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[1].link_id) {
		System_linkStop(gEnc2000.deiLink[1].link_id);
	}


	//merge1 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[1].link_id) {
		System_linkStop(gEnc2000.mergeLink[1].link_id);
	}

	if(gEnc2000.enableOsdAlgLink) {
		System_linkStop(gEnc2000.ipcFramesOutVpssId[0]);
		System_linkStop(gEnc2000.ipcFrames_indsp_link[0].link_id);
		System_linkStop(gEnc2000.osd_dspAlg_Link[0].link_id);
		PRINTF("STOP adv ipcFrames_indsp_link!\n");
		PRINTF("STOP adv osd_dspAlg_Link!\n");
	}

	//selectLink1 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[1].link_id) {
		System_linkStop(gEnc2000.selectLink[1].link_id);
	}

	//dup stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.dupLink[0].link_id) {
		System_linkStop(gEnc2000.dupLink[0].link_id);
	}

	if(SYSTEM_LINK_ID_INVALID != gEnc2000.dupLink[1].link_id) {
		System_linkStop(gEnc2000.dupLink[1].link_id);
	}

	//merge2 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[2].link_id) {
		System_linkStop(gEnc2000.mergeLink[2].link_id);
	}

	//selectLink2 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[2].link_id) {
		System_linkStop(gEnc2000.selectLink[2].link_id);
		PRINTF("STOP adv selectLink!\n");
	}

	//sclr stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.sclrLink[1].link_id) {
		System_linkStop(gEnc2000.sclrLink[1].link_id);
		PRINTF("STOP adv sclrLink!\n");
	}

	//nfs stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nsfLink[1].link_id) {
		System_linkStop(gEnc2000.nsfLink[1].link_id);
		PRINTF("STOP adv nsfLink!\n");
	}

	//merge3 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[3].link_id) {
		System_linkStop(gEnc2000.mergeLink[3].link_id);
		PRINTF("STOP adv mergeLink!\n");
	}

	//selectLink3 stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[3].link_id) {
		System_linkStop(gEnc2000.selectLink[3].link_id);
		PRINTF("STOP adv selectLink!\n");
	}

	//ipc_outvpss_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_outvpss_Link.link_id) {
		System_linkStop(gEnc2000.ipc_outvpss_Link.link_id);
		PRINTF("STOP adv selectLink!\n");
	}

	//ipc_invideo_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_invideo_Link.link_id) {
		System_linkStop(gEnc2000.ipc_invideo_Link.link_id);
		PRINTF("STOP adv ipc_invideo_Link!\n");
	}

	//enc_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.encLink.link_id) {
		System_linkStop(gEnc2000.encLink.link_id);
		PRINTF("STOP adv encLink!\n");
	}

	//ipcbit_outvideo_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_outvideo_Link.link_id) {
		System_linkStop(gEnc2000.ipcbit_outvideo_Link.link_id);
		PRINTF("STOP adv ipcbit_outvideo_Link!\n");
	}


	//ipcbit_inhost_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_inhost_Link.link_id) {
		System_linkStop(gEnc2000.ipcbit_inhost_Link.link_id);
		PRINTF("STOP adv ipcbit_inhost_Link!\n");
	}


	ipcbit_delete_bitsprocess_inst(g_bit_handle);
	PRINTF("STOP  bits sprocess!\n");


	//................................................
	// delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.capLink.link_id) {
		System_linkDelete(gEnc2000.capLink.link_id);
		PRINTF("DELETE link capLink\n");
	}

	//NULL delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nullSrcLink.link_id) {
		System_linkDelete(gEnc2000.nullSrcLink.link_id);
		PRINTF("DELETE link nullSrcLink\n");
	}

	//merge0 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[0].link_id) {
		System_linkDelete(gEnc2000.mergeLink[0].link_id);
		PRINTF("DELETE link mergeLink\n");
	}

	if(gEnc2000.enableOsdAlgLink) {
		System_linkDelete(gEnc2000.ipcFramesOutVpssId[0]);
		System_linkDelete(gEnc2000.ipcFrames_indsp_link[0].link_id);
		System_linkDelete(gEnc2000.osd_dspAlg_Link[0].link_id);
		SD_Demo_osdDeinit();
		PRINTF("DELETE link ipcFrames_indsp_link\n");
		PRINTF("DELETE link osd_dspAlg_Link\n");
	}

	//select0 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[0].link_id) {
		System_linkDelete(gEnc2000.selectLink[0].link_id);
		PRINTF("DELETE link selectLink\n");
	}

	//dei delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[0].link_id) {
		System_linkDelete(gEnc2000.deiLink[0].link_id);
		PRINTF("DELETE link deiLink\n");
	}

	if(SYSTEM_LINK_ID_INVALID != gEnc2000.deiLink[1].link_id) {
		System_linkDelete(gEnc2000.deiLink[1].link_id);
		PRINTF("DELETE link deiLink\n");
	}

	//merge1 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[1].link_id) {
		System_linkDelete(gEnc2000.mergeLink[1].link_id);
		PRINTF("DELETE link mergeLink\n");
	}

	//selectLink1 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[1].link_id) {
		System_linkDelete(gEnc2000.selectLink[1].link_id);
		PRINTF("DELETE link selectLink\n");
	}

	//dup delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.dupLink[0].link_id) {
		System_linkDelete(gEnc2000.dupLink[0].link_id);
		PRINTF("DELETE link dupLink\n");
	}

	if(SYSTEM_LINK_ID_INVALID != gEnc2000.dupLink[1].link_id) {
		System_linkDelete(gEnc2000.dupLink[1].link_id);
	}

	//merge2 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[2].link_id) {
		System_linkDelete(gEnc2000.mergeLink[2].link_id);
	}

	//selectLink2 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[2].link_id) {
		System_linkDelete(gEnc2000.selectLink[2].link_id);
	}

	//sclr delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.sclrLink[1].link_id) {
		System_linkDelete(gEnc2000.sclrLink[1].link_id);
	}

	//nfs delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.nsfLink[1].link_id) {
		System_linkDelete(gEnc2000.nsfLink[1].link_id);
	}

	//merge3 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.mergeLink[3].link_id) {
		System_linkDelete(gEnc2000.mergeLink[3].link_id);
	}

	//selectLink3 delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.selectLink[3].link_id) {
		System_linkDelete(gEnc2000.selectLink[3].link_id);
	}

	//ipc_outvpss_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_outvpss_Link .link_id) {
		System_linkDelete(gEnc2000.ipc_outvpss_Link .link_id);
	}

	//ipc_invideo_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipc_invideo_Link.link_id) {
		System_linkDelete(gEnc2000.ipc_invideo_Link.link_id);
	}

	//enc_Link stop
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.encLink.link_id) {
		System_linkDelete(gEnc2000.encLink.link_id);
	}


	//ipcbit_outvideo_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_outvideo_Link.link_id) {
		System_linkDelete(gEnc2000.ipcbit_outvideo_Link.link_id);
	}

	//ipcbit_inhost_Link delete
	if(SYSTEM_LINK_ID_INVALID != gEnc2000.ipcbit_inhost_Link.link_id) {
		System_linkDelete(gEnc2000.ipcbit_inhost_Link.link_id);
	}

	PRINTF("DELETE link Done\n");

	tiler_enable_allocator();

	host_system_deinit();

	PRINTF("\n\n........... separate stop done ..........\n\n");
	return 0;
}
int reach_begin(void)
{
	//	Device_Adv7442Obj adv7442;
	//	Device_Adv7441Obj adv7441;

	PRINTF("\n\n........... separate begin ..........\n\n");

	reach_config_cap_device(&gEnc2000);
	writeWatchDog();
	reach_demo_link_init(&gEnc2000);
	writeWatchDog();
	host_system_init();
	writeWatchDog();
	vpss_detect_board();
	writeWatchDog();
	vpss_reset_video_devices();
	writeWatchDog();
	tiler_disable_allocator();
	writeWatchDog();
	video_set_ch2ivahd_map_tbl(&systemVid_encDecIvaChMapTbl);
	PRINTF("************* init!\n");
	writeWatchDog();
	/* 单画面 */
	reach_vcapture_process(&gEnc2000);
	writeWatchDog();
	reach_select_process(&gEnc2000);
	writeWatchDog();
	reach_venc_process(&gEnc2000, 4, IS_IND_STATUS);
	writeWatchDog();
	reach_host_bits_process(&gEnc2000);
	writeWatchDog();
	reach_demo_create_and_start(&gEnc2000);
	PRINTF("************* start!\n");

	PRINTF(".........ADFKASJF\n");
	sleep(2);
	writeWatchDog();
	set_mp_status(IS_IND_STATUS);
	writeWatchDog();
	reach_video_detect_task_process(&gEnc2000);
	writeWatchDog();
	app_video_control_set(IS_IND_STATUS);
	writeWatchDog();
	app_init_no_signal_res();
	writeWatchDog();
	//	Device_adv7442Control(&adv7442, DEVICE_CMD_START, NULL, NULL);
	//	Device_adv7441Control(&adv7441, DEVICE_CMD_START, NULL, NULL);

	if(get_mp_status() == IS_MP_STATUS) {
		PRINTF("-------MP --SET I FREAM---\n");
		app_video_request_iframe(CHANNEL_INPUT_MP);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_MP_LOW);
	} else if(get_mp_status() == IS_IND_STATUS) {
		PRINTF("-------ind --SET I FREAM---\n");
		app_video_request_iframe(CHANNEL_INPUT_1);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_1_LOW);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_2);
		writeWatchDog();
		app_video_request_iframe(CHANNEL_INPUT_2_LOW);
	}

	writeWatchDog();

	if(0 == program_begin) {
		g_osd_enable = TRUE;
		update_osd_change_mp();
	}

	writeWatchDog();
	program_begin = 0;

	PRINTF("\n\n........... separate begin done..........\n\n");
	return 0;
}

static int init_camera_ctrl(void)
{
#if 1
	printf("MODULE_DEBUG_ENABLE=%d \n", MODULE_DEBUG_ENABLE);

	printf("INIT camera ctrl New!\n");
	input_set_remote(0, PORT_COM0);
	CameraCtrlInit(PORT_COM0);

	if(ENC2000 == sys_get_product_type()) {
		input_set_remote(1, PORT_COM1);
		CameraCtrlInit(PORT_COM1);
	}

#endif
	return 0;
}

#if 1
int main()
{
	int mp_status = 0;
	int have_audio  =  1;
	printf("\n... git version is=%s ...\n", _VERSION);
	setpriority(PRIO_PROCESS, 0, -20);
	/*取消PIPE坏的信号*/
	Signal(SIGPIPE, SIG_IGN);
	initWatchDog();
	mid_task_init();
	trace_init();
	writeWatchDog();

	Device_Init(2);

	init_config_file();
	writeWatchDog();
	init_HV_table();

	read_CbCr_table();
	writeWatchDog();
	xmlInitParser();
	//	log_server_init();

	mid_timer_init();
	app_init_product_info();
	writeWatchDog();
	init_MP_info(&mp_status);

	app_init_network_info();
	//只调一次，不能在begin里调
	app_init_video_control();
	app_init_audio_control(mp_status);
	app_module_init();
	writeWatchDog();
	GPIOInit(&gEnc2000.gpiofd);

	if(1 == have_audio) {
		audio_dsp_system_init();
	}

	//enc16000_main_start();

	if(IS_MP_STATUS == mp_status) {
		mp_reach_begin();
	} else {
		reach_begin();
	}

	writeWatchDog();
	//must init fpga version
	app_init_version_info();

	if(1 == have_audio) {
		reach_audio_enc_process(&gEnc2000);
	}

	app_weblisten_init();
	StartEncoderMangerServer1();

	if(ENC2000 == sys_get_product_type()) {
		StartEncoderMangerServer2();
	}

#ifdef HAVE_IP_XML
	int ret = 0;
	pthread_t new_tcp_thread_id[MAX_SOURCE_NUM];
	int index1 = 0, index2 = 1;
	pthread_t st_report_id[MAX_SOURCE_NUM];

	ret = pthread_create(&new_tcp_thread_id[index1], NULL, open_new_tcp_task, (void *)&index1);

	if(ret < 0) {
		ERR_PRN("create DSP1TCPTask() failed\n");
		return 0;
	}

	ret = pthread_create(&st_report_id[index1], NULL, report_pthread_fxn, (void *)&index1);

	if(ret < 0) {
		ERR_PRN("create DSP1TCPTask() failed\n");
		return 0;
	}

	if(ENC2000 == sys_get_product_type()) {
		ret = pthread_create(&new_tcp_thread_id[index2], NULL, open_new_tcp_task, (void *)&index2);

		if(ret < 0) {
			ERR_PRN("create DSP2TCPTask() failed\n");
			return 0;
		}

		ret = pthread_create(&st_report_id[index2], NULL, report_pthread_fxn, (void *)&index2);

		if(ret < 0) {
			ERR_PRN("create DSP2TCPTask() failed\n");
			return 0;
		}
	}

#endif

	//	create_TextTime_info();

#if WRITE_YUV_TASK
	reach_writeyuv_process(&gEnc2000);
#endif
	init_camera_ctrl();

	g_osd_enable = TRUE;
	create_TextTime_info();
	init_osd_info();

	lcd_show();
	front_panel_led(&gEnc2000);
	//	listenSdkTask();	//需起为线程


	//init time adjust thread
	mid_time_adjust_init();

	reach_print_current_status(&gEnc2000, 5);

	xmlCleanupParser();
	return 0;
}
#else
int main(int argc, char **argv)
{
	setpriority(PRIO_PROCESS, 0, -20);
	/*取消PIPE坏的信号*/
	Signal(SIGPIPE, SIG_IGN);

	mid_task_init();
	trace_init();
	printf("haaaaaaa\n");
	GPIOInit(&gEnc2000.gpiofd);

	updatefpga(gEnc2000.gpiofd);
	PRINTF("END FPGA,begin start kernel update\n");
	updatekernel();

	while(1) {
		sleep(5);
		printf("---------hahah--------------");
	}

	return 0;
}
#endif

