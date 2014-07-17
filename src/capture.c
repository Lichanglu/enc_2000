#include <stdio.h>

#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>


#include "reach_enc2000.h"
#include "capture.h"
#include "common.h"
#include "params.h"
#include "select.h"

#include "osd.h"
#include "sd_demo_osd.h"
#include "middle_control.h"
#include "input_to_channel.h"
#include "app_signal.h"
#include "app_video_control.h"
#include "app_head.h"

typedef struct _rt_video_params_ {
	Uint32 channel;
	Uint32 width;
	Uint32 height;
} rt_video_params;


static rt_video_params g_video_contex[SIGNAL_INPUT_MAX];

//dvi or sdi
static int g_input1_capture_mode = INPUT_DVI;
static int g_input2_capture_mode = INPUT_DVI;

//1--有源，0--无源
static int g_input1_have_signal = 1;
static int g_input2_have_signal = 1;


int app_get_gpio_fd()
{
	return gEnc2000.gpiofd;
}

int get_capture_state(SIGNAL_INPUT input)
{
	int state = -1;

	switch(input) {
		case SIGNAL_INPUT_1:
			state = g_input1_have_signal;
			break;

		case SIGNAL_INPUT_2:
			state = g_input2_have_signal;
			break;

		default:
			state = -1;
			break;
	}

	return state;
}

extern int g_color_space[SIGNAL_INPUT_MAX];
static void init_input_type(void)
{
	//	int input = 0;
	int ret = 0;
	int color1 = 1, color2 = 1;
	//	int out = 0;
	ret = read_input_info(&g_input1_capture_mode, &g_input2_capture_mode, &color1, &color2);

	if(ret < 0) {
		g_input1_capture_mode = INPUT_DVI;
		g_input2_capture_mode = INPUT_DVI;
	}

	PRINTF("color1=%d,color2=%d\n", color1, color2);
	g_color_space[SIGNAL_INPUT_1] = color1;
	g_color_space[SIGNAL_INPUT_2] = color2;
}


//获取采集是dvi还是sdi
int capture_get_input_type(int input)
{
	if(input == SIGNAL_INPUT_1) {
		return g_input1_capture_mode;
	} else if(input == SIGNAL_INPUT_2) {
		return g_input2_capture_mode;
	} else {
		ERR_PRN("the %d input is invaild\n", input);
		return -1;
	}
}
//设置dvi /sdi状态
int capture_set_input_type(int input, int mode)
{
	if(mode != INPUT_DVI && mode != INPUT_SDI) {
		ERR_PRN("the %d mode is invaild\n", mode);
		return -1;
	}

	if(input == SIGNAL_INPUT_1) {
		if(mode != g_input1_capture_mode) {
			PRINTF("i will change input from %d to %d\n", g_input1_capture_mode, mode);
			g_input1_capture_mode = mode;

			if(mode == INPUT_DVI) {	// DVI
				SelectAD7441(gEnc2000.gpiofd);
			} else {
				SelectGS2971(gEnc2000.gpiofd);
			}
		} else {
			PRINTF("the input = %d is same\n", mode);
		}
	} else if(input == SIGNAL_INPUT_2) {
		if(mode != g_input2_capture_mode) {
			PRINTF("i will change input from %d to %d\n", g_input2_capture_mode, mode);
			g_input2_capture_mode = mode;

			if(mode == INPUT_DVI) {	// DVI
				SelectAD7442(gEnc2000.gpiofd);
			} else {
				SelectGS2972(gEnc2000.gpiofd);
			}
		} else {
			PRINTF("the input = %d is same\n", mode);
		}
	} else {
		ERR_PRN("the %d input is invaild\n", input);
		return -1;
	}

	PRINTF("g_input1_capture_mode=%d -g_input2_capture_mode=%d\n", g_input1_capture_mode, g_input2_capture_mode);

	return 0;
}


//设置采集的图像的大小
int capture_set_input_hw(Uint32 input, Uint32 width, Uint32 height)
{
	if(input >= SIGNAL_INPUT_MAX) {
		fprintf(stderr, "set_video_hw error, channel = %d\n", input);
		return 0;
	}

	if(width > 1920 || height > 1200) {
		WARN_PRN("width =%d,height=%d\n", width, height);
	}

	// 互斥
	g_video_contex[input].channel = input;
	g_video_contex[input].width = width;
	g_video_contex[input].height = height;

	return 0;
}

int capture_get_input_hw(int input, int *width, int *height)
{
	if(input >= SIGNAL_INPUT_MAX) {
		fprintf(stderr, "get_video_hw error, channel = %d\n", input);
		return 0;
	}

	if(width == NULL || height == NULL) {
		fprintf(stderr, "get_video_hw error, *width = %p, *height = %p\n",
		        width, height);
	}

	// 互斥
	*width = 	g_video_contex[input].width;
	*height = 	g_video_contex[input].height;

	return 0;
}

