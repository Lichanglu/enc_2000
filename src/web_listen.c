#if 0
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>


#include "middle_control.h"
#include "app_sdk_tcp.h"
#include "log.h"
#include "extern_update_header/app_update_header.h"
#include "common.h"
#include "log.h"
#include "log_common.h"

#define KERNELFILENAME	"uImage"


void msgPacket(int identifier, unsigned char *data, webparsetype type, int len, int cmd, int ret)
{
	webMsgInfo  msghead;
	int	cmdlen = sizeof(type);
	int retlen = sizeof(ret);
	msghead.identifier = identifier;
	msghead.type = type;
	msghead.len = len;
	memcpy(data, &msghead, MSGINFOHEAD);
	memcpy(data + MSGINFOHEAD, &cmd, cmdlen);
	memcpy(data + MSGINFOHEAD + cmdlen, &ret, retlen);
	//PRINTF("msghead.len=%d\n", msghead.len);
	return ;
}









int midParseString(int identifier, int fd, char *data, int len)
{
	int recvlen;
	int cmd = 0;
	char actdata[4096] = {0};
	int vallen = 0;
	//	int needsend = 0;

	char senddata[1024] = {0};
	int totallen = 0;

	char  out[4096] = "unknown cmd.";
	int web_ret = SERVER_RET_OK;
	int need_send = 0;

	recvlen = recv(fd, data, len, 0);

	vallen = len - sizeof(int);

	if(recvlen < 0 || vallen > sizeof(actdata)) {
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}


	//actdata = (char *)malloc(vallen);
	memcpy(&cmd, data, sizeof(int));
	memcpy(actdata, data + sizeof(int), vallen);
	PRINTF("cmd = %04x\n", cmd);

	switch(cmd) {
		case MSG_SIGNALDETAILINFO:
			PRINTF("cmd = 0x%04x is MSG_SIGNALDETAILINFO.\n", cmd);
			//			getSignaldetailInfo(identifier,fd, cmd, actdata, vallen);
			break;

		case MSG_GETINPUTSIGNALINFO:
			PRINTF("cmd = 0x%04x is MSG_GETINPUTSIGNALINFO.\n", cmd);
			//			getInputSignalInfo(identifier,fd, cmd, actdata, vallen);
			break;

		case MSG_REVISE_PICTURE:
			PRINTF("cmd = 0x%04x is MSG_REVISE_PICTURE.\n", cmd);
			//			revisePicture(identifier,fd, cmd, actdata, vallen);
			break;

		case MSG_FAR_CTRL:
			PRINTF("cmd = 0x%04x is MSG_FAR_CTRL.\n", cmd);
			//FarCtrlCamera(int dsp, unsigned char * data, int len);
			//			farControl(identifier,fd, cmd, actdata, vallen);
			break;

		case MSG_UPDATESYS:
			PRINTF("cmd = 0x%04x is MSG_UPDATESYS.\n", cmd);
			updateFile(identifier, fd, cmd, actdata, vallen);
			break;

		case MSG_SETDEVICETYPE:
			PRINTF("cmd = 0x%04x is MSG_SETDEVICETYPE.\n", cmd);
			//			SetDeviceType_1260(identifier,fd, cmd, actdata, vallen);
			break;

		case MSG_GETDEVICETYPE:
			PRINTF("cmd = 0x%04x is MSG_GETDEVICETYPE.\n", cmd);
			//			app_get_device_type(identifier,fd, cmd, actdata, vallen);
			break;

		case MSG_GETSOFTCONFIGTIME:
			PRINTF("cmd = 0x%04x is MSG_GETSOFTCONFIGTIME.\n", cmd);
			//getsoftconfigtime(fd, cmd, actdata, vallen);
			//			app_get_build_time(identifier,fd, cmd, actdata, vallen);
			break;

		case MSG_GETSOFTVERSION:
			PRINTF("cmd = 0x%04x is MSG_GETSOFTVERSION.\n", cmd);
			//			app_get_soft_version(out, sizeof(out));
			need_send = 1;
			break;

			//case	MSG_GET_BUILD_TIME:
			//	app_get_build_time(fd, cmd, actdata, vallen);
			//	break;
		case MSG_UPLOADIMG:
			PRINTF("cmd = 0x%04x is MSG_UPLOADIMG.\n", cmd);
			//		web_ret = webupLoadLogo(actdata, out);

			need_send = 1;
			break;

		case MSG_SDK_LOGIN: {
			PRINTF("MSG_LOGIN fd =%d, data=%s\n", fd, actdata);
			web_ret = app_sdk_login_check(actdata, out);

			need_send = 1;
		}
		break;

		case MSG_GET_SDP_INFO: {
			PRINTF("MSG_GET_SDP_INFO  \n");
			//				web_ret = rtsp_get_sdp_describe(out, sizeof(out));
			PRINTF("SDP:#%s#,len=%d\n", out, strlen(out));
			need_send = 1;
		}
		break;

		default:
			PRINTF("Warnning,the cmd %d is UNKOWN\n", cmd);
			need_send = 1;
			break;
	}

EXIT:

	if(need_send == 1) {
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + strlen(out);
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, cmd, web_ret);
		memcpy(senddata + (totallen - strlen(out)), out, strlen(out));
		PRINTF("the cmd =%04x,the out=%s,the ret=%04x\n", cmd, out, web_ret);
		send(fd, senddata, totallen, 0);
	}

	//	free(actdata);
	return 0;
}

#endif
