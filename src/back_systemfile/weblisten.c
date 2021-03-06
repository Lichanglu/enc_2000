
#include "weblisten.h"
#include "webmiddle.h"
#include "../extern_update_header/app_product.h"

#define printf printf
#define PRINTF printf
#define ADD_UPDATE_HEADER
#define THREAD_SUCCESS      (void *) 0
#define THREAD_FAILURE      (void *) -1

#define MAX_LISTEN		 10
#define VERIFYLEN        (4*1024)
#define SYSCMDLEN		 (1024)
//##########################定义升级包临时路径及名称#########################
#define	UPDATE_FILE	"/var/log/recserver/update.tgz"
#define VERIFYFILE        "hash"            //校验文件的名称

#define APPDIRPATH 		  "/usr/local/reach"		    //实际应用程序
#define BACKUPDIR         "/var/log/recserver/reach/.appbackup"       //备份分区
#define TMPBACKUPDIR      "/var/log/recserver/reach/.tmpappbackup"    //备份临时区
#define TMPAPPDIRPATH     "/var/log/recserver/reach/.tmpapp"          //应用临时分区

#define IsHaveErrorApp    "/var/log/recserver/reach/.errorapp"             //标示升级或备份异常 应用备份
#define IsHaveErrorBack   "/var/log/recserver/reach/.errorback"             //标示升级或备份异常 应用备份
#define IsHaveBack		  "/var/log/recserver/reach/.backup" 			 //标示是否存在备份

#define SETETH0STATIC(file,ip,gateway,netmask)\
fprintf(file,"ifconfig eth0 %s netmask %s\n"\
 			"route add default gw %s\n"\
			,ip,netmask,gateway);

#define SETETH0DHCP(file)\
fprintf(file,"/sbin/dhcpcd eth0\n");

#define SETETH1STATIC(file,ip,netmask)\
fprintf(file,"ifconfig eth1 %s netmask %s\n"\
			,ip,netmask);

#define	UBI_ATTACH_MAIN_FILESYS		"ubiattach /dev/ubi_ctrl -m 5"

#define BACK_KERNELFILENAME		"/mnt/nand/opt/dvr_rdk/ti816x_2.8/update/uImage"
#define BACK_FPGAFILENAME		"/mnt/nand/opt/dvr_rdk/ti816x_2.8/update/fpga.bin"
#define BACK_CONFIG				"/mnt/nand/opt/dvr_rdk/ti816x_2.8/.config"
typedef unsigned char  BYTE;
typedef unsigned short WORD;

/*消息头结构*/
typedef struct __MSGHEAD__ {
	/*
	## nLen:
	## 通过htons转换的值
	## 包括结构体本身和数据
	##
	*/
	WORD	nLen;
	WORD	nVer;							//版本号(暂不用)
	BYTE	nMsg;							//消息类型
	BYTE	szTemp[3];						//保留字节
} MSGHEAD;

static int mount_ok = 0;

#define MSG_UpdataFile_FAILS		21
#define MSG_UpdataFile_SUCC		22
#define HEAD_LEN					sizeof(MSGHEAD)
#define FIRST_UPDATE_HEADER "##################SZREACH#######"  //32 char 
#define UPDATE_HEARER_LEN 256 //空位填充0xff
typedef struct {
	int info_len; //sizeof(UP_HEADER_INFO)
	unsigned int checksum; //结构体的checksum
	char product_id[16]; //ENC1200VGA/SDI
	int major_version;         // 预留, 主版本必须一致，否则不升级 比如3.0,2.0等
	int minor_version;		  // 预留 0
	int pcb_version; //预留,
	int kernel_version; 		//预留,
	int fpga_version; 			//预留,
	int vss_version;		  // 预留,
	int force;                //预留,强制  //不必关注主版本号一致性???
	int compressmode;         //预留,压缩模式，预留
	unsigned int filesystem;  //预留,文件系统 yaffs ,jaffs
	char target[16];          //预留 ，比如kernel ,app等
	unsigned char custom_info[32];   //定制产品信息
	char reserve[8];		//预留
} UP_HEADER_INFO; //len

