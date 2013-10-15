
#include <unistd.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <stdlib.h>
#include <time.h>
#include <linux/watchdog.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <net/if_arp.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <dirent.h>

#include "input_to_channel.h"
#include "middle_control.h"
#include "log_common.h"
#include "log.h"
#include "common.h"
#include "params.h"
#include "MMP_control.h"
#include "sysparams.h"

#define PORT_ONE  0

/*得到系统时间*/
static void getSysTime(DATE_TIME_INFO *dtinfo)
{
	long ts;
	struct tm *ptm = NULL;

	ts = time(NULL);
	ptm = localtime((const time_t *)&ts);
	dtinfo->year = ptm->tm_year + 1900;
	dtinfo->month = ptm->tm_mon + 1;
	dtinfo->mday = ptm->tm_mday;
	dtinfo->hours = ptm->tm_hour;
	dtinfo->min = ptm->tm_min;
	dtinfo->sec = ptm->tm_sec;

}

#ifndef  RTC_DEV_NAME
#define RTC_DEV_NAME "/dev/rtc"
#endif


void get_rtc_clock(struct rtc_time *rtc_tm)
{

	int fd = 0;
	int  retval;

	//	struct rtc_time rtc_tm;
	if(fd == 0) {
		fd = open("/dev/rtc0", O_RDWR);
	}

	if(fd == -1) {
		fprintf(stderr, "Cannot open RTC device due to following reason.\n");
		perror("/dev/rtc0");
		fd = 0;
		return;
	}

	retval = ioctl(fd, RTC_RD_TIME, rtc_tm);

	if(retval == -1) {
		PRINTF("Cannot do ioctl() due to following reason.\n");
		perror("ioctl");
		close(fd);
		return;
	} else {
		PRINTF("%d-%d-%d hour:%d:%d:%d", rtc_tm->tm_year + 1900, rtc_tm->tm_mon + 1, rtc_tm->tm_mday,
		       rtc_tm->tm_hour, rtc_tm->tm_min, rtc_tm->tm_sec);
	}

	close(fd);
}

static void set_rtc_clock(int nYear, int nMonth, int nDay, int nHour, int nMinute, int nSecond)
{
	if(nSecond < 1) {
		nSecond  = 10;
	}

	int fd = 0;
	struct rtc_time rtc_tm;
	fd = open("/dev/rtc0", O_RDWR);

	if(fd == -1) {
		fprintf(stderr, "Cannot open RTC device due to following reason.\n");
		perror("/dev/rtc0");
		fd = 0;
		return;
	}

	rtc_tm.tm_mday = nDay;
	rtc_tm.tm_mon = nMonth - 1;
	rtc_tm.tm_year = nYear - 1900;
	rtc_tm.tm_hour = nHour;
	rtc_tm.tm_min = nMinute;
	rtc_tm.tm_sec = nSecond;

	char setStr[256] = {0};
	sprintf(setStr, "date %02d%02d%02d%02d%04d", nMonth, nDay, nHour, nMinute, nYear);
	system(setStr);

	PRINTF("%s\n", setStr);

	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	rtc_tm.tm_wday = p->tm_wday;
	int retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);

	if(retval == -1) {
		fprintf(stderr, "Cannot do ioctl() due to following reason.\n");
		perror("ioctl");
		close(fd);
		return;
	}

	close(fd);
}

/*设置RTC的时间*/
static int setRtcTime(int nYear, int nMonth, int nDay, int nHour, int nMinute, int nSecond)
{
	if(nSecond < 1) {
		nSecond  = 10;
	}

#if 0

	int fd = 0;

	struct rtc_time rtc_tm;

	fd = open(RTC_DEV_NAME, O_RDWR);

	if(fd == -1) 	{
		DEBUG(DL_ERROR, "ERROR:Cannot open RTC device due to following reason.\n");
		//	perror("/dev/rtc",RTC_DEV_NAME);
		fd = 0;
		return -1;
	}

	rtc_tm.tm_mday = nDay;
	rtc_tm.tm_mon = nMonth - 1;
	rtc_tm.tm_year = nYear - 1900;
	rtc_tm.tm_hour = nHour;
	rtc_tm.tm_min = nMinute;
	rtc_tm.tm_sec = nSecond;

	//	PRINTF("%d:%d:%d    %d-%d-%d\n", rtc_tm.tm_hour,
	//	       rtc_tm.tm_min, rtc_tm.tm_sec, rtc_tm.tm_year, rtc_tm.tm_mon, rtc_tm.tm_mday);
	//更新操作系统时间
	//普通服务器上可以使用   date   -s   '2005-12-12   22:22:22'
	//arm用date   -s   121222222005.22   MMDDhhmmYYYY.ss
	char setStr[256] = {0};
	/*
		sprintf(setStr, "date -s %04d-%02d-%02d", nYear, nMonth, nDay);
		system(setStr);
		PRINTF("%s\n", setStr);

		memset(setStr, 0, 256);
		sprintf(setStr, "date -s %02d:%02d:%02d", nHour, nMinute, nSecond);
		system(setStr);
		PRINTF("%s\n", setStr);
	*/
	sprintf(setStr, "date -s \"%04d-%02d-%02d %02d:%02d:%02d\"", nYear, nMonth, nDay, nHour, nMinute, nSecond);
	system(setStr);
	//	sprintf(setStr, "date -s ", nHour, nMinute, nSecond);
	//	system(setStr);
	system("date");


	//取得星期几
	time_t timep;
	struct tm *p;
	time(&timep);
	p = localtime(&timep);
	rtc_tm.tm_wday = p->tm_wday;
	int retval = ioctl(fd, RTC_SET_TIME, &rtc_tm);
	PRINTF("retval=%d\n", retval);

	if(retval == -1) 	{
		system("hwclock -s");
		fprintf(stderr, "Cannot do ioctl() due to following reason.\n");
		perror("ioctl");
		close(fd);
		return -1;
	}

	system("hwclock -s");

	close(fd);
#endif

	set_rtc_clock(nYear, nMonth, nDay, nHour,
	              nMinute, nSecond);
	return 0;
}