Int32 reach_vcapture_process(ENC2000_LINK_STRUCT *pstruct)
{
	CaptureLink_CreateParams        *pCapturePrm;

	CaptureLink_VipInstParams		*pCaptureInstPrm;
	CaptureLink_OutParams			*pCaptureOutPrm;
	NullSrcLink_CreateParams		*pNullSrcPrm;
	DeiLink_CreateParams			*pdeiPrm0;
	DeiLink_CreateParams			*pdeiPrm1;
	MergeLink_CreateParams			*pMergePrm0;
	MergeLink_CreateParams			*pMergePrm1;
	SelectLink_CreateParams			*pSelectPrm0;
	SelectLink_CreateParams			*pSelectPrm1;

	IpcFramesInLinkRTOS_CreateParams  *pIpcFramesInDspPrm;
	AlgLink_CreateParams                         *pOsdDspAlgPrm;
	//解决sdi拨插死机
#if 1
	UInt32 standard[SIGNAL_INPUT_MAX];

	if(INPUT_SDI == capture_get_input_type(SIGNAL_INPUT_1)) {
		standard[SIGNAL_INPUT_1]  = SYSTEM_STD_1080P_60;
	} else {
		standard[SIGNAL_INPUT_1]  = SYSTEM_STD_1920x1200_60;
	}

	if(INPUT_SDI == capture_get_input_type(SIGNAL_INPUT_2)) {
		standard[SIGNAL_INPUT_2]  = SYSTEM_STD_1080P_60;
	} else {
		standard[SIGNAL_INPUT_2]  = SYSTEM_STD_1920x1200_60;
	}

#endif

	//two que and two channel (input 1 and input 2)
	pCapturePrm = &(pstruct->capLink.create_params);
	pCapturePrm->numVipInst               = 2;
	pCapturePrm->tilerEnable              = FALSE;
	pCapturePrm->fakeHdMode               = TRUE;
	pCapturePrm->enableSdCrop             = FALSE;
	pCapturePrm->doCropInCapture          = FALSE;

	//注意和后面板的dvi in 的对应关系
	//adv7442 == dvi input 1
	//adv7441 == dvi input 2
	//dvi input 1
	pCaptureInstPrm                     = &(pCapturePrm->vipInst[0]);
	pCaptureInstPrm->vipInstId          = SYSTEM_CAPTURE_INST_VIP0_PORTA;
	//    pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_ADV7441_DRV;
	pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
	pCaptureInstPrm->standard           = standard[SIGNAL_INPUT_1];// SYSTEM_STD_1920x1200_60;//SYSTEM_STD_1080P_60;//SYSTEM_STD_1920x1200_60;
	pCaptureInstPrm->numOutput          = 1;

	pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
	pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV420SP_UV;
	pCaptureOutPrm->scEnable            = FALSE;
	pCaptureOutPrm->scOutWidth          = 1920;
	pCaptureOutPrm->scOutHeight         = 1200;
	pCaptureOutPrm->outQueId            = 0; // ad7441
	pCapturePrm->outQueParams[0].nextLink = pstruct->mergeLink[0].link_id;


	pCaptureInstPrm                     = &(pCapturePrm->vipInst[1]);
	pCaptureInstPrm->vipInstId          = SYSTEM_CAPTURE_INST_VIP1_PORTA;
	//    pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_ADV7441_DRV;
	pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
	pCaptureInstPrm->standard           = standard[SIGNAL_INPUT_2] ;//SYSTEM_STD_1920x1200_60;
	pCaptureInstPrm->numOutput          = 1;

	pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
	pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV420SP_UV;
	pCaptureOutPrm->scEnable            = FALSE;
	pCaptureOutPrm->scOutWidth          = 1920;
	pCaptureOutPrm->scOutHeight         = 1200;
	pCaptureOutPrm->outQueId            = 1; // ad7442
	pCapturePrm->outQueParams[1].nextLink = pstruct->mergeLink[0].link_id;


	pCapturePrm->maxBlindAreasPerCh = 2;

	//	PRINTF("standard =%d\n", SYSTEM_STD_1920x1200_60);
	/* 纯色源 */
	//one que and two ch (null 0 and  null 1)
	pNullSrcPrm = &(pstruct->nullSrcLink.create_params);
	pNullSrcPrm->outQueParams.nextLink = pstruct->mergeLink[0].link_id;
	pNullSrcPrm->tilerEnable = FALSE;
	pNullSrcPrm->timerPeriod = 33;		/* 10 fps */
	pNullSrcPrm->inputInfo.numCh = 2;

	pNullSrcPrm->inputInfo.chInfo[0].bufType = SYSTEM_BUF_TYPE_VIDFRAME;
	pNullSrcPrm->inputInfo.chInfo[0].memType = SYSTEM_MT_NONTILEDMEM;
	pNullSrcPrm->inputInfo.chInfo[0].codingformat = 0;
	pNullSrcPrm->inputInfo.chInfo[0].dataFormat = SYSTEM_DF_YUV420SP_UV;
	pNullSrcPrm->inputInfo.chInfo[0].scanFormat = 1;
	pNullSrcPrm->inputInfo.chInfo[0].width = 1920;
	pNullSrcPrm->inputInfo.chInfo[0].height = 1200;
	pNullSrcPrm->inputInfo.chInfo[0].startX = 0;
	pNullSrcPrm->inputInfo.chInfo[0].startY = 0;
	pNullSrcPrm->inputInfo.chInfo[0].pitch[0] = 1920;
	pNullSrcPrm->inputInfo.chInfo[0].pitch[1] = 1920;   //???pitch only one ?? zm

	pNullSrcPrm->inputInfo.chInfo[1].bufType = SYSTEM_BUF_TYPE_VIDFRAME;
	pNullSrcPrm->inputInfo.chInfo[1].memType = SYSTEM_MT_NONTILEDMEM;
	pNullSrcPrm->inputInfo.chInfo[1].codingformat = 0;
	pNullSrcPrm->inputInfo.chInfo[1].dataFormat = SYSTEM_DF_YUV420SP_UV;
	pNullSrcPrm->inputInfo.chInfo[1].scanFormat = 1;
	pNullSrcPrm->inputInfo.chInfo[1].width = 1920;
	pNullSrcPrm->inputInfo.chInfo[1].height = 1200;
	pNullSrcPrm->inputInfo.chInfo[1].startX = 0;
	pNullSrcPrm->inputInfo.chInfo[1].startY = 0;
	pNullSrcPrm->inputInfo.chInfo[1].pitch[0] = 1920;
	pNullSrcPrm->inputInfo.chInfo[1].pitch[1] = 1920;


	/* MergeLink1合成高低码流 */  //max in que is 6
	// in:3 que in  and 4 ch in     .ch = {in1,in2,null1,null2}
	// out: 1 que out and 4 ch out  .ch = {in1,in2,null1,null2}
	pMergePrm0 = &(pstruct->mergeLink[0].create_params);
	pMergePrm0->numInQue						= 3;
	pMergePrm0->inQueParams[0].prevLinkId	= pstruct->capLink.link_id;
	pMergePrm0->inQueParams[0].prevLinkQueId	= 0;  //ad7441 ,ch0
	pMergePrm0->inQueParams[1].prevLinkId	= pstruct->capLink.link_id;
	pMergePrm0->inQueParams[1].prevLinkQueId	= 1;  //ad7442 ,ch1
	pMergePrm0->inQueParams[2].prevLinkId	= pstruct->nullSrcLink.link_id;
	pMergePrm0->inQueParams[2].prevLinkQueId	= 0;  // null one que two ch2,ch3
	pMergePrm0->outQueParams.nextLink		= pstruct->selectLink[0].link_id;
	pMergePrm0->notifyNextLink				= TRUE;

	/* SelectLink筛选流 */
	// in: 1 que in and 4 ch in   .ch = {in1,in2,null1,null2}
	// out: 4 que out and 4 ch out  .ch = {in1,in2,null1,null2}
	pSelectPrm0 = &(pstruct->selectLink[0].create_params);
	pSelectPrm0->inQueParams.prevLinkId	  = pstruct->mergeLink[0].link_id;
	pSelectPrm0->inQueParams.prevLinkQueId = 0;
	pSelectPrm0->numOutQue = 4;
	pSelectPrm0->outQueParams[0].nextLink = pstruct->deiLink[0].link_id;
	pSelectPrm0->outQueChInfo[0].numOutCh = 1;
	pSelectPrm0->outQueChInfo[0].inChNum[0] = 0;
	pSelectPrm0->outQueParams[1].nextLink = pstruct->deiLink[1].link_id;
	pSelectPrm0->outQueChInfo[1].numOutCh = 1;
	pSelectPrm0->outQueChInfo[1].inChNum[0] = 1;
	pSelectPrm0->outQueParams[2].nextLink = pstruct->mergeLink[1].link_id;
	pSelectPrm0->outQueChInfo[2].numOutCh = 1;
	pSelectPrm0->outQueChInfo[2].inChNum[0] = 2;
	pSelectPrm0->outQueParams[3].nextLink = pstruct->mergeLink[1].link_id;
	pSelectPrm0->outQueChInfo[3].numOutCh = 1;
	pSelectPrm0->outQueChInfo[3].inChNum[0] = 3;

	//only for input 1 dei
	//in: 1 que in and 1 ch in .  ch = {in1}
	//out: 1 que out and 1 ch out .ch= {in1}  //may be no input ,if the input1 is p siginal
	pdeiPrm0 = &(pstruct->deiLink[0].create_params);
	pdeiPrm0->inQueParams.prevLinkId						= pstruct->selectLink[0].link_id;
	pdeiPrm0->inQueParams.prevLinkQueId						= 0;
	pdeiPrm0->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = pstruct->mergeLink[1].link_id;
	pdeiPrm0->enableOut[DEI_LINK_OUT_QUE_DEI_SC]			= FALSE;
	pdeiPrm0->enableOut[DEI_LINK_OUT_QUE_VIP_SC]			= TRUE;
	pdeiPrm0->tilerEnable									= FALSE;
	pdeiPrm0->comprEnable									= FALSE;
	pdeiPrm0->setVipScYuv422Format							= FALSE;
	pdeiPrm0->enableDeiForceBypass							= FALSE;
	pdeiPrm0->enableLineSkipSc								= FALSE;

	pdeiPrm0->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]       = 60;
	pdeiPrm0->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]      = 30;  //???
	pdeiPrm0->inputDeiFrameRate = 60;
	pdeiPrm0->outputDeiFrameRate = 60;


	pdeiPrm0->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_ABSOLUTE;
	pdeiPrm0->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outWidth = 1920;
	pdeiPrm0->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outHeight = 1080;


	//only for input 1 dei
	//in: 1 que in and 1 ch in .  ch = {in1}
	//out: 1 que out and 1 ch out .ch= {in1}  //may be no input ,if the input1 is p siginal
	pdeiPrm1 = &(pstruct->deiLink[1].create_params);
	pdeiPrm1->inQueParams.prevLinkId						= pstruct->selectLink[0].link_id;
	pdeiPrm1->inQueParams.prevLinkQueId						= 1;
	pdeiPrm1->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink = pstruct->mergeLink[1].link_id;
	pdeiPrm1->enableOut[DEI_LINK_OUT_QUE_VIP_SC]			= TRUE;
	pdeiPrm1->enableOut[DEI_LINK_OUT_QUE_DEI_SC]			= FALSE;
	pdeiPrm1->tilerEnable									= FALSE;
	pdeiPrm1->comprEnable									= FALSE;
	pdeiPrm1->setVipScYuv422Format							= FALSE;
	pdeiPrm1->enableDeiForceBypass							= FALSE;
	pdeiPrm1->enableLineSkipSc								= FALSE;

	pdeiPrm1->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]       = 60;
	pdeiPrm1->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC]      = 30;
	pdeiPrm1->inputDeiFrameRate = 60;
	pdeiPrm1->outputDeiFrameRate = 60;

	pdeiPrm1->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode = DEI_SCALE_MODE_ABSOLUTE;
	pdeiPrm1->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outWidth = 1920;
	pdeiPrm1->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outHeight = 1080;

	/* MergeLink1合成高低码流 */
	/* merge1Link 接收去隔行后的图，或是蓝屏，或是直通过来的图 */
	// in: 4 que and 2 ch  .ch = {cin1/null1 ,in2 /null2}
	// out: 1 que and 2 ch  .ch = {cin1/null1 ,in2 /null2}
	pMergePrm1 = &(pstruct->mergeLink[1].create_params);
	pMergePrm1->numInQue						= 4;
	pMergePrm1->inQueParams[0].prevLinkId	= pstruct->deiLink[0].link_id;
	pMergePrm1->inQueParams[0].prevLinkQueId	= 1; //vp 0 ,正常应该是0，因为DEI输出在第2路que,所以是1
	pMergePrm1->inQueParams[1].prevLinkId	= pstruct->deiLink[1].link_id;
	pMergePrm1->inQueParams[1].prevLinkQueId	= 1; //vp 1
	pMergePrm1->inQueParams[2].prevLinkId	= pstruct->selectLink[0].link_id;
	pMergePrm1->inQueParams[2].prevLinkQueId	= 2; //null0
	pMergePrm1->inQueParams[3].prevLinkId	= pstruct->selectLink[0].link_id;
	pMergePrm1->inQueParams[3].prevLinkQueId	= 3; //null 1
	pMergePrm1->outQueParams.nextLink		= pstruct->selectLink[1].link_id;
	pMergePrm1->notifyNextLink				= TRUE;


	//set input 1
	input_set_mer1_chid(SIGNAL_INPUT_1, 0);

	//set input 2
	input_set_mer1_chid(SIGNAL_INPUT_2, 1);

	//add osd link
	if(pstruct->enableOsdAlgLink) {
		pMergePrm1->outQueParams.nextLink		=  pstruct->ipcFramesOutVpssId[0];

		//pNsfPrm->outQueParams[0].nextLink 	= pstruct->ipcFramesOutVpssId[0];
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.inQueParams.prevLinkId = pstruct->mergeLink[1].link_id;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.inQueParams.prevLinkQueId = 0;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.notifyPrevLink = TRUE;

		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.numOutQue = 1;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.outQueParams[0].nextLink = pstruct->selectLink[1].link_id;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.notifyNextLink = TRUE;

		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.processLink = pstruct->ipcFrames_indsp_link[0].link_id; ;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.notifyProcessLink = TRUE;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.noNotifyMode = FALSE;
	}

	if(pstruct->enableOsdAlgLink) {
		pIpcFramesInDspPrm = &(pstruct->ipcFrames_indsp_link[0].create_params);
		pIpcFramesInDspPrm->baseCreateParams.inQueParams.prevLinkId =  pstruct->ipcFramesOutVpssId[0]; //pstruct->ipcFramesOutVpssId[0];
		pIpcFramesInDspPrm->baseCreateParams.inQueParams.prevLinkQueId = 0;
		pIpcFramesInDspPrm->baseCreateParams.numOutQue   = 1;
		pIpcFramesInDspPrm->baseCreateParams.outQueParams[0].nextLink = pstruct->osd_dspAlg_Link[0].link_id;
		pIpcFramesInDspPrm->baseCreateParams.notifyPrevLink = TRUE;
		pIpcFramesInDspPrm->baseCreateParams.notifyNextLink = TRUE;
		pIpcFramesInDspPrm->baseCreateParams.noNotifyMode   = FALSE;


		pOsdDspAlgPrm = &(pstruct->osd_dspAlg_Link[0].create_params);
		pOsdDspAlgPrm->inQueParams.prevLinkId = pstruct->ipcFrames_indsp_link[0].link_id;
		pOsdDspAlgPrm->inQueParams.prevLinkQueId = 0;

	}

	/* OSD initialization of OSD param. Detailed configiration is done in SD_Demo_osdInit() Call */
	if(pstruct->enableOsdAlgLink) {
		int chId;
		(pstruct->osd_dspAlg_Link[0].create_params).enableOSDAlg    = TRUE;
		(pstruct->osd_dspAlg_Link[0].create_params).enableSCDAlg    = FALSE;
		(pstruct->osd_dspAlg_Link[0].create_params).outQueParams[ALG_LINK_SCD_OUT_QUE].nextLink = SYSTEM_LINK_ID_INVALID;

		for(chId = 0; chId < 12; chId++) {
			AlgLink_OsdChWinParams *chWinPrm = &(pstruct->osd_dspAlg_Link[0].create_params).osdChCreateParams[chId].chDefaultParams;

			/* set osd window max width and height */
			//会影响长度
			//in sw_osd.h
			(pstruct->osd_dspAlg_Link[0].create_params).osdChCreateParams[chId].maxWidth  = OSD_MAX_WIDTH;
			(pstruct->osd_dspAlg_Link[0].create_params).osdChCreateParams[chId].maxHeight = OSD_MAX_HEIGHT;

			chWinPrm->chId = chId;
			chWinPrm->numWindows = 0; //0
		}
	}




	/* SelectLink筛选流 */
	pSelectPrm1 = &(pstruct->selectLink[1].create_params);
	pSelectPrm1->inQueParams.prevLinkId	  = pstruct->mergeLink[1].link_id;
	pSelectPrm1->inQueParams.prevLinkQueId = 0;
	pSelectPrm1->numOutQue = 2;
	pSelectPrm1->outQueParams[0].nextLink = pstruct->dupLink[0].link_id;
	pSelectPrm1->outQueChInfo[0].numOutCh   = 1;
	pSelectPrm1->outQueChInfo[0].inChNum[0] = 2;		/* 先给蓝屏 */ //null 1
	pSelectPrm1->outQueParams[1].nextLink = pstruct->dupLink[1].link_id;
	pSelectPrm1->outQueChInfo[1].numOutCh   = 1;
	pSelectPrm1->outQueChInfo[1].inChNum[0] = 3;		/* 先给蓝屏 */  //null 2


	if(pstruct->enableOsdAlgLink) {
		pSelectPrm1->inQueParams.prevLinkId		= pstruct->ipcFramesOutVpssId[0];
		pSelectPrm1->inQueParams.prevLinkQueId	= 0;
	}


	return 0;
}

