#ifndef SYSPARAMS_H
#define SYSPARAMS_H
#include "middle_control.h"
#include "MMP_control.h"
#define	ENC2000	2000
#define	ENC1600	1600

//network info ,save in config.xml
typedef struct _SYS_NETWORK_INFO_ {
	unsigned char szMacAddr[8];	     //just read from kernel
	unsigned int dhcp[2];
	unsigned int dwAddr[2] ;           //IP addr
	unsigned int dwGateWay[2];         //gateway
	unsigned int dwNetMask[2];         //sub network mask
	unsigned int dwMaindns[2];             //main dns
	unsigned int dwSlavedns[2];         //slave dns
} SysNetworkInfo;

void app_init_network_info(void);
void app_init_product_info(void);
int sys_get_product_type(void);

int MMP_get_sysparam(int input, MMPSysParam *info, int mmp);

int MMP_set_sysparam(int input, MMPSysParam *info, int mmp);

int update_program(int sSocket, char *data, int len);

int sys_get_network(int eth, unsigned int *ip, unsigned int  *gateway, unsigned int  *netmask);


void app_init_version_info();
int app_get_build_time(char *data);
int app_get_device_type(char *data, int vallen);
int app_get_soft_version(char *buff, int len);
int app_set_serialno(char *serialno);
int app_get_network(Enc2000_Sys *out, int *out_len);
int app_set_network(MMPSysParam *info1, MMPSysParam *info2);




#endif
