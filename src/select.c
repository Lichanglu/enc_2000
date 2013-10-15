#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <assert.h>



#include <mcfw/interfaces/common_def/ti_vsys_common_def.h>
#include <mcfw/interfaces/common_def/ti_vdis_common_def.h>
#include <mcfw/src_linux/mcfw_api/reach_system_priv.h>


#include "reach_enc2000.h"
#include "capture.h"
#include "select.h"
#include "common.h"
#include "log.h"
#include "log_common.h"
#include "middle_control.h"
#include "sd_demo_osd.h"
#include "app_mp_control.h"

#define MMAP_MEM_PAGEALIGN         (4*1024-1)



int g_vp0_writeyuv_flag = 0;
int g_vp1_writeyuv_flag = 0;
int g_mp_writeyuv_flag = 0;

/*==============================================================================
    函数:  Int32 set_resolution(Int32 VpNum, Int32 streamId, UInt32 outWidth, UInt32 outHeight)
    功能:  设置各VP口高低码率通道分辨率
    参数:  input :SIGNAL_INPUT_1,SIGNAL_INPUT_2
           streamId: 高低码流ID (LOW_STREAM,HIGH_STREAM)
		   1:如果需要直通的话，既不缩放的时候，设置outwidth,outheight等于采集宽高即可

    Created By 徐崇 2012.09.14 09:09:14

           |           out 0               |  out  1      |
           | ch0 | ch1 | ch2 | ch3  |  ch0  | ch1  |  chnum1 |
           |  0   |  2   |  1    |  3   |  3    |   3   |    2    | INPUT 2直通模式
           |  0   |  2   |  1    |  3   |  1    |   3   |    2    | INPUT 1 ,INPUT 2直通模式
           |  0   |  2   |  1    |  3   |  1    |   1   |    2    | INPUT 1直通模式
           |  0   |  2   |  1    |  3   |  x    |   x   |    0    | INPUT 1，INPUT 2 都不直通
=============== ===============================================================*/
Int32 set_resolution(Int32 input, Int32 streamId, UInt32 outWidth, UInt32 outHeight, UInt32 inWidth, UInt32 inHeight)
{
	PRINTF("input =%d,streamid=%d,outwidth=%d,h=%d,in=%d,=%d\n", input, streamId, outWidth, outHeight, inWidth, inHeight);

	if((input >= SIGNAL_INPUT_MAX) || (streamId >= MAX_STREAM)) {
		return -1;
	}

	if((outWidth <= 0) || (outHeight <= 0)) {
		return -1;
	}

	outWidth  = Reach_floor(outWidth, 16u); // Reach_align(outWidth, 16u);
	outHeight = Reach_floor(outHeight, 8u); //Reach_align(outHeight, 8u);

	if(LOW_STREAM == streamId) {
		SclrLink_chDynamicSetOutRes params;

		/* 设置各通道输出分辨率  */
		params.width  = outWidth;
		params.height = outHeight;
		params.pitch[0]  = outWidth * 2;
		params.pitch[1]  = outWidth * 2;
		params.pitch[2]  = outWidth * 2;
		params.chId = input;

		sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
		sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, input, 60, 30);
	} else if(HIGH_STREAM == streamId) {
		int width, height;

		SelectLink_OutQueChInfo prm;
		SclrLink_chDynamicSetOutRes params;
		prm.outQueId = 1;
		capture_get_input_hw(input, &width, &height);

		if(height == 540 && width == 1920) {
			height = height * 2;
		}

		if(inWidth != 0) {
			width = inWidth;
		}

		if(inHeight != 0) {
			height = inHeight;
		}

		select_get_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2, &prm);
		//		PRINTF("prm.inChNum[0] =%d %d %d\n", prm.numOutCh, prm.inChNum[0], prm.inChNum[1]);

		/* 当没有直通时 */
		if(prm.numOutCh == 0) {
			if(input == SIGNAL_INPUT_2) {

				/* 进行缩放 */
				if((width != outWidth) || (height != outHeight)) {
					params.width  = outWidth;
					params.height = outHeight;
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
					params.chId = 3;
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 3, 60, 60);
				}
				/* input 2 直通 */
				else {
					//设置2次
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 2;
					prm.inChNum[0] = 3;
					prm.inChNum[1] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);

					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0; //  input 1/adv7442 ,low stream
					prm.inChNum[1] = 2;	// input 1/adv7442,high scale stream
					prm.inChNum[2] = 1;  // input2/adv7441,low stream
					prm.inChNum[3] = 5;  //  input2/adv7441,high stream
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}
			} else if(input == SIGNAL_INPUT_1) {
				/*  input 1进行缩放 */
				if((width != outWidth) || (height != outHeight)) {
					params.width  = outWidth;
					params.height = outHeight;
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
					params.chId = 2;
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 2, 60, 60);
				}
				/* input 1 直通 */
				else {
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 2;
					prm.inChNum[0] = 1;
					prm.inChNum[1] = 1;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);

					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;//  input 1/adv7442 ,low stream
					prm.inChNum[1] = 5; //  input1/adv7442,origin stream
					prm.inChNum[2] = 1; // input2/adv7441,low stream
					prm.inChNum[3] = 3;// input 2/adv7441,high scale stream
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}
			} else {
				assert(0);
			}
		}

		/* 当只有input 2 直通时 */
		if((prm.inChNum[0] == 3) && (prm.inChNum[1] == 3)) {

			if(input == SIGNAL_INPUT_2) {
				/* 已验证 */

				if((width != outWidth) || (height != outHeight)) {
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 0;
					prm.inChNum[0] = 1;
					prm.inChNum[1] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					params.width  = outWidth;
					params.height = outHeight;
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
					params.chId = 3;
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 3, 60, 60);
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}
			} else if(input == SIGNAL_INPUT_1) {

				PRINTF("2133 outWidth [%d] outHeight[%d] %d %d", outWidth, outHeight, width, height);

				/* 进行缩放 */
				if((width != outWidth) || (height != outHeight)) {
					params.width  = outWidth;
					params.height = outHeight;
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
					params.chId = 2;
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 3, 60, 60);
				}
				/* input 1 直通 */
				else {
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 2;
					prm.inChNum[0] = 1;
					prm.inChNum[1] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);

					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}
			} else {
				assert(0);
			}

		}
		/* 当input 1, input 2都直通 */
		else if((prm.inChNum[0] == 1) && (prm.inChNum[1] == 3)) {
			PRINTF(",======================================\n");

			if(input == SIGNAL_INPUT_2) {
				/* 已验证 */
				if((width != outWidth) || (height != outHeight)) {
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 2;
					prm.inChNum[0] = 1;
					prm.inChNum[1] = 1;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					params.width  = outWidth;
					params.height = outHeight;
#if 1
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
#else
					params.pitch[0]  = outWidth;
					params.pitch[1]  = outWidth;
					params.pitch[2]  = outWidth;
#endif
					params.chId = 3;

					PRINTF("outWidth = %d outHeight = %d %d\n", outWidth, outHeight, params.pitch[0]);
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 3, 60, 60);
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 5;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}
			} else if(input == SIGNAL_INPUT_1) {
				PRINTF("outWidth [%d] outHeight[%d] %d %d", outWidth, outHeight, width, height);

				/* VP0进行缩放 */
				if((width != outWidth) || (height != outHeight)) {

					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 2;
					prm.inChNum[0] = 3;
					prm.inChNum[1] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					params.width  = outWidth;
					params.height = outHeight;
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
					params.chId = 2;
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 2, 60, 60);
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 5;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}

			} else {
				assert(0);
			}

		}
		/* 当只有input 1直通 */
		else if((prm.inChNum[0] == 1) && (prm.inChNum[1] == 1)) {
			if(input == SIGNAL_INPUT_2) {


				/* input 2进行缩放 */
				if((width != outWidth) || (height != outHeight)) {
					params.width  = outWidth;
					params.height = outHeight;
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
					params.chId = 3;
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 3, 60, 60);
				}
				/* input 2 直通 */
				else {
					/* 已验证 */
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 2;
					prm.inChNum[0] = 1;
					prm.inChNum[1] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);

					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 4;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 5;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}
			} else if(input == SIGNAL_INPUT_1) {


				if((width != outWidth) || (height != outHeight)) {
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					prm.outQueId = 1;
					prm.numOutCh = 0;
					prm.inChNum[0] = 1;
					prm.inChNum[1] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_2,  &prm);
					params.width  = outWidth;
					params.height = outHeight;
					params.pitch[0]  = outWidth * 2;
					params.pitch[1]  = outWidth * 2;
					params.pitch[2]  = outWidth * 2;
					params.chId = 2;
					sclr_set_output_resolution(SYSTEM_LINK_ID_SCLR_INST_1, &params);
					sclr_set_framerate(SYSTEM_LINK_ID_SCLR_INST_1, 2, 60, 60);
					prm.outQueId = 0;
					prm.numOutCh = 4;
					prm.inChNum[0] = 0;
					prm.inChNum[1] = 2;
					prm.inChNum[2] = 1;
					prm.inChNum[3] = 3;
					select_set_outque_chinfo(SYSTEM_VPSS_LINK_ID_SELECT_3,  &prm);
				}
			} else {
				assert(0);
			}
		}

	}

	return 0;
}



