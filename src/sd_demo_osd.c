
#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>
#include <sys/resource.h>
#include <assert.h>


#include "osd.h"
#include "sd_demo_osd.h"
#include "log_common.h"
#include "reach_enc2000.h"

#define OSD_BUF_HEAP_SR_ID          (0)      //    (0)

Bool g_osd_enable = FALSE;

/**
    \brief Allocated buffer info
*/
typedef struct {

	UInt8  *physAddr;
	/**< Physical address */

	UInt8  *virtAddr;
	/**< Virtual address */

	UInt32  srPtr;
	/**< Shared region Pointer SRPtr */

	UInt32 bufsize;

} SD_Demo_AllocBufInfo;


static AlgLink_OsdChWinParams   g_osd_ChWinParam[SIGNAL_INPUT_MAX];
static SD_Demo_AllocBufInfo g_textosd_bufInfo[SIGNAL_INPUT_MAX];
static SD_Demo_AllocBufInfo g_timeosd_bufInfo ;
static SD_Demo_AllocBufInfo g_pngosd_bufInfo[SIGNAL_INPUT_MAX];

static int g_alg_osd_id = 0;

static Int32 SD_Demo_allocBuf(UInt32 srRegId, UInt32 bufSize, UInt32 bufAlign, SD_Demo_AllocBufInfo *bufInfo);
static Int32 SD_Demo_freeBuf(UInt32 srRegId, UInt8 *virtAddr, UInt32 bufSize);

static Int32 SD_Demo_allocBuf(UInt32 srRegId, UInt32 bufSize, UInt32 bufAlign, SD_Demo_AllocBufInfo *bufInfo)
{
	IHeap_Handle heapHndl;

	heapHndl = SharedRegion_getHeap(srRegId);
	OSA_assert(heapHndl != NULL);

	bufInfo->virtAddr = NULL;
	bufInfo->physAddr = NULL;
	bufInfo->srPtr    = 0;

	bufInfo->virtAddr = Memory_alloc(heapHndl, bufSize, bufAlign, NULL);

	PRINTF("bufInfo->virtAddr size=%d\n", bufSize);

	if(bufInfo->virtAddr == NULL) {
		return -1;
	}

	bufInfo->physAddr = Memory_translate(bufInfo->virtAddr, Memory_XltFlags_Virt2Phys);

	if(bufInfo->physAddr == NULL) {
		return -1;
	}

	bufInfo->srPtr = SharedRegion_getSRPtr(bufInfo->virtAddr, srRegId);

	bufInfo->bufsize = bufSize;
	return 0;
}

static Int32 SD_Demo_freeBuf(UInt32 srRegId, UInt8 *virtAddr, UInt32 bufSize)
{
	IHeap_Handle heapHndl;

	heapHndl = SharedRegion_getHeap(srRegId);
	OSA_assert(heapHndl != NULL);

	OSA_assert(virtAddr != NULL);

	Memory_free(heapHndl, virtAddr, bufSize);

	return 0;
}

Void SD_Demo_osdDeinit()
{
	int i = 0;

	for(i = 0 ; i < SIGNAL_INPUT_MAX; i++) {
		if(g_textosd_bufInfo[i].virtAddr != NULL) {
			SD_Demo_freeBuf(OSD_BUF_HEAP_SR_ID, g_textosd_bufInfo[i].virtAddr, g_textosd_bufInfo[i].bufsize);
			g_textosd_bufInfo[i].virtAddr = NULL;
			g_textosd_bufInfo[i].bufsize = 0;
		}
	}

	if(g_timeosd_bufInfo.virtAddr != NULL) {
		SD_Demo_freeBuf(OSD_BUF_HEAP_SR_ID, g_timeosd_bufInfo.virtAddr, g_timeosd_bufInfo.bufsize);
		g_timeosd_bufInfo.virtAddr = NULL;
		g_timeosd_bufInfo.bufsize = 0;
	}

	for(i = 0 ; i < SIGNAL_INPUT_MAX; i++) {
		if(g_pngosd_bufInfo[i].virtAddr != NULL) {
			SD_Demo_freeBuf(OSD_BUF_HEAP_SR_ID, g_pngosd_bufInfo[i].virtAddr, g_pngosd_bufInfo[i].bufsize);
			g_pngosd_bufInfo[i].virtAddr = NULL;
			g_pngosd_bufInfo[i].bufsize = 0;
		}
	}

	return ;

}