#define FLASH_ENABLE	41
#define FPGA_PRO	2
#define FPGA_DONE	3

typedef struct _R_GPIO_data_ {
	unsigned int gpio_num;
	unsigned int gpio_value;
} R_GPIO_data;


#define SET_GPIO (0x55555555)
#define GET_GPIO (0xAAAAAAAA)


#define GPIO_4			4
#define GPIO_5			5
#define GPIO_6			6
#define GPIO_7			7
#define GPIO_8			8
#define GPIO_9			9


static int GPIOInit(void)
{
	int fd = open("/dev/Rgpio",O_RDWR | O_SYNC);
	if(fd < 0) {
		printf("[GPIOInit] failed error[%d]:[%s]\n", errno, strerror(errno));
	}
	return fd;
}

static void GPIODeInit(int fd)
{
	if(fd > 0) {
		close(fd);
	}
}


//获取文件的总长
static unsigned int get_file_size(FILE *fp)
{
	if(fp == NULL) {
		return 0;
	}

	unsigned int file_len = 0;
	fseek(fp, 0L, SEEK_END);
	file_len = ftell(fp);
	printf("info:file len =%u,the fp =%p\n", file_len, fp);
	fseek(fp, 0L, SEEK_SET);
	return file_len;
}


static char *basename(char *path)
{
  /* Ignore all the details above for now and make a quick and simple
     implementaion here */
  char *s1;
  char *s2;

  s1=(char *)strrchr((char *)path, '/');
  s2=(char *)strrchr((char *)path, '\\');

  if(s1 && s2) {
    path = (s1 > s2? s1 : s2)+1;
  }
  else if(s1)
    path = s1 + 1;
  else if(s2)
    path = s2 + 1;

  return path;
}

int back_updatekernel(void)
{
	printf("[back_updatekernel] start ...\n");
	char command[256] = {0};
	char kernel_uImage[256] = {0};
	int ret =  -2;

	sprintf(kernel_uImage, "%s", BACK_KERNELFILENAME);

	/*更新内核*/
	//主系统是mtd3 ,副系统是mtd4
	if(access(kernel_uImage, F_OK) == 0) {
		PRINTF("i Will 'flash_eraseall /dev/mtd3'\n");
		system("flash_eraseall /dev/mtd3");
		sprintf(command, "nandwrite -p /dev/mtd3 %s", kernel_uImage);
		PRINTF("i Will '%s'\n", command);
		system(command);
		memset(command, 0, 256);
		sprintf(command, "rm -rf %s", kernel_uImage);
		system(command);
		system("sync");
		PRINTF("update kernel is end\n");
	} else {
		printf("have no file [%s]\n", kernel_uImage);
	}
	printf("[back_updatekernel] end ...\n");
	return 0;
}


/*==============================================================================
    函数: <GetLanIpconfig>
    功能: 获取局域网IP
    参数: 
    返回值: 
    Created By 徐崇 2012.12.02 15:16:36 For Web
==============================================================================*/
int GetLanIpconfig(void)
{
	int ret = -1;
	

	return 1;
}






/*消息头打包*/
static void msgPacket(int identifier, unsigned char *data, webparsetype type, int len, int cmd, int ret)
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
	//printf("msghead.len=%d\n", msghead.len);
	return ;
}


