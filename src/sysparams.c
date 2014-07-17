/*****************************************************************************************
*     描述：1.网络相关信息,产品型号信息等的保存，读取和设置实现
*           2.板卡的串号等相关地址的读取
*
*
*
*
*
*
*
*
*
******************************************************************************************/

//mutex
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include "reach_enc2000.h"
#include "MMP_control.h"
#include "log_common.h"
#include "sysparams.h"
#include "extern_update_header/app_update_header.h"
#include "build_info.h"
#include "middle_control.h"
#include "../version.h"
#include "app_version.h"
#include "app_signal.h"
#include "app_network.h"
#include "app_head.h"
#include "app_update.h"

#define HEAD_LEN			sizeof(MSGHEAD)


//定义产品型号，串号，产品 name等信息, save in factory.ini 和product.ini
typedef struct _SYS_PRODUCT_INFO_ {
	char strName[16];      //encode box name
	char serialNum[32];     //serialNum  ,just init by factory
	unsigned	int  producttype ;  // 1:enc2000; 2:enc2060
	char bType;              //encodemange need this type
} SysProductInfo;

typedef struct _Version_Info {
	char app_version[32];
	char kernel_version[32];
	char fpga_version[32];
	char built_time[64];
} Version_Info;

static SysNetworkInfo g_network_info;
static SysProductInfo g_product_info;

static Version_Info g_curr_version;



//创建并初始化配置
static void create_product_info(SysProductInfo *info)
{
	info->bType = 6; //enc1200
	info->producttype = ENC2000;
	snprintf(info->serialNum, sizeof(info->serialNum), "%s", "815896475");
	snprintf(info->strName, sizeof(info->strName), "%s", DEV_ID);
	return ;
}
//读取产品化相关信息
static int read_product_info(SysProductInfo *info)
{

	int ret = 0;
	char temp[256];
	char title[64];
	char config_file[64] = PRODUCT_CONFIG;

	memcpy(title, "product", 64);

	ret =  ConfigGetKey((char *)config_file, title, "name", info->strName);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		return -1;
	}

	ret =  ConfigGetKey((char *)config_file, title, "serialNum", info->serialNum);

	if(ret != 0) {
		PRINTF("Failed to read serialNum\n");
		return -1;
	}

	ret =  ConfigGetKey((char *)config_file, title, "type", temp);

	if(ret != 0) {
		PRINTF("Failed to read type\n");
		return -1;
	}

	info->producttype = atoi(temp);

	ret =  ConfigGetKey((char *)config_file, title, "MMP_type", temp);

	if(ret != 0) {
		PRINTF("Failed to read MMP_type\n");
		return -1;
	}

	info->bType = atoi(temp);

	return 0;
}
//保存产品化相关信息
static int write_product_info(SysProductInfo *info)
{
	int ret = 0;
	char temp[256] = {0};
	char title[64] = {0};
	char *config_file = PRODUCT_CONFIG;

	MMPSysParam net_info ;
	//	struct in_addr int_addr;

	memset(&net_info, 0, sizeof(MMPSysParam));
	MMP_get_sysparam(0, &net_info, MAIN_MMP);

	memcpy(title, "product", 64);


	ret =  ConfigSetKey(config_file, title, "name", info->strName);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

	ret =  ConfigSetKey(config_file, title, "serialNum", info->serialNum);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

	sprintf(temp, "%d", info->producttype);
	ret =  ConfigSetKey(config_file, title, "type", temp);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

	sprintf(temp, "%d", info->bType);
	ret =  ConfigSetKey(config_file, title, "MMP_type", temp);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}


	//write to factory.ini
	return 0;
}

//获取产品型号
int sys_get_product_type(void)
{
	return ENC2000;
	//	return g_product_info.producttype;
}

//设置串号并保存
int app_set_serialno(char *serialno)
{
	if(serialno == NULL) {
		ERR_PRN("serial no is NULL\n");
		return -1;
	}

	PRINTF("serialno=%s\n", serialno);

	if(strcmp(g_product_info.serialNum, serialno) != 0) {
		snprintf(g_product_info.serialNum, sizeof(g_product_info.serialNum), "%s", serialno);
		write_product_info(&g_product_info);
		PRINTF("change serial NO = [%s]\n", g_product_info.serialNum);
		return 0;
	}

	return 0;
}
int app_get_serialno(char *serialno)
{
	if(serialno != NULL && g_product_info.serialNum != NULL) {
		sprintf(serialno, "%s", g_product_info.serialNum);
	}

	return 0;
}

int app_get_device_type(char *data, int vallen)
{
	/*获得设备型号*/
	DEBUG(DL_DEBUG, "Get Device Type\n");

	if(data != NULL) {
		memcpy(data, g_product_info.strName, vallen);
	}

	PRINTF("device : %s\n", data);
	return 0;
}