#if HAVE_MP_MODULE
#if 1
Int32 reach_MP_vcapture_process(ENC2000_LINK_STRUCT *pstruct)
{
	CaptureLink_CreateParams        *pCapturePrm;

	CaptureLink_VipInstParams		*pCaptureInstPrm;
	CaptureLink_OutParams			*pCaptureOutPrm;
	NullSrcLink_CreateParams		*pNullSrcPrm;
	DeiLink_CreateParams			*pdeiPrm0;
	DeiLink_CreateParams			*pdeiPrm1;
	MergeLink_CreateParams			*pMergePrm0;
	MergeLink_CreateParams			*pMergePrm1;
	SelectLink_CreateParams			*pSelectPrm0;
	SelectLink_CreateParams			*pSelectPrm1;


	//	IpcFramesInLinkRTOS_CreateParams *pIpcFramesInDspPrm;
	//	AlgLink_CreateParams             *pOsdDspAlgPrm;
	//解决sdi拨插死机
#if 1
	UInt32 standard[SIGNAL_INPUT_MAX];

	if(INPUT_SDI == capture_get_input_type(SIGNAL_INPUT_1)) {
		standard[SIGNAL_INPUT_1]  = SYSTEM_STD_1080P_60;
	} else {
		standard[SIGNAL_INPUT_1]  = SYSTEM_STD_1920x1200_60;
	}

	if(INPUT_SDI == capture_get_input_type(SIGNAL_INPUT_2)) {
		standard[SIGNAL_INPUT_2]  = SYSTEM_STD_1080P_60;
	} else {
		standard[SIGNAL_INPUT_2]  = SYSTEM_STD_1920x1200_60;
	}

#endif


	pCapturePrm = &(pstruct->capLink.create_params);
	pCapturePrm->numVipInst               = 2;
	pCapturePrm->tilerEnable              = FALSE;
	pCapturePrm->fakeHdMode               = TRUE;
	pCapturePrm->enableSdCrop             = FALSE;
	pCapturePrm->doCropInCapture          = FALSE;

	//注意和后面板的dvi in 的对应关系
	//adv7441 == dvi input 1
	//adv7442 == dvi input 2
	//dvi input 1
	pCaptureInstPrm                     = &(pCapturePrm->vipInst[0]);
	pCaptureInstPrm->vipInstId          = SYSTEM_CAPTURE_INST_VIP0_PORTA;
	//    pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_ADV7441_DRV;
	pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;//SYSTEM_STD_1920x1200_60;
	pCaptureInstPrm->standard           = standard[SIGNAL_INPUT_1];
	pCaptureInstPrm->numOutput          = 1;

	pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
	pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV420SP_UV;
	pCaptureOutPrm->scEnable            = FALSE;
	pCaptureOutPrm->scOutWidth          = 1920;
	pCaptureOutPrm->scOutHeight         = 1200;
	pCaptureOutPrm->outQueId            = 0;
	pCapturePrm->outQueParams[0].nextLink = pstruct->mergeLink[0].link_id;


	//dvi input 2
	pCaptureInstPrm                     = &(pCapturePrm->vipInst[1]);
	pCaptureInstPrm->vipInstId          = SYSTEM_CAPTURE_INST_VIP1_PORTA;
	//    pCaptureInstPrm->videoDecoderId     = SYSTEM_DEVICE_VID_DEC_ADV7441_DRV;
	pCaptureInstPrm->inDataFormat       = SYSTEM_DF_YUV422P;
	pCaptureInstPrm->standard           = standard[SIGNAL_INPUT_2];//SYSTEM_STD_1920x1200_60;
	pCaptureInstPrm->numOutput          = 1;

	pCaptureOutPrm                      = &pCaptureInstPrm->outParams[0];
	pCaptureOutPrm->dataFormat          = SYSTEM_DF_YUV420SP_UV;
	pCaptureOutPrm->scEnable            = FALSE;
	pCaptureOutPrm->scOutWidth          = 1920;
	pCaptureOutPrm->scOutHeight         = 1200;
	pCaptureOutPrm->outQueId            = 1;
	pCapturePrm->outQueParams[1].nextLink = pstruct->mergeLink[0].link_id;


	pCapturePrm->maxBlindAreasPerCh = 2;

	PRINTF("standard =%d\n", SYSTEM_STD_1920x1200_60);


	/* 纯色源 */
	//one que and two ch (null 0 and  null 1)
	pNullSrcPrm = &(pstruct->nullSrcLink.create_params);
	pNullSrcPrm->outQueParams.nextLink = pstruct->mergeLink[0].link_id;
	pNullSrcPrm->tilerEnable = FALSE;
	pNullSrcPrm->timerPeriod = 100; 	/* 10 fps */
	pNullSrcPrm->inputInfo.numCh = 2;

	pNullSrcPrm->inputInfo.chInfo[0].bufType = SYSTEM_BUF_TYPE_VIDFRAME;
	pNullSrcPrm->inputInfo.chInfo[0].memType = SYSTEM_MT_NONTILEDMEM;
	pNullSrcPrm->inputInfo.chInfo[0].codingformat = 0;
	pNullSrcPrm->inputInfo.chInfo[0].dataFormat = SYSTEM_DF_YUV420SP_UV;
	pNullSrcPrm->inputInfo.chInfo[0].scanFormat = 1;
	pNullSrcPrm->inputInfo.chInfo[0].width = 1920;
	pNullSrcPrm->inputInfo.chInfo[0].height = 1200;
	pNullSrcPrm->inputInfo.chInfo[0].startX = 0;
	pNullSrcPrm->inputInfo.chInfo[0].startY = 0;
	pNullSrcPrm->inputInfo.chInfo[0].pitch[0] = 1920;
	pNullSrcPrm->inputInfo.chInfo[0].pitch[1] = 1920;

	pNullSrcPrm->inputInfo.chInfo[1].bufType = SYSTEM_BUF_TYPE_VIDFRAME;
	pNullSrcPrm->inputInfo.chInfo[1].memType = SYSTEM_MT_NONTILEDMEM;
	pNullSrcPrm->inputInfo.chInfo[1].codingformat = 0;
	pNullSrcPrm->inputInfo.chInfo[1].dataFormat = SYSTEM_DF_YUV420SP_UV;
	pNullSrcPrm->inputInfo.chInfo[1].scanFormat = 1;
	pNullSrcPrm->inputInfo.chInfo[1].width = 1920;
	pNullSrcPrm->inputInfo.chInfo[1].height = 1200;
	pNullSrcPrm->inputInfo.chInfo[1].startX = 0;
	pNullSrcPrm->inputInfo.chInfo[1].startY = 0;
	pNullSrcPrm->inputInfo.chInfo[1].pitch[0] = 1920;
	pNullSrcPrm->inputInfo.chInfo[1].pitch[1] = 1920;


	/* mergeLink0 两路视频及两路蓝屏放于同一队列 */
	// in:3 que in  and 4 ch in     .ch = {in1,in2,null1,null2}
	// out: 1 que out and 4 ch out  .ch = {in1,in2,null1,null2}
	pMergePrm0 = &(pstruct->mergeLink[0].create_params);
	pMergePrm0->numInQue						= 3;
	pMergePrm0->inQueParams[0].prevLinkId	= pstruct->capLink.link_id;
	pMergePrm0->inQueParams[0].prevLinkQueId	= 0;
	pMergePrm0->inQueParams[1].prevLinkId	= pstruct->capLink.link_id;
	pMergePrm0->inQueParams[1].prevLinkQueId	= 1;
	pMergePrm0->inQueParams[2].prevLinkId	= pstruct->nullSrcLink.link_id;
	pMergePrm0->inQueParams[2].prevLinkQueId	= 0;
	pMergePrm0->outQueParams.nextLink		= pstruct->selectLink[0].link_id;
	pMergePrm0->notifyNextLink				= TRUE;



	/* selectLink 选择是进行去隔行，或是直通，或是出蓝屏 */
	// in: 1 que in and 4 ch in   .ch = {in1,in2,null1,null2}
	// out: 4 que out and 4 ch out  .ch = {in1,in2,null1,null2}
	pSelectPrm0 = &(pstruct->selectLink[0].create_params);
	pSelectPrm0->inQueParams.prevLinkId	  = pstruct->mergeLink[0].link_id;
	pSelectPrm0->inQueParams.prevLinkQueId = 0;
	pSelectPrm0->numOutQue = 4;
	pSelectPrm0->outQueParams[0].nextLink = pstruct->deiLink[0].link_id;
	pSelectPrm0->outQueChInfo[0].numOutCh = 1;
	pSelectPrm0->outQueChInfo[0].inChNum[0] = 0;
	pSelectPrm0->outQueParams[1].nextLink = pstruct->deiLink[1].link_id;
	pSelectPrm0->outQueChInfo[1].numOutCh = 1;
	pSelectPrm0->outQueChInfo[1].inChNum[0] = 1;
	pSelectPrm0->outQueParams[2].nextLink = pstruct->mergeLink[1].link_id;
	pSelectPrm0->outQueChInfo[2].numOutCh = 1;
	pSelectPrm0->outQueChInfo[2].inChNum[0] = 2;
	pSelectPrm0->outQueParams[3].nextLink = pstruct->mergeLink[1].link_id;
	pSelectPrm0->outQueChInfo[3].numOutCh = 1;
	pSelectPrm0->outQueChInfo[3].inChNum[0] = 3;//  2 ?

	//only for input 1 dei
	//in: 1 que in and 1 ch in .  ch = {in1}
	//out: 1 que out and 1 ch out .ch= {in1}  //may be no input ,if the input1 is p siginal
	pdeiPrm0 = &(pstruct->deiLink[0].create_params);
	pdeiPrm0->inQueParams.prevLinkId				= pstruct->selectLink[0].link_id;
	pdeiPrm0->inQueParams.prevLinkQueId				= 0;
	pdeiPrm0->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink
	    = pstruct->mergeLink[1].link_id;
	pdeiPrm0->enableOut[DEI_LINK_OUT_QUE_DEI_SC]	= FALSE;
	pdeiPrm0->enableOut[DEI_LINK_OUT_QUE_VIP_SC]	= TRUE;
	pdeiPrm0->tilerEnable							= FALSE;
	pdeiPrm0->comprEnable							= FALSE;
	pdeiPrm0->setVipScYuv422Format					= FALSE;
	pdeiPrm0->enableDeiForceBypass					= FALSE;
	pdeiPrm0->enableLineSkipSc						= FALSE;

	pdeiPrm0->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 60;
	pdeiPrm0->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 60;
	pdeiPrm0->inputDeiFrameRate						= 60;
	pdeiPrm0->outputDeiFrameRate					= 30;


	pdeiPrm0->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode
	    = DEI_SCALE_MODE_ABSOLUTE;
	pdeiPrm0->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outWidth
	    = 1920;
	pdeiPrm0->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outHeight
	    = 1080;

	//only for input 2 dei
	//in: 1 que in and 1 ch in .  ch = {in2}
	//out: 1 que out and 1 ch out .ch= {in2}  //may be no input ,if the input2 is p siginal
	pdeiPrm1 = &(pstruct->deiLink[1].create_params);
	pdeiPrm1->inQueParams.prevLinkId				= pstruct->selectLink[0].link_id;
	pdeiPrm1->inQueParams.prevLinkQueId				= 1;
	pdeiPrm1->outQueParams[DEI_LINK_OUT_QUE_VIP_SC].nextLink
	    = pstruct->mergeLink[1].link_id;
	pdeiPrm1->enableOut[DEI_LINK_OUT_QUE_VIP_SC]	= TRUE;
	pdeiPrm1->enableOut[DEI_LINK_OUT_QUE_DEI_SC]	= FALSE;
	pdeiPrm1->tilerEnable							= FALSE;
	pdeiPrm1->comprEnable							= FALSE;
	pdeiPrm1->setVipScYuv422Format					= FALSE;
	pdeiPrm1->enableDeiForceBypass					= FALSE;
	pdeiPrm1->enableLineSkipSc						= FALSE;

	pdeiPrm1->inputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 60;
	pdeiPrm1->outputFrameRate[DEI_LINK_OUT_QUE_VIP_SC] = 60;
	pdeiPrm1->inputDeiFrameRate						= 60;
	pdeiPrm1->outputDeiFrameRate					= 30;

	pdeiPrm1->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].scaleMode
	    = DEI_SCALE_MODE_ABSOLUTE;
	pdeiPrm1->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outWidth
	    = 1920;
	pdeiPrm1->outScaleFactor[DEI_LINK_OUT_QUE_VIP_SC][0].absoluteResolution.outHeight
	    =  540;	/* ???不明白 */

	/* merge1Link 接收去隔行后的图，或是蓝屏，或是直通过来的图 */
	// in: 4 que and 2 ch  .ch = {cin1/null1 ,in2 /null2}
	// out: 1 que and 2 ch  .ch = {cin1/null1 ,in2 /null2}
	pMergePrm1 = &(pstruct->mergeLink[1].create_params);
	pMergePrm1->numInQue						= 4;
	pMergePrm1->inQueParams[0].prevLinkId		= pstruct->deiLink[0].link_id;
	pMergePrm1->inQueParams[0].prevLinkQueId	= 1;
	pMergePrm1->inQueParams[1].prevLinkId		= pstruct->deiLink[1].link_id;
	pMergePrm1->inQueParams[1].prevLinkQueId	= 1;
	pMergePrm1->inQueParams[2].prevLinkId		= pstruct->selectLink[0].link_id;
	pMergePrm1->inQueParams[2].prevLinkQueId	= 2;
	pMergePrm1->inQueParams[3].prevLinkId		= pstruct->selectLink[0].link_id;
	pMergePrm1->inQueParams[3].prevLinkQueId	= 3;
	pMergePrm1->outQueParams.nextLink			= pstruct->selectLink[1].link_id;
	pMergePrm1->notifyNextLink					= TRUE;


	//set input 2
	input_set_mer1_chid(SIGNAL_INPUT_MP, 0);


	/* selectLink 有选择性地给swmsLink进行合成 */
	pSelectPrm1 = &(pstruct->selectLink[1].create_params);
	pSelectPrm1->inQueParams.prevLinkId		= pstruct->mergeLink[1].link_id;
	pSelectPrm1->inQueParams.prevLinkQueId	= 0;
	pSelectPrm1->numOutQue					= 1;
