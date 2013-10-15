

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


/*��Ϣͷ�ṹ*/
typedef struct __LOGMSGHEAD__							
{
	/*   
	## nLen:
	## ͨ��htonsת����ֵ
	## �����ṹ�屾�������
	## 
	*/
	unsigned short	nLen;		
	unsigned short	nVer;							//�汾��(�ݲ���)
	unsigned char	nMsg;							//��Ϣ����
	unsigned char	szTemp[3];						//�����ֽ�
}LOGMSGHEAD;

#define MSG_PRINTF_INFO 0x10
#define MSG_DEBUG_OUT 0x11
#define MSG_LOGIN 0x12
//#define MSG_CONNECTSUCC 0x13


extern int test_log_server_init();
