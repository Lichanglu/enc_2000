#ifndef _APP_UPDATE_H__
#define _APP_UPDATE_H__

int updatefpga(int fd);
int get_is_update_fpga();
int updatekernel(void);
int updatesystem(const char *filename);

#endif