#if WRITE_YUV_TASK
	pSelectPrm1->outQueParams[0].nextLink	= pstruct->dupLink[1].link_id;
#else
	pSelectPrm1->outQueParams[0].nextLink	= pstruct->swmsLink[0].link_id;
#endif
	pSelectPrm1->outQueChInfo[0].numOutCh   = 2;
	pSelectPrm1->outQueChInfo[0].inChNum[0] = 2;		/* 先给蓝屏 */
	pSelectPrm1->outQueChInfo[0].inChNum[1] = 3;		/* 先给蓝屏 */

	return 0;
}
#endif
#endif

int set_control_dei(int vport, int flag, int mp_flag)
{
	SelectLink_OutQueChInfo select_prm;
	SelectLink_OutQueChInfo old_select_prm;

	PRINTF("input = %d,flag=%d,mp_flag=%d\n", vport, flag, mp_flag);

	if(mp_flag == IS_MP_STATUS) {
		if(flag == 1) {	// 使用DEI
			if(vport == SIGNAL_INPUT_1) { //INPUT 1
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 0;

				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);


				old_select_prm.outQueId = 0;
				select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,  &old_select_prm);
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 2;
				select_prm.inChNum[0] = 0;
				select_prm.inChNum[1] = old_select_prm.inChNum[1];
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				PRINTF("inChNum[0] =%d,inChNum[1]=%d\n ", select_prm.inChNum[0], select_prm.inChNum[1]);

				//		input_set_mer1_chid(SIGNAL_INPUT_MP, 0);

			} else if(vport == SIGNAL_INPUT_2) {
				select_prm.outQueId   = 1;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 1;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				old_select_prm.outQueId = 0;
				select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,  &old_select_prm);
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 2;
				select_prm.inChNum[0] = old_select_prm.inChNum[0];
				select_prm.inChNum[1] = 1;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);

				PRINTF("inChNum[0] =%d,inChNum[1]=%d\n ", select_prm.inChNum[0], select_prm.inChNum[1]);

				PRINTF("set_dei:select_prm-inChNum[0]=%d-select_prm.inChNum[1]=%d-"
				       "select_prm.numOutCh=%d,%d\n", select_prm.inChNum[0], select_prm.inChNum[1],
				       select_prm.numOutCh, select_prm.outQueId);
				//		input_set_mer1_chid(SIGNAL_INPUT_MP, 0);
			}
		} else if(flag == 0) {	// 关闭DEI
			PRINTF("no dei!\n");

			if(vport == SIGNAL_INPUT_1) {
				select_prm.outQueId   = 2;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 0;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				old_select_prm.outQueId = 0;
				select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,  &old_select_prm);
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 2;
				select_prm.inChNum[0] = 2;
				select_prm.inChNum[1] = old_select_prm.inChNum[1];
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				PRINTF("inChNum[0] =%d,inChNum[1]=%d\n ", select_prm.inChNum[0], select_prm.inChNum[1]);

				//		input_set_mer1_chid(SIGNAL_INPUT_MP, 0);

			} else if(vport == SIGNAL_INPUT_2) {
				select_prm.outQueId   = 3;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 1;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				old_select_prm.outQueId = 0;
				select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,  &old_select_prm);
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 2;
				select_prm.inChNum[0] = old_select_prm.inChNum[0];
				select_prm.inChNum[1] = 3;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				PRINTF("inChNum[0] =%d,inChNum[1]=%d\n ", select_prm.inChNum[0], select_prm.inChNum[1]);

				//		input_set_mer1_chid(SIGNAL_INPUT_MP, 0);
			}
		} else if(flag == 2) {	// 蓝屏
			if(vport == SIGNAL_INPUT_1) {
				select_prm.outQueId   = 2;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 2;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				old_select_prm.outQueId = 0;
				select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,  &old_select_prm);
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 2;
				select_prm.inChNum[0] = 2;
				select_prm.inChNum[1] = old_select_prm.inChNum[1];
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				PRINTF("inChNum[0] =%d,inChNum[1]=%d\n ", select_prm.inChNum[0], select_prm.inChNum[1]);

				//	input_set_mer1_chid(SIGNAL_INPUT_1, -1);
			} else if(vport == SIGNAL_INPUT_2) {
				select_prm.outQueId   = 3;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 3;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				old_select_prm.outQueId = 0;
				select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_1,  &old_select_prm);
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 2;
				select_prm.inChNum[0] = old_select_prm.inChNum[0];
				select_prm.inChNum[1] = 3;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				PRINTF("inChNum[0] =%d,inChNum[1]=%d\n ", select_prm.inChNum[0], select_prm.inChNum[1]);

				//	input_set_mer1_chid(SIGNAL_INPUT_2, -1);
			}
		}
	}


	else {
		if(flag == 1) {	// 使用DEI
			if(vport == SIGNAL_INPUT_1) {
				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 0;//input 1
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 0;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				//set input 1
				input_set_mer1_chid(SIGNAL_INPUT_1, 0);
			} else if(vport == SIGNAL_INPUT_2) {
				select_prm.outQueId   = 1;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 1;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				select_prm.outQueId   = 1;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 1;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);

				//set input 2
				input_set_mer1_chid(SIGNAL_INPUT_2, 1);
			}
		} else if(flag == 0) {	// 关闭DEI
			if(vport == SIGNAL_INPUT_1) {
				select_prm.outQueId   = 2;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 0;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 2;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				//set input 1
				input_set_mer1_chid(SIGNAL_INPUT_1, 2);
			} else if(vport == SIGNAL_INPUT_2) {
				select_prm.outQueId   = 3;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 1;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				select_prm.outQueId   = 1;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 3;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				//set input 1
				input_set_mer1_chid(SIGNAL_INPUT_2, 3);

			}
		} else if(flag == 2) {	// 蓝屏
			if(vport == SIGNAL_INPUT_1) {
				select_prm.outQueId   = 2;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 2;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				select_prm.outQueId   = 0;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 2;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				//set input 1
				PRINTF("set g_input1_mer1_chid == -1\n");
				input_set_mer1_chid(SIGNAL_INPUT_1, -1);
			} else if(vport == SIGNAL_INPUT_2) {
				select_prm.outQueId   = 3;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 3;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_0,  &select_prm);

				select_prm.outQueId   = 1;
				select_prm.numOutCh   = 1;
				select_prm.inChNum[0] = 3;
				select_set_outque_chinfo2(SYSTEM_VPSS_LINK_ID_SELECT_1,  &select_prm);
				//set input 2
				PRINTF("set g_input2_mer1_chid == -1\n");
				input_set_mer1_chid(SIGNAL_INPUT_2, -1);
			}
		}
	}

	return 0;
}