int SinglemidParseStruct(int identifier, int fd, char *data, int len)
{
	int recvlen;
	int cmd = 0;
	char actualdata[1024] = {0};
	char out[2048] = {0};
	int  vallen = 0;
	int  status = 0;

	char senddata[1024] = {0};
	int totallen = 0;
	
	int web_ret =   SERVER_RET_OK;
	int need_send = 0;

	int subcmd = 0;
	recvlen = recv(fd, data, len, 0);
	
	if(recvlen < 0)
	{
		printf("recv failed,errno = %d,error message:%s \n", errno, strerror(errno));
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		status = -1;
		goto EXIT;
	}
	
	//vallen = len - sizeof(int);

	memset(out, 0, 2048);
	memcpy(&cmd, data, sizeof(int));
	memcpy(actualdata, data + sizeof(int), len - sizeof(int));
	
	printf("-----> recv msgtype[%x]\n", cmd);
	switch(cmd)
	{
	
		case MSG_GETDEVINFO:
			{
				printf("MSG_GETDEVINFO\n");
				
  				web_ret = SERVER_RET_OK;
				
				vallen = sizeof(DevInfo);
				need_send = 1;
				
			}
		break;
		
		case MSG_GETDISKINFO:
			{
				printf("MSG_GETDISKINFO\n");
		
			
				web_ret = SERVER_RET_OK;
			
				vallen = sizeof(DiskInfo);
				need_send = 1;
			}
			break;
		case MSG_SETLANIPCONFIG:
			{
				printf("MSG_SETLANIPCONFIG\n");
				
				vallen = sizeof(IpConfig);
				need_send = 1;
			}
			break;
			
		case MSG_SETWANIPCONFIG:
			{
				printf("MSG_SETWANIPCONFIG\n");
			
				vallen = sizeof(IpConfig);
				need_send = 1;
			}
			break;
			
		case MSG_GETLANIPCONFIG:
			{
				printf("MSG_GETLANIPCONFIG\n");
				
				web_ret = SERVER_RET_OK;
				
				vallen = sizeof(IpConfig);
				need_send = 1;
				
			}
			break;
			
		case MSG_GETWANIPCONFIG:
			{
				printf("MSG_GETWANIPCONFIG\n");
				
				web_ret = SERVER_RET_OK;
				
				vallen = sizeof(IpConfig);
				need_send = 1;
				
			}
			break;
			
		default
				:
				
			break;
			
	}
	
EXIT:

	if(need_send == 1)
	{
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + vallen;
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, cmd, web_ret);
		memcpy(senddata + (totallen - vallen), out, vallen);
		printf("-----> the cmd =%04x,the ret=%04x\n", cmd,  web_ret);
		send(fd, senddata, totallen, 0);
		
		if(web_ret != SERVER_RET_OK)
		{
			printf("ERROR,the cmd =0x%x,ret= 0x%x\n", subcmd, web_ret);
		}
		
	}
	
	return status;
}


int SinglemidParseInt(int identifier, int fd, char *data, int len)
{
	int recvlen;
	int cmd = 0;
	int actdata = 0;
	int ret = 0;
	int web_ret = SERVER_RET_OK;
	
	int need_send = 0;
	
	
	char senddata[1024] = {0};
	int totallen = 0;
	
	recvlen = recv(fd, data, len, 0);
	int out = 0;
	printf("recvlen = %d\n",recvlen);
	if(recvlen < 0)
	{
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}
	
	memcpy(&cmd, data, sizeof(int));
	memcpy(&actdata, data + sizeof(int), len - sizeof(int));
	
	printf("cmd = 0x%04x\n", cmd);
	
	switch(cmd)
	{
		case MSG_SETLANGUAGE:
			{
				printf("MSG_SETLANGUAGE %d\n",actdata);
				need_send = 1;
			}
			break;
		case MSG_GETLANGUAGE:
			{
				printf("MSG_GETLANGUAGE\n");
				out = 1;
				need_send = 1;
			}
			break;
		case MSG_SYSREBOOT:
			{
				printf("MSG_SYSREBOOT\n");
				system((char*)"sync");
				sleep(3);
				system((char*)"reboot -f");
				need_send = 1;	
			}
			break;
		default:			
			printf("unkonwn cmd = %04x\n", cmd);
			need_send = 1;
			web_ret = SERVER_RET_UNKOWN_CMD;
			break;
			//	case
	}
	
	if(ret < 0)
	{
		web_ret = SERVER_RET_INVAID_PARM_VALUE;
	}
	
EXIT:

	if(need_send == 1)
	{
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + sizeof(out);
		msgPacket(identifier, (unsigned char *)senddata, INT_TYPE, totallen, cmd, web_ret);
		memcpy(senddata + (totallen - sizeof(out)), &out, sizeof(out));
		printf("the cmd =%04x,the value=%d,the ret=%04x\n", cmd, out, web_ret);
		send(fd, senddata, totallen, 0);
		
		if(web_ret != SERVER_RET_OK)
		{
			printf("ERROR,the cmd =0x%x,ret= 0x%x\n", cmd, web_ret);
		}
	}
	
	return 0;
}