void print_product_info(SysProductInfo *product_info)
{
	printf("product_info strName:[%s] serialNum:[%s] producttype:[%d] bType:[%d]\n",
	       product_info->strName,
	       product_info->serialNum,
	       product_info->producttype,
	       product_info->bType);
}

//应用初始化产品相关信息
void app_init_product_info(void)
{
	int ret = 0;
	memset(&g_product_info, 0, sizeof(SysProductInfo));
	create_product_info(&g_product_info);
	ret = read_product_info(&g_product_info);
	print_product_info(&g_product_info);
	return ;
}


//////////////////////////////获取版本////////////////////////////////////////////
static int read_version_info(Version_Info *curr, Version_Info *prev)
{
	char title[64] ;
	//	char temp[512] = {0};
	int ret  = 0 ;
	const char config_file[64] = VERSION_CONFIG;

	memcpy(title,  "current", 20);

	ret =  ConfigGetKey((char *)config_file, title, "app", curr->app_version);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("app_version=%s\n", curr->app_version);

	ret =  ConfigGetKey((char *)config_file, title, "kernel", curr->kernel_version);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("kernel_version=%s\n", curr->kernel_version);

	ret =  ConfigGetKey((char *)config_file, title, "fpga", curr->fpga_version);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("fpga_version=%s\n", curr->fpga_version);
#if 0
	ret =  ConfigGetKey((char *)config_file, title, "built_time", curr->built_time);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("built_time=%s\n", curr->built_time);
#endif


	memcpy(title, "previous", 20);

	ret =  ConfigGetKey((char *)config_file, title, "app", prev->app_version);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("app_version=%s\n", prev->app_version);

	ret =  ConfigGetKey((char *)config_file, title, "kernel", prev->kernel_version);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("kernel_version=%s\n", prev->kernel_version);

	ret =  ConfigGetKey((char *)config_file, title, "fpga", prev->fpga_version);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("fpga_version=%s\n", prev->fpga_version);
#if 0
	ret =  ConfigGetKey((char *)config_file, title, "built_time", prev->built_time);

	if(ret != 0) {
		PRINTF("Failed to read version\n");
		//return -1;
	}

	PRINTF("built_time=%s\n", prev->built_time);
#endif
	return 0;
}

static int write_version_info(Version_Info *curr, Version_Info *prev)
{
	char title[64] ;
	//	char temp[512] = {0};
	int ret  = 0 ;
	const char config_file[64] = VERSION_CONFIG;

	memcpy(title, "current", 20);

	ret =  ConfigSetKey((char *)config_file, title, "app", curr->app_version);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

	ret =  ConfigSetKey((char *)config_file, title, "kernel", curr->kernel_version);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

	ret =  ConfigSetKey((char *)config_file, title, "fpag", curr->fpga_version);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

#if 0
	ret =  ConfigSetKey((char *)config_file, title, "built_time", curr->built_time);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

#endif


	memcpy(title, "previous", 20);


	ret =  ConfigSetKey((char *)config_file, title, "app", prev->app_version);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

	ret =  ConfigSetKey((char *)config_file, title, "kernel", prev->kernel_version);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

	ret =  ConfigSetKey((char *)config_file, title, "fpag", prev->fpga_version);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

#if 0
	ret =  ConfigSetKey((char *)config_file, title, "built_time", prev->built_time);

	if(ret != 0) {
		PRINTF("Failed to write version\n");
		return -1;
	}

#endif
	return 0;
}

int app_get_soft_version(char *buff, int len)
{
	char *tmp = NULL;

	if(buff != NULL) {
		tmp = g_curr_version.kernel_version;
		snprintf(buff, len, "%s(%s/%s)", g_curr_version.app_version, tmp + strlen("ENC2000 "), g_curr_version.fpga_version);
		PRINTF("soft version is %s\n", buff);
	}

	return 0;
}


int app_get_build_time(char *data)
{
	if(data != NULL) {
		sprintf(data,  "%s", g_curr_version.built_time);
		PRINTF("build_date:%s\n", data);
	} else {
		PRINTF("WARNING: data is NULL!\n");
	}

	return 0;
}