static int no_signal_hold_res(int channel, int input, Uint32 *old_width,
                              Uint32 *old_height)
{
	int mp_status = get_mp_status();
	int width = 0, height = 0, scale = 0;
	int scalewidth = 0, scaleheight = 0;
	int outwidth = 0, outheight = 0;
	//PRINTF("no_signal_hold_res oldwidth =%d,old_heig=%d\n",*old_width,*old_height);
	//	set_control_dei(input, 2, mp_status);	/* 蓝屏 */

	if(IS_MP_STATUS == mp_status) {
		return 0;
	}

	capture_get_input_hw(input, &width, &height);
	capture_set_input_hw(input, 1920, 1080);

	scale = scalewidth = scaleheight = 0;
	app_get_scale_status(channel, &scale, &scalewidth, &scaleheight);
	PRINTF("scale=%d:%dx%d\n", scale, scalewidth, scaleheight);

	if(height > 1080) {
		height = 1080;
	}

	if(scaleheight > 1080) {
		scaleheight = 1080;
	}

	if(scale != LOCK_SCALE) {
		outwidth = width;
		outheight = height;
	} else {
		outwidth = scalewidth;
		outheight = scaleheight;
	}

	set_resolution(input, HIGH_STREAM, outwidth, outheight, 1920, 1200);
	PRINTF("resolution hold !! %dx%d\n", outwidth, outheight);
	return 0;
}