int get_encode_time(char *data, int vallen)
{
	//	int ret = 0;
	DATE_TIME_INFO dtinfo;
	getSysTime(&dtinfo);
	memcpy(data, &dtinfo, vallen);
	PRINTF("getSysTime...\n");
	return 0;
}

int set_encode_time(char *data, int vallen)
{
	if(vallen  != sizeof(DATE_TIME_INFO)) {
		PRINTF("WRONG:vallen  !== sizeof(DATE_TIME_INFO\n");
		return -1;
	}

	DATE_TIME_INFO *dtinfo = (DATE_TIME_INFO *)data;
	//	int ret = 0;
	//	memcpy(&dtinfo, data, vallen);
	PRINTF("dtinfo.year:%d,dtinfo.month:%d,dtinfo.mday:%d,dtinfo.hours:%d,dtinfo.min:%d,dtinfo.sec:%d\n",
	       dtinfo->year, dtinfo->month, dtinfo->mday, dtinfo->hours, dtinfo->min, dtinfo->sec);
	//	PRINTF("1 the run time =%d=%lld\n", get_run_time(), getCurrentTime());
	setRtcTime(dtinfo->year, dtinfo->month, dtinfo->mday, dtinfo->hours,
	           dtinfo->min, dtinfo->sec);
	//	PRINTF("2 the run time =%d=%lld\n", get_run_time(), getCurrentTime());

	return 0;
}


//============
int app_web_rebootSys(int data, int *out)
{
	int ret = 0;
	system("sync");
	usleep(500000);
	DEBUG(DL_DEBUG, "Restart sys\n");
	PRINTF("-----------------reboot-----------------\n");
	ret = system("reboot -f");

	if(ret < 0) {
		usleep(500000);
		ret = system("reboot -f");
	}

	return ret;
}




/*
## 将MAC地址的字符串转换成字符数组中
##
## 如:  src = "00:11:22:33:44:55" --->
##		dst[0] = 0x00 dst[1] = 0x11 dst[2] = 0x22;
##
*/
int SplitMacAddr(char *src, unsigned char *dst, int num)
{
	char *p;
	char *q = src;
	int val = 0  , i = 0;

	for(i = 0 ; i < num ; i++) {
		p = strstr(q, ":");

		if(!p) {
			val = strtol(q, 0, 16);
			dst[i]  = val ;

			if(i == num - 1) {
				continue;
			} else {
				return -1;
			}
		}

		*(p++) = '\0';
		val = strtol(q, 0, 16);
		dst[i]  = val ;
		q = p;
	}

	return 0;
}

/*
##
## 例如:  src[0] = 0x00,src[1] = 0x11 src[2] = 0x22
##      src[3[ = 0x33,src[4] = 0x44 src[5] = 0x55
##		dst = "00:11:22:33:44:55"
##
*/
int JoinMacAddr(char *dst, unsigned char *src, int num)
{
	sprintf(dst, "%02x:%02x:%02x:%02x:%02x:%02x", src[0],
	        src[1], src[2], src[3], src[4], src[5]);
	DEBUG(DL_DEBUG, "mac addr  = %s \n", dst);

	return 0;
}

#define WATCH_DOG_DEV  ("/dev/watchdog")
static int g_watchdog_fd = 0;

/*喂狗*/
int writeWatchDog(void)
{
	int dummy;

	//	PRINTF("\n");
	if(ioctl(g_watchdog_fd, WDIOC_KEEPALIVE, &dummy) != 0) {
		DEBUG(DL_ERROR, "write WatchDog failed strerror(errno) = %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

/*初始化看门狗*/
int initWatchDog(void)
{
	struct rtc_time rtc_tm;

	get_rtc_clock(&rtc_tm);
	set_rtc_clock(rtc_tm.tm_year + 1900, rtc_tm.tm_mon + 1, rtc_tm.tm_mday,
	              rtc_tm.tm_hour, rtc_tm.tm_min, rtc_tm.tm_sec + 1);
	g_watchdog_fd = open(WATCH_DOG_DEV, O_WRONLY);

	if(g_watchdog_fd == -1) {
		DEBUG(DL_ERROR, "open watch dog failed strerror(errno) = %s\n", strerror(errno));
		return -1;
	}

	struct timeval timeout = {8, 0};

	ioctl(g_watchdog_fd, WDIOC_SETTIMEOUT, &timeout);

	PRINTF("init watch dog finished!\n");

	return 0;
}

/*关闭看门狗*/
int closeWatchDog(void)
{

	if(g_watchdog_fd) {
		close(g_watchdog_fd);
		g_watchdog_fd = 0;
	}

	return 0;
}