#if HAVE_MP_MODULE

#if 1
static Void swms_set_swms_layout2(UInt32 devId, SwMsLink_CreateParams *swMsCreateArgs, UInt32 num)
{
	SwMsLink_LayoutPrm *layoutInfo;
	SwMsLink_LayoutWinInfo *winInfo;
	UInt32 outWidth, outHeight, row, col, winId, widthAlign, heightAlign;
	UInt32 outputfps;

	getoutsize(swMsCreateArgs->maxOutRes, &outWidth, &outHeight);

	widthAlign = 8;
	heightAlign = 1;

	if(devId > 1) {
		devId = 0;
	}

	layoutInfo = &swMsCreateArgs->layoutPrm;
	outputfps = layoutInfo->outputFPS;

	memset(layoutInfo, 0, sizeof(*layoutInfo));
	layoutInfo->onlyCh2WinMapChanged = FALSE;
	layoutInfo->outputFPS = outputfps;

	layoutInfo->numWin = num;

	if(num == 1) {
		for(row = 0; row < 1; row++) {
			for(col = 0; col < 1; col++) {
				winId = row * 2 + col;
				winInfo = &layoutInfo->winInfo[winId];

				winInfo->width	= SystemUtils_align((outWidth * 2) / 4, widthAlign);
				winInfo->height = SystemUtils_align(outHeight / 2, heightAlign);
				winInfo->startX = winInfo->width * col;
				winInfo->startY = winInfo->height * row;

#if 1
				winInfo->width	= 1920;
				winInfo->height = 1080;
				winInfo->startX = 0;
				winInfo->startY = 0;
#endif

				winInfo->bypass = FALSE;
				winInfo->channelNum = devId * SYSTEM_SW_MS_MAX_WIN + winId;

			}
		}
	} else if(num == 2) {
		winInfo = &layoutInfo->winInfo[0];
		winInfo->width	= SystemUtils_align((outWidth), widthAlign);
		winInfo->height = SystemUtils_align(outHeight, heightAlign);
		winInfo->startX = 0;
		winInfo->startY = 0;
		winInfo->bypass = FALSE;
		winInfo->channelNum = 0;

		winInfo = &layoutInfo->winInfo[1];
		winInfo->width	= SystemUtils_align((outWidth) / 3, widthAlign);
		winInfo->height = SystemUtils_align(outHeight / 3, heightAlign);
		winInfo->startX = outWidth - winInfo->width;
		winInfo->startY = outHeight - winInfo->height;
		winInfo->bypass = FALSE;
		winInfo->channelNum = 1;
	}
}
#endif
#endif