#if 0
static Void *detect_video_tsk(Void *prm)
{
	VCAP_VIDEO_SOURCE_CH_STATUS_S videoStatus;
	VCAP_VIDEO_SOURCE_CH_STATUS_S historyStatus2 = {0};
	VCAP_VIDEO_SOURCE_CH_STATUS_S historyStatus1 = {0};
	cap_struct *pcaphand;
	//	ResizeParam tmp;
	pcaphand  = (cap_struct *)prm;
	int scale = UNLOCK_SCALE;
	int scalewidth = 0;
	int adv1_cnt = 0, adv2_cnt = 0;
	int scaleheight = 0;
	int mp_status = get_mp_status();;

	int capture_width = 0;
	int capture_height = 0;
	//capture_get_input_hw(0, &historyStatus1.frameWidth, &historyStatus1.frameHeight);
	//capture_get_input_hw(1, &historyStatus2.frameWidth, &historyStatus2.frameHeight);

	while(pcaphand->isDetect) {
		//input 2 ,adv 7441-> input1 adv7441 by lichl
		if(1) {
			memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));

			if(capture_get_input_type(SIGNAL_INPUT_1) == INPUT_DVI) {
				cap_get_7441_resolution(&(pcaphand->adv7441_phandle), &videoStatus);
			} else {
				cap_get_2971_resolution(&(pcaphand->gs2971_phandle), &videoStatus);
			}

			if(1 == videoStatus.isVideoDetect) {
				adv1_cnt++;

				if(adv1_cnt > 10) {
					adv1_cnt = 0;
					PRINTF("input1------videoStatus.isVideoDetect=%d,%dx%d\n", videoStatus.isVideoDetect,
					       videoStatus.frameWidth, videoStatus.frameHeight);
				}

				if(g_input1_have_signal == 0) {
					g_input1_have_signal = 1;

					//need dei
					//if(videoStatus.frameHeight == 540 || videoStatus.isInterlaced == 1) {
					if(videoStatus.frameHeight == 540) {
						set_control_dei(SIGNAL_INPUT_1, 1, mp_status);
					} else {
						set_control_dei(SIGNAL_INPUT_1, 0, mp_status);
					}
				}



				if(videoStatus.frameWidth != historyStatus1.frameWidth
				   || videoStatus.frameHeight != historyStatus1.frameHeight)

				{
					capture_width = videoStatus.frameWidth;
					capture_height =  videoStatus.frameHeight;

					if(capture_width == 1920 && capture_height == 540) {
						capture_width = 1920;
						capture_height = 1080;
					}

					PRINTF("change 1 ------%dx%d==%d\n", videoStatus.frameWidth, videoStatus.frameHeight, videoStatus.isInterlaced);

					if(IS_IND_STATUS == mp_status) {
						scale = scalewidth = scaleheight = 0;
						app_get_scale_status(CHANNEL_INPUT_1, &scale, &scalewidth, &scaleheight);

						if(scale == LOCK_SCALE) {
							set_resolution(SIGNAL_INPUT_1, HIGH_STREAM, scalewidth, scaleheight, videoStatus.frameWidth, videoStatus.frameHeight);
						} else {

							//规避不同源不同输入带来的分辨率不随源改变的问题
							//有待商榷
							set_resolution(SIGNAL_INPUT_1, HIGH_STREAM, capture_width, capture_height,
							               capture_width, capture_height);
						}

						scale = scalewidth = scaleheight = 0;
						app_get_scale_status(CHANNEL_INPUT_1_LOW, &scale, &scalewidth, &scaleheight);

						if(scale == LOCK_SCALE) {
							set_resolution(SIGNAL_INPUT_1, LOW_STREAM, scalewidth, scaleheight, videoStatus.frameWidth, videoStatus.frameHeight);
						}
					}


					historyStatus1.isVideoDetect = videoStatus.isVideoDetect;
					historyStatus1.frameWidth    = videoStatus.frameWidth;
					historyStatus1.frameHeight   = videoStatus.frameHeight;
					historyStatus1.frameInterval = videoStatus.frameInterval;
					historyStatus1.isInterlaced  = videoStatus.isInterlaced;
					historyStatus1.chId      = 0;
					historyStatus1.vipInstId = 0;
					PRINTF("DetectVideo_drvTsk: resolution change !! frameWidth[%d] frameHeight[%d]=historyStatus1.isInterlaced=%d\n",
					       videoStatus.frameWidth, videoStatus.frameHeight, historyStatus1.isInterlaced);
					//		System_linkControl(pcaphand->link_id, CAPTURE_LINK_CMD_FORCE_RESET,
					//							NULL, 0, TRUE);
					capture_set_input_hw(SIGNAL_INPUT_1, videoStatus.frameWidth, videoStatus.frameHeight);
					System_linkControl(pcaphand->link_id, CAPTURE_LINK_CMD_A8_DETECT_VIDEO,
					                   &historyStatus1, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S), TRUE);

#if 0
					//刷黑屏幕
					capture_fillbuff(historyStatus1.chId, historyStatus1.vipInstId, pcaphand->link_id, 1);
					usleep(200000);
					capture_fillbuff(historyStatus1.chId, historyStatus1.vipInstId, pcaphand->link_id, 0);
#endif

					//need dei
					//	if(videoStatus.frameHeight == 540) {
					//if(videoStatus.isInterlaced == 1 || videoStatus.frameHeight == 540) {
					if(videoStatus.frameHeight == 540) {
						set_control_dei(SIGNAL_INPUT_1, 1, mp_status);
					} else {
						set_control_dei(SIGNAL_INPUT_1, 0, mp_status);
					}

					adjust_analog_signal_hv(SIGNAL_INPUT_1);

					if(mp_status == IS_IND_STATUS) {
						update_osd_view(INPUT_1);
					}

				}

			} else
				// 无源
			{
				if(g_input1_have_signal == 1) {
					//					int input = SIGNAL_INPUT_2, high = HIGH_STREAM;
					usleep(150000);
					g_input1_have_signal = 0;
					// no input ,not need dei
					//	set_control_dei(SIGNAL_INPUT_1, 2, mp_status);	/* 蓝屏 */

					no_signal_hold_res(pcaphand->link_id, CHANNEL_INPUT_1, SIGNAL_INPUT_1,
					                   &historyStatus1.frameWidth, &historyStatus1.frameHeight);

				}
			}
		}

		//input 1 ,adv7442->lichl change 7442 input2
		if(1) {
			memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));

			if(capture_get_input_type(SIGNAL_INPUT_2) == INPUT_DVI) {
				cap_get_7442_resolution(&(pcaphand->adv7442_phandle), &videoStatus);
			} else {
				cap_get_2972_resolution(&(pcaphand->gs2972_phandle), &videoStatus);
			}

			if(videoStatus.isVideoDetect == 1) {
				adv2_cnt++;

				if(adv2_cnt > 10) {
					PRINTF("input2------videoStatus.isVideoDetect=%d,%dx%d\n", videoStatus.isVideoDetect,
					       videoStatus.frameWidth, videoStatus.frameHeight);
					adv2_cnt = 0;
				}

				if(g_input2_have_signal == 0) {
					g_input2_have_signal = 1;

					//adv7442 need dei
					//if(videoStatus.frameHeight == 540) {

					//	if(videoStatus.frameHeight == 540 || videoStatus.isInterlaced == 1) {
					if(videoStatus.frameHeight == 540) {
						set_control_dei(SIGNAL_INPUT_2, 1, mp_status);
					} else {
						set_control_dei(SIGNAL_INPUT_2, 0, mp_status);
					}
				}



				if(videoStatus.frameWidth != historyStatus2.frameWidth
				   || videoStatus.frameHeight != historyStatus2.frameHeight)

				{

					capture_width = videoStatus.frameWidth;
					capture_height =  videoStatus.frameHeight;

					if(capture_width == 1920 && capture_height == 540) {
						capture_width = 1920;
						capture_height = 1080;
					}

					PRINTF("change 2------%dx%d\n", videoStatus.frameWidth, videoStatus.frameHeight);

					if(IS_IND_STATUS == mp_status) {
						scale = scalewidth = scaleheight = 0;
						app_get_scale_status(CHANNEL_INPUT_2, &scale, &scalewidth, &scaleheight);

						if(scale == LOCK_SCALE) {
							set_resolution(SIGNAL_INPUT_2, HIGH_STREAM, scalewidth, scaleheight, videoStatus.frameWidth, videoStatus.frameHeight);
						} else {
							//规避不同源不同输入带来的分辨率不随源改变的问题
							//有待商榷
							set_resolution(SIGNAL_INPUT_2, HIGH_STREAM, capture_width, capture_height,
							               capture_width, capture_height);
						}

						scale = scalewidth = scaleheight = 0;
						app_get_scale_status(CHANNEL_INPUT_2_LOW, &scale, &scalewidth, &scaleheight);

						if(scale == LOCK_SCALE) {
							set_resolution(SIGNAL_INPUT_2, LOW_STREAM, scalewidth, scaleheight, videoStatus.frameWidth, videoStatus.frameHeight);
						}
					}


					historyStatus2.isVideoDetect = videoStatus.isVideoDetect;
					historyStatus2.frameWidth    = videoStatus.frameWidth;
					historyStatus2.frameHeight   = videoStatus.frameHeight;
					historyStatus2.frameInterval = videoStatus.frameInterval;
					historyStatus2.isInterlaced  = videoStatus.isInterlaced;
					historyStatus2.chId      = 0; // ch 0
					historyStatus2.vipInstId = 1;
					PRINTF("DetectVideo_drvTsk: resolution change !! frameWidth[%d] frameHeight[%d]\n",
					       videoStatus.frameWidth, videoStatus.frameHeight);
					//		System_linkControl(pcaphand->link_id, CAPTURE_LINK_CMD_FORCE_RESET,
					//							NULL,0 , TRUE);
					capture_set_input_hw(SIGNAL_INPUT_2, videoStatus.frameWidth, videoStatus.frameHeight);
					System_linkControl(pcaphand->link_id, CAPTURE_LINK_CMD_A8_DETECT_VIDEO,
					                   &historyStatus2, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S), TRUE);
#if 0
					capture_fillbuff(historyStatus2.chId, historyStatus2.vipInstId, pcaphand->link_id, 1);
					usleep(200000);
					capture_fillbuff(historyStatus2.chId, historyStatus2.vipInstId, pcaphand->link_id, 0);
#endif

					//if(videoStatus.frameHeight == 540) {
					//if(videoStatus.isInterlaced == 1 || videoStatus.frameHeight == 540) {
					if(videoStatus.frameHeight == 540) {
						set_control_dei(SIGNAL_INPUT_2, 1, mp_status);
					} else {
						set_control_dei(SIGNAL_INPUT_2, 0, mp_status);
					}

					adjust_analog_signal_hv(SIGNAL_INPUT_2);

					if(mp_status == IS_IND_STATUS) {
						update_osd_view(INPUT_2);
					}

				}
			} else {
				if(g_input2_have_signal == 1) {
					//					int input = CHANNEL_INPUT_1, high = HIGH_STREAM;
					usleep(150000);
					g_input2_have_signal = 0;
					//	set_control_dei(SIGNAL_INPUT_2, 2, mp_status);	/* 蓝屏 */

					no_signal_hold_res(pcaphand->link_id, CHANNEL_INPUT_2, SIGNAL_INPUT_2,
					                   &historyStatus2.frameWidth, &historyStatus2.frameHeight);
				}
			}
		}

		usleep(600000);
		//sleep(2);
	}

	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return NULL;
}
#else
static Void *detect_video_tsk(Void *prm)
{
	VCAP_VIDEO_SOURCE_CH_STATUS_S videoStatus;
	VCAP_VIDEO_SOURCE_CH_STATUS_S historyStatus[2];
	cap_struct *pcaphand;
	//	ResizeParam tmp;
	pcaphand  = (cap_struct *)prm;
	int scale = UNLOCK_SCALE;
	int scalewidth = 0;
	//	int adv1_cnt = 0, adv2_cnt = 0;
	int scaleheight = 0;
	int mp_status = get_mp_status();

	int capture_width = 0;
	int capture_height = 0;
	int high_channel = 0;
	int low_channel = 0;
	//capture_get_input_hw(0, &historyStatus1.frameWidth, &historyStatus1.frameHeight);
	//capture_get_input_hw(1, &historyStatus2.frameWidth, &historyStatus2.frameHeight);


	int input = 0;
	int osd_input = 0;
	int *input_have_signal = NULL;
	int i = 0;

	int oldmode[2] = { -1, -1};
	int current_mode = 0;

	while(pcaphand->isDetect) {

		for(i = 0; i < 2; i++) {
			if(pcaphand->isDetect != TRUE) {
				break;
			}

			current_mode = -1;

			if(i == 0) {
				input = SIGNAL_INPUT_1;
				osd_input = INPUT_1;
				high_channel = CHANNEL_INPUT_1;
				low_channel = CHANNEL_INPUT_1_LOW;
				input_have_signal = &g_input1_have_signal;
				memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));

				if(capture_get_input_type(input) == INPUT_DVI) {
					cap_get_7441_resolution(&(pcaphand->adv7441_phandle), &videoStatus);
				} else {
					cap_get_2971_resolution(&(pcaphand->gs2971_phandle), &videoStatus);
				}

				if(1 == videoStatus.isVideoDetect) {
					current_mode = detect_signal_info(input, videoStatus.isInterlaced);    // 获取SIGNAL info
				}


			} else if(i == 1) {
				input = SIGNAL_INPUT_2;
				osd_input = INPUT_2;
				high_channel = CHANNEL_INPUT_2;
				low_channel = CHANNEL_INPUT_2_LOW;
				input_have_signal = &g_input2_have_signal;
				memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));

				if(capture_get_input_type(input) == INPUT_DVI) {
					cap_get_7442_resolution(&(pcaphand->adv7442_phandle), &videoStatus);
				} else {
					cap_get_2972_resolution(&(pcaphand->gs2972_phandle), &videoStatus);
				}

				if(1 == videoStatus.isVideoDetect) {
					current_mode = detect_signal_info(input, videoStatus.isInterlaced);    // 获取SIGNAL info
				}

			}

			writeWatchDog();

			if(1) {
				if(1 == videoStatus.isVideoDetect) {
					if(oldmode[i] != current_mode) {
						PRINTF("mode is change ,old is %d,new is %d\n", oldmode[i], current_mode);
					}

					PRINTF("input =%d,videoStatus.isInterlaced =%d,w=%d,h=%d\n", input, videoStatus.isInterlaced, videoStatus.frameWidth, videoStatus.frameHeight);

					if((*input_have_signal == 0) ||
					   ((current_mode != -1) && (current_mode != oldmode[i]))
					   || videoStatus.frameWidth != historyStatus[i].frameWidth || videoStatus.frameHeight != historyStatus[i].frameHeight) {

						oldmode[i] = current_mode;

						if(videoStatus.frameHeight == 540) {
							capture_set_input_hw(input, videoStatus.frameWidth, videoStatus.frameHeight * 2);
						} else {
							capture_set_input_hw(input, videoStatus.frameWidth, videoStatus.frameHeight);
						}

						PRINTF("change %d -widht-----%dx%d=inter=%d\n", i, videoStatus.frameWidth, videoStatus.frameHeight, videoStatus.isInterlaced);





						historyStatus[i].isVideoDetect = videoStatus.isVideoDetect;
						historyStatus[i].frameWidth    = videoStatus.frameWidth;
						historyStatus[i].frameHeight   = videoStatus.frameHeight;
						historyStatus[i].frameInterval = videoStatus.frameInterval;
						historyStatus[i].isInterlaced  = videoStatus.isInterlaced;
						historyStatus[i].chId      = 0;
						historyStatus[i].vipInstId = i;
						PRINTF("DetectVideo_drvTsk:  %d resolution change !! frameWidth[%d] frameHeight[%d]=historyStatus1.isInterlaced=%d\n",
						       i, videoStatus.frameWidth, videoStatus.frameHeight, historyStatus[i].isInterlaced);
						System_linkControl(pcaphand->link_id, CAPTURE_LINK_CMD_A8_DETECT_VIDEO,
						                   &(historyStatus[i]), sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S), TRUE);

						//判断是视频信号
						if(1) {
							//adv7441 need dei
							if(videoStatus.frameHeight == 540) {
								Signal_Info signal_info;
								app_get_signal_info(input, &signal_info);

								//模拟不进栽边
								if(signal_info.digital  == 0) {
									PRINTF("signal_info.digital=%d\n", signal_info.digital);
									//capture_fill_dei_frame(historyStatus[i].chId, historyStatus[i].vipInstId, pcaphand->link_id, 1);
								}

								//DeiLink_chDynamicSetOutRes params = {0};
								//params.chId = 0;
								//params.queId = 1;
								//	params.width = 1920;
								//	params.height = 540;
								//	params.pitch[0] = 1920;
								//	params.pitch[1] = 1920;
								set_control_dei(input, 1, mp_status);
								PRINTF("i will use dei\n");

								//	dei_set_output_resolution(gEnc2000.deiLink[input].link_id,&params);
								//	PRINTF("dei[%d]_set_output_resolution [%d] [%d]\n", i, params.width, params.height);
							} else {
								//capture_fill_dei_frame(historyStatus[i].chId, historyStatus[i].vipInstId, pcaphand->link_id, 0);
								set_control_dei(input, 0, mp_status);
							}
						}

						if(IS_IND_STATUS == mp_status) {
							scale = scalewidth = scaleheight = 0;
							app_get_scale_status(high_channel, &scale, &scalewidth, &scaleheight);

							if(scale == LOCK_SCALE) {
								set_resolution(input, HIGH_STREAM, scalewidth, scaleheight, videoStatus.frameWidth, videoStatus.frameHeight);
							} else {
								//规避不同源不同输入带来的分辨率不随源改变的问题
								//有待商榷
								capture_get_input_hw(input, &capture_width, &capture_height);
								set_resolution(input, HIGH_STREAM, capture_width, capture_height, capture_width, capture_height);
							}

							scale = scalewidth = scaleheight = 0;
							app_get_scale_status(low_channel, &scale, &scalewidth, &scaleheight);

							if(scale == LOCK_SCALE) {
								set_resolution(input, LOW_STREAM, scalewidth, scaleheight, capture_width, capture_height);
							}

						}

						update_enc_preset_frame_qp(input);
						update_video_fps(input);
						adjust_analog_signal_hv(input);

						set_CSC_value(input, g_color_space[input]);

						//only dvi port need cbcr change
						if(capture_get_input_type(input) == INPUT_DVI) {
							adjust_signal_CbCr(input);
						}

						if(mp_status == IS_IND_STATUS) {
							update_osd_view(input, 1);
						}
					}

					else {
						if(videoStatus.frameHeight == 540) {
							PRINTF(" i will close fill dei frame\n");
							//capture_fill_dei_frame(historyStatus[i].chId, historyStatus[i].vipInstId, pcaphand->link_id, 0);
						}
					}

					*input_have_signal = 1;
				} else {
					if(*input_have_signal == 1) {
						usleep(150000);
						*input_have_signal = 0;
						PRINTF("input_have_signal = %d\n", *input_have_signal);

						update_osd_view(input, 0);


						no_signal_hold_res(high_channel, input,
						                   &historyStatus[i].frameWidth, &historyStatus[i].frameHeight);
						no_signal_clean_signal_info(input);
						set_control_dei(input, 2, mp_status);

						writeWatchDog();
					}
				}

			}

			usleep(200000);
		}

		usleep(300000);
		//sleep(2);
	}

	pthread_detach(pthread_self());
	pthread_exit(NULL);
	return NULL;
}
#endif