void app_init_version_info()
{
	Version_Info temp_version;
	Version_Info prev_version;
	int gpio_fd = gEnc2000.gpiofd;
	memset(&temp_version, 0, sizeof(Version_Info));

	memset(&prev_version, 0, sizeof(Version_Info));
	memset(&g_curr_version, 0, sizeof(Version_Info));

	//read from flash last version
	read_version_info(&temp_version, &prev_version);
	PRINTF("............\n");

	//set current version info
	snprintf(g_curr_version.built_time, sizeof(g_curr_version.built_time), "%s", g_make_build_date);
	snprintf(g_curr_version.app_version, sizeof(g_curr_version.app_version), "%s", SOFT_VERSION);

	if(gpio_fd > 0) {
		ioctl(gpio_fd, 0x11111111, g_curr_version.kernel_version);
	} else {
		snprintf(g_curr_version.kernel_version, sizeof(g_curr_version.kernel_version), "0000");
	}

	snprintf(g_curr_version.fpga_version, sizeof(g_curr_version.fpga_version), "0x%x", app_get_fpga_version());

	PRINTF("current time =%s,app_soft=[%s],kernal_version=[%s],fpga_version=[%s]\n", g_curr_version.built_time, g_curr_version.app_version, g_curr_version.kernel_version, g_curr_version.fpga_version);


	if((strcmp(g_curr_version.app_version, temp_version.app_version) != 0)
	   || (strcmp(g_curr_version.kernel_version, temp_version.kernel_version) != 0)
	   || (strcmp(g_curr_version.fpga_version, temp_version.fpga_version) != 0)) {
		PRINTF("current version is change ,need to save\n");
		PRINTF("app_soft=[%s],kernal_version=[%s],fpga_version=[%s]\n", temp_version.app_version, temp_version.kernel_version, temp_version.fpga_version);
		//verison not need read from file ,but need save to file
		write_version_info(&g_curr_version, &prev_version);
	}

	PRINTF("Init version successed!\n");

}


//创建并初始化网络信息
static void create_network_info(int ptype, SysNetworkInfo *info)
{
	memset(info, 0, sizeof(SysNetworkInfo));

	info->szMacAddr[0] = 0x00;
	info->szMacAddr[1] = 0x09;
	info->szMacAddr[2] = 0x30;
	info->szMacAddr[3] = 0x38;
	info->szMacAddr[4] = 0x22;
	info->szMacAddr[5] = 0x92;


	info->dhcp[0] = 0;
	info->dwAddr[0] = inet_addr(IP_ADDR_1);
	info->dwGateWay[0] =  inet_addr("192.168.4.254");
	info->dwNetMask[0] = inet_addr("255.255.255.0");
	info->dwMaindns[0] = inet_addr("192.168.4.1");
	info->dwSlavedns[0] = inet_addr("192.168.4.1");

	if(ENC2000 == ptype) {
		info->dhcp[1] = 0;
		info->dwAddr[1] = inet_addr(IP_ADDR_2);
		info->dwGateWay[1] =  inet_addr("192.168.4.254");
		info->dwNetMask[1] = inet_addr("255.255.255.0");
		info->dwMaindns[1] = inet_addr("192.168.4.1");
		info->dwSlavedns[1] = inet_addr("192.168.4.1");
	}

	return ;

}