Int32 write_yuv(UInt32 physaddr, Uint32 framesize, int chid)
{
	int fd = 0;
	unsigned int mmap_offset = 0;
	unsigned int mmap_memaddr = 0;
	unsigned int mmap_memsize = 0;
	volatile unsigned int *mmap_pvirtaddr = NULL;

	UInt32 pvirtaddr;

	fd = open("/dev/mem", O_RDWR | O_SYNC);

	if(fd < 0) {
		PRINTF(" ERROR: /dev/mem open failed !!!\n");
		return -1;
	}

	mmap_offset		= physaddr & MMAP_MEM_PAGEALIGN;
	mmap_memaddr	= physaddr - mmap_offset;
	mmap_memsize	= framesize + mmap_offset;

	mmap_pvirtaddr = mmap(
	                     (void *)mmap_memaddr,
	                     mmap_memsize,
	                     PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED,
	                     fd,
	                     mmap_memaddr
	                 );

	if(mmap_pvirtaddr == NULL) {
		PRINTF(" ERROR: mmap() failed !!!\n");
		return -1;
	}

	pvirtaddr = (UInt32)((UInt32)mmap_pvirtaddr + mmap_offset);

#if 0
	static int time0 = 0;
	static int time1 = 0;
	char buf[256] = {0};

	if(chid == 0) {
		sprintf(buf, "picture_vp0_%d.yuv", time0++);
	} else {
		sprintf(buf, "picture_vp1_%d.yuv", time1++);
	}

	FILE *fp = fopen(buf, "w+");

	if(!fp) {
		munmap((void *)mmap_pvirtaddr, mmap_memsize);
		close(fd);
		return -1;
	}

	fwrite((unsigned char *)(pvirtaddr), 1, framesize, fp);
	fclose(fp);
#endif

#if 1
	unsigned char *pY, *pU, *pV;
	pY = (unsigned char *)pvirtaddr;
	pU = (unsigned char *)pvirtaddr + 1920 * 1200;
	pV = (unsigned char *)pvirtaddr + 1920 * 1200 + 1;

	unsigned int vwidth = 1920;
	unsigned int vheight = 1080;

	//	capture_get_input_hw(chid, &vwidth, &vheight);
	if(chid == 2) {
		write_yuv_420(pY, pU, pV, vwidth, vheight, chid);
	} else {
		write_yuv_420(pY, pU, pV, vwidth, vheight, chid);
	}

#endif
	munmap((void *)mmap_pvirtaddr, mmap_memsize);
	close(fd);

	return 0;
}


static Void *writeyuv_tsk(ENC2000_LINK_STRUCT *pstruct)
{
	Int status;
	int index = 0;
	int framesize = 0;

	VIDFrame_BufList bufList;

	while(1) {
		usleep(16000);
		status = IpcFramesInLink_getFullVideoFrames(pstruct->ipcFramesInHostId, &bufList);
		OSA_assert(0 == status);

		if(bufList.numFrames) {
			for(index = 0; index < bufList.numFrames; index++) {
				//		fprintf(stderr, "channel = %d\n", bufList.frames[index].channelNum);
				//		fprintf(stderr, "frameWidth = %d\n", bufList.frames[index].frameWidth);
				//		fprintf(stderr, "frameHeight = %d\n", bufList.frames[index].frameHeight);

				if(g_vp0_writeyuv_flag && bufList.frames[index].channelNum == 0) {
					g_vp0_writeyuv_flag = 0;
					framesize = 1920 * 1080 * 2;
					write_yuv((UInt32)(bufList.frames[index].phyAddr[0][0]), framesize, 0);
				}

				if(g_vp1_writeyuv_flag && bufList.frames[index].channelNum == 1) {
					g_vp1_writeyuv_flag = 0;
					framesize = 1920 * 1080 * 2;
					write_yuv((UInt32)(bufList.frames[index].phyAddr[0][0]), framesize, 1);
				}

				if(g_mp_writeyuv_flag && bufList.frames[index].channelNum == 2) {
					g_mp_writeyuv_flag = 0;
					framesize = 1920 * 1080 * 2;
					write_yuv((UInt32)(bufList.frames[index].phyAddr[0][0]), framesize, 2);
				}
			}

		}

		//		fprintf(stderr, "bufList.numFrames = %d\n", bufList.numFrames);
		if(bufList.numFrames) {
			status = IpcFramesInLink_putEmptyVideoFrames(pstruct->ipcFramesInHostId, &bufList);
			OSA_assert(0 == status);
		}
	}

	return NULL;
}