int printf_select_chaninfo(int linkId, int num)
{
	SelectLink_OutQueChInfo select_prm;
	int index = 0;
	int que = 0;

	PRINTF("======================================\n");

	for(que = 0; que < num; que++) {
		select_prm.outQueId = que;
		select_get_outque_chinfo(linkId, &select_prm);
		PRINTF("outQueId = %d\n", select_prm.outQueId);
		PRINTF("numOutCh = %d\n", select_prm.numOutCh);

		for(index = 0; index < select_prm.numOutCh; index++) {
			PRINTF("inChNum[%d] = %d\n", index, select_prm.inChNum[index]);
		}
	}

	PRINTF("======================================\n");

	return 0;
}

Int32 reach_video_detect_task_process(ENC2000_LINK_STRUCT *pstruct)
{
	Int32 status = 0;
	pstruct->capLink.isDetect = TRUE;
	//	printf("detect_video_tsk = %p\n",detect_video_tsk);
	status = OSA_thrCreate(&pstruct->capLink.taskHand,
	                       (OSA_ThrEntryFunc)detect_video_tsk,
	                       99,
	                       0,
	                       &pstruct->capLink);
	OSA_assert(status == 0);

#if 0
	OSA_ThrHndl   				taskHand;
	status = OSA_thrCreate(&taskHand,
	                       (OSA_ThrEntryFunc)test,
	                       99,
	                       0,
	                       NULL);
	OSA_assert(status == 0);
#endif
	return 0;
}

