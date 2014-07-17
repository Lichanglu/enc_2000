#ifndef _NET_FAR_CTRL_H__
#define _NET_FAR_CTRL_H__

#define	 EDUKIT_FAR_CTRL 		1
int StartEdukitFarCtrlServerEx(int32_t eindex,unsigned int dwAddr ,unsigned short port);
#if EDUKIT_FAR_CTRL
int init_edukit_far_ctrl(void);
#endif

#endif