static void PackHeaderMSG(BYTE *data,
                          BYTE type, WORD len)
{
	MSGHEAD  *p;
	p = (MSGHEAD *)data;
	memset(p, 0, HEAD_LEN);
	p->nLen = htons(len);
	p->nMsg = type;
	return ;
}


//判断头文件是否匹配
//先阶段只匹配product_id
static int  app_compare_header_info(UP_HEADER_INFO *old, UP_HEADER_INFO *newinfo)
{
	if(strcmp(old->product_id, newinfo->product_id) == 0) {
		return 0;
	}

	printf("[app_compare_header_info] old->product_id : [%s], newinfo->product_id : [%s]\n", old->product_id, newinfo->product_id);
	return -1;
}
//#define UPDATE_PRODUCT_ID "ENC1200DVI"   //1200 dvi/sdi 是一样的
static int set_markinfo(UP_HEADER_INFO *markinfo)
{
	memset(markinfo, 0, sizeof(UP_HEADER_INFO));
	memcpy(markinfo->product_id, UPDATE_PRODUCT_ID, 15);
	return 0;
}

//读入256字节，确认是否是带头的。
//0 表示是带正确header
// -2 表示不带header头
//-1 表示带header头，但是不正确
int app_header_info_ok(char *name)
{
	char buff[320] = {0};
	FILE *fp = NULL;
	unsigned int fp_len = 0;
	int ret = 0;
	fp = fopen(name, "r");

	if(fp == NULL) {
		printf("ERROR,fopen the file %s is failed \n", name);
		return 2;
	}

	fp_len = get_file_size(fp);
	printf("21the old file  len is %ld.\n", fp_len);

	if(fp_len <= 256) {
		printf("1ERROR!\n");
		fclose(fp);
		fp = NULL;
		return 2;
	}

	ret = fread(buff, 1, 256, fp);

	if(ret < 256) {
		printf("2ERROR\n");
		fclose(fp);
		fp = NULL;
		return 2;
	}

	fclose(fp);
	fp = NULL;

	if(memcmp(buff, FIRST_UPDATE_HEADER, 32) != 0) {

		return 2;
	}

	UP_HEADER_INFO newinfo, markinfo;
	set_markinfo(&markinfo);
	memset(&newinfo, 0, sizeof(UP_HEADER_INFO));
	memcpy(&newinfo, buff + 32, sizeof(UP_HEADER_INFO));

	return app_compare_header_info(&markinfo, &newinfo);
}

