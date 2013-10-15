
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "lcd.h"
#include "lcdshow.h"

typedef struct lcd_show_ {
	char ipaddr[16];
	char netmask[16];
	char gateway[16];
} lcd_show_t;


//获取本机ip地址
static int getIPaddr(char *ip)
{
	FILE *fp = popen("ifconfig eth0| awk '/inet addr:/{printf(\"%s\", substr($2, 6))}'", "r");
	fread(ip, 15, 1, fp);
	pclose(fp);
	return 0;
}

//获取本机子网掩码
static int getNetmask(char *netmask)
{
	FILE *fp = popen("ifconfig eth0| awk '/Mask:/{printf(\"%s\", substr($4, 6))}'", "r");
	fread(netmask, 15, 1, fp);
	pclose(fp);
	return 0;
}

static int getGateWay(char *gateWay)
{
	FILE *fp = popen("route | awk '/default/{printf(\"%s\", $2)}'", "r");
	fread(gateWay, 15, 1, fp);
	pclose(fp);
	return 0;
}

int lcd_show_string(int fd, lcd_show_t *network)
{
	char pCh[16] = {0};
	ClearScreen(fd);
	snprintf(pCh, 9, "NETWORK:");
	ShowStringLine(fd, pCh, LCD_FIRST_LINE);
	ShowStringLine(fd, network->ipaddr, LCD_SECOND_LINE);
	ShowStringLine(fd, network->netmask, LCD_THIRD_LINE);
	ShowStringLine(fd, network->gateway, LCD_FOURTH_LINE);
		
}

void *lcd_show_process(void *arg)
{
	//	printf("[lcd_show_process] start...\n");
	lcd_show_t network;
	int fd;
	fd = InitLed();

	if(fd < 0) {
		printf("InitLed is fail !!\n");
		return NULL;
	}
	memset(&network, 0, sizeof(lcd_show_t));
	getIPaddr(network.ipaddr);
	getNetmask(network.netmask);
	getGateWay(network.gateway);
	lcd_show_string(fd, &network);
	while(1) {
		sleep(1);
	}
	return arg;
}

int lcd_show(void)
{
	//	PRINTF("[lcd_show] start ...\n");
	pthread_t p;

	if(pthread_create(&p, NULL, lcd_show_process, NULL)) {
		//		PRINTF( "r_pthread_create failed !!!\n");
		return -1;
	}

	return 0;
}