static int write_network_info()
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	//	int 			enable = 0;
	int 			rst = -1;
	const char config_file[64] = NETWORK_CONFIG;
	//pthread_mutex_lock(&gSetP_m.save_sys_m);
	char    title[20] = "system";
	MMPSysParam net_info ;
	struct in_addr int_addr;

	memset(&net_info, 0, sizeof(MMPSysParam));
	MMP_get_sysparam(0, &net_info, MAIN_MMP);
	memcpy(&int_addr, &(net_info.dwAddr), 4);

	PRINTF("eth0:ip=%s\n", inet_ntoa(int_addr));
	ret =  ConfigSetKey((char *)config_file, title, "eth0_ip", inet_ntoa(int_addr));

	if(ret != 0) {
		PRINTF("Failed to write eth0 ip\n");
		goto EXIT;
	}

	memset(&int_addr, 0, sizeof(int_addr));
	memcpy(&int_addr, &(net_info.dwNetMask), 4);
	PRINTF("eth0:mark=%s\n", inet_ntoa(int_addr));
	ret =  ConfigSetKey((char *)config_file, title, "eth0_mark", inet_ntoa(int_addr));

	if(ret != 0) {
		PRINTF("Failed to write eth0 mark\n");
		goto EXIT;
	}

	memset(&int_addr, 0, sizeof(int_addr));
	memcpy(&int_addr, &(net_info.dwGateWay), 4);
	PRINTF("eth0:gateway=%s\n", inet_ntoa(int_addr));
	ret =  ConfigSetKey((char *)config_file, title, "eth0_gateway", inet_ntoa(int_addr));

	if(ret != 0) {
		PRINTF("Failed to write eth0 gateway\n");
		goto EXIT;
	}

	JoinMacAddr(temp, net_info.szMacAddr, 8);
	PRINTF("eth0:MAC=%s\n", temp);
	ret =  ConfigSetKey((char *)config_file, title, "eth0_Mac", temp);

	if(ret != 0) {
		PRINTF("Failed to eth0 mac\n");
		goto EXIT;
	}


	memset(&net_info, 0, sizeof(MMPSysParam));
	MMP_get_sysparam(1, &net_info, SECOND_MMP);
	memcpy(&int_addr, &(net_info.dwAddr), 4);
	PRINTF("eth1:ip=%s\n", inet_ntoa(int_addr));
	ret =  ConfigSetKey((char *)config_file, title, "eth1_ip", inet_ntoa(int_addr));

	if(ret != 0) {
		PRINTF("Failed to write eth1 ip\n");
		goto EXIT;
	}

	memset(&int_addr, 0, sizeof(int_addr));
	memcpy(&int_addr, &(net_info.dwNetMask), 4);
	PRINTF("eth1:mark=%s\n", inet_ntoa(int_addr));
	ret =  ConfigSetKey((char *)config_file, title, "eth1_mark", inet_ntoa(int_addr));

	if(ret != 0) {
		PRINTF("Failed to write eth1 mark!\n");
		goto EXIT;
	}

	memset(&int_addr, 0, sizeof(int_addr));
	memcpy(&int_addr, &(net_info.dwGateWay), 4);
	PRINTF("eth1:gateway=%s\n", inet_ntoa(int_addr));
	ret =  ConfigSetKey((char *)config_file, title, "eth1_gateway", inet_ntoa(int_addr));

	if(ret != 0) {
		PRINTF("Failed to write eth1 gateway!\n");
		goto EXIT;
	}


	JoinMacAddr(temp, net_info.szMacAddr, 8);
	PRINTF("eth1:MAC=%s\n", temp);
	ret =  ConfigSetKey((char *)config_file, title, "eth1_Mac", temp);

	if(ret != 0) {
		PRINTF("Failed to eth1  mac!\n");
		goto EXIT;
	}

	rst = 1;
EXIT:
	return rst;
}


static int read_network_info(SysNetworkInfo *net)
{
	char 			temp[512] = {0};
	int 			ret  = 0 ;
	int 			rst = -1;
	const char config_file[64] = NETWORK_CONFIG;
	char    title[32] = "system";

	//pthread_mutex_lock(&gSetP_m.save_sys_m);

	ret =  ConfigGetKey((char *)config_file, title, "eth0_ip", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth0_ip\n");
		goto EXIT;
	}

	net->dwAddr[0] = inet_addr(temp);
	PRINTF("eth0_ip=%s\n", temp);


	ret =  ConfigGetKey((char *)config_file, title, "eth0_mark", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth0_mark\n");
		goto EXIT;
	}

	net->dwNetMask[0] = inet_addr(temp);
	PRINTF("eth0_mark=%s\n", temp);


	ret =  ConfigGetKey((char *)config_file, title, "eth0_gateway", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth0_gateway\n");
		goto EXIT;
	}

	net->dwGateWay [0] = inet_addr(temp);
	PRINTF("eth0_gateway=%s\n", temp);

	memset(temp, 0, sizeof(temp));
	ret =  ConfigGetKey((char *)config_file, title, "eth0_Mac", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth0_Mac\n");
		goto EXIT;
	}

	PRINTF("eth0:MAC=%s\n", temp);
	SplitMacAddr(temp, net->szMacAddr, 8);


	ret =  ConfigGetKey((char *)config_file, title, "eth1_ip", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth1_ip\n");
		goto EXIT;
	}

	net->dwAddr[1] = inet_addr(temp);
	PRINTF("eth1_ip=%s\n", temp);


	//sprintf(temp, "%d",inet_ntoa(net_info.dwNetMark));
	ret =  ConfigGetKey((char *)config_file, title, "eth1_mark", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth1_mark\n");
		goto EXIT;
	}

	net->dwNetMask[1] = inet_addr(temp);
	PRINTF("eth1_mark=%s\n", temp);


	//sprintf(temp, "%d",inet_ntoa(net_info.dwGateWay));
	ret =  ConfigGetKey((char *)config_file, title, "eth1_gateway", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth1_gateway\n");
		goto EXIT;
	}

	net->dwGateWay[1] = inet_addr(temp);
	PRINTF("eth1_gateway=%s\n", temp);

	//	sprintf(temp,"%02X-%02X-%02X-%02X-%02X-%02X",net_info.szMacAddr[0],net_info.szMacAddr[1],net_info.szMacAddr[2],net_info.szMacAddr[3],net_info.szMacAddr[4],net_info.szMacAddr[5]);
	ret =  ConfigGetKey((char *)config_file, title, "eth1_Mac", temp);

	if(ret != 0) {
		PRINTF("Failed to Get eth1_Mac\n");
		goto EXIT;
	}

	PRINTF("eth1:MAC=%s\n", temp);
	SplitMacAddr(temp, net->szMacAddr, 8);

	rst = 1;
EXIT:
	return rst;
}



