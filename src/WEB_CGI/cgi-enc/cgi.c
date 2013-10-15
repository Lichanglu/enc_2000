       /* cgi.c */
#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>		/* time() */
#include <fcntl.h>
#include <sys/stat.h>
#define LINUX
#ifdef LINUX
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#endif
//

#include "htmllib.h"
#include "cgic.h"
#include "Tools.h"
#include "cgi.h"
#include "weblib.h"
#include "../../middle_control.h"
#include "../../input_to_channel.h"
#include "webTcpCom.h"
#include "app_web_stream_output.h"

#include "app_web_stream_output.h"


#define ACTION_START 100
#define ACTION_UPLOADFILE  ACTION_START+10
#define ACTION_LOGIN  ACTION_START+20
#define ACTION_QUERYSYSPARAM  ACTION_START+30
#define ACTION_CHANGEPWD  ACTION_START+40
#define ACTION_NETWORKSET  ACTION_START+50
#define ACTION_UPDATEVIDEOPARAM  ACTION_START+60
#define ACTION_QUERYAVPARAM  ACTION_START+70
#define ACTION_UPDATEAUDIOPARAM  ACTION_START+80
#define ACTION_TEXT_SET  ACTION_START+90
#define ACTION_LOGOUT  ACTION_START+100

//added by Adams begin.
#define PAGE_LEFTMENU_SYSPARAMS_SHOW 301
#define PAGE_LEFTMENU_SYSSET_SHOW 302
#define PAGE_SYSINFO_SHOW 303
#define PAGE_INPUT_SHOW 304
#define PAGE_VIDEO_SHOW 305
#define PAGE_AUDIO_SHOW 306
#define PAGE_OUTPUT_SHOW 307
#define PAGE_CAPTIONLOGO_SHOW 308
#define PAGE_REMOTECTRL_SHOW 309
#define PAGE_NETWORK_SHOW 310
#define PAGE_MODIFYPASSWORD_SHOW 311
#define PAGE_OTHERSET_SHOW 312
#define PAGE_SYSUPGRADE_SHOW 313
#define PAGE_RESETDEFAULT_SHOW 314
#define PAGE_INPUTDETAILS_SHOW 315
#define PAGE_OUTPUTADVANCEDSET_SHOW 316
#define PAGE_OUTPUTRTSPSET_SHOW 317
#define PAGE_VIDEOADVANCEDSET_SHOW 318
#define PAGE_ADDOUTPUT_SHOW 319
#define PAGE_MERGE_SHOW 320
#define PAGE_RTSP_PLAYURL_SHOW 321

#define ACTION_VIDEO_SET 401
#define ACTION_OUTPUT_SET 402
#define ACTION_OUTPUTADD_SET 403
#define ACTION_OUTPUTUPDATE_GET 404
#define ACTION_OUTPUTUPDATE_SET 405
#define ACTION_NETWORK_SET 406
#define ACTION_INPUT_SET 407
#define ACTION_INPUTDETAILS_GET 408
#define ACTION_ADJUSTSCREEN_SET 409
#define ACTION_AUDIO_SET 410
#define ACTION_CAPTIONLOGO_SET 411
#define PAGE_REMOTECTRL_SET 412
#define ACTION_PTZ_CTRL 413
#define ACTION_VIDEOADVANCEDSET_GET 414
#define ACTION_VIDEOADVANCEDSET_SET 415
#define ACTION_OUTPUTRTSPSET_GET 416
#define ACTION_OUTPUTRTSPSET_SET 417
#define ACTION_SETDEFAULT_SET 418
#define ACTION_UPLOADLOGOPIC_SET 419
#define ACTION_DOWNLOAD_SDP 420
#define ACTION_MERGE_SET 421
#define ACTION_ENCADVANCED_SET 422
#define ACTION_SERIALNO_SET 423
#define ACTION_CBCR_SET 424
#define ACTION_INPUTTYPE_GET 425





#define RESULT_SUCCESS 1
#define RESULT_FAILURE 2
#define RESULT_MUSTMULTIIP 3

//added by Adams end.


#define PAGE_LOGIN_SHOW 201
#define PAGE_TIMEOUT_SHOW 202
#define PAGE_TOP_SHOW 203
#define PAGE_DOWN_SHOW 204
#define PAGE_MAIN_SHOW 205
#define PAGE_SYSTEM_INFO 206
#define PAGE_SYSTEM_SHOW 207
#define PAGE_PARAMETER_SHOW 208
#define SAVE_PLAY_CFG 209
#define TVOD_FILE 210
#define TVOD_OPER 211
#define SAVE_SHOW_MODE 212
#define SAVE_OUTPUT 213

#define RESTART_SHOW 214
#define RESTART_SYS 215
#define SERVER_SHOW 216
#define SERVER_LIST 217
#define SERVER_ADD_SHOW 218
#define SERVER_ADD 219
#define SERVER_MOD_SHOW 220
#define SERVER_MOD 221
#define SERVER_DEL 222
#define SERVER_LIST_SHOW 223
#define ACTION_SYNC 224
#define ACTION_SAVEUPDATE 225
#define UPDATE_MULTI_SET 31
#define SCREEN_CHANGE 152
#define SCREEN_GREEN 151
#define YTKZ_PROTO 226
#define YTKZ 227
#define UPDATE_LANGUAGE 24
#define UPDATE_HIDDEN_VALUE 300
//#define SETPHY	228
//#define GETPHY	229

#define CGI_NAME "encoder.cgi"
#define BUFFER_LEN					1024
#define MAXFILE_NUM					1024
#define FILENAME_MAXLEN				200

#if (defined(DSS_ENC_1200) || defined(DSS_ENC_1260))

#define UPDATEFILEHEAD "7E7E7E7E48454E43"
#else
#define UPDATEFILEHEAD "7E7E7E7E31313030"
#endif
#define BufferLen 500
#define UPFILEHEADLEN 8

#define WEBVERSION "1.1.3"


#define INPUT1_HIGH_STRING "input 1 / high"
#define INPUT1_LOW_STRING  "input 1 / low"
#define INPUT2_HIGH_STRING "input 2 / high"
#define INPUT2_LOW_STRING  "input 2 / low"			


char sys_password[100];
char sys_webpassword[100];
char sys_timeout[100];
char sys_language[100];
char script_language[100];


char* trim(char * str)
{
	int len=0;
	int i=0;
	len=strlen(str);
	i=len-1;
	for(;i>0;i--)
	{
		if(str[i]==' '||str[i]==0x0A)
			continue;
		else
			break;
	}
	str[i+1]='\0';
	return str;
}

static void gettexttodisplay(TextInfo* info)
{
	strcpy(info->msgtext,"");
	info->xpos=0;
	info->ypos=0;
	info->showtime=1;

}
//0成功
//-1失败
static int loginaction()
{
	char username[100] = {0};
    char password[100] = {0};
	//char strTime[100] = {0};
	cgiFormString("username", username, sizeof(username));
	cgiFormString("password", password, sizeof(password));

	if((!strcmp(username,"admin")&&!strcmp(password,sys_password))||(!strcmp(username,"operator")&&!strcmp(password,sys_webpassword))||(!strcmp(username,"ReachWebAdmin")&&!strcmp(password,"ReachWebAdmin2006")))
	{
		long now;
		now = (long)time(NULL);
		showPage("./index.html",sys_language);
		fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
		fprintf(cgiOut,"<script type='text/javascript'>");
		fprintf(cgiOut, "setLanguageType('%s');", sys_language);
		fprintf(cgiOut,"</script>");
		cgiSetCookieUser(username);
		cgiSetCookienoTime("sessiontime",now);
		return 0;
	}
	else
	{
		char tmp[150]={0};
		getLanguageValue(sys_language,"UserOrPasswordError",tmp);
		forwardPage(CGI_NAME,PAGE_LOGIN_SHOW,tmp);
		return -1;
	}
}
//升级系统
static int updatesystems(void)
{
	FILE *targetFile;
	cgiFilePtr file;
	char name[128];
	//int retCode = 0;
	int got = 0;
	int t;
	char *tmpStr=NULL;
	char fileNameOnServer[64];
	char buffer[1024];
	int rec = 0;
	int flag = 0;


	char languagebuf[150]={0};
	getLanguageValue(sys_language,"uploadFileFail",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) !=cgiFormSuccess)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}

	t=-1;
	//从路径名解析出用户文件名
	while(1){
		tmpStr=strstr(name+t+1,"\\");
		if(NULL==tmpStr)
			tmpStr=strstr(name+t+1,"/");//if "\\" is not path separator, try "/"
		if(NULL!=tmpStr)
			t=(int)(tmpStr-name);
		else
			break;
	}

	if(strstr(name,".bin")==NULL||(name-strstr(name,".bin"))!=(4-strlen(name)))
	{
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"binfile",languagebuf);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}
	//根据表单中的值打开文件
	if ((rec=cgiFormFileOpen("file", &file)) != cgiFormSuccess) {
		//memset(languagebuf,0,150);
		//sprintf(languagebuf,"%d",rec);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}
#ifdef LINUX
	strcpy(fileNameOnServer,"/update.tgz");
#else
	strcpy(fileNameOnServer,"D:\\update\\update.bin");
#endif
	targetFile=fopen(fileNameOnServer,"wb+");
	if(targetFile==NULL){
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
		return -1;
	}
       	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess){		
		if(got>0){			 
			if(flag == 0){				
				char tmpsync[20]={0};
				sprintf(tmpsync,"%02X%02X%02X%02X%02X%02X%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
				if(strcmp(UPDATEFILEHEAD,tmpsync))
				{
					memset(languagebuf,0,150);
					getLanguageValue(sys_language,"uploadFileFail",languagebuf);
					//forwardPage(CGI_NAME,207,languagebuf);
					fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
					return -1;
				}else{
					fwrite(buffer+8,1,got-8,targetFile);
				}
				flag = 1;
			}else
				fwrite(buffer,1,got,targetFile);
		}
	}
	cgiFormFileClose(file);
	fclose(targetFile);
	sync();
	memset(languagebuf,0,150);
	getLanguageValue(sys_language,"uploadFileSuccess",languagebuf);
	rec = WebUpdateFile(fileNameOnServer);
        //forwardPage(CGI_NAME,207,languagebuf);

	if(0 != rec) {
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadFileFail",languagebuf);
	}	
        fprintf(cgiOut,"<script type='text/javascript'>alert('%s');parent.closeUploading();</script>",languagebuf);
	return 0;
}
#if 0
//上传LOGO图片
static int updateLogoPic(void)
{
	FILE *targetFile;
	cgiFilePtr file;
	char name[128];
	//int retCode = 0;
	int got = 0;
	int t;
	char *tmpStr=NULL;
	char fileNameOnServer[64];
	char buffer[1024];
	int rec = 0;
	int flag = 0;


	char languagebuf[150]={0};
	getLanguageValue(sys_language,"uploadFileFail",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) !=cgiFormSuccess)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}

	t=-1;
	//从路径名解析出用户文件名
	while(1){
		tmpStr=strstr(name+t+1,"\\");
		if(NULL==tmpStr)
			tmpStr=strstr(name+t+1,"/");//if "\\" is not path separator, try "/"
		if(NULL!=tmpStr)
			t=(int)(tmpStr-name);
		else
			break;
	}

	if(strstr(name,".png")==NULL||(name-strstr(name,".png"))!=(4-strlen(name)))
	{
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"pngfile",languagebuf);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
	//根据表单中的值打开文件
	if ((rec=cgiFormFileOpen("file", &file)) != cgiFormSuccess) {
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
#ifdef LINUX
	strcpy(fileNameOnServer,"/opt/reach/logo.png");
#else
	strcpy(fileNameOnServer,"D:\\update\\logoimg\\logo.png");
#endif
	targetFile=fopen(fileNameOnServer,"wb+");
	if(targetFile==NULL){
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}

	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess){
		if(got>0){
			if(flag == 0){
			//	char tmpsync[20]={0};
				/*sprintf(tmpsync,"%02X%02X%02X%02X%02X%02X%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
				if(strcmp(UPDATEFILEHEAD,tmpsync))
				{
					memset(languagebuf,0,150);
					getLanguageValue(sys_language,"uploadFileFail",languagebuf);
					//forwardPage(CGI_NAME,207,languagebuf);
					fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
					return -1;
				}else{*/
					fwrite(buffer+8,1,got-8,targetFile);
				//}
				flag = 1;
			}else
				fwrite(buffer,1,got,targetFile);
		}
	}
	cgiFormFileClose(file);
	fclose(targetFile);
	memset(languagebuf,0,150);
	getLanguageValue(sys_language,"uploadFileSuccess",languagebuf);
	fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
	return 0;
}