Int32 SD_Demo_osdInit(int algid, int mp_status)
{
	UInt32 osdBufSize, osdBufSizeY, bufAlign;
	int i = 0;
	int chId = 0;
	int winId = 0;
	int status = 0;

	//text osd is yuv422spi
	osdBufSizeY = TEXT_BITMAP_WIDTH * TEXT_BITMAP_HEIGHT;
	osdBufSize = osdBufSizeY * 2 ;
	bufAlign = 128;

	for(i = 0 ; i < SIGNAL_INPUT_MAX; i ++) {
		status = SD_Demo_allocBuf(OSD_BUF_HEAP_SR_ID, osdBufSize, bufAlign, &g_textosd_bufInfo[i]);
		OSA_assert(status == OSA_SOK);
		PRINTF("i will malloc %d buff,the status=%d\n", osdBufSize, status);
	}

	//time osd is yuv 422spi
	osdBufSizeY = TIME_BITMAP_WIDTH * TIME_BITMAP_HEIGHT;
	osdBufSize = osdBufSizeY * 2 ;
	bufAlign = 128;

	status = SD_Demo_allocBuf(OSD_BUF_HEAP_SR_ID, osdBufSize, bufAlign, &g_timeosd_bufInfo);
	OSA_assert(status == OSA_SOK);
	PRINTF("i will malloc %d buff,the status=%d\n", osdBufSize, status);

#if 1
	//png osd is yuv 420sp
	osdBufSizeY = PNG_MAX_WIDTH * PNG_MAX_HEIGHT;
	osdBufSize = osdBufSizeY * 2 ;
	bufAlign = 128;

	for(i = 0 ; i < SIGNAL_INPUT_MAX; i ++) {

		status = SD_Demo_allocBuf(OSD_BUF_HEAP_SR_ID, osdBufSize, bufAlign, &g_pngosd_bufInfo[i]);
		OSA_assert(status == OSA_SOK);
		PRINTF("i will malloc %d buff,the status=%d\n", osdBufSize, status);
	}

#endif


	for(chId = 0; chId < SIGNAL_INPUT_MAX; chId++) {
		AlgLink_OsdChWinParams *chWinPrm = &g_osd_ChWinParam[chId];

		memset(chWinPrm, 0, sizeof(AlgLink_OsdChWinParams));

		chWinPrm->chId = chId;
		chWinPrm->numWindows = WINDOW_MAX_OSD;// 0:text osd 1:time osd 3:png

		chWinPrm->colorKey[0] = 0xfa; /* Y */
		chWinPrm->colorKey[1] = 0x7d; /* U */
		chWinPrm->colorKey[2] = 0x7e; /* V */

		for(winId = 0; winId < chWinPrm->numWindows; winId++) {
			chWinPrm->winPrm[winId].startX             	= SD_DEMO_OSD_WIN0_STARTX ;
			chWinPrm->winPrm[winId].startY             	= SD_DEMO_OSD_WIN0_STARTY + (TIME_BITMAP_HEIGHT + SD_DEMO_OSD_WIN0_STARTY) * winId;

			chWinPrm->winPrm[winId].globalAlpha        = 0X80;
			chWinPrm->winPrm[winId].transperencyEnable = FALSE;
			chWinPrm->winPrm[winId].enableWin          = FALSE;

			//text osd
			if(winId == WINDOW_TEXT_OSD) {
				chWinPrm->winPrm[winId].width        	= TEXT_BITMAP_WIDTH;
				chWinPrm->winPrm[winId].height          = TEXT_BITMAP_HEIGHT;
				chWinPrm->winPrm[winId].addr[0][0] = (g_textosd_bufInfo[chId].physAddr);
			} else if(winId == WINDOW_TIME_OSD) {
				chWinPrm->winPrm[winId].width        	= TIME_BITMAP_WIDTH;
				chWinPrm->winPrm[winId].height         	= TIME_BITMAP_HEIGHT;
				chWinPrm->winPrm[winId].addr[0][0] = (g_timeosd_bufInfo.physAddr);
			} else if(winId == WINDOW_LOGO_OSD) {
				chWinPrm->winPrm[winId].width        	= PNG_DF_WIDTH;
				chWinPrm->winPrm[winId].height          = PNG_DF_HEIGHT;
				chWinPrm->winPrm[winId].addr[0][0] = (g_pngosd_bufInfo[chId].physAddr);
			}

			chWinPrm->winPrm[winId].lineOffset			= chWinPrm->winPrm[winId].width ;

			if(mp_status != IS_MP_STATUS) {
				chWinPrm->winPrm[winId].format     =  SYSTEM_DF_YUV420SP_UV;
				chWinPrm->winPrm[winId].addr[0][1] =  chWinPrm->winPrm[winId].addr[0][0] + chWinPrm->winPrm[winId].lineOffset * chWinPrm->winPrm[winId].height;
			} else {
				//mp ---> SYSTEM_DF_YUV422I_YUYV
				chWinPrm->winPrm[winId].format     =  SYSTEM_DF_YUV422I_YUYV;
			}


		}

	}

	g_alg_osd_id = algid;

	return 0;
}