int app_del_extern_buff(char *name)
{
	FILE *fp_old = NULL;
	FILE *fp = NULL;
	//int ret = 0;
	unsigned int fp_len = 0;
	unsigned int fp_old_len = 0;
	char filename[256] = {0};
	char tempbuff[320] = {0};
	fp_old = fopen(name, "r");

	if(fp_old == NULL) {
		PRINTF("ERROR,fopen the file %s is failed \n", name);
		return -1;
	}

	memset(tempbuff, 0, sizeof(tempbuff));

	if(fread(tempbuff, 1, UPDATE_HEARER_LEN, fp_old) != UPDATE_HEARER_LEN) {
		PRINTF("ERROR,this file is to small\n");
		return -1;
	}

	sprintf(filename, "%s_temp", name);
	fp = fopen(filename, "w+");

	if(fp == NULL) {
		fclose(fp_old);
		PRINTF("ERROR,fopen the file %s is failed \n", filename);
		return -1;
	}

	fp_old_len = get_file_size(fp_old);
	PRINTF("23the old file  len is %ld.\n", fp_old_len);

	if(fp_old_len < 256) {
		fclose(fp_old);
		fclose(fp);
		fp = fp_old = NULL;
		return -1;
	}



	char buff[1024 * 100] = {0};
	int read_len = 0;
	int write_len = 0;

	read_len = fread(buff, 1, 256, fp_old);
	PRINTF("fread read_len = %d\n", read_len);

	if(read_len < 256) {
		fclose(fp);
		fp = fp_old = NULL;
		return -1;
	}

	while(feof(fp_old) == 0) {
		memset(buff, 0, sizeof(buff));
		read_len = fread(buff, 1, sizeof(buff), fp_old);
		//PRINTF("fread read_len = %d\n",read_len);
		write_len = fwrite(buff, 1, read_len, fp);

		//PRINTF("fwrite read_len = %d\n",write_len);
		if(read_len != write_len) {
			fclose(fp);
			fp = fp_old = NULL;
			PRINTF("error\n");
			return -1;
		}
	}


	fp_len = get_file_size(fp);
	PRINTF("1the new file  len is %ld.\n", fp_len);

	fclose(fp);
	fclose(fp_old);
	sync();
	return 0;
}

static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	//	pthread_mutex_lock(&gSetP_m.send_m);
	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			printf("WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, errno, s);

			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr, "network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}

			//	pthread_mutex_unlock(&gSetP_m.send_m);
			return nRet;
		} else if(nRet == 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen,  errno, s);
			fprintf(stderr, "Send Net Data Error nRet= %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	//	pthread_mutex_unlock(&gSetP_m.send_m);
	return nWriteLen;
}


/*set GPIO port Bit value*/
static int SetGPIOBitValue(int fd, int bit)
{
	R_GPIO_data op;
	int ret ;

	op.gpio_value = 1 ;
	op.gpio_num = bit;
	ret = ioctl(fd, SET_GPIO, &op);

	if(ret < 0)	{
		return -1 ;
	}

	return 0;
}

/*clear GPIO port Bit value*/
static int ClearGPIOBitValue(int fd, int bit)
{
	R_GPIO_data op;
	int ret ;

	op.gpio_value = 0 ;
	op.gpio_num = bit;
	ret = ioctl(fd, SET_GPIO, &op);

	if(ret < 0)	{
		return -1 ;
	}

	return 0;
}
/*get GPIO port Bit value*/
static int GetGPIOBitValue(int fd, int bit)
{

	R_GPIO_data op;
	int ret , val = -1 ;

	op.gpio_value = 0 ;
	op.gpio_num = bit;
	ret = ioctl(fd,	GET_GPIO, &op);

	if(ret < 0)	{
		return -1 ;
	}

	val = op.gpio_value;
	return (val);
}





static int set_gpio_bit(int bit, int fd)
{
	int ret = 0;
	ret = SetGPIOBitValue(fd, bit);

	if(ret < 0) {
		PRINTF("set_gpio_bit() failed!!!\n ");
		return -1 ;
	}

	PRINTF("balance mode switch !!!!bit=%d\n",bit);
	return 0;
}



static int clear_gpio_bit(int bit, int fd)
{
	int ret = 0;
	ret = ClearGPIOBitValue(fd, bit);

	if(ret < 0) {
		PRINTF("clear_gpio_bit() failed!!!\n ");
		return -1 ;
	}

	return 0;
}

static int get_gpio_bit(int bit, int fd)
{
	int ret =  -1;
	ret = GetGPIOBitValue(fd, bit);

	if(ret < 0) {
		PRINTF("GetGPIOBitValue() failed!!!\n ");
		return -1 ;
	}

	return (ret);
}