#endif
static int uploadLogo(int channel)
{
	FILE *targetFile;
	cgiFilePtr file;
	char name[256] ={0};
	//int retCode = 0;
	int got = 0;
	int t;
	char *tmpStr=NULL;
	char fileNameOnServer[64];
	char buffer[1024];
	int ret = 0;
//	int flag = 0;

	char tempname[512] = {0}; 

	snprintf(tempname,sizeof(tempname),"/opt/dvr_rdk/ti816x_2.8/logo_temp.png");
	char languagebuf[150]={0};
	getLanguageValue(sys_language,"uploadFileFail",languagebuf);
	if (cgiFormFileName("file", name, sizeof(name)) !=cgiFormSuccess)
	{
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}

	t=-1;
	while(1){
		tmpStr=strstr(name+t+1,"\\");
		if(NULL==tmpStr)
			tmpStr=strstr(name+t+1,"/");//if "\\" is not path separator, try "/"
		if(NULL!=tmpStr)
			t=(int)(tmpStr-name);
		else
			break;
	}

	if((strstr(name,".png")==NULL && (strstr(name,".PNG")==NULL))
	||((name-strstr(name,".png"))!=(4-strlen(name))&&(name-strstr(name,".PNG"))!=(4-strlen(name))))
	{
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"pngfile",languagebuf);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
	if ((ret=cgiFormFileOpen("file", &file)) != cgiFormSuccess) {
		//memset(languagebuf,0,150);
		//sprintf(languagebuf,"%d",rec);
		//forwardPage(CGI_NAME,207,languagebuf);
		fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
		return -1;
	}
#ifdef LINUX
	strcpy(fileNameOnServer,tempname);
#else
	strcpy(fileNameOnServer,"D:\\update\\update.bin");
#endif
	targetFile=fopen(fileNameOnServer,"wb+");

//check file size
	if(targetFile==NULL){
		forwardPage(CGI_NAME,207,languagebuf);
		return -1;
	}

	while (cgiFormFileRead(file, buffer, BufferLen, &got) ==cgiFormSuccess){
		if(got>0){
			#if 0
			if(flag == 0){
				char tmpsync[20]={0};
				sprintf(tmpsync,"%02X%02X%02X%02X%02X%02X%02X%02X",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5],buffer[6],buffer[7]);
				if(strcmp(UPDATEFILEHEAD,tmpsync))
				{
					memset(languagebuf,0,150);
					getLanguageValue(sys_language,"uploadFileFail",languagebuf);
					forwardPage(CGI_NAME,207,languagebuf);
					return -1;
				}else{
					fwrite(buffer+8,1,got-8,targetFile);
				}
				flag = 1;
			}else
			#endif
				fwrite(buffer,1,got,targetFile);
		}
	}
	cgiFormFileClose(file);
	fclose(targetFile);
	ret = WebUploadLogoFile(tempname,channel);
	if(ret == -4) {
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadLogoTooLarge",languagebuf);
	} else if(ret == -3) {
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"pngfile",languagebuf);
	}else if(ret == -5){
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadFileSizeFailed",languagebuf);
	}else {
		memset(languagebuf,0,150);
		getLanguageValue(sys_language,"uploadFileSuccess",languagebuf);
	}
	fprintf(cgiOut,"<script type='text/javascript'>alert('%s');</script>",languagebuf);
	return 0;
}

static int mid_ip_is_multicast(char *ip) {
	struct in_addr	 addr ;
	unsigned int 	dwAddr;
	unsigned int 	value;

	inet_aton(ip, &addr);
	memcpy(&value, &addr, 4);
	dwAddr = htonl(value);

	//PRINTF("ip=%s.dwAddr=0x%08x\n", ip, dwAddr);

	if(((dwAddr & 0xFF000000) >> 24) > 223 && ((dwAddr & 0xFF000000) >> 24) < 240) {
		return 1;
	}

	return 0;
}

static int mid_ip_is_vailed(char *ip) {
	struct in_addr	 addr ;
	unsigned int 	dwAddr;
	unsigned int 	value;

	inet_aton(ip, &addr);
	memcpy(&value, &addr, 4);
	dwAddr = htonl(value);

	//PRINTF("ip=%s.dwAddr=0x%08x\n", ip, dwAddr);


	if((((dwAddr & 0xFF000000) >> 24) >= 240) || ((((dwAddr & 0xFF000000) >> 24 == 127)))) {
		return 0;
	}

	if(((dwAddr & 0xFF000000) >> 24) < 1) {
		return 0;
	}
	

	return 1;
}