//eth0 ,eth0:1
//获取实际网口的网络信息
int sys_get_network(int eth, unsigned int *ip, unsigned int  *gateway, unsigned int  *netmask)
{
	if(eth != ETH0 && eth != ETH0_1) {
		ERR_PRN("eth %d is invaild\n", eth);
		return -1;
	}

	int dhcp = 0;
	dhcp = g_network_info.dhcp[eth];

	//static ip info
	if(dhcp == 0) {
		*ip = g_network_info.dwAddr[eth];
		*gateway = g_network_info.dwGateWay[eth];
		*netmask = g_network_info.dwNetMask[eth];
		return 0;
	} else if(dhcp == 1) {
		//just eht0 have dhcp
		if(eth != ETH0) {
			ERR_PRN("eth %d is invaild\n", eth);
			return -1;
		}

		*ip =	GetIPaddr("eth0");
		*netmask = GetNetmask("eth0");
		*gateway = GetBroadcast("eth0");
		return 0;
	}

	return -1;
}


static int sys_set_network(int eth, int dhcp, unsigned int ipaddr, unsigned int gatewayaddr, unsigned int netmaskaddr)
{
	if(eth != ETH0 && eth != ETH0_1) {
		ERR_PRN("eth %d is invaild\n", eth);
		return -1;
	}

	//	char cmd[256] = {0};
	//	char ip[16] = {0};
	//	char netmask[16] = {0};
	//	char gateway[16] = {0};
	//	struct in_addr	addr ;

	if(dhcp == 0) {
		SetEthConfigIP(eth, ipaddr, netmaskaddr);
		SetEthConfigGW(eth, gatewayaddr);
	} else if(dhcp == 1) {
		//just eht0 have dhcp
		if(eth != ETH0) {
			ERR_PRN("eth %d is invaild\n", eth);
			return -1;
		}

		SetEthDhcp();
	}

	return 0;
}
unsigned long atohex(char *str)
{
	unsigned long var = 0;
	unsigned long t;
	int len = strlen(str);

	if(len > 8) { //最长8位
		return -1;
	}

	for(; *str; str++) {
		if(*str >= 'A' && *str <= 'F') {
			t = *str - 55;    //a-f之间的ascii与对应数值相差55如'A'为65,65-55即为A
		} else if(*str >= 'a' && *str <= 'f') {
			t = *str - 87;    //a-f之间的ascii与对应数值相差55如'A'为65,65-55即为A
		} else {
			t = *str - 48;
		}

		var <<= 4;
		var |= t;
	}

	return var;
}

static int getMACaddr()
{
	//	unsigned char mac[8] = {0};
	char buf[64] = {0};
	unsigned char temp[8] = {0};
	int i = 0, j = 0, k = 0;
	FILE *fp = popen("ifconfig eth0| awk '/HWaddr/{printf(\"%s\", $5)}'", "r");
	fread(buf, 31, 1, fp);
	pclose(fp);
	PRINTF("mac:%s\n", buf);

	for(i = 0; i < 8; i++) {
		if(buf[j] == '\0') {
			break;
		}

		temp[0] = buf[j];
		temp[1] = buf[j + 1];
		temp[3] = '\0';
		PRINTF("temp=%s \n", temp);
		g_network_info.szMacAddr[i] = atohex(temp);
		PRINTF("%2x \n", g_network_info.szMacAddr[i]);
		j += 3;
	}

	return 0;
}