#define PER_READ_LEN  256
/*升级FPGA程序*/
static int back_updateFpgaProgram(const char *fpgafile, int fd)
{
	int ret = 0;
	char spidata[PER_READ_LEN] = {0};
	int spifd = -1;
	FILE *fpgafp = NULL;
	int readlen = 0;
	int writelen = 0;
	int totalwritelen  = 0;

	PRINTF("Enter into update FPGA Program \n");

//	ret = set_gpio_bit(41, fd); //写保护

	ret = clear_gpio_bit(FLASH_ENABLE, fd);
	PRINTF("FLASH_ENABLE ret=%x \n", ret);

	system("flash_eraseall /dev/mtd0");
	spifd =  open("/dev/mtd0", O_RDWR, 0);

	if(spifd < 0)	{
		PRINTF("open the SPI flash Failed \n");
		ret = -1;
		goto cleanup;
	}

	fpgafp = fopen(fpgafile, "r+b");

	if(fpgafp == NULL)	{
		PRINTF("open the FPGA bin Failed \n");
		ret = -1;
		goto cleanup;
	}

	rewind(fpgafp);

	while(1) {
		readlen = fread(spidata, 1, PER_READ_LEN, fpgafp);

		if(readlen < 1)	{
			PRINTF("file read end \n");
			break;
		}

		writelen = write(spifd, spidata, readlen);
		PRINTF("writelen = %d \n", writelen);
		totalwritelen += writelen;

		if(feof(fpgafp)) {
			PRINTF("writelen = %d \n", writelen);
			writelen = write(spifd, spidata, readlen);
			break;
		}
	}

	close(spifd);
	spifd  = -1;
	PRINTF("totalwritelen = %d \n", totalwritelen);
cleanup:
	PRINTF("002 flash_eraseall updateFpgaProgram \n");
	ret = set_gpio_bit(FLASH_ENABLE, fd);
	PRINTF("FLASH_ENABLE ret=%x \n", ret);

	if(spifd > 0) {
		close(spifd);
	}

	if(fpgafp) {
		fclose(fpgafp);
	}

	ret = get_gpio_bit(FPGA_DONE, fd);

	while(!ret) {
		ret = get_gpio_bit(FPGA_DONE, fd);
	}

	ret = clear_gpio_bit(FPGA_PRO, fd);
	ret = set_gpio_bit(FPGA_PRO, fd);
	PRINTF("FPGA_PRO ret2=%x \n", ret);
	PRINTF("gpio done   bit = %d\n", ret);
	return ret;
}

int back_updatefpga(void)
{
	printf("[back_updatefpga] start ...\n");
	int ret = 0;
	int fd = -1;
	char fpga_file[256] = {0};
	char cmd[256] = {0};
	/*更新FPGA*/
	sprintf(fpga_file, "%s", BACK_FPGAFILENAME);

	if(access(fpga_file, F_OK) == 0) {
		fd = GPIOInit();
		if(fd < 0) {
			return -1;
		}
		PRINTF("----begin to update fpga---\n");
		ret = back_updateFpgaProgram(fpga_file, fd);
		PRINTF("----end to update fpga---\n");
		memset(cmd, 0, 256);
		sprintf(cmd, "rm -rf %s", fpga_file);
		system(cmd);
		printf("cmd:[%s]\n", cmd);
	} else {
		PRINTF("have no file [%s]\n", fpga_file);
	}
	printf("[back_updatefpga] end ...\n");
	return ret;
}

static void remove_config(void)
{
	char cmd[256] = {0};
	int ret=0;
	sprintf(cmd, "rm -rf %s", BACK_CONFIG);
	ret=system(cmd);
	PRINTF("ret=%d,%s\n",ret,cmd);
}

static int update_pkg(int fd, char *filename)
{
#ifdef  ADD_UPDATE_HEADER

	int ret = -1;
	unsigned char temp[20] = {0};
	PRINTF("ADD_UPDATE_HEADER start...\n");
	char command[256] = {0};
	ret = app_header_info_ok(filename);
	PRINTF("ADD_UPDATE_HEADER start...ret : [%d]\n", ret);

	if(ret == 0) {
		if(app_del_extern_buff(filename) == -1) {
			PRINTF("error! del the file extern_buff is error\n");
			return -1;
		}

		sprintf(command, "rm -rf %s;mv %s_temp %s", filename, filename, filename);
		PRINTF("success del extern header,command=%s\n", command);
		system(command);

	} else {
		ret = -1;
	}

	PRINTF("ADD_UPDATE_HEADER end...\n");
#endif
	memset(command, 0, 256);
	sprintf((char *)command, "tar xfvz %s -C %s", filename,"/mnt/nand");
	PRINTF("[%s]\n", command);
	if(0 != system(command))
	{
		ret = -1;
	}
	back_updatekernel();
	back_updatefpga();
	system("sync");
	return ret;
}