static int changePassword()
{
	char password[21]={0};
	char passwordagain[21]={0};
	char oldpassword[21]={0};
	char tmpbuf[100]={0};
	int rec=0;
	char username[20]={0};
	cgiFormString("password1",password,21);
	cgiFormString("password2",passwordagain,21);
	cgiFormString("oldpassword",oldpassword,21);
	cgiFormString("username",username,20);

	if(strcmp(username, USERNAME) != 0 && strcmp(username, WEBUSERNAME) != 0) {
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"usernameNotExist",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return -1;
	}

	if(strcmp(username, USERNAME) == 0) {
		if(strcmp(oldpassword,sys_password))//旧密码不正确
		{
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"oldpassworderror",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(strcmp(password,passwordagain)){//密码不同
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordTwiceInputNotSame",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(password==NULL||strlen(password)==0||!strcmp(password,"")){//密码为空
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordIsNull",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		rec=updateConfigFile("sysinfo.txt","password",password);
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"modifyPasswordSuccess",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return 0;

	} else {
		if(strcmp(oldpassword,sys_webpassword))//旧密码不正确
		{
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"oldpassworderror",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(strcmp(password,passwordagain)){//密码不同
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordTwiceInputNotSame",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		if(password==NULL||strlen(password)==0||!strcmp(password,"")){//密码为空
			memset(tmpbuf,0,100);
			getLanguageValue(sys_language,"passwordIsNull",tmpbuf);
			fprintf(cgiOut, tmpbuf);
			return -1;
		}
		rec=updateConfigFile("sysinfo.txt","webpassword",password);
		memset(tmpbuf,0,100);
		getLanguageValue(sys_language,"modifyPasswordSuccess",tmpbuf);
		fprintf(cgiOut, tmpbuf);
		return 0;
	}

}

char* Getline(char* pBuf, char* pLine)
{
	char* ptmp = strchr(pBuf, '\n');
	if(ptmp == NULL)
		return pBuf;
	strncpy(pLine, pBuf, ptmp - pBuf);
	return ptmp + 1;
}
int webinput_to_channel(int webinput)
{
	int channel = CHANNEL_INPUT_1;
	if(webinput == WEB_INPUT_1)
	{
		channel = CHANNEL_INPUT_1;
	}
	else if(webinput == WEB_INPUT_2)
	{
		channel = CHANNEL_INPUT_2;
	}else {
		channel = CHANNEL_INPUT_MP;
	}
	return channel;
}

static int CheckIPNetmask(int ipaddr, int netmask, int gw)
{
	int mask, ip, gateip;
	mask = netmask;
	mask = htonl(mask);
	ip = ipaddr;
	ip = htonl(ip);
	gateip = gw;
	gateip = htonl(gateip);

	if((((ip & 0xFF000000) >> 24) > 223) || ((((ip & 0xFF000000) >> 24 == 127)))) {
		return 0;
	}

	if(((ip & 0xFF000000) >> 24) < 1) {
		return 0;
	}

	if((((gateip & 0xFF000000) >> 24) > 223) || (((gateip & 0xFF000000) >> 24 == 127))) {
		return 0;
	}

	if(((gateip & 0xFF000000) >> 24) < 1) {
		return 0;
	}

	if((ip & mask) == 0) {
		return 0;
	}

	if((ip & (~mask)) == 0) {
		return 0;
	}

	if((~(ip | mask)) == 0) {
		return 0;
	}

	while(mask != 0) {
		if(mask > 0) {
			return 0;
		}

		mask <<= 1;
	}

	return 1;
}

int cgiMain(void) {
	int actionCode  = 0;
	int ret = 0;
	int lang = 0;
	
	int max_x = 0;
	int max_y = 0;
	
	cgiFormInteger("actioncode", &actionCode, 0);
	memset(sys_password,0,100);
	memset(sys_webpassword,0,100);
	memset(sys_timeout,0,100);
	memset(sys_language,0,100);

	
	ret = getLanguageValue("sysinfo.txt","password",sys_password);
	if(ret == -1 || strlen(sys_password) == 0) {
		strcpy(sys_password,USERNAME);
	}
	getLanguageValue("sysinfo.txt","webpassword",sys_webpassword);
	if(ret == -1 || strlen(sys_webpassword) == 0) {
		strcpy(sys_webpassword,WEBUSERNAME);
	}
	getLanguageValue("sysinfo.txt","timeout",sys_timeout);
	getLanguageValue("sysinfo.txt","language",sys_language);
	if(ret == -1 || strlen(sys_language) == 0) {
		strcpy(sys_language,"us");
	}

	if(!strcmp(sys_language,"cn"))
	{
		lang = 0;
		strcpy(script_language,"zh_CN");
	}else{
		lang = 1;
		strcpy(script_language,"en");
	}
	
	if(actionCode == UPDATE_LANGUAGE)
	{
		char local[10]={0};
		cgiFormString("local",local,10);
		if(strcmp(local,"us"))
			//fprintf(cgiOut,"Content-Type:text/html;charset=gbk\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
		else
			//fprintf(cgiOut,"Content-Type:text/html;charset=utf-8\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
	}else if(actionCode == ACTION_DOWNLOAD_SDP) {
		fprintf(cgiOut,"Content-Type:text/plain;charset=gb2312\n");
	}else{
		if(strcmp(sys_language,"us"))
			//fprintf(cgiOut,"Content-Type:text/html;charset=gbk\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
		else
			//fprintf(cgiOut,"Content-Type:text/html;charset=utf-8\n\n");
			fprintf(cgiOut,"Content-Type:text/html;charset=gb2312\n\n");
	}
	switch (actionCode)
	{

		case YTKZ_PROTO:
		{
			int index = 0;
		//	int ptzProtocal = 0;
		//	int outvalue = 0;
			int webinput =WEB_INPUT_1;
			int channel=0;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}else {
				channel = CHANNEL_INPUT_MP;
			}

			
			cgiFormInteger("use",&index,0);
			WebSetCtrlProto(index,&index,channel);
			forwardPages(CGI_NAME,208);
			break;
		}
		case UPDATE_LANGUAGE:
			{
				char local[10]={0};
				cgiFormString("local",local,10);
				updateConfigFile("sysinfo.txt","language",local);
				fprintf(cgiOut, "window.location.href='encoder.cgi';");
				break;
			}

		case ACTION_LOGIN:
			{
			  loginaction();
			}
			break;

		case ACTION_CHANGEPWD:
		{
			changePassword();
		}
		break;
		case ACTION_UPLOADFILE:
			{
				updatesystems();
			}
		break;
		case ACTION_LOGOUT:
		{
			fprintf(cgiOut, "document.cookie = '';\n");
			fprintf(cgiOut, "parent.document.location.replace('/index.html');");
		}
		break;
		case PAGE_LOGIN_SHOW:
		{
			showPage("./login.html",sys_language);
			fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
			break;
		}
		case PAGE_TIMEOUT_SHOW:
			showPage("./timeout.html",sys_language);
			break;
		case PAGE_TOP_SHOW:
			showPage("./top.html",sys_language);
			break;
		case PAGE_DOWN_SHOW:
			showPage("./down.html",sys_language);
			break;
		case PAGE_MAIN_SHOW:
		{
			showPage("./index.html",sys_language);
			fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
		}
		break;

		case PAGE_SYSTEM_INFO:

			break;
		case PAGE_PARAMETER_SHOW:
			break;


		case RESTART_SHOW:
			{
				showPage("./restart.html",sys_language);
			}
			break;
		case RESTART_SYS:
			{
				webRebootSystem();
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			}
			break;
		case ACTION_SAVEUPDATE:
			break;
		case ACTION_SYNC:
			{
				char time[30]={0};
				char tmpbuff[100]={0};
				int i = 0;
				int rec = 0;
				DATE_TIME_INFO ctime={0};
				char *d="/";
				char *p;
				cgiFormString("clienttime",time,30);
				p=strtok(time,d);
				while(p)
				{
					if(i==0)
						ctime.year = atoi(p);
					if(i==1)
						ctime.month = atoi(p);
					if(i==2)
						ctime.mday = atoi(p);
					if(i==3)
						ctime.hours = atoi(p);
					if(i==4)
						ctime.min = atoi(p);
					if(i==5)
						ctime.sec = atoi(p);
					p = strtok(NULL,d);
					i++;
				}
				rec = Websynctime(&ctime,&ctime);
				if(rec == 0){
					getLanguageValue(sys_language,"synchronizedTimeSuccess",tmpbuff);
				}else {
					getLanguageValue(sys_language,"synchronizedTimeFail",tmpbuff);
				}
				fprintf(cgiOut, tmpbuff);
			}
			break;
		case PAGE_LEFTMENU_SYSPARAMS_SHOW: {
			showPage("./leftmenuParamsSettings.html", sys_language);
			break;
		}
		case PAGE_LEFTMENU_SYSSET_SHOW: {
			showPage("./leftmenuSystemSettings.html", sys_language);
			break;
		}

		/*
		 * sysinfo page show action
		 */
		case PAGE_SYSINFO_SHOW: {
			Enc2000_Sys sysParamsOut;
			struct in_addr inAddr; 

			memset(&sysParamsOut, 0, sizeof(Enc2000_Sys));
			int outlen = 0;
			char compiledTime[300] = {0};
			int cpuUseRate = 0;
			char deviceModelNO[64] = {0};
			char webVersion[64] = {0};
			char serialNO[64] = {0};
			char softwareVersion[64] = {0};
			int IPAddress = 0;
			int submask = 0;
			int IPAddress1 = 0;
			int submask1 = 0;
			int networkGate = 0;
			char maAdress[64] = {0};

			WebGetDevideType(deviceModelNO, sizeof(deviceModelNO));
			appCmdStructParse(MSG_GETSYSPARAM, NULL, sizeof(Enc2000_Sys), &sysParamsOut, &outlen);
			appCmdIntParse(MSG_GETCPULOAD, 0, sizeof(int), &cpuUseRate, &outlen);
			appCmdStringParse(MSG_GETSOFTCONFIGTIME, NULL, strlen(compiledTime), compiledTime, &outlen);
			appCmdStringParse(MSG_GETSOFTVERSION, NULL, strlen(softwareVersion), softwareVersion, &outlen);
			
			appCmdStringParse(MSG_GETSERIALNO, NULL, strlen(serialNO), serialNO, &outlen);

//			strcat(serialNO, sysParamsOut.eth0.strName);
			//strcat(softwareVersion, sysParamsOut.strVer);
			IPAddress = sysParamsOut.eth0.dwAddr;
			submask = sysParamsOut.eth0.dwNetMask;
			
			IPAddress1 = sysParamsOut.eth1.dwAddr;
			submask1 = sysParamsOut.eth1.dwNetMask;
			
			networkGate = sysParamsOut.eth0.dwGateWay;
//			strcat(maAdress, sysParamsOut.eth0.szMacAddr);
			strcat(webVersion, WEBVERSION);
			sprintf(maAdress,"%02X-%02X-%02X-%02X-%02X-%02X",sysParamsOut.eth0.szMacAddr[0],sysParamsOut.eth0.szMacAddr[1],sysParamsOut.eth0.szMacAddr[2],
				sysParamsOut.eth0.szMacAddr[3],sysParamsOut.eth0.szMacAddr[4],sysParamsOut.eth0.szMacAddr[5]);
			showPage("./sysinfo.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'deviceModelNO','value': '%s','type': 'text' }", deviceModelNO);
				fprintf(cgiOut, ",{'name': 'webVersion','value': '%s','type': 'text' }", webVersion);
				fprintf(cgiOut, ",{'name': 	'compiledTime','value': '%s','type': 'text' }", compiledTime);
			//	fprintf(cgiOut, ",{'name': 'cpuUseRate','value': '%d%','type': 'text' }", cpuUseRate);
				fprintf(cgiOut, ",{'name': 'serialNO','value': '%s','type': 'text' }", serialNO);
				fprintf(cgiOut, ",{'name': 'softwareVersion','value': '%s','type': 'text' }", softwareVersion);
				memcpy(&inAddr,&IPAddress,4);
				fprintf(cgiOut, ",{'name': 'IPAddress','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				memcpy(&inAddr,&submask,4);
				fprintf(cgiOut, ",{'name': 'submask','value': '%s','type': 'text' }", inet_ntoa(inAddr));

				memcpy(&inAddr,&IPAddress1,4);
				fprintf(cgiOut, ",{'name': 'IPAddress1','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				memcpy(&inAddr,&submask1,4);
				fprintf(cgiOut, ",{'name': 'submask1','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				
				memcpy(&inAddr,&networkGate,4);
				fprintf(cgiOut, ",{'name': 'networkGate','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				fprintf(cgiOut, ",{'name': 'maAdress','value': '%s','type': 'text' }]);\n", maAdress);
			fprintf(cgiOut, "</script>\n");
			break;
		}


		/*
		 * page input show action
		 */
		case PAGE_INPUT_SHOW: {
		//	int outlen = 0;
			char inputInfo[100] = {0};
			int colorSpace = 0;
			int hdcp = 9;
			int inputType = 0;
			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;

			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			
			channel = webinput_to_channel(webinput);
			//webSetChannel(channel);


			webGetColorSpace(&colorSpace,channel);
			webGetHDCPFlag(&hdcp,channel);
			webgetInputSignalInfo(inputInfo,channel);
			WebGetinputSource(&inputType,channel);
			
			showPage("./input.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'inputInfo','value': '%s','type': 'text' }", inputInfo);
				fprintf(cgiOut, ",{'name': 'colorSpace','value': '%d','type': 'select' }",colorSpace);
				fprintf(cgiOut, ",{'name': 'inputType','value': '%d','type': 'select' }",inputType);
				fprintf(cgiOut, ",{'name': 'hdcp','value': '%d','type': 'text'}]);\n", hdcp);
				fprintf(cgiOut, "fixedHDCPText();\n");
				//fprintf(cgiOut, "formBeautify();\n");
				//fprintf(cgiOut, "initInputType(%d);\n",inputType);
				
				fprintf(cgiOut, "initInputTab(%d);\n", webinput);
			fprintf(cgiOut, "</script>\n");
			break;
		}

		/*
		 * set input action
		 */
		case ACTION_INPUT_SET: {
			int outValue = 0;
//			int outlen = 0;
			int colorSpace = 0,old_colorSpace=0;
			int inputType = 0;
			int oldinput = 0;
			int netreboot =0;
		
			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;

			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}
			//	webSetChannel(webinput);
			
			cgiFormInteger("colorSpace", &colorSpace, 0);
			cgiFormInteger("inputType", &inputType, 0);
			WebGetinputSource(&oldinput,channel);
			webGetColorSpace(&old_colorSpace,channel);

			//set color space ,for 8168,not need 
			
			if(oldinput != inputType)
			{
				WebSetinputSource(inputType, &outValue,channel);				
				netreboot = 5;	
			}
			if( old_colorSpace!=colorSpace){
				webSetColorSpace(colorSpace,&old_colorSpace,channel);
			}
			if( 5 == netreboot ){
				webRebootSystem();
			}

			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_INPUTTYPE_GET:
			{
 				int oldinput = 0;
				int webinput = WEB_INPUT_1;
				int channel = CHANNEL_INPUT_1;
				cgiFormInteger("input", &webinput, WEB_INPUT_1);
			//	webSetChannel(webinput);
				if(webinput == WEB_INPUT_1)
				{
					channel = CHANNEL_INPUT_1;
				}
				else if(webinput == WEB_INPUT_2)
				{
					channel = CHANNEL_INPUT_2;
				}
				WebGetinputSource(&oldinput,channel);
				
				fprintf(cgiOut, "%d", oldinput);
				break;
			}

		/*
		 * video page show action
		 */
		case PAGE_VIDEO_SHOW: {
//			int outlen = 0;
			int resolution = 0;
			int zoomModel = 0;
			int encodeLevel = 0;
//			int sceneSet = 0;
			int fpsValue = 0;
			int interframeSpace = 0;
			int codeRate = 0;
//			int captionLogoType = 0;
//			int caption = 0;
//			int logo = 0;

//			int enable = 0;

			int webinput =WEB_INPUT_1;
			int high = 1;
			int channel = 0;
			int mp_status=1;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			cgiFormInteger("quality", &high, 1);
			//modify by zm 2012-11-12
			WebScaleInfo     sinfo;
			WebVideoEncodeInfo vinfo;
			Mp_Info mp_info;
			memset(&sinfo,0,sizeof(WebScaleInfo));
			memset(&vinfo,0,sizeof(WebVideoEncodeInfo));
//			memset(&tinfo,0,sizeof(WebEnableTextInfo));		
			WebGetMpInfo(&mp_info);
			mp_status = mp_info.mp_status;
			if(mp_status == IS_MP_STATUS)
			{
				webinput = WEB_INPUT_3;
			}
			

			if(webinput == WEB_INPUT_1)
			{
				if(high == WEB_LOW_STREAM)
					channel = CHANNEL_INPUT_1_LOW;
				else
					channel = CHANNEL_INPUT_1;
			}
			else if (webinput == WEB_INPUT_2){
				if(high == WEB_LOW_STREAM)
					channel = CHANNEL_INPUT_2_LOW;
				else
					channel = CHANNEL_INPUT_2;
			}
			else
			{
				if(high == WEB_LOW_STREAM)
					channel = CHANNEL_INPUT_MP_LOW;
				else
					channel = CHANNEL_INPUT_MP;
			}

//			webSetChannel(channel);
			WebGetScaleParam(&sinfo,channel);
			WebGetVideoEncodeParam(&vinfo,channel);

			resolution = sinfo.resolution;
			zoomModel = sinfo.scalemode;
			if( resolution > 0 )
				resolution-=1;
			if(zoomModel == 0)
				zoomModel =1;
			else 
				zoomModel = 0;
			
			encodeLevel = 66;
			fpsValue = vinfo.nFrameRate;
			interframeSpace = vinfo.IFrameinterval;
			codeRate = vinfo.sBitrate;
			
			showPage("./video.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'resolution','value': '%d','type': 'select' }", resolution);
				fprintf(cgiOut, ",{'name': 'preset','value': '%d','type': 'select' }", vinfo.preset);
				fprintf(cgiOut, ",{'name': 'quality','value': '%d','type': 'radio' }", high);
				fprintf(cgiOut, ",{'name': 'zoomModel','value': '%d','type': 'radio' }", zoomModel);
				fprintf(cgiOut, ",{'name': 'encodeLevel','value': '%d','type': 'select' }", encodeLevel);
				fprintf(cgiOut, ",{'name': 'sceneSet','value': '%d','type': 'select' }", 2);
				fprintf(cgiOut, ",{'name': 'fpsValue','value': '%d','type': 'text' }", fpsValue);
				fprintf(cgiOut, ",{'name': 'interframeSpace','value': '%d','type': 'text' }", interframeSpace);
				fprintf(cgiOut, ",{'name': 'codeRate','value': '%d','type': 'text' }", codeRate);
//				fprintf(cgiOut, ",{'name': 'caption','value': '%d','type': 'checkbox' }", caption);
//				fprintf(cgiOut, ",{'name': 'logo','value': '%d','type': 'checkbox' }", logo);
				fprintf(cgiOut, "]);\n");
				fprintf(cgiOut, "initSceneSet();\n");
				fprintf(cgiOut, "initInputTab(%d);\n", webinput);
				fprintf(cgiOut, "initMergeTab(%d);\n", mp_status);
				fprintf(cgiOut, "initUIEnable(%d);\n", high);
				//fprintf(cgiOut, "formBeautify();\n");
			fprintf(cgiOut, "</script>\n");
			break;
		}

		/*
		 * video set action
		 */
		case ACTION_VIDEO_SET: {
			int resolution = 0;
			int zoomModel = 0;
			int encodeLevel = 0;
			int sceneSet = 0;
			int fpsValue = 0;
			int interframeSpace = 0;
			int codeRate = 0;
			int enable = 0;
			int preset = -1;
		
			int scale_change = 0;

			WebScaleInfo     sinfo;
			WebVideoEncodeInfo vinfo;

			WebScaleInfo     newsinfo;
			WebVideoEncodeInfo newvinfo;
		
			memset(&sinfo,0,sizeof(WebScaleInfo));
			memset(&vinfo,0,sizeof(WebVideoEncodeInfo));
			
			memset(&newsinfo,0,sizeof(WebScaleInfo));
			memset(&newvinfo,0,sizeof(WebVideoEncodeInfo));
		
			int webinput =WEB_INPUT_1;
			int high = 1;
			int channel = 0;
			int mp_status=1;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
	//		webSetChannel(webinput);
			cgiFormInteger("quality", &high, 1);
			Mp_Info mp_info;
			WebGetMpInfo(&mp_info);	
			mp_status = mp_info.mp_status;
			if(mp_status == IS_MP_STATUS)
			{
				webinput = WEB_INPUT_3;
			}
			
			if(webinput == WEB_INPUT_1)
			{
				if(high == WEB_LOW_STREAM)
					channel = CHANNEL_INPUT_1_LOW;
				else
					channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				if(high == WEB_LOW_STREAM)
					channel = CHANNEL_INPUT_2_LOW;
				else
					channel = CHANNEL_INPUT_2;
			}else{
				if(high == WEB_LOW_STREAM)
					channel = CHANNEL_INPUT_MP_LOW;
				else
					channel = CHANNEL_INPUT_MP;
			}
			
			WebGetScaleParam(&sinfo,channel);
			WebGetVideoEncodeParam(&vinfo,channel);

			cgiFormInteger("enable", &enable, 0);
			cgiFormInteger("resolution", &resolution, 0);
//			cgiFormInteger("zoomModel", &zoomModel, 0);
			cgiFormInteger("encodeLevel", &encodeLevel, 0);
			cgiFormInteger("sceneSet", &sceneSet, 0);
			cgiFormInteger("fpsValue", &fpsValue, 0);
			cgiFormInteger("interframeSpace", &interframeSpace, 0);
			cgiFormInteger("codeRate", &codeRate, 0);
			cgiFormInteger("preset", &preset, 0);
			if(resolution>0)
				resolution+=1;

			if(sinfo.resolution != resolution)
				scale_change =1;
			
			sinfo.resolution = resolution;
			
			if(zoomModel == 0)
				zoomModel = 1;//按比例
			else 
				zoomModel = 0;//拉伸
			sinfo.scalemode= zoomModel;
			
			vinfo.nFrameRate = fpsValue;
			vinfo.IFrameinterval =  interframeSpace;
			vinfo.sBitrate = codeRate;
			vinfo.sCbr =1; //enc1260 need scbr= 1
			vinfo.preset = preset;
			ret =0;
			if(scale_change == 1)
			{
				ret = WebSetScaleParam(&sinfo,&newsinfo,channel);
			}
			WebSetVideoEncodeParam(&vinfo,&newvinfo,channel);	
			
			if( SERVER_RET_USER_INVALIED ==  ret )
				fprintf(cgiOut, "%d", -2);
			else 
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		/*
		 * audio page show action
		 */
		case PAGE_AUDIO_SHOW: {
		//	int outlen = 0;
			int SampleRateIndex = 0;
			int audio_input = 0;
			char audioLevelLeft = '0';
			char audioLevelRight = '0';
			int bitRate = 0;
			int mute = 0;
			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;
			int mp_input = 0;
			int  mp_status = 0; //
			//add by zm			
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
			Mp_Info mp_info;
			WebGetMpInfo(&mp_info);	
			mp_status=mp_info.mp_status;
			if(mp_status == IS_MP_STATUS)
			{
				webinput = WEB_INPUT_3;
			}

			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}
			else 
			{
				channel = CHANNEL_INPUT_MP;
			}

		
		//	webSetChannel(webinput);
			
			AudioEncodeParam audioParam;
			memset(&audioParam,0,sizeof(AudioEncodeParam));
			WebGetAudioEncodeParam(&audioParam, channel);

			audio_input 	= audioParam.InputMode;
			SampleRateIndex 		= audioParam.SampleRateIndex;
			//modify by zhangmin ,the app volume is from 0-30
			audioLevelLeft 	= audioParam.LVolume /3;
			audioLevelRight = audioParam.RVolume /3;
			bitRate 		= audioParam.BitRate;
			mute			= audioParam.Mute;
			mp_input 		= audioParam.mp_input;

			showPage("./audio.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [{'name': 'audioInput','value': '%d','type': 'radio' }", mp_input);
				fprintf(cgiOut, ",{'name': 'sampleRate','value': '%d','type': 'select' }", SampleRateIndex);
				fprintf(cgiOut, ",{'name': 'audioLevelLeft','value': '%d','type': 'select' }", (int)audioLevelLeft);
				fprintf(cgiOut, ",{'name': 'audioLevelRight','value': '%d','type': 'select' }", (int)audioLevelRight);
				fprintf(cgiOut, ",{'name': 'mute','value': '%d','type': 'checkbox' }", mute);
				fprintf(cgiOut, ",{'name': 'bitRate','value': '%d','type': 'select' }]);\n", bitRate);
				fprintf(cgiOut, "initInputTab(%d);\n", webinput);
				fprintf(cgiOut, "initMergeTab(%d);\n", mp_status);

			fprintf(cgiOut, "</script>\n");
			break;
		}

		/*
		 * audio page set action
		 */
		case ACTION_AUDIO_SET: {
		//	int outvalue = 0;
		//	int outlen = 0;
			int audio_input = 0;
			int SampleRateIndex = 0;
			int audioLevelLeft = 0;
			int audioLevelRight = 0;
			int bitRate = 0;
			int mute = 0;
			
			int webinput = WEB_INPUT_1;
			int mp_status = IS_IND_STATUS;
			int channel = CHANNEL_INPUT_1;

			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
			Mp_Info mp_info;
			WebGetMpInfo(&mp_info);	
			mp_status = mp_info.mp_status;
			if(mp_status == IS_MP_STATUS)
			{
				webinput = WEB_INPUT_3;
			}

			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}else{
				channel = CHANNEL_INPUT_MP;
			}
			

			cgiFormInteger("audioInput", &audio_input, 0);
			cgiFormInteger("sampleRate", &SampleRateIndex, 0);
			cgiFormInteger("audioLevelLeft", &audioLevelLeft, 0);
			cgiFormInteger("audioLevelRight", &audioLevelRight, 0);
			cgiFormInteger("bitRate", &bitRate, 0);
			cgiFormInteger("mute", &mute, 0);
			
			AudioEncodeParam audioParamIn;
			AudioEncodeParam audioParamOut;
			memset(&audioParamIn, 0, sizeof(AudioEncodeParam));
			memset(&audioParamOut, 0, sizeof(AudioEncodeParam));

			WebGetAudioEncodeParam(&audioParamIn, channel);

			audioParamIn.mp_input = audio_input;
			
			audioParamIn.SampleRateIndex = SampleRateIndex;
			audioParamIn.LVolume 	= audioLevelLeft &0xff;
			audioParamIn.RVolume 	= audioLevelRight &0xff;
			audioParamIn.BitRate 	= bitRate;

			//add by zm becase the volume is from 0-30
			audioParamIn.LVolume 	= audioParamIn.LVolume*3;
			audioParamIn.RVolume 	= audioParamIn.RVolume*3;
			audioParamIn.Mute 		= mute;
			WebSetAudioEncodeParam(&audioParamIn, &audioParamOut,channel);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		/*
		 * output stream page show action.
		 */
		case PAGE_OUTPUT_SHOW: {
			int rowIndex = 0;
			char temp[1024] = {0};
			char formData[1024*2] = {0};
			stream_output_server_config output_out;
			rtsp_server_config rtsp_config;
			memset(&rtsp_config,0,sizeof(rtsp_server_config));
			memset(&output_out,0,sizeof(stream_output_server_config));
			int isRtspUsed = 0;
			int mutilCount = 0;
			int i = 0;
			int stream_channel = 0;

			char inputstring[256] = {0};
			char playurl[256]= {0};
			
			strcat(formData,"var formData = [");
			isRtspUsed = app_rtsp_get_used();
				
			if(isRtspUsed == 1) {
				//get rtsp info
				app_rtsp_get_cinfo(&output_out);
				app_rtsp_get_ginfo(&rtsp_config);
				snprintf(inputstring,sizeof(inputstring),"', 'inputStream': '%s',","all / all");
				snprintf(playurl,sizeof(playurl),"rtsp://%s:%d/stream1/high",output_out.main_ip,rtsp_config.s_port);

				sprintf(temp,"%d", 0);
					strcat(formData, "{'rowIndex': '");
					strcat(formData, temp);
					strcat(formData, inputstring);
					strcat(formData, "'type': '");
				sprintf(temp,"%d", output_out.type);
					strcat(formData, temp);
					strcat(formData, "', 'ip': '");
					strcat(formData, output_out.main_ip);
					strcat(formData, "', 'playurl': '");
					strcat(formData, playurl);
					strcat(formData, "', 'videoPort': '");
				sprintf(temp,"%d", output_out.video_port);
					strcat(formData, temp);
					strcat(formData, "', 'status': '");
				sprintf(temp,"%d", output_out.status);
					strcat(formData, temp);
					strcat(formData, "'},");
			}

				// get mutil server info
				mutilCount = app_mult_get_total_num();
			for(i=0; i< mutilCount; i++) {
				app_mult_get_server_config(i, &output_out);
				memset(inputstring,0,sizeof(inputstring));
				memset(playurl,0,sizeof(playurl));
	
				//for 8168
				stream_channel = output_out.stream_channel;
				if(stream_channel == CHANNEL_INPUT_1)
				{
					snprintf(inputstring,sizeof(inputstring),"', 'inputStream': '%s',",INPUT1_HIGH_STRING);
				}
				else if(stream_channel == CHANNEL_INPUT_1_LOW)
				{
					snprintf(inputstring,sizeof(inputstring),"', 'inputStream': '%s',",INPUT1_LOW_STRING);
				}
				else if(stream_channel == CHANNEL_INPUT_2)
				{
					snprintf(inputstring,sizeof(inputstring),"', 'inputStream': '%s',",INPUT2_HIGH_STRING);
				}
				else if(stream_channel == CHANNEL_INPUT_2_LOW)
				{
					snprintf(inputstring,sizeof(inputstring),"', 'inputStream': '%s',",INPUT2_LOW_STRING);
				}


				if(output_out.type == TYPE_TS)
				{
					snprintf(playurl,sizeof(playurl),"udp://@%s:%d",output_out.main_ip,output_out.video_port);
				}
				else if(output_out.type == TYPE_TS_OVER_RTP)
				{
					if(mid_ip_is_multicast(output_out.main_ip) == 0)
						snprintf(playurl,sizeof(playurl),"rtp://%s:%d",output_out.main_ip,output_out.video_port);
					else
						snprintf(playurl,sizeof(playurl),"rtp://@%s:%d",output_out.main_ip,output_out.video_port);
		
				}
				
				sprintf(temp,"%d", ++rowIndex);
					strcat(formData, "{'rowIndex': '");
					strcat(formData, temp);
					strcat(formData, inputstring);
					strcat(formData, "'type': '");
				sprintf(temp,"%d", output_out.type);
					strcat(formData, temp);
					strcat(formData, "', 'ip': '");
					strcat(formData, output_out.main_ip);
					
					strcat(formData, "', 'playurl': '");
					strcat(formData, playurl);
					
					strcat(formData, "', 'videoPort': '");
					sprintf(temp,"%d", output_out.video_port);
					strcat(formData, temp);
				
					strcat(formData, "', 'audioPort': '");
					sprintf(temp,"%d", output_out.audio_port);
					strcat(formData, temp);
					
					strcat(formData, "', 'status': '");
				sprintf(temp,"%d", output_out.status);
					strcat(formData, temp);
				if(i==(mutilCount-1)) {
					strcat(formData, "'}");
				} else {
					strcat(formData, "'},");
				}
			}
			strcat(formData, "];\n");
			showPage("./output.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>");
				fprintf(cgiOut, formData);
				fprintf(cgiOut, "initTableData(formData);\n");
				//fprintf(cgiOut, "formBeautify();\n");
				fprintf(cgiOut, "initDetailSetHandler();\n");
			fprintf(cgiOut, "</script>");

			break;
		}

		/*
		 * output stream set action
		 */
		case ACTION_OUTPUT_SET: {
			char temp[1024] = {0};
			char itemName[1024] = {0};
			int rowCount = 0;
			int type = 0;
			char ip[30] = {0};
			int videoPort = 0;
			int status = 0;
			int i = 0;
			stream_output_server_config in;
			stream_output_server_config out;
			memset(&in,0,sizeof(stream_output_server_config));
			memset(&out,0,sizeof(stream_output_server_config));
			cgiFormInteger("rowCount", &rowCount, 0);
			for(i=1; i<=rowCount; i++) {
				sprintf(temp,"%d", i);

				memset(itemName,0,sizeof(itemName));
				strcpy(itemName,"type");
				strcat(itemName, temp);
				cgiFormInteger(itemName, &type, 0);
				in.type = type;

				memset(itemName,0,sizeof(itemName));
				strcpy(itemName,"ip");
				strcat(itemName, temp);
				cgiFormString(itemName, ip, 16);
				if(strlen(ip) == 0)
					strcpy(in.main_ip,"1.2.3.4");
				else
					strcpy(in.main_ip ,ip);
				
				memset(itemName,0,sizeof(itemName));
				strcpy(itemName,"videoPort");
				strcat(itemName, temp);
				cgiFormInteger(itemName, &videoPort, 0);
				in.video_port = videoPort;

				memset(itemName,0,sizeof(itemName));
				strcpy(itemName,"status");
				strcat(itemName, temp);
				cgiFormInteger(itemName, &status, 0);
				in.status = status;

				if(type == TYPE_RTSP) {
					// RTSP
					if(status == S_DEL) {
						app_rtsp_delete();
					} else {
						app_rtsp_set_status(&in, &out);
					}
				} else {
					// mutil
					if(status == S_DEL) {
						app_mult_del_server(&in);
					} else {
						app_mult_set_server_status(&in, &out);
					}
				}
			}

			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		/*
		 * output add set action.
		 */
		case ACTION_OUTPUTADD_SET: {
			int type = 0;
			char ip[20] = {0};
			int videoPort = 0;
			int audioPort = 0;
			int mtu = 0;
			int ttl = 0;
			int tos = 0;
			int tc_flag = 0;
			int tc_rate = 0;
			//add by tanqh
			int input = 0;
			int quality = 0;

			int stream_channel = 0;
			
			stream_output_server_config in;
			stream_output_server_config out;
			
			memset(&in,0,sizeof(stream_output_server_config));
			memset(&out,0,sizeof(stream_output_server_config));

			cgiFormInteger("displayFrame", &input, WEB_INPUT_1);
			cgiFormInteger("quality", &quality, WEB_HIGH_STREAM);
			
			cgiFormInteger("type", &type, 0);
			cgiFormString("ip", ip, 20);
			cgiFormInteger("videoPort", &videoPort, 0);
			cgiFormInteger("audioPort", &audioPort, 0);
			cgiFormInteger("mtu", &mtu, 0);
			cgiFormInteger("ttl", &ttl, 0);
			cgiFormInteger("tos", &tos, 0);
			cgiFormInteger("tc_flag", &tc_flag, 0);
			cgiFormInteger("tc_rate", &tc_rate, 0);

			if(tc_flag == 0) {
				tc_rate = 0;
			}
			
			if((type == TYPE_TS && mid_ip_is_multicast(ip) == 0)

				|| ( (type == TYPE_RTP || type == TYPE_TS_OVER_RTP) && mid_ip_is_vailed(ip) == 0)) {
				fprintf(cgiOut, "%d", RESULT_MUSTMULTIIP);
				return 0;
			}

			in.type = type;
			//must set status is active
			in.status = S_ACTIVE;
			strcpy(in.main_ip,ip);
			in.video_port = videoPort;
			in.audio_port = audioPort;
			in.mtu = mtu;
			in.ttl = ttl;
			in.tos = tos;
			in.tc_flag = tc_flag;
			in.tc_rate = tc_rate;

			//input 1 ,2  quality 1 high 2 low
			if(input == WEB_INPUT_1 && quality == WEB_HIGH_STREAM)
			{
				stream_channel = CHANNEL_INPUT_1;
			}
			else if(input == WEB_INPUT_1 && quality == WEB_LOW_STREAM)
			{
				stream_channel = CHANNEL_INPUT_1_LOW;
			}
			else if(input == WEB_INPUT_2 && quality == WEB_HIGH_STREAM)
			{
				stream_channel = CHANNEL_INPUT_2;
			}
			else if(input == WEB_INPUT_2 && quality == WEB_LOW_STREAM)
			{
				stream_channel = CHANNEL_INPUT_2_LOW;
			}

			in.stream_channel = stream_channel;
			
			if(in.type == TYPE_RTSP)
				app_rtsp_add_server(&in, &out);
			else
				app_mult_add_server(&in, &out);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		/*
		 * output update show action
		 */
		case ACTION_OUTPUTUPDATE_GET: {
			int rowIndex = 0;
			int type = 0;
			char ip[20] = {0};
			int videoPort = 0;
			int status = 0;
			int avgBandwidth = 0;
			int ceilingBandwidth = 0;
			stream_output_server_config out;


			
			char temp[1024] = {0};
			char formData[1024*2] = {0};
			cgiFormInteger("rowIndex", &rowIndex, 0);
			cgiFormInteger("type", &type, 0);
			cgiFormString("ip", ip, 20);
			cgiFormInteger("videoPort", &videoPort, 0);
			cgiFormInteger("status", &status, 0);
			out.type = type;
			strcpy(out.main_ip,ip);
			out.video_port = videoPort;
			out.status = status;
			
			if(rowIndex == 0) {
				app_rtsp_get_cinfo(&out);
			} else {
				app_mult_get_server_config(rowIndex-1, &out);
			}


			avgBandwidth = app_mult_get_rate(type,out.stream_channel);
			ceilingBandwidth = avgBandwidth*(100+out.tc_rate)/100;

			int display = 0;
			int high = 0;
			if(out.stream_channel == CHANNEL_INPUT_1)
			{
				display =WEB_INPUT_1;
				high =WEB_HIGH_STREAM;
			}
			else if(out.stream_channel == CHANNEL_INPUT_1_LOW)
			{
				display =WEB_INPUT_1;
				high =WEB_LOW_STREAM;
				
			}			
			else if(out.stream_channel == CHANNEL_INPUT_2)
			{
				display =WEB_INPUT_2;
				high =WEB_HIGH_STREAM;
			}
			else if(out.stream_channel == CHANNEL_INPUT_2_LOW)
			{
				display =WEB_INPUT_2;
				high =WEB_LOW_STREAM;
			}			

				strcat(formData, "setFormItemValue('wmform_outputAdvancedSet', [{'type':'select', 'name':'type', 'value':'");
				sprintf(temp,"%d", out.type);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'ip', 'value':'");
				strcat(formData, out.main_ip);


				sprintf(temp,"%d", display);
				strcat(formData, "'}, {'type':'radio', 'name':'displayFrame', 'value':'");
				strcat(formData, temp);
				sprintf(temp,"%d", high);
				strcat(formData, "'}, {'type':'radio', 'name':'quality', 'value':'");
				strcat(formData, temp);

				strcat(formData, "'}, {'type':'text', 'name':'videoPort', 'value':'");
				sprintf(temp,"%d", out.video_port);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'audioPort', 'value':'");
				sprintf(temp,"%d", out.audio_port);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'mtu', 'value':'");
			sprintf(temp,"%d", out.mtu);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'ttl', 'value':'");
			sprintf(temp,"%d", out.ttl);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'tos', 'value':'");
			sprintf(temp,"%d", out.tos);
			
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'checkbox', 'name':'tc_flag', 'value':'");
			sprintf(temp,"%d", out.tc_flag);
			webSetChannel(out.tc_flag);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'tc_rate', 'value':'");
			sprintf(temp,"%d", out.tc_rate);
			webSetChannel(out.tc_rate);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'avgBandwidth', 'value':'");
			sprintf(temp,"%d", avgBandwidth);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'text', 'name':'ceilingBandwidth', 'value':'");
			sprintf(temp,"%d", ceilingBandwidth);
				strcat(formData, temp);
				strcat(formData, "'}, {'type':'hidden', 'name':'rowIndex', 'value': '");
			sprintf(temp,"%d", rowIndex);
				strcat(formData, temp);
				strcat(formData, "'}]);");


			fprintf(cgiOut, formData);
			break;
		}

		/*
		 * output update set action
		 */
		case ACTION_OUTPUTUPDATE_SET: {
		//	int rowIndex = 0;
			int type = 0;
			char ip[20] = {0};
			int videoPort = 0;
			int audioPort = 0;
			int mtu = 0;
			int ttl = 0;
			int tos = 0;
			int tc_flag = 0;
			int tc_rate = 0;
		//	cgiFormInteger("rowIndex", &rowIndex, 0);
			stream_output_server_config in;
			stream_output_server_config out;
			memset(&in,0,sizeof(stream_output_server_config));
			memset(&out,0,sizeof(stream_output_server_config));
			cgiFormInteger("type", &type, 0);
			cgiFormString("ip", ip, 20);
			cgiFormInteger("videoPort", &videoPort, 9000);
			cgiFormInteger("audioPort", &audioPort, 9000);
			cgiFormInteger("mtu", &mtu, 1500);
			cgiFormInteger("ttl", &ttl, 0);
			cgiFormInteger("tos", &tos, 0);
			cgiFormInteger("tc_flag", &tc_flag, 0);
			cgiFormInteger("tc_rate", &tc_rate, 0);

			if(tc_flag == 0) {
				tc_rate = 0;
			}

			in.type = type;
			strcpy(in.main_ip,ip);
			in.video_port = videoPort;
			in.audio_port = audioPort;
			in.mtu = mtu;
			in.ttl = ttl;
			in.tos = tos;
			in.tc_flag = tc_flag;
			in.tc_rate = tc_rate;
			if(in.type == TYPE_RTSP) {
				app_rtsp_set_cinfo(&in, &out);
			} else {
				app_mult_set_server_config(&in, &out);
			}

			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}
		case ACTION_OUTPUTRTSPSET_GET: {
			int serverPort = 0;
			int rtspActive = 0;
			int rtspMtu = 0;

			int webinput = WEB_INPUT_1;
						
			int streamMode = 0;
			char rtspMultiIP[16] = "234.255.60.92";
			int rtspMultiPort = 7000;

			int streamMode2 = 1;
			char rtspMultiIP2[16] = "234.255.60.90";
			int rtspMultiPort2 = 7500;			


			cgiFormInteger("input", &webinput, WEB_INPUT_1);
	//		webSetChannel(webinput);
			
			rtsp_server_config out;
			memset(&out, 0, sizeof(rtsp_server_config));
			app_rtsp_get_ginfo(&out);
			serverPort = out.s_port;
			rtspActive = out.active;
			rtspMtu = out.s_mtu;

			if(webinput == WEB_INPUT_1)
			{
				streamMode = out.mult_flag[0];
				strcpy(rtspMultiIP, out.mult_ip[0]);
				rtspMultiPort = out.mult_port[0];

				streamMode2 = out.mult_flag[1];
				strcpy(rtspMultiIP2, out.mult_ip[1]);
				rtspMultiPort2 = out.mult_port[1];
			}
			else if(webinput == WEB_INPUT_2)
			{
				streamMode = out.mult_flag[2];
				strcpy(rtspMultiIP, out.mult_ip[2]);
				rtspMultiPort = out.mult_port[2];

				streamMode2 = out.mult_flag[3];
				strcpy(rtspMultiIP2, out.mult_ip[3]);
				rtspMultiPort2 = out.mult_port[3];				
			}
			

			
			fprintf(cgiOut, "setFormItemValue('wmform_outputRTSPSet', [");
			fprintf(cgiOut, "{'name': 'rtspActive','value': '%d','type': 'text' }", rtspActive);
			fprintf(cgiOut, ",{'name': 'rtspMtu','value': '%d','type': 'text' }", rtspMtu);
			fprintf(cgiOut, ",{'name': 'serverPort','value': '%d','type': 'text' }", serverPort);
			fprintf(cgiOut, ",{'name': 'streamMode','value': '%d','type': 'select' }", streamMode);
			fprintf(cgiOut, ",{'name': 'rtspMultiIP','value': '%s','type': 'text' }", rtspMultiIP);
			fprintf(cgiOut, ",{'name': 'rtspMultiPort','value': '%d','type': 'text' }", rtspMultiPort);
			
			fprintf(cgiOut, ",{'name': 'inputStream','value': '%d','type': 'select' }", webinput);
			fprintf(cgiOut, ",{'name': 'streamMode2','value': '%d','type': 'select' }", streamMode2);
			fprintf(cgiOut, ",{'name': 'rtspMultiIP2','value': '%s','type': 'text' }", rtspMultiIP2);
			fprintf(cgiOut, ",{'name': 'rtspMultiPort2','value': '%d','type': 'text' }]);\n", rtspMultiPort2);
			break;
		}

		case ACTION_OUTPUTRTSPSET_SET: {
			int serverPort = 0;
			int rtspActive = 0;
			int rtspMtu = 0;
			int streamMode = 0;
			char rtspMultiIP[16] = {0};
			int rtspMultiPort = 0;

			int streamMode2 = 0;
			char rtspMultiIP2[16] = {0};
			int rtspMultiPort2 = 0;			
			int input = 0;

			
			rtsp_server_config in;
			rtsp_server_config out;
			memset(&in, 0, sizeof(rtsp_server_config));
			memset(&out, 0, sizeof(rtsp_server_config));
	
			app_rtsp_get_ginfo(&in);			
			cgiFormInteger("serverPort", &serverPort, 0);
			cgiFormInteger("rtspActive", &rtspActive, 0);
			cgiFormInteger("rtspMtu", &rtspMtu, 0);
			cgiFormInteger("streamMode", &streamMode, 0);
			cgiFormString("rtspMultiIP", rtspMultiIP, 16);
			cgiFormInteger("rtspMultiPort", &rtspMultiPort, 0);

			cgiFormInteger("inputStream", &input, 0);
			cgiFormInteger("streamMode2", &streamMode2, 0);
			cgiFormString("rtspMultiIP2", rtspMultiIP2, 16);
			cgiFormInteger("rtspMultiPort2", &rtspMultiPort2, 0);
			
			in.s_port = serverPort;
			in.active = rtspActive;
			in.s_mtu = input;//rtspMtu;

			if(input == WEB_INPUT_1)
			{
				in.mult_flag[0] = streamMode;
				strcpy(in.mult_ip[0], rtspMultiIP);
				in.mult_port[0] = rtspMultiPort;

				in.mult_flag[1] = streamMode2;
				strcpy(in.mult_ip[1], rtspMultiIP2);
				in.mult_port[1] = rtspMultiPort2;
			}
			else if (input == WEB_INPUT_2)
			{
				in.mult_flag[2] = streamMode;
				strcpy(in.mult_ip[2], rtspMultiIP);
				in.mult_port[2] = rtspMultiPort;

				in.mult_flag[3] = streamMode2;
				strcpy(in.mult_ip[3], rtspMultiIP2);
				in.mult_port[3] = rtspMultiPort2;				
			}
			
			//need return value for check ip and port
			app_rtsp_set_ginfo(&in, &out);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		/*
		 * page captionlogo show action
		 */
		case PAGE_CAPTIONLOGO_SHOW: {
			//int outvalue = 0;
			//int outlen = 0;
			int cap_position = 0;
			
			int cap_x = 0;
			int cap_y = 0;
			
			
			char cap_text[128] = {0};
			int cap_alpha = 0;
			int cap_displaytime = 0;

			int logo_position = 0;
			int logo_x = 0;
			int logo_y = 0;
			int logo_alpha = 0;
			TextInfo textInfo;
			LogoInfo logoInfo;
			TextInfo textIn;
			LogoInfo logoIn;

			int channel = CHANNEL_INPUT_1;
			int webinput = WEB_INPUT_1;
			int mp_status = 1;
			


//			WebEnableTextInfo osd_enable;
			int isLogoOn = 0;
			int isTextOn = 0;

			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
			Mp_Info mp_info;
			WebGetMpInfo(&mp_info);	
			mp_status = mp_info.mp_status;
			if(mp_status == IS_MP_STATUS)
			{
				webinput = WEB_INPUT_3;
			}

			
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}else {
				channel = CHANNEL_INPUT_MP;
			}
			
			memset(&textInfo, 0, sizeof(TextInfo));
			memset(&logoInfo, 0, sizeof(LogoInfo));
			memset(&textIn, 0, sizeof(TextInfo));
			memset(&logoIn, 0, sizeof(LogoInfo));
//			memset(&osd_enable,0,sizeof(WebEnableTextInfo));

			WebGetTextOsd(&textIn, &textInfo,channel);
			WebGetLogoOsd(&logoIn, &logoInfo,channel);
//			WebGetEnabelTextParam(&osd_enable,channel);
			
			WebGetMaxPos(channel, &max_x, &max_y);

			isLogoOn = logoInfo.enable;
			isTextOn = textInfo.enable;
			
			cap_position = textInfo.postype;
			cap_x = textInfo.xpos;
			cap_y = textInfo.ypos;
			cap_alpha =(1- (textInfo.alpha)/128.0)*100;//to 0-100
			cap_displaytime = textInfo.showtime;
			strcpy(cap_text, textInfo.msgtext);

			logo_position = logoInfo.postype;
			logo_x = logoInfo.x;
			logo_y = logoInfo.y;
			logo_alpha =(1-(logoInfo.alpha)/128.0)*100;//to 0-100
			if( cap_alpha < 0){
				cap_alpha = 0;
				}
			if( logo_alpha < 0){
				logo_alpha = 0;
				}		
			showPage("./captionlogo.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [");
				fprintf(cgiOut, "{'name': 'cap_x','value': '%d','type': 'text' }", cap_x);
				fprintf(cgiOut, ",{'name': 'cap_y','value': '%d','type': 'text' }", cap_y);
				fprintf(cgiOut, ",{'name': 'cap_position','value': '%d','type': 'select' }", cap_position);
				fprintf(cgiOut, ",{'name': 'cap_brightness','value': '%d','type': 'text' }", cap_alpha);
				fprintf(cgiOut, ",{'name': 'cap_text','value': '%s','type': 'text' }", cap_text);
				fprintf(cgiOut, ",{'name': 'cap_displaytime','value': '%d','type': 'checkbox' }", cap_displaytime);
				fprintf(cgiOut, ",{'name': 'logo_x','value': '%d','type': 'text' }", logo_x);
				fprintf(cgiOut, ",{'name': 'logo_y','value': '%d','type': 'text' }", logo_y);
				fprintf(cgiOut, ",{'name': 'logo_position','value': '%d','type': 'select' }", logo_position);
				fprintf(cgiOut, ",{'name': 'logo_brightness','value': '%d','type': 'text' }]);\n", logo_alpha);
				fprintf(cgiOut, "initInputTab(%d,%d);\n", webinput, lang);
				fprintf(cgiOut, "initMaxPos(%d,%d);\n", max_x, max_y);
				fprintf(cgiOut, "initMergeTab(%d);\n", mp_status);
				//fprintf(cgiOut, "formBeautify();\n");
				fprintf(cgiOut, "fixBrightnessSlider();\n");
				fprintf(cgiOut, "captionCtrlInit(%d);\n", isTextOn);
				fprintf(cgiOut, "logoCtrlInit(%d);\n", isLogoOn);
			fprintf(cgiOut, "</script>\n");
			break;
		}


		/*
		 * page captionlogo set action.
		 */
		case ACTION_CAPTIONLOGO_SET: {
			char tmp[150]={0};
			//int outlen = 0;
			int cap_position = 0;
			int cap_x = 0;
			int cap_y = 0;
			char cap_text[128] = {0};
			int cap_alpha = 0;
			int cap_displaytime = 0;

			int logo_position = 0;
			int logo_x = 0;
			int logo_y = 0;
			int logo_alpha = 0;
			TextInfo textInfoIn;
			TextInfo textInfoOut;
			LogoInfo logoInfoIn;
			LogoInfo logoInfoOut;

			int isLogoOn =0;
			int isTextOn= 0;
			
			int mp_status = 1;
			
			int channel = CHANNEL_INPUT_1;
			int webinput =WEB_INPUT_1;
			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);

			Mp_Info mp_info;
			WebGetMpInfo(&mp_info);	
			mp_status = mp_info.mp_status;
			if(mp_status == IS_MP_STATUS)
			{
				webinput = WEB_INPUT_3;
			}
			
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			} 
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}else {
				channel = CHANNEL_INPUT_MP;
			}
				
			memset(&textInfoIn, 0, sizeof(TextInfo));
			memset(&textInfoOut, 0, sizeof(TextInfo));
			memset(&logoInfoIn, 0, sizeof(LogoInfo));
			memset(&logoInfoOut, 0, sizeof(LogoInfo));

			cgiFormInteger("cap_position", &cap_position, 0);
			cgiFormInteger("cap_x", &cap_x, 0);
			cgiFormInteger("cap_y", &cap_y, 0);
			cgiFormString("cap_text", cap_text, 1000);
			cgiFormInteger("cap_displaytime", &cap_displaytime, 0);
			cgiFormInteger("cap_brightness", &cap_alpha, 0);
			cgiFormInteger("logo_position", &logo_position, 0);
			cgiFormInteger("logo_x", &logo_x, 0);
			cgiFormInteger("logo_y", &logo_y, 0);
			cgiFormInteger("logo_brightness", &logo_alpha, 0);

			cgiFormInteger("logo", &isLogoOn, 0);
			cgiFormInteger("caption",&isTextOn,0);
		
			if(cap_position != ABSOLUTE2) {
				WebGetTextPos(cap_position, &cap_x, &cap_y);
			}

			if(logo_position != ABSOLUTE2) {
				WebGetLogoPos(logo_position, &logo_x, &logo_y);
			}
			if( cap_alpha < 0){
				cap_alpha = 0;
			}
			if( logo_alpha < 0){
				logo_alpha = 0;
			}	
			
			logo_alpha =128*(1- logo_alpha/100.0);
			cap_alpha =128*(1-  cap_alpha/100.0);
			
			
			textInfoIn.postype = cap_position;
			textInfoIn.xpos = cap_x;
			textInfoIn.ypos = cap_y;
			strcpy(textInfoIn.msgtext, cap_text);
			textInfoIn.alpha = cap_alpha;
			textInfoIn.showtime = cap_displaytime;
			textInfoIn.enable = isTextOn;

			logoInfoIn.postype = logo_position;
			logoInfoIn.x = logo_x;
			logoInfoIn.y = logo_y;
			logoInfoIn.alpha = logo_alpha;
			logoInfoIn.enable = isLogoOn;
			
			WebSetTextOsd(&textInfoIn,  &textInfoOut,channel);
			WebSetLogoOsd(&logoInfoIn,  &logoInfoOut,channel);

			getLanguageValue(sys_language,"opt.success",tmp);
			fprintf(cgiOut, "<script type='text/javascript'>alert('%s');</script>", tmp);
			break;
		}

		case ACTION_UPLOADLOGOPIC_SET: {
			// update logo pictrue.

			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;

			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}else {
				channel = CHANNEL_INPUT_MP;
			}

			uploadLogo(channel);
	//		fprintf(cgiOut, "<script type='text/javascript'>alert('%s');</script>", "Success!");
			break;
		}

		/*
		 * page remotectrl show action.
		 */
		case PAGE_REMOTECTRL_SHOW: {
			int ptzProtocal = 0;
			int webinput =WEB_INPUT_1;
			int channel=0;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);

			
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}else {
				channel = CHANNEL_INPUT_1;
			}
			WebGetCtrlProto(&ptzProtocal,channel);
			showPage("./remotectrl.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			fprintf(cgiOut, "{'name': 'ptzProtocal','value': '%d','type': 'select'}]);", ptzProtocal);
			fprintf(cgiOut, "initInputTab(%d);\n", webinput);
			//fprintf(cgiOut, "formBeautify();");
			fprintf(cgiOut, "</script>\n");
			break;
		}

		/*
		 * page remotectrl set action.
		 */
		case PAGE_REMOTECTRL_SET: {
			int ptzProtocal = 0;
			int outvalue = 0;
			int webinput =WEB_INPUT_1;
			int channel=0;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}else {
				channel = CHANNEL_INPUT_1;
			}
			
			cgiFormInteger("ptzProtocal",&ptzProtocal,0);
			WebSetCtrlProto(ptzProtocal, &outvalue,channel);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		/*
		 * PTZ control.
		 */
		case ACTION_PTZ_CTRL: {
			int speed = 0;
			int type = 0;
			int addr = 1;
			int webinput =WEB_INPUT_1;
			int channel ;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
	//		webSetChannel(webinput);
			
			channel =  webinput_to_channel(webinput);
			
			cgiFormInteger("speed", &speed, 1);
			cgiFormInteger("type", &type, 14);
			cgiFormInteger("addressbit",&addr,1);
			
			webFarCtrlCamera(addr,type, speed,channel);
			break;
		}

		/*
		 * page network show action
		 */
		case PAGE_NETWORK_SHOW: {
			int outlen = 0;
			int dhcp = 0;
			int IPAddress[2] = {0};
			int gateWay[2] = {0};
			int subMask[2] = {0};
			SYSPARAMS sysParamsOut;
			struct in_addr inAddr;   
//			int channel=0;			
			Enc2000_Sys sysOut;
			memset(&sysOut, 0, sizeof(Enc2000_Sys));
			//webGetDHCPFlag(&dhcp);
			
			webGetSysInfo(&sysOut,&outlen);
			memcpy(&sysParamsOut,&(sysOut.eth0),sizeof(SYSPARAMS));
			dhcp = sysParamsOut.nTemp[0];
			IPAddress[0] = sysParamsOut.dwAddr;
			subMask[0] = sysParamsOut.dwNetMask ;
			gateWay[0]	 = sysParamsOut.dwGateWay;

			memset(&sysParamsOut,0,sizeof(sysParamsOut));
			memcpy(&sysParamsOut,&(sysOut.eth1),sizeof(SYSPARAMS));
			dhcp = sysParamsOut.nTemp[0];
			IPAddress[1] = sysParamsOut.dwAddr;
			subMask[1] = sysParamsOut.dwNetMask;
			gateWay[1]	 = sysParamsOut.dwGateWay;

			showPage("./network.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [");
				memcpy(&inAddr,&IPAddress[0],4); 
				fprintf(cgiOut, "{'name': 'IPAddress','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				//fprintf(cgiOut, "{'name': 'IPAddress','value': '%s','type': 'text' }", "192.168.7.9");
				memcpy(&inAddr,&subMask[0],4); 
				fprintf(cgiOut, ",{'name': 'subMask','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				//fprintf(cgiOut, ",{'name': 'subMask','value': '%s','type': 'text' }", "255.255.255.0");
				memcpy(&inAddr,&gateWay[0],4); 
				fprintf(cgiOut, ",{'name': 'gateWay','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				//fprintf(cgiOut, ",{'name': 'gateWay','value': '%s','type': 'text' }", "192.168.7.254");
			//	fprintf(cgiOut, ",{'name': 'dhcp','value': '%d','type': 'checkbox' }", dhcp);

				memcpy(&inAddr,&IPAddress[1],4); 
				fprintf(cgiOut, ",{'name': 'IPAddress1','value': '%s','type': 'text' }", inet_ntoa(inAddr));
				memcpy(&inAddr,&subMask[1],4); 
				fprintf(cgiOut, ",{'name': 'subMask1','value': '%s','type': 'text' }]);\n", inet_ntoa(inAddr));
				//memcpy(&inAddr,&subMask[1],4); 
				//fprintf(cgiOut, ",{'name': 'gateWay1','value': '%s','type': 'text' }]);\n", inet_ntoa(inAddr));
				//fprintf(cgiOut, ",{'name': 'dhcp1','value': '%d','type': 'checkbox' }]);\n", dhcp);

				//fprintf(cgiOut, "formBeautify();");
				//fprintf(cgiOut, "initNetConfig2(%d);", 1);//?¨?????§?????|???ˉ?￥???|?|???ˉENC2000  0:?￥???| 1:?|???ˉ
				//fprintf(cgiOut, "initDHCPCheckBox();");
			fprintf(cgiOut, "</script>\n");
			break;
		}

		/*
		 * page network set action
		 */
		case ACTION_NETWORK_SET: {
//			int cmd = MSG_SETSYSPARAM;
		//	int outval = 0;
			int outlen = 0;
			int dhcp = 0;
			char IPAddress[20] = {0};
			char IPAddress1[20] = {0};
			char gateWay[20] = {0};
			char subMask[20] = {0};
			char subMask1[20] = {0};
			int r1=0,r2=0;

		
			SYSPARAMS sysParamsIn;
			SYSPARAMS sysParamsIn1;
//			Enc2000_Sys sysOut;
			memset(&sysParamsIn, 0, sizeof(SYSPARAMS));
			//memset(&sysParamsOut, 0, sizeof(SYSPARAMS));
	//		appCmdStructParse(MSG_GETSYSPARAM, NULL, sizeof(Enc2000_Sys), &sysOut, &outlen);
			
			cgiFormInteger("dhcp", &dhcp, 0);
			cgiFormString("IPAddress", IPAddress, 20);
			cgiFormString("IPAddress1", IPAddress1, 20);

			cgiFormString("gateWay", gateWay, 20);
			
			cgiFormString("subMask", subMask, 20);
			cgiFormString("subMask1", subMask1, 20);
			
			sysParamsIn.dwAddr = inet_addr(IPAddress);
			sysParamsIn.dwNetMask = inet_addr(subMask);
			sysParamsIn.dwGateWay = inet_addr(gateWay);
			sysParamsIn.nTemp[0] = dhcp;
			sysParamsIn1.dwAddr = inet_addr(IPAddress1);
			sysParamsIn1.dwNetMask = inet_addr(subMask1);
			sysParamsIn1.dwGateWay = inet_addr(gateWay);
			sysParamsIn1.nTemp[0] = dhcp;
			
			ret=0;
			r1 = CheckIPNetmask( sysParamsIn.dwAddr,sysParamsIn.dwNetMask,sysParamsIn.dwGateWay);
			if( 0 == r1){
				ret=SERVER_RET_INVAID_PARM_VALUE;
			}
			r2= CheckIPNetmask( sysParamsIn1.dwAddr,sysParamsIn1.dwNetMask,sysParamsIn1.dwGateWay);
			if( 0 == r2){
				ret=SERVER_RET_INVAID_PARM_VALUE;
			}

			if(1==r2  && 1 ==r1 && 0 == ret){
				Enc2000_Sys sysin,sysout;
				memset(&sysin,0,sizeof(Enc2000_Sys));
				memset(&sysout,0,sizeof(Enc2000_Sys));
				memcpy(&(sysin.eth0),&sysParamsIn,sizeof(SYSPARAMS));
				memcpy(&(sysin.eth1),&sysParamsIn1,sizeof(SYSPARAMS));
				ret = appCmdStructParse(MSG_SETSYSPARAM, &sysin, sizeof(Enc2000_Sys), &sysout, &outlen);
			}
			
			if( ret == SERVER_RET_INVAID_PARM_VALUE ){
				//fprintf(cgiOut, "%d", RESULT_SUCCESS);
				fprintf(cgiOut, "%d", RESULT_FAILURE);
			}else if (SERVER_RET_OK == ret ){
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
				webRebootSystem();
			}else{
				fprintf(cgiOut, "%d", RESULT_SUCCESS);
			}

			break;
		}

		case PAGE_MODIFYPASSWORD_SHOW: {
//			char username[20] = {0};
//			char webusername[20] = {0};
			//getLanguageValue("sysinfo.txt","username",username);
			//getLanguageValue("sysinfo.txt","webusername",webusername);
			showPage("./modifypassword.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "initUsernameSelect(['%s', '%s']);", USERNAME, WEBUSERNAME);
				//fprintf(cgiOut, "formBeautify();");
			fprintf(cgiOut, "</script>\n");
			break;
		}
		case PAGE_OTHERSET_SHOW: {

			char temp[10] = {0};
			char encoderTime[256] = {0};

			int zoomModel = 0;
			int enclevel = 0;
			
			DATE_TIME_INFO date_time_info;
			memset(&date_time_info, 0, sizeof(DATE_TIME_INFO));
			WebgetEncodetime(&date_time_info);

			webGetEncodingFrofile(&enclevel);
			webGetScaleMode(&zoomModel);
			if(zoomModel == 0)
				zoomModel=1;
			else 
				zoomModel=0;
	
			
			sprintf(temp,"%d", date_time_info.year);
			strcpy(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.month);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.mday);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.hours);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.min);
			strcat(encoderTime, temp);
			strcat(encoderTime, "/");
			memset(temp,0,strlen(temp));
			sprintf(temp,"%d", date_time_info.sec);
			strcat(encoderTime, temp);

			showPage("./otherset.html", sys_language);

			fprintf(cgiOut, "<script type='text/javascript'>\n");
				fprintf(cgiOut, "setFormItemValue('wmform', [");
				fprintf(cgiOut, "{'name': 'zoomModel','value': '%d','type': 'radio' }", zoomModel);
				fprintf(cgiOut, ",{'name': 'enclevel','value': '%d','type': 'radio'}]);\n", enclevel);
				fprintf(cgiOut, "initPageTime('%s', '%s');", encoderTime, sys_language);
			fprintf(cgiOut, "</script>\n");
			break;
		}
		case PAGE_SYSUPGRADE_SHOW: {
			showPage("./sysupgrade.html", sys_language);
			break;
		}
		case PAGE_RESETDEFAULT_SHOW: {
			showPage("./resetdefault.html", sys_language);
			break;
		}

		case ACTION_SETDEFAULT_SET: {
			Websetrestore();
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case PAGE_INPUTDETAILS_SHOW: {
			showPage("./input_details.html", sys_language);
			break;
		}

		case ACTION_INPUTDETAILS_GET: {
			char inputDetailInfo[2048];

			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;
			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}


			webSignalDetailInfo(inputDetailInfo,2048,channel);
			fprintf(cgiOut, "setFormItemValue('wmform_videoAdvancedSet', [");
			fprintf(cgiOut, "{'name': 'inputDetailInfo','value': '%s','type': 'textarea' }]);\n", inputDetailInfo);
			break;
		}

		case ACTION_ADJUSTSCREEN_SET: {
			int speed = 0;
			int value = 0;
			short hporch = 0;
			short vporch =0;

			int sdiajust = 0;//tanqh

			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;

			//add by zm
			cgiFormInteger("input", &webinput, WEB_INPUT_1);
		//	webSetChannel(webinput);
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}

			
			cgiFormInteger("sdiajust", &sdiajust ,0);
			cgiFormInteger("speed", &speed, 1);
			cgiFormInteger("cmdType" ,&value, 1);
			//webSetChannel(sdiajust);
			if(speed==1)
			{
				hporch = 1;
				vporch = 1;
			}
			if(speed == 5)
			{
				hporch=6;
				vporch = 2;
			}
			if(speed == 10)
			{
				hporch = 10;
				vporch = 4;
			}
			if(sdiajust == 1 ){
				if(value == 0)//Reset
					webSDIRevisePicture(0,0,channel);
				if(value == 5)//right
					webSDIRevisePicture(0-hporch,0,channel);
				if(value == 6)//left
					webSDIRevisePicture(hporch,0,channel);
				if(value == 7)//down
					webSDIRevisePicture(0,0-vporch,channel);
				if(value == 8)//up
					webSDIRevisePicture(0,vporch,channel);		
				break;
			}
			
			if(value == 0)//Reset
				webRevisePicture(0,0,channel);
			if(value == 5)//right
				webRevisePicture(0-hporch,0,channel);
			if(value == 6)//left
				webRevisePicture(hporch,0,channel);
			if(value == 7)//down
				webRevisePicture(0,0-vporch,channel);
			if(value == 8)//up
				webRevisePicture(0,vporch,channel);

			
			break;
		}

		case PAGE_OUTPUTADVANCEDSET_SHOW: {
			showPage("./output_detailSet.html", sys_language);
			break;
		}
		case PAGE_OUTPUTRTSPSET_SHOW: {
			showPage("./output_rtspSet.html", sys_language);
			break;
		}



		case PAGE_VIDEOADVANCEDSET_SHOW: {
			showPage("./video_advancedSet.html", sys_language);
			break;
		}

		case ACTION_VIDEOADVANCEDSET_GET: {
			int param1 = 0;
			int param2 = 0;
			int param3 = 0;
			int param4 = 0;
			int param5 = 0;
			H264EncodeParam h264EncodeParam;
			memset(&h264EncodeParam, 0, sizeof(H264EncodeParam));
			/*webGetH264encodeParam(&h264EncodeParam);
			param1 = h264EncodeParam.param1;
			param2 = h264EncodeParam.param2;
			param3 = h264EncodeParam.param3;
			param4 = h264EncodeParam.param4;
			param5 = h264EncodeParam.param5;*/

			fprintf(cgiOut, "setFormItemValue('wmform_videoAdvancedSet', [");
			fprintf(cgiOut, "{'name': 'param1','value': '%d','type': 'text' }", param1);
			fprintf(cgiOut, ",{'name': 'param2','value': '%d','type': 'text' }", param2);
			fprintf(cgiOut, ",{'name': 'param3','value': '%d','type': 'text' }", param3);
			fprintf(cgiOut, ",{'name': 'param4','value': '%d','type': 'text' }", param4);
			fprintf(cgiOut, ",{'name': 'param5','value': '%d','type': 'text' }]);\n", param5);
			break;
		}

		case ACTION_VIDEOADVANCEDSET_SET: {
			int param1 = 0;
			int param2 = 0;
			int param3 = 0;
			int param4 = 0;
			int param5 = 0;
			H264EncodeParam h264EncodeParamIn;
			H264EncodeParam h264EncodeParamOut;
			memset(&h264EncodeParamIn, 0, sizeof(H264EncodeParam));
			memset(&h264EncodeParamOut, 0, sizeof(H264EncodeParam));
			cgiFormInteger("param1", &param1, 0);
			cgiFormInteger("param2", &param2, 0);
			cgiFormInteger("param3", &param3, 0);
			cgiFormInteger("param4", &param4, 0);
			cgiFormInteger("param5", &param5, 0);
			h264EncodeParamIn.param1 = param1;
			h264EncodeParamIn.param2 = param2;
			h264EncodeParamIn.param3 = param3;
			h264EncodeParamIn.param4 = param4;
			h264EncodeParamIn.param5 = param5;
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case PAGE_ADDOUTPUT_SHOW: {
			showPage("./output_addSet.html", sys_language);
			break;
		}

		case PAGE_MERGE_SHOW: {
			int layout = 0;
			Mp_Info info;
			int mp_status = 0;

			int mergeModel= 0 ;
			WebGetMpInfo(&info);	
			layout = info.layout;
			mp_status = info.mp_status;
			if(info.win1 == SIGNAL_INPUT_1 && info.win2 == SIGNAL_INPUT_2)
			{
				mergeModel  = 1;
			}
			else
				mergeModel  = 0;

			webSetChannel(info.win1);
			webSetChannel(info.win2);
			
			showPage("./merge.html", sys_language);
			fprintf(cgiOut, "<script type='text/javascript'>\n");
			fprintf(cgiOut, "setFormItemValue('wmform', [");
			fprintf(cgiOut, "{'name': 'mergestatus','value': '%d','type': 'radio'},", mp_status);
			fprintf(cgiOut, "{'name': 'layout','value': '%d','type': 'radio'}]);", layout);

		//	fprintf(cgiOut, "{'name': 'mergeModel','value': '%d','type': 'radio'}]);", layout);
			//fprintf(cgiOut, "formBeautify();\n");
			fprintf(cgiOut, "initMerge(%d,%d);\n", mp_status,mergeModel);
			fprintf(cgiOut, "</script>\n");
			break;
		}

		case PAGE_RTSP_PLAYURL_SHOW:{
			char url[2048] = {0};
			int isRtspUsed = 0;
			int len = 0;
			char ip_temp[64] = {0};
			stream_output_server_config output_out;
			rtsp_server_config rtsp_config;
			memset(&output_out,0,sizeof(stream_output_server_config));
			memset(&rtsp_config,0,sizeof(rtsp_server_config));
	
			isRtspUsed = app_rtsp_get_used();
				
			if(isRtspUsed == 1) {
				//get rtsp info
				app_rtsp_get_cinfo(&output_out);
				app_rtsp_get_ginfo(&rtsp_config);
				sprintf(ip_temp,"rtsp://%s:%d",output_out.main_ip,rtsp_config.s_port);
				len += snprintf(url,sizeof(url) - len,"CH0/HIGH:\n<br/>  %s/stream0/high\n<br/>",ip_temp);
				len += snprintf(url+len,sizeof(url) - len,"CH0/LOW:\n<br/>  %s/stream0/low\n<br/>",ip_temp);
				len += snprintf(url+len,sizeof(url) - len,"CH1/HIGH:\n<br/>  %s/stream1/high\n<br/>",ip_temp);
				len += snprintf(url+len,sizeof(url) - len,"CH1/LOW:\n<br/>  %s/stream1/low\n<br/>",ip_temp);
	
			}
			else
			{
				sprintf(url,"%s\n","rtsp is invaild\n");
			}

			fprintf(cgiOut,"%s", url);
			break;
		}

		case ACTION_MERGE_SET:{			
		//	char tmp[150]={0};
			int layout = 1;
			Mp_Info info;
			int mp_status = 1;
			int mergeModel = 0;
			cgiFormInteger("mergestatus", &mp_status, 0);
			cgiFormInteger("layout", &layout, 0);
			cgiFormInteger("mergeModel", &mergeModel, 0);
			if( mergeModel  == 0){
				info.win1 = SIGNAL_INPUT_1;
				info.win2 = SIGNAL_INPUT_2;
			}else{
				info.win1 = SIGNAL_INPUT_2;
				info.win2 = SIGNAL_INPUT_1;
			}
			info.mp_status = mp_status; 
			info.layout  = layout;
			WebSetMpInfo(&info);
		//	getLanguageValue(sys_language,"opt.success",tmp);
		//	fprintf(cgiOut, "<script type='text/javascript'>alert('%s');</script>", tmp);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_ENCADVANCED_SET:{
			int zoomModel = 0;
			int enclevel = 0;
			cgiFormInteger("zoomModel", &zoomModel, 0);
			cgiFormInteger("enclevel", &enclevel, 0);
			if(zoomModel==0)
				zoomModel=1;
			else 
				zoomModel=0;
		
			webSetEncodingFrofile(enclevel);
			webSetScaleMode(zoomModel);
			//
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_DOWNLOAD_SDP: {
			char  buff[4096];
			int total_len = 0;
			int w_len = 0;
			char inputstring[32]={0};
			
		 	char ip[16] = {0};
			int videoport = 0;
			int audioport = 0;
		//	int type = 0;

			int channel = CHANNEL_INPUT_1;
			cgiFormString("ip", ip, 16);
			cgiFormInteger("vPort", &videoport, 0);
			cgiFormInteger("aPort", &audioport, 0);
			cgiFormString("iStream",inputstring,sizeof(inputstring));

			if (strcmp(INPUT1_HIGH_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if (strcmp(INPUT1_LOW_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_1_LOW;
			}			
			else if (strcmp(INPUT2_HIGH_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_2;
			}
			else if (strcmp(INPUT2_LOW_STRING,inputstring) == 0)
			{
				channel = CHANNEL_INPUT_2_LOW;
			}		

			
			fprintf(cgiOut, "Content-Disposition:attachment;filename=encode.sdp\n\n");

		//	snprintf(buff,sizeof(buff),"ip=%s,port=%d,type=%d\n",ip,videoport,type);

			//need channel num
			webGetSdpInfo(buff , sizeof(buff),channel,ip,videoport,audioport);  

			total_len = strlen(buff);

			while(total_len>0)
			{
				w_len = fwrite(buff, 1, total_len -w_len, cgiOut);
				total_len -= w_len;
				if(w_len <= 0)
					break;
			}
		
			break;
		}

		case ACTION_SERIALNO_SET:
		{
			char serialNo[256]={0};
			cgiFormString("serialNumber", serialNo, 256);
			WebSetSerialNo(serialNo,256);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

		case ACTION_CBCR_SET:
		{
			int webinput = WEB_INPUT_1;
			int channel = CHANNEL_INPUT_1;
			int out = -1;
			cgiFormInteger("input", &webinput, WEB_INPUT_1);

			//这里调用
			
			if(webinput == WEB_INPUT_1)
			{
				channel = CHANNEL_INPUT_1;
			}
			else if(webinput == WEB_INPUT_2)
			{
				channel = CHANNEL_INPUT_2;
			}
			webSetCbCr(&out, channel);
			fprintf(cgiOut, "%d", RESULT_SUCCESS);
			break;
		}

	
		default:
		{
			showPage("./login.html",sys_language);
			fprintf(cgiOut, "<script type='text/javascript' src='../ValidationEngine/js/languages/jquery.validationEngine-%s.js'></script>", script_language);
		}
		break;

}
	return 0;

}
	fflush(stdout);