//设置mac地址
static int sys_change_mac(int eth, unsigned char *mac)
{
	if(eth > 1) {
		PRINTF("WARNING:eth is error!\n");
		return -1;
	}

	if(memcmp(mac, (g_network_info.szMacAddr), sizeof(g_network_info.szMacAddr)) != 0) {
		memcpy(g_network_info.szMacAddr, mac, sizeof(g_network_info.szMacAddr));
		return 1;
	}

	return 0;
}
//设置网络信息  if change,ret = 1;
static int sys_change_network_info(int eth, int dhcp, unsigned int ip, unsigned int gateway, unsigned int netmask)
{
	if((eth != ETH0 && eth != ETH0_1) || (dhcp != 0 && dhcp != 1)) {
		ERR_PRN("eth =%d ,dhcp=%d is invaild\n", eth, dhcp);
		return -1;
	}

	int ret = 0;
	int change = 0;

	struct in_addr addr1, addr2;
	//	char temp[200];

	//memset(temp, 0, sizeof(temp));
	memcpy(&addr1, &ip, 4);
	memcpy(&addr2, &(g_network_info.dwAddr[eth]), 4);


	if(dhcp == g_network_info.dhcp[eth] && dhcp == 1) {
		PRINTF("the %d eth network is not change\n", eth);
		return 0;
	}

	ret = CheckIPNetmask(ip, netmask, gateway);

	if(ret == 0) {
		PRINTF("Set IP addr error!!!!\n");
		return -1;
	}

	if(dhcp == 0) { //dhcp == 0

		PRINTF("new ip is #%s#,----old=#%s#\n", inet_ntoa(addr1), inet_ntoa(addr2));

		if(g_network_info.dwAddr[eth] != ip) {
			change = 1;
			g_network_info.dwAddr[eth] = ip;
		}

		if(g_network_info.dwGateWay[eth] != gateway) {
			change = 1;
			g_network_info.dwGateWay[ETH0] = gateway;
			g_network_info.dwGateWay[ETH0_1] = gateway;
		}

		if(g_network_info.dwNetMask[eth] != netmask) {
			change = 1;
			g_network_info.dwNetMask[eth] = netmask;
		}
	}

	if(dhcp != g_network_info.dhcp[eth]) {
		change = 1;
		g_network_info.dhcp[eth] = dhcp;
	}

	return change;
}

//初始化网络相关信息
void app_init_network_info(void)
{
	//	SysNetworkInfo net;
	int ret = 0;
	memset(&g_network_info, 0, sizeof(SysNetworkInfo));

	//	create_network_info(producttype, &g_network_info);
	//	read_network_info(producttype, &g_network_info);
	ret = read_network_info(&g_network_info);   //mac地址 不对

	if(ret < 0) {
		PRINTF("read_network_info failed!\n");

		create_network_info(sys_get_product_type(), &g_network_info);
	}

	getMACaddr();

	sys_set_network(ETH0, g_network_info.dhcp[0], g_network_info.dwAddr[0], g_network_info.dwGateWay[0], g_network_info.dwNetMask[0]);

	sys_set_network(ETH0_1, g_network_info.dhcp[1], g_network_info.dwAddr[1], g_network_info.dwGateWay[1], g_network_info.dwNetMask[1]);


	PRINTF("\n");
	system("ifconfig -a");
	PRINTF("\n");
	return ;

}

int app_get_network(Enc2000_Sys *out, int *out_len)
{
	unsigned int ip, gateway, netmask;
	MMPSysParam info1;
	MMPSysParam info2;
	ip = gateway = netmask = 0;
	getMACaddr();


	if(g_network_info.dhcp[0] == 1) {
		PRINTF("DHCP ip\n");
	}

	sys_get_network(ETH0, &ip, &gateway, &netmask);
	info1.dwAddr = ip;
	info1.dwGateWay = gateway;
	info1.dwNetMask = netmask;
	info1.szMacAddr[0] =  g_network_info.szMacAddr[0];
	info1.szMacAddr[1] =  g_network_info.szMacAddr[1];
	info1.szMacAddr[2] =  g_network_info.szMacAddr[2];
	info1.szMacAddr[3] =  g_network_info.szMacAddr[3];
	info1.szMacAddr[4] =  g_network_info.szMacAddr[4];
	info1.szMacAddr[5] =  g_network_info.szMacAddr[5];
	info1.szMacAddr[6] =  g_network_info.szMacAddr[6];
	info1.szMacAddr[7] =  g_network_info.szMacAddr[7];
	snprintf(info1.strName , sizeof(info1.strName), "%s", g_product_info.strName);
	snprintf(info1.strVer, sizeof(info1.strVer), "%s", SOFT_VERSION);
	info1.bType = g_product_info.bType;


	PRINTF("MIC:%2x,%2x,%2x,%2x,%2x,%2x\n", info1.szMacAddr[0], info1.szMacAddr[1], info1.szMacAddr[2],
	       info1.szMacAddr[2], info1.szMacAddr[3], info1.szMacAddr[4], info1.szMacAddr[5]);

	if(g_network_info.dhcp[1] == 1) {
		PRINTF("DHCP ip\n");
	}

	sys_get_network(ETH0_1, &ip, &gateway, &netmask);
	info2.dwAddr = ip;
	info2.dwGateWay = gateway;
	info2.dwNetMask = netmask;
	memcpy(info2.szMacAddr, g_network_info.szMacAddr, sizeof(info2.szMacAddr));
	snprintf(info2.strName , sizeof(info2.strName), "%s", g_product_info.strName);
	snprintf(info2.strVer, sizeof(info2.strVer), "%s", SOFT_VERSION);
	info2.bType = g_product_info.bType;

	memcpy(&(out->eth0), &info1, sizeof(MMPSysParam));
	memcpy(&(out->eth1), &info2, sizeof(MMPSysParam));
	*out_len = sizeof(Enc2000_Sys);
	PRINTF("g_product_info.strName=%s\n", g_product_info.strName);
	return 0;
}