Int32 reach_writeyuv_process(ENC2000_LINK_STRUCT *pstruct)
{
	Int32 status = 0;
	pstruct->capLink.isDetect = TRUE;
	status = OSA_thrCreate(&pstruct->capLink.taskHand,
	                       (OSA_ThrEntryFunc)writeyuv_tsk,
	                       99,
	                       0,
	                       pstruct);
	OSA_assert(status == 0);

	return 0;
}


Int32 reach_select_process(ENC2000_LINK_STRUCT *pstruct)
{
	MergeLink_CreateParams			*pMergePrm1;
	MergeLink_CreateParams			*pMergePrm2;
	SelectLink_CreateParams			*pSelectPrm0;

	SelectLink_CreateParams			*pSelectPrm1;

	NsfLink_CreateParams			*pNsfPrm1;
	IpcLink_CreateParams			*pIpcOutVpssPrm;
	DupLink_CreateParams            *pDupPrm0;
	DupLink_CreateParams            *pDupPrm1;
	SclrLink_CreateParams           *pSclrPrm1;

#if HAVE_MP_MODULE
	//	MergeLink_CreateParams			*pMergePrm4;
	//	SwMsLink_CreateParams           *pSwmsprm;
	//	SelectLink_CreateParams			*pSelectPrm4;
#endif

#if WRITE_YUV_TASK
	MergeLink_CreateParams			*pMergePrm4;
	IpcFramesOutLinkRTOS_CreateParams  *pIpcFramesOutVpssToHostPrm;
	IpcFramesInLinkHLOS_CreateParams   *pIpcFramesInHostPrm;
#endif




	//IpcFramesInLinkRTOS_CreateParams  *pIpcFramesInDspPrm;
	//AlgLink_CreateParams                         *pOsdDspAlgPrm;

	//dup0 ,注意此时dup不是真实复制出来的
	//in : 1 que  and 1 ch  .ch={input1 ,or null 1}
	//out: 2 que  and 2 ch

	pDupPrm0 = &(pstruct->dupLink[0].create_params);
	pDupPrm0->inQueParams.prevLinkId		= pstruct->selectLink[1].link_id;
	pDupPrm0->inQueParams.prevLinkQueId	= 0; //input 1 ,adv7442
#if WRITE_YUV_TASK
	pDupPrm0->numOutQue					= 3;
	pDupPrm0->outQueParams[2].nextLink 	= pstruct->mergeLink[4].link_id;
#else
	pDupPrm0->numOutQue					= 2;
#endif
	pDupPrm0->outQueParams[0].nextLink 	= pstruct->mergeLink[2].link_id;
	pDupPrm0->outQueParams[1].nextLink 	= pstruct->mergeLink[2].link_id;
	pDupPrm0->notifyNextLink			= TRUE;


	//dup1 ,注意此时dup不是真实复制出来的
	//in : 1 que  and 1 ch  .ch={input2 ,or null 2}
	//out: 2 que  and 2 ch

	pDupPrm1 = &(pstruct->dupLink[1].create_params);
	pDupPrm1->inQueParams.prevLinkId		= pstruct->selectLink[1].link_id;
	pDupPrm1->inQueParams.prevLinkQueId	= 1; //input 2 ,adv7441
#if WRITE_YUV_TASK
	pDupPrm1->numOutQue					= 3;
	pDupPrm1->outQueParams[2].nextLink 	= pstruct->mergeLink[4].link_id;
#else
	pDupPrm1->numOutQue					= 2;
#endif

	pDupPrm1->outQueParams[0].nextLink 	= pstruct->mergeLink[2].link_id;
	pDupPrm1->outQueParams[1].nextLink 	= pstruct->mergeLink[2].link_id;
	pDupPrm1->notifyNextLink			= TRUE;


#if WRITE_YUV_TASK/* 写YUV */
	/*
		pMergePrm4 = &(pstruct->mergeLink[4].create_params);
		pMergePrm4->numInQue						= 2;

		pMergePrm4->inQueParams[0].prevLinkId	= pstruct->dupLink[0].link_id;
		pMergePrm4->inQueParams[0].prevLinkQueId	= 2;
		pMergePrm4->inQueParams[1].prevLinkId	= pstruct->dupLink[1].link_id;
		pMergePrm4->inQueParams[1].prevLinkQueId	= 2;
		pMergePrm4->outQueParams.nextLink		= pstruct->ipcFramesOutVpssToHostId;
		pMergePrm4->notifyNextLink				= TRUE;

		pIpcFramesOutVpssToHostPrm = &(pstruct->ipcFramesOutVpssToHostPrm);
		pIpcFramesOutVpssToHostPrm->baseCreateParams.noNotifyMode = TRUE;
		pIpcFramesOutVpssToHostPrm->baseCreateParams.notifyNextLink = FALSE;
		pIpcFramesOutVpssToHostPrm->baseCreateParams.notifyPrevLink = TRUE;
		pIpcFramesOutVpssToHostPrm->baseCreateParams.inQueParams.prevLinkId = pstruct->mergeLink[4].link_id;
		pIpcFramesOutVpssToHostPrm->baseCreateParams.inQueParams.prevLinkQueId = 0;
		pIpcFramesOutVpssToHostPrm->baseCreateParams.outQueParams[0].nextLink = pstruct->ipcFramesInHostId;

		pIpcFramesInHostPrm = &(pstruct->ipcFramesInHostPrm);
		pIpcFramesInHostPrm->baseCreateParams.noNotifyMode = TRUE;
		pIpcFramesInHostPrm->baseCreateParams.notifyNextLink = FALSE;
		pIpcFramesInHostPrm->baseCreateParams.notifyPrevLink = FALSE;
		pIpcFramesInHostPrm->baseCreateParams.inQueParams.prevLinkId = pstruct->ipcFramesOutVpssToHostId;
		pIpcFramesInHostPrm->baseCreateParams.inQueParams.prevLinkQueId = 0;
		pIpcFramesInHostPrm->baseCreateParams.outQueParams[0].nextLink = SYSTEM_LINK_ID_INVALID;
		pIpcFramesInHostPrm->exportOnlyPhyAddr = TRUE;
		pIpcFramesInHostPrm->cbCtx = NULL;
		pIpcFramesInHostPrm->cbFxn = NULL;
	*/
#endif

	/* MergeLink1合成高低码流 */
	//in :4 que and 4 ch .ch={input1,input1,input2,input2}
	//out: 1 que and 4 ch .ch={input1,input1,input2,input2}
	pMergePrm1 = &(pstruct->mergeLink[2].create_params);
	pMergePrm1->numInQue						= 4;

	pMergePrm1->inQueParams[0].prevLinkId	= pstruct->dupLink[0].link_id;
	pMergePrm1->inQueParams[0].prevLinkQueId	= 0; //input 1,adv7442 or null 1
	pMergePrm1->inQueParams[1].prevLinkId	= pstruct->dupLink[0].link_id;
	pMergePrm1->inQueParams[1].prevLinkQueId	= 1;

	pMergePrm1->inQueParams[2].prevLinkId	= pstruct->dupLink[1].link_id;
	pMergePrm1->inQueParams[2].prevLinkQueId	= 0; //input 2, adv7441 or null 2
	pMergePrm1->inQueParams[3].prevLinkId	= pstruct->dupLink[1].link_id;
	pMergePrm1->inQueParams[3].prevLinkQueId	= 1;

	pMergePrm1->outQueParams.nextLink		= pstruct->selectLink[2].link_id;
	pMergePrm1->notifyNextLink				= TRUE;

	/* SelectLink筛选流 */
	//in: 1 que and 4 ch .ch={inpu1,input1,input2,input2}
	//out: 2 que and 4ch+2ch .4ch ={input1,input2,input1,input2} .2ch = {input1,input2}
	//直通的情况下，直接跳到2ch
	//现有的流程，强制缩放小
	//后续性能不够的情况下，可以考虑重写这部分代码

	pSelectPrm0 = &(pstruct->selectLink[2].create_params);
	pSelectPrm0->inQueParams.prevLinkId	  = pstruct->mergeLink[2].link_id;
	pSelectPrm0->inQueParams.prevLinkQueId = 0;
	pSelectPrm0->numOutQue = 2;
	pSelectPrm0->outQueParams[0].nextLink = pstruct->sclrLink[1].link_id;
	pSelectPrm0->outQueChInfo[0].numOutCh   = 4;
	pSelectPrm0->outQueChInfo[0].inChNum[0] = 0;
	pSelectPrm0->outQueChInfo[0].inChNum[1] = 2;
	pSelectPrm0->outQueChInfo[0].inChNum[2] = 1;
	pSelectPrm0->outQueChInfo[0].inChNum[3] = 3;

	pSelectPrm0->outQueParams[1].nextLink = pstruct->mergeLink[3].link_id;
	pSelectPrm0->outQueChInfo[1].numOutCh = 2;
	pSelectPrm0->outQueChInfo[1].inChNum[0] = 1;
	pSelectPrm0->outQueChInfo[1].inChNum[1] = 3;



	//sclr 缩放link
	//其中输入的4个ch,
	//ch 0 : input 1/adv7442 ,low scale
	//ch1: input 2/adv7441 ,low scale
	//ch2:input 1/adv7442,high scale  or NULL
	//ch3:input 2/adv7441,high scale or null
	pSclrPrm1 = &(pstruct->sclrLink[1].create_params);
	pSclrPrm1->inQueParams.prevLinkId             = pstruct->selectLink[2].link_id;
	pSclrPrm1->inQueParams.prevLinkQueId          = 0;
	pSclrPrm1->pathId                             = SCLR_LINK_SC5;
	pSclrPrm1->outQueParams.nextLink              = pstruct->nsfLink[1].link_id;
	pSclrPrm1->tilerEnable                        = FALSE;
	pSclrPrm1->enableLineSkipSc                   = FALSE;
	pSclrPrm1->inputFrameRate                     = 60;
	pSclrPrm1->outputFrameRate                    = 30;
	pSclrPrm1->scaleMode                          = SCLR_SCALE_MODE_ABSOLUTE;
	pSclrPrm1->numBufsPerCh					  = 4;

	if(pSclrPrm1->scaleMode == SCLR_SCALE_MODE_ABSOLUTE) {
		pSclrPrm1->outScaleFactor.absoluteResolution.outWidth = 1920;
		pSclrPrm1->outScaleFactor.absoluteResolution.outHeight = 1200;
	} else {
		pSclrPrm1->outScaleFactor.ratio.widthRatio.numerator    = 1;
		pSclrPrm1->outScaleFactor.ratio.widthRatio.denominator  = 1;
		pSclrPrm1->outScaleFactor.ratio.heightRatio.numerator   = 1;
		pSclrPrm1->outScaleFactor.ratio.heightRatio.denominator = 1;
	}

	/* 滤波成420数据 */
	pNsfPrm1 = &(pstruct->nsfLink[1].create_params);
	pNsfPrm1->bypassNsf                = FALSE;
	pNsfPrm1->tilerEnable              = FALSE;
	pNsfPrm1->inQueParams.prevLinkId   = pstruct->sclrLink[1].link_id;
	pNsfPrm1->inQueParams.prevLinkQueId = 0;
	pNsfPrm1->numOutQue                = 1;
	pNsfPrm1->outQueParams[0].nextLink = pstruct->mergeLink[3].link_id;

	/* MergeLink2合成多路流 送去编码 */
	// in: 2 que and 4 ch
	// out:1 que and 4 ch
	pMergePrm2 = &(pstruct->mergeLink[3].create_params);
	pMergePrm2->numInQue						= 2;
	pMergePrm2->inQueParams[0].prevLinkId	= pstruct->nsfLink[1].link_id;
	pMergePrm2->inQueParams[0].prevLinkQueId	= 0;
	pMergePrm2->inQueParams[1].prevLinkId	= pstruct->selectLink[2].link_id;
	pMergePrm2->inQueParams[1].prevLinkQueId	= 1;

	pMergePrm2->outQueParams.nextLink		= pstruct->selectLink[3].link_id;
	pMergePrm2->notifyNextLink				= TRUE;

	/* SelectLink筛选流 */
	//in: 1 que and 4 ch
	//out :1 que and 4 ch
	//主要用于重新排序ch 顺序，用于后续的video编码一致性
	pSelectPrm1 = &(pstruct->selectLink[3].create_params);
	pSelectPrm1->inQueParams.prevLinkId	  = pstruct->mergeLink[3].link_id;
	pSelectPrm1->inQueParams.prevLinkQueId = 0;
	pSelectPrm1->numOutQue = 1;
	pSelectPrm1->outQueParams[0].nextLink = pstruct->ipc_outvpss_Link.link_id;
	pSelectPrm1->outQueChInfo[0].numOutCh   = 4;
	//	pSelectPrm1->outQueChInfo[0].inChNum[0] = 0; //xiao 7441
	//	pSelectPrm1->outQueChInfo[0].inChNum[1] = 4;
	//	pSelectPrm1->outQueChInfo[0].inChNum[2] = 1; //da 7441
	//	pSelectPrm1->outQueChInfo[0].inChNum[3] = 5;

	pSelectPrm1->outQueChInfo[0].inChNum[0] = 0;    // input 1 /adv7442,low stream,
	pSelectPrm1->outQueChInfo[0].inChNum[1] = 4;	//input 1 /adv7442,high stream or origin stream
	pSelectPrm1->outQueChInfo[0].inChNum[2] = 1;   //input 2 /adv7441 ,low stream
	pSelectPrm1->outQueChInfo[0].inChNum[3] = 5;   //input 2/adv7441, high stream


	channel_set_enc_chid(CHANNEL_INPUT_1_LOW, 0);
	channel_set_enc_chid(CHANNEL_INPUT_1, 1);
	channel_set_enc_chid(CHANNEL_INPUT_2_LOW, 2);
	channel_set_enc_chid(CHANNEL_INPUT_2, 3);


	//in : 1 que and 4 ch .ch={low input1,high input 1,low input2,high input2}
	//out: 1 que and 4 ch .ch = {low input1,high input 1,low input2,high input2}
	pIpcOutVpssPrm = &(pstruct->ipc_outvpss_Link.create_params);
	pIpcOutVpssPrm->inQueParams.prevLinkId		= pstruct->selectLink[3].link_id;
	pIpcOutVpssPrm->inQueParams.prevLinkQueId	= 0;
	pIpcOutVpssPrm->numOutQue					= 1;
	pIpcOutVpssPrm->outQueParams[0].nextLink	= pstruct->ipc_invideo_Link.link_id;
	pIpcOutVpssPrm->notifyNextLink				= TRUE;
	pIpcOutVpssPrm->notifyPrevLink				= TRUE;
	pIpcOutVpssPrm->noNotifyMode				= FALSE;
	return 0;
}