//init config input vp device
Int32 reach_config_cap_device(ENC2000_LINK_STRUCT *pstruct)
{
	Int32 ret = 0;
	VCAP_VIDEO_SOURCE_CH_STATUS_S	videoStatus;
	int input1_dvi = 0;
	int input1_sdi = 0;
	int input2_dvi = 0;
	int input2_sdi  = 0;

	//get input from input.dat
	init_input_type();

	//input 1 ,adv7442
	if(capture_get_input_type(SIGNAL_INPUT_1) == INPUT_DVI) {	// DVI
		SelectAD7441(gEnc2000.gpiofd);
		input1_dvi = 1;
		input1_sdi = 0;
	} else {
		SelectGS2971(gEnc2000.gpiofd);
		input1_dvi = 0;
		input1_sdi = 1;
	}


	//input 2,adv7442
	if(capture_get_input_type(SIGNAL_INPUT_2) == INPUT_DVI) {	// DVI
		SelectAD7442(gEnc2000.gpiofd);
		input2_dvi = 1;
		input2_sdi = 0;
	} else {
		SelectGS2972(gEnc2000.gpiofd);
		input2_dvi = 0;
		input2_sdi = 1;
	}


	memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));
	ret = cap_config_adv7441(&(pstruct->capLink.adv7441_phandle), 0, &videoStatus);

	//set input 1
	if(input1_dvi == 1) {
		if(ret == 0 && videoStatus.isVideoDetect) {
			fprintf(stderr, "input1:ad7441 video detect, width = %d, height = %d\n",
			        videoStatus.frameWidth, videoStatus.frameHeight);
			capture_set_input_hw(SIGNAL_INPUT_1, videoStatus.frameWidth, videoStatus.frameHeight);
		} else {
			capture_set_input_hw(SIGNAL_INPUT_1, 1920, 1080);
		}

		//设置成高清模式
		Cap_SetSource7441Chan(&(pstruct->capLink.adv7441_phandle), 1);
	}


	memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));
	ret = cap_config_adv7442(&(pstruct->capLink.adv7442_phandle), 0, &videoStatus);

	//set input 2
	if(input2_dvi == 1) {
		if(ret == 0 && videoStatus.isVideoDetect) {
			fprintf(stderr, "input1:ad7442 video detect, width = %d, height = %d\n",
			        videoStatus.frameWidth, videoStatus.frameHeight);
			capture_set_input_hw(SIGNAL_INPUT_2, videoStatus.frameWidth, videoStatus.frameHeight);
		} else {
			capture_set_input_hw(SIGNAL_INPUT_2, 1920, 1080);
		}

		Cap_SetSource7442Chan(&(pstruct->capLink.adv7442_phandle), 1);
	}

	memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));
	ret = cap_config_gs2971(&(pstruct->capLink.gs2971_phandle), 0, &videoStatus);


	//初始化FPGA的版本
	init_fpga_version();





	//input 1,gs2971
	if(input1_sdi == 1) {
		if(ret == 0 && videoStatus.isVideoDetect) {
			fprintf(stderr, "gs2971 video detect, width = %d, height = %d\n",
			        videoStatus.frameWidth, videoStatus.frameHeight);
			capture_set_input_hw(SIGNAL_INPUT_1, videoStatus.frameWidth, videoStatus.frameHeight);
		} else {
			capture_set_input_hw(SIGNAL_INPUT_1, 1920, 1080);
		}
	}

	memset(&videoStatus, 0, sizeof(VCAP_VIDEO_SOURCE_CH_STATUS_S));
	ret = cap_config_gs2972(&(pstruct->capLink.gs2972_phandle), 0, &videoStatus);

	if(input2_sdi == 1) {
		if(ret == 0 && videoStatus.isVideoDetect) {
			fprintf(stderr, "gs2972 video detect, width = %d, height = %d\n",
			        videoStatus.frameWidth, videoStatus.frameHeight);
			capture_set_input_hw(SIGNAL_INPUT_2, videoStatus.frameWidth, videoStatus.frameHeight);
		} else {
			capture_set_input_hw(SIGNAL_INPUT_2, 1920, 1080);
		}
	}

	return 0;
}

#if  0
#define UTILS_DMA_GENERATE_FILL_PATTERN(y,u,v)             ((((y) & 0xFF) << 0)  | \
        (((u) & 0xFF) << 8)  | \
        (((y) & 0xFF) << 16) | \
        (((v) & 0xFF) << 24))
int capture_fill_all_frame(int chid, int queid, int linkid, int open)
{
	CaptureLink_BlindInfo blindinfo;
	CaptureLink_BlindWin blindwin;
	memset(&blindinfo, 0, sizeof(CaptureLink_BlindInfo));
	memset(&blindwin, 0, sizeof(CaptureLink_BlindWin));

	blindinfo.channelId = chid;
	blindinfo.queId = queid;
	blindinfo.numBlindArea = 2;

	blindwin.startX = 0;
	blindwin.startY = 0;
	blindwin.width = 1920;
	blindwin.height = 1080;
	blindwin.fillColorYUYV =  UTILS_DMA_GENERATE_FILL_PATTERN(0x00,
	                          0x80,
	                          0x80);

	if(open == 1) {
		blindwin.enableWin = 1;
	} else {
		blindwin.enableWin = 0;
	}

	blindinfo.win[0] = blindwin;
	blindinfo.win[1] = blindwin;
	PRINTF("\n");
	System_linkControl(linkid, CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA,
	                   &blindinfo, sizeof(CaptureLink_BlindInfo), TRUE);
	return 0;

}

static int g_dei_open = 0;
int capture_fill_dei_frame(int chid, int queid, int linkid, int open)
{
	if(g_dei_open == 0 && open == 0) {
		return 0;
	}

	CaptureLink_BlindInfo blindinfo;
	CaptureLink_BlindWin blindwin;
	memset(&blindinfo, 0, sizeof(CaptureLink_BlindInfo));
	memset(&blindwin, 0, sizeof(CaptureLink_BlindWin));

	blindinfo.channelId = chid;
	blindinfo.queId = queid;
	blindinfo.numBlindArea = 2;

	int width = 2;
	blindwin.startX = 0;
	blindwin.startY = 0 ;
	blindwin.width = width;
	blindwin.height = 540;  //隔行信号，刷黑2个像素数据
	blindwin.fillColorYUYV =  UTILS_DMA_GENERATE_FILL_PATTERN(0x00,
	                          0x80,
	                          0x80);

	if(open == 1) {
		blindwin.enableWin = 1;
		g_dei_open  = 1;
	} else {
		blindwin.enableWin = 0;
		g_dei_open  = 0;
	}

	blindinfo.win[0] = blindwin;
	blindinfo.win[1] = blindwin;
	PRINTF("blindwin.enableWin = %d\n", blindwin.enableWin);
	System_linkControl(linkid, CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA,
	                   &blindinfo, sizeof(CaptureLink_BlindInfo), TRUE);
	return 0;

}


int capture_fill_dvi_frame(int chid, int queid, int linkid, int open)
{
	CaptureLink_BlindInfo blindinfo;
	CaptureLink_BlindWin blindwin;
	memset(&blindinfo, 0, sizeof(CaptureLink_BlindInfo));
	memset(&blindwin, 0, sizeof(CaptureLink_BlindWin));

	blindinfo.channelId = chid;
	blindinfo.queId = queid;
	blindinfo.numBlindArea = 1;

	blindwin.startX = 0;
	blindwin.startY = 0;
	blindwin.width = 4;
	blindwin.height = 540;  //刷黑最左边的4个像素的图像
	blindwin.fillColorYUYV =  UTILS_DMA_GENERATE_FILL_PATTERN(0x00,
	                          0x80,
	                          0x80);

	if(open == 1) {
		blindwin.enableWin = 1;
	} else {
		blindwin.enableWin = 0;
	}

	blindinfo.win[0] = blindwin;
	//blindinfo.win[1] = blindwin;
	PRINTF("blindwin.enableWin = %d\n", blindwin.enableWin);
	System_linkControl(linkid, CAPTURE_LINK_CMD_CONFIGURE_BLIND_AREA,
	                   &blindinfo, sizeof(CaptureLink_BlindInfo), TRUE);
	return 0;

}
#endif