/* osd add text and time subtitle*/
Int32 SD_subtitle_osdUpdate(int input, int osdtype, unsigned char *osdbuff, int osdlen, TextInfo *info)
{
	if(osdtype != WINDOW_TIME_OSD) {
		PRINTF("input =%d,osdtype=%d,osdlen=%d,\n", input, osdtype, osdlen);
		PRINTF("info.x=%d,y=%d,w=%d,h=%d,alpha=%d\n", info->xpos, info->ypos, info->width, info->height, info->alpha);
	}

	if(FALSE == g_osd_enable || 0 == check_input_valid(input)) {
		ERR_PRN("\n");
		return -1;
	}

	int algid = g_alg_osd_id ;
	int winId = 0;
	int status = 0;
	UInt8 *curVirtAddr = NULL;
	AlgLink_OsdChWinParams *chWinPrm = NULL;
	//	int patch_len = 32 - info->width % 32;
	int  chid = 0;

	if(input >= SIGNAL_INPUT_MAX) {
		PRINTF("error,the chnum =%d > max osd windows\n", input);
		return -1;
	}

	chid = input_get_mer1_chid(input);

	if(osdtype != WINDOW_TIME_OSD) {
		PRINTF("input=%d,chid=%d,osdtype=%d\n", input, chid, osdtype);
	}

	if(chid < 0) {
		PRINTF("input=%d,chid=%d,will return !\n", input, chid);
		return -1;
	}

	//change osd type  to window num
	if(osdtype == WINDOW_TEXT_OSD) { //text osd
		winId = osdtype;
		curVirtAddr = g_textosd_bufInfo[input].virtAddr ;
	} else if(osdtype == WINDOW_TIME_OSD) { //time osd
		winId = osdtype;
		curVirtAddr = g_timeosd_bufInfo.virtAddr ;
	}

	chWinPrm = &g_osd_ChWinParam[input];

	//must transperencyEnable = True
	chWinPrm->colorKey[0] = 0xff;
	chWinPrm->colorKey[1] = 0xff;
	chWinPrm->colorKey[2] = 0xff;

	//set vp ʵ�ʶ�Ӧ�� merout ch
	chWinPrm->chId = chid;

	if(winId < chWinPrm->numWindows) {
		if(osdlen != 0) {
			chWinPrm->winPrm[winId].startX             =	info->xpos;//60+SD_DEMO_OSD_WIN0_STARTX ;
			chWinPrm->winPrm[winId].startY             =	info->ypos;//100+SD_DEMO_OSD_WIN0_STARTY + (SD_DEMO_OSD_WIN_HEIGHT + SD_DEMO_OSD_WIN0_STARTY) * winId;
			chWinPrm->winPrm[winId].width			   =   	4 * (info->width / 4);
			chWinPrm->winPrm[winId].height             =    4 * (info->height / 4);

			if(chWinPrm->winPrm[winId].width > OSD_MAX_WIDTH) {
				WARN_PRN("chWinPrm->winPrm[%d].width = %d\n", winId, chWinPrm->winPrm[winId].width);
				chWinPrm->winPrm[winId].width = OSD_MAX_WIDTH;
			}

			if(chWinPrm->winPrm[winId].height > OSD_MAX_HEIGHT) {
				WARN_PRN("chWinPrm->winPrm[%d].height = %d\n", winId, chWinPrm->winPrm[winId].height);
				chWinPrm->winPrm[winId].height = OSD_MAX_HEIGHT;
			}

			chWinPrm->winPrm[winId].globalAlpha        = info->alpha;

			chWinPrm->winPrm[winId].transperencyEnable = TRUE; //͸���Ǹ���ɫ����color key�й�ϵ
			chWinPrm->winPrm[winId].enableWin          = TRUE;

			if(curVirtAddr != NULL) {
				memcpy(curVirtAddr, osdbuff, osdlen);
			}

		} else {
			chWinPrm->winPrm[winId].enableWin          = FALSE;
			PRINTF("i will close the input=%d ,close the %d type osd\n", input, winId);
		}

	}


	status = System_linkControl(
	             algid,
	             ALG_LINK_OSD_CMD_SET_CHANNEL_WIN_PRM,
	             chWinPrm,
	             sizeof(AlgLink_OsdChWinParams),
	             TRUE);

	//	PRINTF("intput=%d ---osdtype=%d done!\n",input,osdtype);


	return 0;

}