int app_set_network(MMPSysParam *info1, MMPSysParam *info2)
{

	int ret = 0;
	int change[2] = {0};
	int mac_change[2] = {0};

	//	int dhcp = 0; //mmp need dhcp = 0
	if(info1->dwAddr == info2->dwAddr) {
		PRINTF("info1->dwAddr == info2->dwAddr\n");
		return -1;
	}

	change[0] = sys_change_network_info(ETH0, 0, info1->dwAddr, info1->dwGateWay, info1->dwNetMask);

	if(change[0] == -1) {
		PRINTF("ERROR network 1!\n");
		return -1;
	}

	mac_change[0] = sys_change_mac(ETH0, info1->szMacAddr);

	change[1] = sys_change_network_info(ETH0_1, 0, info2->dwAddr, info2->dwGateWay, info2->dwNetMask);

	if(change[1] == -1) {
		PRINTF("ERROR network 2!\n");
		return -1;
	}

	mac_change[1] = sys_change_mac(ETH0_1, info2->szMacAddr);

	PRINTF("change=%d \n\n", change);

	if(change[1] == 1 || mac_change[1] == 1
	   || change[0] == 1 || mac_change[0] == 1) {
		PRINTF("the network is change=%d \n", change);
		ret = 1;
		//	write_network_info(0, &g_network_info);
		write_network_info();
	}

	PRINTF(".ret=%d \n\n", ret);

	return ret;
}


int MMP_get_sysparam(int input, MMPSysParam *info, int mmp)
{
	unsigned int ip, gateway, netmask;
	ip = gateway = netmask = 0;
	int mp_status 	= get_mp_status();

	if(mp_status == IS_MP_STATUS) {
		if(mmp == MAIN_MMP) {
			input = SIGNAL_INPUT_1;
		} else if(mmp == SECOND_MMP) {
			input = SIGNAL_INPUT_2;
		}
	}

	if(input == SIGNAL_INPUT_1) {
		if(g_network_info.dhcp[0] == 1) {
			PRINTF("DHCP ip\n");
		}

		sys_get_network(ETH0, &ip, &gateway, &netmask);
		info->dwAddr = ip;
		info->dwGateWay = gateway;
		info->dwNetMask = netmask;
		//		memcpy(info->szMacAddr, g_network_info.szMacAddr, sizeof(info->szMacAddr));

		snprintf(info->strName , sizeof(info->strName), "%s", g_product_info.strName);
		//		snprintf(info->strVer, sizeof(info->strVer), "%s", "v1.0.1");
		info->bType = g_product_info.bType;
	} else if(input == SIGNAL_INPUT_2) {
		if(g_network_info.dhcp[1] == 1) {
			PRINTF("DHCP ip\n");
		}

		sys_get_network(ETH0_1, &ip, &gateway, &netmask);
		info->dwAddr = ip;
		info->dwGateWay = gateway;
		info->dwNetMask = netmask;

		//		memcpy(info->szMacAddr, g_network_info.szMacAddr, sizeof(info->szMacAddr));
		snprintf(info->strName , sizeof(info->strName), "%s", g_product_info.strName);
		//		snprintf(info->strVer, sizeof(info->strVer), "%s", SOFT_VERSION);
		info->bType = g_product_info.bType;
	}

	snprintf(info->strVer, sizeof(info->strVer), "%s", SOFT_VERSION);
	info->szMacAddr[0] =  g_network_info.szMacAddr[0];
	info->szMacAddr[1] =  g_network_info.szMacAddr[1];
	info->szMacAddr[2] =  g_network_info.szMacAddr[2];
	info->szMacAddr[3] =  g_network_info.szMacAddr[3];
	info->szMacAddr[4] =  g_network_info.szMacAddr[4];
	info->szMacAddr[5] =  g_network_info.szMacAddr[5];
	info->szMacAddr[6] =  g_network_info.szMacAddr[6];
	info->szMacAddr[7] =  g_network_info.szMacAddr[7];

	//szMacAddr不能打印
	//	PRINTF("g_network_info.szMacAddr =%s,g_product_info.strName=%s\n", g_network_info.szMacAddr, g_product_info.strName);
	return 0;
}

