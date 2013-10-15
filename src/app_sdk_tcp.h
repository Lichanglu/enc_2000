#ifndef APP_SDK_COM_H
#define APP_SDK_COM_H


#define SDK_USERNAME 	"admin"
#define SDK_PASSWORD	"admin"
#define SERVPORT 		4000


#define APP_SDK_MAX_CLIENT		10

int app_sdk_login_check(char *in,char *out);

void listenSdkTask();


#endif