static int create_null_file(int8_t *filename)
{
	int fd = -1;

	if((fd = open(filename, O_CREAT)) < 0) {
		PRINTF("open[%s]  : %s", filename, strerror(errno));
		return -1;
	}
	close(fd);
	return 0;
}


int SinglemidParseString(int identifier, int fd, char *data, int len)
{
	PRINTF("[SinglemidParseString] start ...\n");
	int recvlen;
	int cmd = 0;
	char actdata[4096] = {0};
	int vallen = 0;
	
	char senddata[1024] = {0};
	int totallen = 0;
	int ret = -1;
	char  out[4096] = "unknown cmd.";
	int web_ret = SERVER_RET_OK;
	int need_send = 0;
	int update_ret = -1;
	recvlen = recv(fd, data, len, 0);
	
	vallen = len - sizeof(int);
	
	if(recvlen < 0 || vallen > sizeof(actdata))
	{
		web_ret = SERVER_RET_INVAID_PARM_LEN;
		need_send = 1;
		goto EXIT;
	}

	memset(out, 0, 4096);
	memcpy(&cmd, data, sizeof(int));
	memcpy(actdata, data + sizeof(int), vallen);
	PRINTF("cmd = %04x\n", cmd);
	
	switch(cmd)
	{
		case MSG_SETUSRPASSWORD:
			{
				PRINTF("MSG_SETUSRPASSWORD %s\n",actdata);

				need_send = 1;
			}
			break;
		case MSG_GETUSRPASSWORD:
			{
				PRINTF("MSG_GETUSRPASSWORD %s\n", actdata);
				strcpy((char *)out, (const char *)"admin");
				need_send = 1;
			}
			break;
			
		case MSG_SYSUPGRADE:
			{
				int ret=0;
				PRINTF("MSG_SYSUPGRADE [%s]\n", actdata);
				char cmd[1024] = {0};


				system(UBI_ATTACH_MAIN_FILESYS);
				if(mount_ok)
				{
					remove_config();
					if(update_ret = update_pkg(fd, UPDATE_FILE) < 0) {
						web_ret = SERVER_VERIFYFILE_FAILED;
					} 
				}
				else
				{
					web_ret = SERVER_VERIFYFILE_FAILED;
				}
			

		
				strcpy(out, (char *)"MSG_SYSUPGRADE");
				need_send = 1;
			
			}
		break;
		
		case MSG_SYSROLLBACK:
			{	
				
				strcpy(out, (char *)"MSG_SYSROLLBACK");
				need_send = 1;
			}
		break;
		case MSG_SETSERIALNUM:
			{
				need_send = 1;
			}
		break;
		default
				:
			printf("Warnning,the cmd %d is UNKOWN\n", cmd);
			need_send = 1;
			break;
	}
	
EXIT:
	
	if(need_send == 1)
	{
		totallen = MSGINFOHEAD + sizeof(cmd) + sizeof(web_ret) + strlen(out);
		msgPacket(identifier, (unsigned char *)senddata, STRING_TYPE, totallen, cmd, web_ret);
		memcpy(senddata + (totallen - strlen(out)), out, strlen(out));
		printf("the cmd =%04x,the out=%s,the ret=%04x\n", cmd, out, web_ret);
		ret = send(fd, senddata, totallen, 0);
		
		if(web_ret != SERVER_RET_OK)
		{
			PRINTF("ERROR,the cmd =0x%x,ret= 0x%x\n", cmd, web_ret);
		}
		
		if(totallen == ret && 0 ==update_ret) {
			sleep(3);
			system("reboot -f");
		}

	}
	
	return 0;
}