#if HAVE_MP_MODULE
#if 1
Int32 reach_MP_select_process(ENC2000_LINK_STRUCT *pstruct)
{
	MergeLink_CreateParams			*pMergePrm2;
	SwMsLink_CreateParams			*pSwMsPrm0;
	NsfLink_CreateParams			*pNsfPrm0;
	DupLink_CreateParams            *pDupPrm0;
	IpcLink_CreateParams			*pIpcOutVpssPrm;
	SclrLink_CreateParams           *pSclrPrm0;

	int layout = 1, win1 = SIGNAL_INPUT_2, win2 = SIGNAL_INPUT_1;


	IpcFramesInLinkRTOS_CreateParams *pIpcFramesInDspPrm;
	AlgLink_CreateParams             *pOsdDspAlgPrm;


#if WRITE_YUV_TASK
	MergeLink_CreateParams			*pMergePrm3;
	DupLink_CreateParams            *pDupPrm1;
	IpcFramesOutLinkRTOS_CreateParams  *pIpcFramesOutVpssToHostPrm;
	IpcFramesInLinkHLOS_CreateParams   *pIpcFramesInHostPrm;
#endif


#if WRITE_YUV_TASK
	pDupPrm1 = &(pstruct->dupLink[1].create_params);
	pDupPrm1->inQueParams.prevLinkId	= pstruct->selectLink[1].link_id;
	pDupPrm1->inQueParams.prevLinkQueId	= 0;
	pDupPrm1->numOutQue					= 2;
	pDupPrm1->outQueParams[0].nextLink 	= pstruct->swmsLink[0].link_id;
	pDupPrm1->outQueParams[1].nextLink 	= pstruct->mergeLink[3].link_id;
	pDupPrm1->notifyNextLink			= TRUE;
#endif

	/* swmsLik0 画面合成 */
	pSwMsPrm0 = &(pstruct->swmsLink[0].create_params);
	pSwMsPrm0->numSwMsInst				= 1;
	pSwMsPrm0->swMsInstId[0]			= SYSTEM_SW_MS_SC_INST_VIP1_SC;
#if WRITE_YUV_TASK
	pSwMsPrm0->inQueParams.prevLinkId	= pstruct->dupLink[1].link_id;
	pSwMsPrm0->inQueParams.prevLinkQueId = 0;
#else
	pSwMsPrm0->inQueParams.prevLinkId	= pstruct->selectLink[1].link_id;
	pSwMsPrm0->inQueParams.prevLinkQueId = 0;
#endif
	pSwMsPrm0->outQueParams.nextLink	= pstruct->dupLink[0].link_id;
	pSwMsPrm0->maxInputQueLen			= SYSTEM_SW_MS_DEFAULT_INPUT_QUE_LEN;
	pSwMsPrm0->maxOutRes				= SYSTEM_STD_1080P_60;
	pSwMsPrm0->lineSkipMode				= FALSE;
	pSwMsPrm0->layoutPrm.outputFPS		= 60;

	layout = get_mp_layout();
	app_get_swap_layout(layout, &win1, &win2);
	app_web_set_mp_layout(layout,  win1, win2);
	/*
	swms_set_swms_layout2(0, pSwMsPrm0, 2);
	pSwMsPrm0->layoutPrm.onlyCh2WinMapChanged = FALSE;
	*/

	//add osd link
	if(pstruct->enableOsdAlgLink) {
		pSwMsPrm0->outQueParams.nextLink		=  pstruct->ipcFramesOutVpssId[0];

		//pNsfPrm->outQueParams[0].nextLink 	= pstruct->ipcFramesOutVpssId[0];
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.inQueParams.prevLinkId = pstruct->swmsLink[0].link_id;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.inQueParams.prevLinkQueId = 0;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.notifyPrevLink = TRUE;

		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.numOutQue = 1;
		(pstruct->ipcFramesOutVpssPrm[0]).baseCreateParams.outQueParams[0].nextLink = pstruct->dupLink[0].link_id;
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




	pDupPrm0 = &(pstruct->dupLink[0].create_params);
	pDupPrm0->inQueParams.prevLinkId	= pstruct->swmsLink[0].link_id;
	pDupPrm0->inQueParams.prevLinkQueId	= 0;
#if WRITE_YUV_TASK
	pDupPrm0->numOutQue					= 3;
	pDupPrm0->outQueParams[2].nextLink 	= pstruct->mergeLink[3].link_id;
#else
	pDupPrm0->numOutQue					= 2;
#endif
	pDupPrm0->outQueParams[0].nextLink 	= pstruct->mergeLink[2].link_id;
	pDupPrm0->outQueParams[1].nextLink 	= pstruct->mergeLink[2].link_id;
	pDupPrm0->notifyNextLink			= TRUE;

	if(pstruct->enableOsdAlgLink) {
		pDupPrm0->inQueParams.prevLinkId	= pstruct->ipcFramesOutVpssId[0];
		pDupPrm0->inQueParams.prevLinkQueId	= 0;
	}


#if WRITE_YUV_TASK
	/* 写YUV */
	pMergePrm3 = &(pstruct->mergeLink[3].create_params);
	pMergePrm3->numInQue						= 2;

	pMergePrm3->inQueParams[0].prevLinkId		= pstruct->dupLink[1].link_id;
	pMergePrm3->inQueParams[0].prevLinkQueId	= 1;
	pMergePrm3->inQueParams[1].prevLinkId		= pstruct->dupLink[0].link_id;
	pMergePrm3->inQueParams[1].prevLinkQueId	= 2;
	pMergePrm3->outQueParams.nextLink			= pstruct->ipcFramesOutVpssToHostId;
	pMergePrm3->notifyNextLink					= TRUE;

	pIpcFramesOutVpssToHostPrm = &(pstruct->ipcFramesOutVpssToHostPrm);
	pIpcFramesOutVpssToHostPrm->baseCreateParams.noNotifyMode = TRUE;
	pIpcFramesOutVpssToHostPrm->baseCreateParams.notifyNextLink = FALSE;
	pIpcFramesOutVpssToHostPrm->baseCreateParams.notifyPrevLink = TRUE;
	pIpcFramesOutVpssToHostPrm->baseCreateParams.inQueParams.prevLinkId = pstruct->mergeLink[3].link_id;
	pIpcFramesOutVpssToHostPrm->baseCreateParams.inQueParams.prevLinkQueId = 0;
	pIpcFramesOutVpssToHostPrm->baseCreateParams.outQueParams[0].nextLink = pstruct->ipcFramesInHostId;

	pIpcFramesInHostPrm = &(pstruct->ipcFramesInHostPrm);
	pIpcFramesInHostPrm->baseCreateParams.noNotifyMode = TRUE;
	pIpcFramesInHostPrm->baseCreateParams.notifyNextLink = FALSE;
	pIpcFramesInHostPrm->baseCreateParams.notifyPrevLink = FALSE;
	pIpcFramesInHostPrm->baseCreateParams.inQueParams.prevLinkId = pstruct->ipcFramesOutVpssToHostId;
	pIpcFramesInHostPrm->baseCreateParams.inQueParams.prevLinkQueId = 0;
	pIpcFramesInHostPrm->baseCreateParams.outQueParams[0].nextLink = SYSTEM_LINK_ID_INVALID;
	pIpcFramesInHostPrm->exportOnlyPhyAddr = TRUE;
	pIpcFramesInHostPrm->cbCtx = NULL;
	pIpcFramesInHostPrm->cbFxn = NULL;

#endif

	/* mergeLink2 高低码流放至同一队列 */
	pMergePrm2 = &(pstruct->mergeLink[2].create_params);
	pMergePrm2->numInQue						= 2;
	pMergePrm2->inQueParams[0].prevLinkId		= pstruct->dupLink[0].link_id;
	pMergePrm2->inQueParams[0].prevLinkQueId	= 0;
	pMergePrm2->inQueParams[1].prevLinkId		= pstruct->dupLink[0].link_id;
	pMergePrm2->inQueParams[1].prevLinkQueId	= 1;
	pMergePrm2->outQueParams.nextLink			= pstruct->sclrLink[0].link_id;
	pMergePrm2->notifyNextLink					= TRUE;

	pSclrPrm0 = &(pstruct->sclrLink[0].create_params);
	pSclrPrm0->inQueParams.prevLinkId             = pstruct->mergeLink[2].link_id;
	pSclrPrm0->pathId                             = SCLR_LINK_SC5;
	pSclrPrm0->inQueParams.prevLinkQueId          = 0;
	pSclrPrm0->outQueParams.nextLink              = pstruct->nsfLink[0].link_id;
	pSclrPrm0->tilerEnable                        = FALSE;
	pSclrPrm0->enableLineSkipSc                   = FALSE;
	pSclrPrm0->inputFrameRate                     = 60;
	pSclrPrm0->outputFrameRate                    = 60;
	pSclrPrm0->scaleMode                          = SCLR_SCALE_MODE_ABSOLUTE;


	if(pSclrPrm0->scaleMode == SCLR_SCALE_MODE_ABSOLUTE) {
		pSclrPrm0->outScaleFactor.absoluteResolution.outWidth = 1920;
		pSclrPrm0->outScaleFactor.absoluteResolution.outHeight = 1080;
	} else {
		pSclrPrm0->outScaleFactor.ratio.widthRatio.numerator    = 1;
		pSclrPrm0->outScaleFactor.ratio.widthRatio.denominator  = 1;
		pSclrPrm0->outScaleFactor.ratio.heightRatio.numerator   = 1;
		pSclrPrm0->outScaleFactor.ratio.heightRatio.denominator = 1;
	}

	/* YUV422-->YUV420 */
	pNsfPrm0 = &(pstruct->nsfLink[0].create_params);
	pNsfPrm0->bypassNsf                = FALSE;
	pNsfPrm0->tilerEnable              = FALSE;
	pNsfPrm0->inQueParams.prevLinkId   = pstruct->sclrLink[0].link_id;
	pNsfPrm0->inQueParams.prevLinkQueId = 0;
	pNsfPrm0->numOutQue                = 1;
	pNsfPrm0->outQueParams[0].nextLink = pstruct->ipc_outvpss_Link.link_id;

	channel_set_enc_chid(CHANNEL_INPUT_MP, 1);
	channel_set_enc_chid(CHANNEL_INPUT_MP_LOW, 0);

	pIpcOutVpssPrm = &(pstruct->ipc_outvpss_Link.create_params);
	pIpcOutVpssPrm->inQueParams.prevLinkId		= pstruct->nsfLink[0].link_id;
	pIpcOutVpssPrm->inQueParams.prevLinkQueId	= 0;
	pIpcOutVpssPrm->numOutQue					= 1;
	pIpcOutVpssPrm->outQueParams[0].nextLink	= pstruct->ipc_invideo_Link.link_id;
	pIpcOutVpssPrm->notifyNextLink				= TRUE;
	pIpcOutVpssPrm->notifyPrevLink				= TRUE;
	pIpcOutVpssPrm->noNotifyMode				= FALSE;


	return 0;
}
#endif
#endif

