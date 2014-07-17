#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <linux/types.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "app_gpio.h"
#include "app_update.h"
#include "log_common.h"
#include "app_head.h"

#include "app_update_header.h"

#define KERNELFILENAME		"/opt/dvr_rdk/ti816x_2.8/update/uImage"
#define FPGAFILENAME		"/opt/dvr_rdk/ti816x_2.8/update/fpga.bin"


#define FLASH_ENABLE	41
#define FPGA_PRO	2
#define FPGA_DONE	3



#define SET_GPIO	(0x55555555)
#define GET_GPIO	(0xAAAAAAAA)


static int SetGPIOBitValue(int fd, int bit);
static int ClearGPIOBitValue(int fd, int bit);
static int GetGPIOBitValue(int fd, int bit);
static int set_gpio_bit(int bit, int fd);
static int clear_gpio_bit(int bit, int fd);
static int get_gpio_bit(int bit, int fd);
static int set_is_update_fpga(int status);








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

	PRINTF("balance mode switch !!!!bit=%d\n", bit);
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
static int updateFpgaProgram(const char *fpgafile, int fd)
{
	int r_cnt1 = 0, r_cnt2 = 0;
	int ret = 0;
	char spidata[PER_READ_LEN];
	int spifd = -1;
	FILE *fpgafd = NULL;
	int readlen = 0;
	int writelen = 0;
	int totalwritelen  = 0;

	PRINTF("Enter into update FPGA Program \n");
	ret = clear_gpio_bit(FLASH_ENABLE, fd);
	PRINTF("FLASH_ENABLE ret=%x \n", ret);

	clear_gpio_bit(FPGA_PRO, fd);

	while(1) {
		usleep(100);
		ret = get_gpio_bit(FPGA_DONE, fd);

		if(ret != 0) {
			PRINTF("fpga Done is Not 0!\n");
		} else {
			PRINTF("fpga Done is 0!\n");
			break;
		}

		r_cnt1++;

		if(r_cnt1 > 10) {
			break;
		}
	}

	ret = system("flash_eraseall /dev/mtd0");
	PRINTF(" flash_eraseall /dev/mtd0 ret=%x \n", ret);
	spifd =  open("/dev/mtd0", O_RDWR, 0);

	if(spifd < 0)	{
		PRINTF("open the SPI flash Failed \n");
		ret = -1;
		goto cleanup;
	}

	fpgafd = fopen(fpgafile, "r+b");

	if(fpgafd == NULL)	{
		PRINTF("open the FPGA bin Failed \n");
		ret = -1;
		goto cleanup;
	}

	rewind(fpgafd);

	while(1) {
		readlen = fread(spidata, 1, PER_READ_LEN, fpgafd);

		if(readlen < 1)	{
			PRINTF("file read end \n");
			break;
		}

		writelen = write(spifd, spidata, readlen);
		//		PRINTF("writelen = %d \n", writelen);
		totalwritelen += writelen;

		if(feof(fpgafd)) {
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

	if(fpgafd) {
		fclose(fpgafd);
	}

	ret = set_gpio_bit(FPGA_PRO, fd);
	usleep(300);

	while(1) {
		ret = get_gpio_bit(FPGA_DONE, fd);

		if(ret == 0) {//正在写，允许写3s
			r_cnt1++;
			r_cnt2 = 0;
		} else if(1 ==  ret) { //写完成，确保5次
			r_cnt2++;
			r_cnt1 = 0;
		} else {
			break;
		}

		if(r_cnt1 > 10 || r_cnt2 > 5) {
			break;
		}

		usleep(300);
	}

	return ret;
}

int updatekernel(void)
{
	char command[256] = {0};
	char filepath[256] = {0};
	//	int ret =  -2;

	//	system(command);
	sprintf(filepath, "%s", KERNELFILENAME);

	//	sprintf(filepath, "/opt/dvr_rdk/ti816x_2.8/bin/%s", "uImage");
	/*更新内核*/
	//主系统是mtd3 ,副系统是mtd4
	if(access(filepath, F_OK) == 0) {
		PRINTF("i Will 'flash_eraseall /dev/mtd3'\n");
		system("flash_eraseall /dev/mtd3");
		sprintf(command, "nandwrite -p /dev/mtd3 %s", filepath);
		PRINTF("i Will '%s'\n", command);
		system(command);

		PRINTF("update kernel is end\n");

		writeWatchDog();
	}

	return 0;
}

int updatefpga(int fd)
{
	int ret = 0;
	char filepath[256] = {0};
	/*更新FPGA*/
	sprintf(filepath, "%s", FPGAFILENAME);

	//	sprintf(filepath,"/opt/dvr_rdk/ti816x_2.8/bin/enc2000.bin");
	if(access(filepath, F_OK) == 0) {
		set_is_update_fpga(1);
		sleep(1);
		PRINTF("----begin to update fpga---\n");
		ret = updateFpgaProgram(filepath, fd);
		PRINTF("----end to update fpga---\n");
	} else {
		PRINTF("have no file \n");
	}

	return ret;
}

static int g_fpga_status = 0;
int get_is_update_fpga()
{
	return g_fpga_status;
}
static int set_is_update_fpga(int status)
{
	g_fpga_status = status;
	return 0;
}

int updatesystem(const char *filename)
{
	char command[256] = {0};
	//	char filepath[256] = {0};
	int ret = 0;
	int fd = 0;
	char file[256] = {0};
	sprintf(command, "mv %s /", filename);
	system(command);

#ifdef  ADD_UPDATE_HEADER
	PRINTF("ADD_UPDATE_HEADER start...\n");
	char comm[320] = {0};

	strcpy(file, "/update.tgz");
	ret = app_header_info_ok(file);

	if(ret == 0) {
		if(app_del_extern_buff(file) == -1) {
			PRINTF("error! del the file extern_buff is error\n");
			return -1;
		}

		sprintf(comm, "rm -rf %s;mv %s_temp %s", file, file, file);
		PRINTF("success del extern header,command=%s\n", comm);
		system(comm);
	} else {
		if(ret == 2) {
			PRINTF("This file isn't have add extern update header\n");
			return -1;
		} else {
			PRINTF("This file have add extern update header,but the header have some error\n");
			return -1;
		}
	}

	PRINTF("ADD_UPDATE_HEADER end...\n");
#endif

	snprintf(command, sizeof(command), "tar -zxvf %s -C /", file);
	ret = system(command);
	printf("system:ret =%d,command=%s\n", ret, command);

	writeWatchDog();
	updatekernel();

	fd = app_get_gpio_fd();

	writeWatchDog();
	ret = updatefpga(fd);

	PRINTF("update fpga =%d\n", ret);
	sleep(1);
	system("rm -rf /opt/dvr_rdk/ti816x_2.8/update");

	return 0;
}


