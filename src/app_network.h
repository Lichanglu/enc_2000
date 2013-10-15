#ifndef _APP_NETWORK_H
#define _APP_NETWORK_H



extern unsigned int GetIPaddr(char *interface_name);
extern int get_gateway(char *gateway);
extern unsigned int GetNetmask(char *interface_name);
extern unsigned int GetBroadcast(char *interface_name);

void SetEthConfigIP(int eth , unsigned int ipaddr, unsigned int netmask);

void SetEthConfigGW(int eth, unsigned int gw);
void SetEthDhcp();

#endif
