

#define LOG_SERVPORT 16200
#define BACKLOG 1
#define BUF 100
#define SOCK_NUM 2

typedef struct _CLIENTDATA_
{
	int login;
	int socket;
}CLIENTDATA;

typedef struct _CLIENTPARAM_
{
	CLIENTDATA clidata[SOCK_NUM];
}CLIENTPARAM;


/*消息头结构*/
typedef struct __LOGMSGHEAD__							
{
	/*   
	## nLen:
	## 通过htons转换的值
	## 包括结构体本身和数据
	## 
	*/
	unsigned short	nLen;		
	unsigned short	nVer;							//版本号(暂不用)
	unsigned char	nMsg;							//消息类型
	unsigned char	szTemp[3];						//保留字节
}LOGMSGHEAD;

#define MSG_PRINTF_INFO 0x10
#define MSG_DEBUG_OUT 0x11
#define MSG_LOGIN 0x12
//#define MSG_CONNECTSUCC 0x13


extern int test_log_server_init();