int MMP_set_sysparam(int input, MMPSysParam *info, int mmp)
{
	if(input < 0 || input > SIGNAL_INPUT_MAX) {
		ERR_PRN("input=%d is failed.\n", input);
		return -1;
	}

	int ret = -1;
	int change = 0;
	int mac_change = 0;
	int eth = ETH0;
	//	int dhcp = 0; //mmp need dhcp = 0
	int mp_status = get_mp_status();
	Enc2000_Sys  net_info;
	int len = 0;

	if(IS_MP_STATUS ==  mp_status) {
		if(mmp == MAIN_MMP) {
			eth = ETH0;
		} else if(mmp == SECOND_MMP) {
			eth = ETH0_1;
		}
	} else {
		if(MAIN_MMP ==  mmp) {
			eth = ETH0;
		} else if(SECOND_MMP == mmp) {
			eth = ETH0_1;
		}
	}

	app_get_network(&net_info, &len);

	if(eth == ETH0) {
		if(info->dwAddr == net_info.eth1.dwAddr) {
			PRINTF("info->dwAddr == net_info.eth1.dwAddr\n");
			return 0;
		}
	} else if(eth == ETH0_1) {
		if(info->dwAddr == net_info.eth0.dwAddr) {
			PRINTF("info->dwAddr == net_info.eth0.dwAddr\n");
			return 0;
		}
	}

	change = sys_change_network_info(eth, 0, info->dwAddr, info->dwGateWay, info->dwNetMask);

	mac_change = sys_change_mac(eth, info->szMacAddr);

	PRINTF("eth=%d.change=%d \n\n", eth, change);

	if(change == 1 || mac_change == 1) {
		PRINTF("the network is change=%d \n", change);

		//	write_network_info(0, &g_network_info);
		ret = write_network_info();
	}

	PRINTF(".ret=%d \n\n", ret);

	return ret;
}

/*消息头打包*/
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


static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	//	pthread_mutex_lock(&gSetP_m.send_m);
	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			WARN_PRN("WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, errno, s);

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


/*update program*/
int update_program(int sSocket, char *data, int len)
{
	unsigned long filesize;
	unsigned char *p;
	unsigned long nLen;
	unsigned char temp[20];
	int ret;
	FILE *fp = NULL;
	p = NULL;
	p = malloc(8 * 1024);
	PRINTF("The File Updata Buffer Is 64KB!\n");

	if(!p) {
		ERR_PRN("Malloc 8KB Buffer Failed!\n");
	} else {
		PRINTF("The Buffer Addr 0x%x\n", (unsigned int)p);
	}

	if((fp = fopen("/update.tgz", "w+")) == NULL) {
		ERR_PRN("open %s error \n", "update.tgz");
		return -1;
	}

	PRINTF("szData:%x, %x, %x, %x, %x, %x,%x\n", data[0], data[1], data[2],
	       data[HEAD_LEN], data[HEAD_LEN + 1], data[HEAD_LEN + 2], data[HEAD_LEN + 3]);
	filesize = ((unsigned char)data[HEAD_LEN]) | ((unsigned char)data[HEAD_LEN + 1]) << 8 |
	           ((unsigned char)data[HEAD_LEN + 2]) << 16 | ((unsigned char)data[HEAD_LEN + 3]) << 24;
	PRINTF("Updata file size = 0x%x! \n", (unsigned int)filesize);

	while(filesize) {
		nLen = recv(sSocket, p, 8 * 1024, 0);
		PRINTF("recv Updata File 0x%x Bytes!\n", (unsigned int)nLen);

		if(nLen <= 0) {
			free(p);
			fclose(fp);
			p = NULL;
			return -1;
		}

		ret = fwrite(p, 1, nLen, fp);

		if(ret < 0) {
			ERR_PRN("product update faith!\n");
			free(p);
			fclose(fp);
			p = NULL;
			PackHeaderMSG((BYTE *)temp, MSG_UpdataFile_FAILS, HEAD_LEN);
			WriteData(sSocket, temp, HEAD_LEN);
			return -1;
		}

		PRINTF("write data ...........................:0x%x\n", (unsigned int)filesize);
		filesize = filesize - nLen;
	}

	PRINTF("recv finish......................\n");
	free(p);
	fclose(fp);


	ret = updatesystem("/update.tgz");


	if(ret == 0) {
		PackHeaderMSG((BYTE *)temp, MSG_UpdataFile_SUCC, HEAD_LEN);
	} else {
		PackHeaderMSG((BYTE *)temp, MSG_UpdataFile_FAILS, HEAD_LEN);
	}

	WriteData(sSocket, temp, HEAD_LEN);
	system("sync");
	sleep(5);
	/*重启*/
	PRINTF("----------------reboot----------\n");
	system("reboot -f");
	return 0;
}