Int32 SD_logo_osdUpdate(int input, int osdtype, unsigned char *osdbuff, int osdlen, LogoInfo *info)
{
	PRINTF("input =%d,osdtype=%d,osdlen=%d,\n", input, osdtype, osdlen);
	PRINTF(">>>>>  info.x=%d,y=%d,w=%d,h=%d,alpha=%d\n", info->x, info->y, info->width, info->height, info->alpha);

	if(FALSE == g_osd_enable || 0 == check_input_valid(input)) {
		ERR_PRN("\n");
		return -1;
	}

	AlgLink_OsdChWinParams *chWinPrm = NULL;
	SD_Demo_AllocBufInfo *temp_bufinfo = NULL;
	int algid = g_alg_osd_id ;
	int winId = 0;
	int status = 0;
	//	UInt8 *curVirtAddr = NULL;
	int 	osdBufSize = 0, bufAlign = 0;
	int  chid = 0;

	if(input >= SIGNAL_INPUT_MAX) {
		PRINTF("error,the chnum =%d > max osd windows\n", input);
		return -1;
	}

	chid = input_get_mer1_chid(input);
	PRINTF("input=%d,chnum=%d\n", input, chid);

	if(chid < 0) {
		PRINTF("chid = %d <0 ,will return !\n", chid);
		return -1;
	}

	winId = WINDOW_LOGO_OSD;
	temp_bufinfo = &(g_pngosd_bufInfo[input]);

	if(temp_bufinfo->virtAddr != NULL) {
		SD_Demo_freeBuf(OSD_BUF_HEAP_SR_ID, temp_bufinfo->virtAddr, temp_bufinfo->bufsize);
		temp_bufinfo->virtAddr = NULL;
		temp_bufinfo->physAddr = NULL;
	}

	if(temp_bufinfo->virtAddr == NULL) {
		osdBufSize = osdlen * 2 ;
		bufAlign = 128;
		status = SD_Demo_allocBuf(OSD_BUF_HEAP_SR_ID, osdBufSize, bufAlign, temp_bufinfo);
	}

	chWinPrm = &g_osd_ChWinParam[input];

	//must transperencyEnable = True
	chWinPrm->colorKey[0] = 0xfa; /* Y--fa */
	chWinPrm->colorKey[1] = 0x7d; /* U--7d */
	chWinPrm->colorKey[2] = 0x7e; /* V --7e*/

	//set input  ʵ�ʶ�Ӧ�� merlink1 out ch
	chWinPrm->chId = chid;
	memcpy(temp_bufinfo->virtAddr, osdbuff, osdlen);

	if(osdlen != 0) {
		chWinPrm->winPrm[winId].startX             =	info->x;//60+SD_DEMO_OSD_WIN0_STARTX ;
		chWinPrm->winPrm[winId].startY             =	info->y;//100+SD_DEMO_OSD_WIN0_STARTY + (SD_DEMO_OSD_WIN_HEIGHT + SD_DEMO_OSD_WIN0_STARTY) * winId;
		chWinPrm->winPrm[winId].width 			   = ((info->width > PNG_MAX_WIDTH) ? PNG_MAX_WIDTH : (info->width));
		chWinPrm->winPrm[winId].height 			   = ((info->height > PNG_MAX_HEIGHT) ? PNG_MAX_HEIGHT : (info->height));

		chWinPrm->winPrm[winId].lineOffset		   = chWinPrm->winPrm[winId].width ;
		chWinPrm->winPrm[winId].globalAlpha        = info->alpha;

		chWinPrm->winPrm[winId].addr[0][0] 		   = temp_bufinfo->physAddr;

		if(SIGNAL_INPUT_MP != input) {
			chWinPrm->winPrm[winId].addr[0][1]         =  chWinPrm->winPrm[winId].addr[0][0] + (info->width * info->height);
			chWinPrm->winPrm[winId].format			   = SYSTEM_DF_YUV420SP_UV;
		} else {
			chWinPrm->winPrm[winId].format			   = SYSTEM_DF_YUV422I_YUYV;
		}

		chWinPrm->winPrm[winId].transperencyEnable = TRUE; //͸���Ǹ���ɫ����color key�й�ϵ
		chWinPrm->winPrm[winId].enableWin          = TRUE;
	} else {
		PRINTF("i will close the logo show\n");
		chWinPrm->winPrm[winId].enableWin          = FALSE;
	}


	{
		PRINTF("chnum=%d,addr =%p==%p\n", input, chWinPrm->winPrm[0].addr[0][0], chWinPrm->winPrm[0].addr[0][1]);
		PRINTF("enableWin = %d=%d\n", chWinPrm->winPrm[0].enableWin, chWinPrm->winPrm[0].enableWin);

		status = System_linkControl(
		             algid,
		             ALG_LINK_OSD_CMD_SET_CHANNEL_WIN_PRM,
		             chWinPrm,
		             sizeof(AlgLink_OsdChWinParams),
		             TRUE);
	}

	return 0;

}