void *Singleweblisten()
{
	printf("Singleweblisten start\n");
	void                   *status              = 0;
	int 					listenSocket  		= 0 , flags = 0;
	struct sockaddr_in 		addr;
	int len, client_sock, opt = 1;
	struct sockaddr_in client_addr;
	webMsgInfo		webinfo;
	ssize_t			recvlen;
	char  data[2048] = {0};
	
	
	len = sizeof(struct sockaddr_in);
	memset(&client_addr, 0, len);
	listenSocket =	socket(PF_INET, SOCK_STREAM, 0);
	
	if(listenSocket < 1)
	{
		status  = THREAD_FAILURE;
		return status;
	}
	
	memset(&addr, 0, sizeof(struct sockaddr_in));
	addr.sin_family =       AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(WEBSERVER);
	
	setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	
	if(bind(listenSocket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
	{
		printf("[weblistenThrFxn] bind failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}
	
	if(-1 == listen(listenSocket, MAX_LISTEN))
	{
		printf("listen failed,errno = %d,error message:%s \n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status;
	}
	
	
	if((flags = fcntl(listenSocket, F_GETFL, 0)) == -1)
	{
		printf("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status ;
	}
	
	if(fcntl(listenSocket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		printf("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));
		status  = THREAD_FAILURE;
		return status ;
	}
	
	while(1)
	{
		fd_set rfds;
		FD_ZERO(&rfds);
		FD_SET(listenSocket, &rfds);
		
		//接收recv_buf 复位为空!
		select(FD_SETSIZE, &rfds , NULL , NULL , NULL);
		
		client_sock = accept(listenSocket, (struct sockaddr *)&client_addr, (socklen_t *)&len);
		
		if(0 > client_sock)
		{
			printf("\n");
			
			if(errno == ECONNABORTED || errno == EAGAIN)
			{
				//usleep(20000);
				continue;
			}
			
			printf("weblisten thread Function errno  = %d\n", errno);
			status  = THREAD_FAILURE;
			return status;
		}
		
		memset(&webinfo, 0, sizeof(webinfo));
		recvlen = recv(client_sock, &webinfo, sizeof(webinfo), 0);

		       
		if(recvlen < 1)
		{
			printf("recv failed,errno = %d,error message:%s,client_sock = %d\n", errno, strerror(errno), client_sock);
			status  = THREAD_FAILURE;
			return status;
		}
		
		if(webinfo.identifier != WEB_IDENTIFIER)
		{
			printf("id  error,client_sock = %d\n", client_sock);
			status  = THREAD_FAILURE;
			return status;
		}
		
		len = webinfo.len - sizeof(webinfo);
		
		printf("----> web deal begin =%d %d\n", webinfo.type,len );
		
		switch(webinfo.type)
		{
			case INT_TYPE:
				SinglemidParseInt(webinfo.identifier, client_sock, data, len);
				break;
				
			case STRING_TYPE:
				SinglemidParseString(webinfo.identifier, client_sock, data, len);
				break;
				
			case STRUCT_TYPE:
				SinglemidParseStruct(webinfo.identifier, client_sock, data, len);
				break;
				
			default
					:
				break;
		}
		
		printf("----> web deal end =%d\n", webinfo.type);
		close(client_sock);
	}
	
	
	close(listenSocket);
	printf("Web listen Thread Function Exit!!\n");
	return status;
}




int app_weblisten_init(void)
{
	pthread_t           webListen;

	system("mkdir -p /var/log/reach");
	system(UBI_ATTACH_MAIN_FILESYS);
	if(0 == system("mount -t ubifs ubi1:rootfs /mnt/nand"))
	{
		mount_ok = 1;
		printf("[app_weblisten_init] mount ok!");
	}
	if(pthread_create(&webListen, NULL, Singleweblisten, NULL))
	{
		printf("Failed to create web listen thread\n");
		return -1;
	}
	printf("app_weblisten_init\n");
	return 0;
}

