#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/in_systm.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include "remotectrl.h"
//#include "tcpcom.h"
#include "common.h"
//#include "app_quality.h"
#include "log_common.h"
#include "xml_base.h"
#include "new_tcp_com.h"
#include "process_xml_cmd.h"
#include "reach_enc2000.h"
#include "encliveplay.h"

#ifdef SUPPORT_XML_PROTOCOL

#define NEW_TCP_SOCK_PORT      3400
#define RESP_ROOT_KEY (BAD_CAST "ResponseMsg")
#define REQ_ROOT_KEY (BAD_CAST "RequestMsg")



static 	sem_t	g_new_tcp_lock;
static pthread_t cli_pthread_id[2][6];
REMOTE_INFO   g_remote_info[2];

int g_xml_run_status = 0;
static inline Int gblGetQuit(void)
{
	Int quit;

	quit = g_xml_run_status;

	return quit;
}

static inline Void gblSetQuit(void)
{
	g_xml_run_status = TRUE;
}



typedef struct enc2000_xml_cli_info {
	int npos;		// 客户端数，描述第几个客户端。
	int input;		// ip1 or ip2
} enc2000_xml_cli_info;

int  read_client_num(int input)
{
	int i, cli_num = 0 ;
	int j = 0;
#if 0

	for(i = 0; i < MAX_CLIENT; i++) {
		if(ISUSED(0, i) && ISLOGIN(0, i))	 {
			cli_num++;
		}
	}

	PRINTF("old tcp client num =%d\n", cli_num);
#endif

	for(j = 0; j < 2; j++) {
		for(i = 0; i < MAX_CLIENT; i++) {
			if(ISUSED_NEW(j, i) && ISLOGIN_NEW(j, i))	 {
				cli_num++;
			}
		}
	}

	PRINTF("input = %d,total  tcp client num = %d\n", input, cli_num);
	return cli_num;
}

//用于记录共有多少个客户端加锁。
static int g_lock_count[6] = {0};
void set_g_lock_count(int pos, int lock_count)
{
	g_lock_count[pos] = lock_count;
}

int get_g_lock_count(int pos)
{
	return g_lock_count[pos];
}


//录制时锁定音频参数标志位

static int g_fix_resolution = 0;
void set_fix_resolution(int value)
{
	if((value == 0) || (value == 1)  || (value == 2)) {
		g_fix_resolution = value;
	}
}

int get_fix_resolution(void)
{
	return g_fix_resolution;
}

//用于与录播服务器通信的心跳接口。
int add_heart_count(int input, int cli)
{
	int value = 0;
	pthread_mutex_lock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));
	g_client_para[input].cliDATA[cli].heartCount++;
	value = g_client_para[input].cliDATA[cli].heartCount;
	pthread_mutex_unlock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));
	return value;
}

int cut_heart_count(int input, int cli)
{
	int value;
	pthread_mutex_lock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));
	g_client_para[input].cliDATA[cli].heartCount--;
	value = g_client_para[input].cliDATA[cli].heartCount;
	pthread_mutex_unlock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));

	return value;
}

int set_heart_count(int input, int cli, int value)
{
	pthread_mutex_lock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));
	g_client_para[input].cliDATA[cli].heartCount = value;
	pthread_mutex_unlock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));
	return value;
}

int get_heart_count(int input, int cli)
{
	int value;
	pthread_mutex_lock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));
	value = g_client_para[input].cliDATA[cli].heartCount;
	pthread_mutex_unlock(&(g_client_para[input].cliDATA[cli].heart_count_mutex));
	return value;
}

int  create_new_tcp_sock()
{
	struct sockaddr_in serv_addr;
	int 	serv_sockfd = -1;
	int     opt = 1;
	bzero(&serv_addr, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	//serv_addr.sin_port = htons(NEW_TCP_SOCK_PORT);
	serv_addr.sin_port = htons(3400);

	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//inet_pton(AF_INET, "0.0.0.0", &serv_addr.sin_addr);

	serv_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(serv_sockfd < 0)  {
		ERR_PRN("ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		gblSetQuit();
		return serv_sockfd;
	}

	setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	return serv_sockfd;
}

int tcp_recv(int  s, char *buff, int len, int flags)
{
	int toplen = 0;
	int readlen = 0;

	while(len - toplen > 0) {
		readlen =  recv(s, buff + toplen, len - toplen, flags);

		if(readlen <= 0) {
			if(errno == EAGAIN) {
				PRINTF("ERROR,errno = %d,msg=%s,len =%d\n", errno, strerror(errno), readlen);
				usleep(10000);
				continue;
			}

			PRINTF("ERROR,errno = %d,msg=%s,len =%d\n", errno, strerror(errno), readlen);
			return -1;
		}

		if(readlen != len - toplen) {
			PRINTF("WARNNING,i read the buff len = %d,i need len = %d\n", readlen, len);
		}

		toplen += readlen;
	}

	return toplen;
}


static int WriteData(int s, void *pBuf, int nSize)
{
	int nWriteLen = 0;
	int nRet = 0;
	int nCount = 0;

	//	pthread_mutex_lock(&gSetP_m.send_m);
	while(nWriteLen < nSize) {
		nRet = send(s, (char *)pBuf + nWriteLen, nSize - nWriteLen, 0);

		if(nRet < 0) {
			PRINTF("WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen, errno, s);

			if((errno == ENOBUFS) && (nCount < 10)) {
				fprintf(stderr, "network buffer have been full!\n");
				usleep(10000);
				nCount++;
				continue;
			}

			//			pthread_mutex_unlock(&gSetP_m.send_m);
			return nRet;
		} else if(nRet == 0) {
			fprintf(stderr, "WriteData ret =%d,sendto=%d,errno=%d,s=%d\n", nRet, nSize - nWriteLen,  errno, s);
			fprintf(stderr, "Send Net Data Error nRet= %d\n", nRet);
			continue;
		}

		nWriteLen += nRet;
		nCount = 0;
	}

	//	pthread_mutex_unlock(&gSetP_m.send_m);
	return nWriteLen;
}


int tcp_send_data(int sockfd, char *send_buf)
{
	int send_len = 0;
	int ret = 0;
	MsgHead msg_head;
	char tmpbuf[2050] = {0};
	send_len = strlen(send_buf) + sizeof(MsgHead);
	msg_head.sLen = htons(send_len);
	msg_head.sVer = htons(2012);
	msg_head.sMsgCode = 1;
	msg_head.sData = 0;
	PRINTF("socket = %d,slen=%d,ver=%d,smsg=%d,sdata=%d\n", sockfd, msg_head.sLen, msg_head.sVer, msg_head.sMsgCode, msg_head.sData);
	memcpy(tmpbuf, &msg_head, sizeof(MSGHEAD));
	memcpy(tmpbuf + sizeof(MSGHEAD), send_buf, strlen(send_buf));
	ret = WriteData(sockfd, tmpbuf, send_len);

	//PRINTF("send_len = %d,len=%d,ret=%d,msgcode=%d\n",send_len,strlen(send_buf),ret,msg_head.sMsgCode);
	if(ret < 0) {
		PRINTF("send xml context failed errno = %d\n", errno);
		return -1;
	}

	return 0;
}



/*Initial client infomation*/
void InitClientData_new(void)
{
	int cli;
	int input;

	for(input = 0; input < 2; input++) {
		for(cli = 0; cli < MAX_CLIENT; cli++) 	{
			SETCLIUSED_NEW(input, cli, FALSE);
			SETSOCK_NEW(input, cli, INVALID_SOCKET);
			SETCLILOGIN_NEW(input, cli, FALSE);
			SETLOWRATEFLAG_NEW(input, cli, 0);
			SET_SEND_AUDIO_NEW(input, cli, FALSE);
			SET_SEND_VIDEO_NEW(input, cli, FALSE);
			SET_PARSE_LOW_RATE_FLAG(input, cli, INVALID_SOCKET);
			SET_FIX_RESOLUTION_FLAG(input, cli, FALSE);
			set_heart_count(input, cli, FALSE);
#ifdef SUPPORT_IP_MATRIX
			SET_PASSKEY_FLAG(input, cli, 0);
#endif
			pthread_mutex_init(&(g_client_para[input].cliDATA[cli].heart_count_mutex), NULL);


		}
	}

	return;
}
void ClearLostClient_new(int input)
{
	int cli ;
	unsigned long currenttime = 0;
	currenttime = getCurrentTime();

	for(cli = 0; cli < MAX_CLIENT; cli++) {
		if(!ISUSED_NEW(input, cli) && ISSOCK_NEW(input, cli)) {
			PRINTF("[ClearLostClient] Socket = %d \n", GETSOCK_NEW(input, cli));
			shutdown(GETSOCK_NEW(input, cli), 2);
			close(GETSOCK_NEW(input, cli));
			SETSOCK_NEW(input, cli, INVALID_SOCKET);
			SETCLILOGIN_NEW(input, cli, FALSE) ;
		}

		if(ISUSED_NEW(input, cli) && (!ISLOGIN_NEW(input, cli))) {
			if((currenttime - GETCONNECTTIME_NEW(input, cli)) > 1000) {
				PRINTF("\n\n\n\n ClearLostClient Socket = %d,client %d \n\n\n\n", GETSOCK_NEW(input, cli), cli);
				shutdown(GETSOCK_NEW(input, cli), 2);
				close(GETSOCK_NEW(input, cli));
				SETSOCK_NEW(input, cli, INVALID_SOCKET);
				SETCLIUSED_NEW(input, cli, FALSE);
				SETCLILOGIN_NEW(input, cli, FALSE) ;
			}
		}

	}
}


/*get null client index*/
static int GetNullClientData_new(int input)
{
	int cli ;
	int connected_num;
	connected_num = read_client_num(input);

	if(connected_num >= MAX_CLIENT) {
		PRINTF("total client still up to max client num %d\n", connected_num);
		return -1;
	} else {
		PRINTF("cli = %d\n", connected_num);

		for(cli = 0; cli < MAX_CLIENT; cli++) {
			PRINTF("ISUSED_NEW(cli)=%d,(ISLOGIN_NEW(cli))=%d\n", (ISUSED_NEW(input, cli)), (ISLOGIN_NEW(input, cli)));

			if((!ISUSED_NEW(input, cli)) && (!ISLOGIN_NEW(input, cli))) {
				PRINTF("[add Client] number = %d \n", cli);
				return cli;
			}
		}
	}

	return -1;
}

int is_request_msg(xmlNodePtr  proot)
{
	if(xmlStrcmp(proot->name, REQ_ROOT_KEY) != 0) {
		return 0;
	}

	return 1;
}

int is_response_msg(xmlNodePtr  proot)
{
	if(xmlStrcmp(proot->name, RESP_ROOT_KEY) != 0) {
		return 0;
	}

	return 1;
}

xmlNodePtr upper_msg_get_next_samename_node(xmlNodePtr curNode)
{
	xmlChar *key = NULL;
	xmlNodePtr node = NULL;

	if(NULL == curNode) {
		return NULL;
	}

	key = (xmlChar *)curNode->name;
	curNode = curNode->next;

	while(NULL != curNode) {
		if(!xmlStrcmp(curNode->name, key)) {
			node = curNode;
			break;
		}

		curNode = curNode->next;
	}

	return node;
}


int32_t package_add_xml_leaf(xmlNodePtr child_node, xmlNodePtr far_node, int8_t *key_name, int8_t *key_value)
{
	child_node = xmlNewNode(NULL, BAD_CAST key_name);
	xmlAddChild(far_node, child_node);
	xmlAddChild(child_node, xmlNewText(BAD_CAST key_value));
	return 0;
}

int32_t package_head_msg(char *send_buf, int msg_code_val, char *passkey, char *return_val, char *user_id)
{
	xmlDocPtr doc = xmlNewDoc(BAD_CAST"1.0");

	xmlNodePtr root_node = xmlNewNode(NULL, BAD_CAST"ResponseMsg");
	xmlDocSetRootElement(doc, root_node);

	xmlNodePtr head_node 			= NULL;
	xmlNodePtr body_node 			= NULL;
	xmlNodePtr code_node 			= NULL;
	xmlNodePtr passkey_node 			= NULL;
	xmlNodePtr return_node 			= NULL;
	xmlNodePtr user_id_node 			= NULL;
	char msgCode[8] = {0};
	sprintf(msgCode, "%d", msg_code_val);
	head_node = xmlNewNode(NULL, BAD_CAST "MsgHead");
	xmlAddChild(root_node, head_node);
	package_add_xml_leaf(code_node, head_node, (int8_t *)"MsgCode", (int8_t *)msgCode);
	package_add_xml_leaf(passkey_node, head_node, (int8_t *)"PassKey", (int8_t *)passkey);

	package_add_xml_leaf(return_node, head_node, (int8_t *)"ReturnCode", (int8_t *)return_val);
	package_add_xml_leaf(body_node, root_node, (int8_t *)"MsgBody", (int8_t *)"");

	if(atoi(user_id) != -1) {
		package_add_xml_leaf(user_id_node, root_node, (int8_t *)"UserID", (int8_t *)user_id);
	}

	xmlChar *temp_xml_buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &temp_xml_buf, &size,  "UTF-8", 1);
	//printf("%s\n", (char *)temp_xml_buf);
	memcpy(send_buf, temp_xml_buf, size);
	//printf("________________________________\n");
	//printf("%s\n",send_buf);
	xmlFree(temp_xml_buf);
	release_dom_tree(doc);
	return 0;
}
xmlNodePtr get_xml_msgcode(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	*pnode = get_children_node(tmp_node, BAD_CAST "MsgCode");
	return *pnode;
}
xmlNodePtr get_xml_passkey(xmlNodePtr *pnode, xmlNodePtr proot)
{
	xmlNodePtr  tmp_node = NULL;
	tmp_node = get_children_node(proot, BAD_CAST "MsgHead");
	*pnode = get_children_node(tmp_node, BAD_CAST "PassKey");
	return *pnode;
}

xmlNodePtr get_xml_msgbody_node(xmlNodePtr *pnode, xmlNodePtr proot)
{
	*pnode = get_children_node(proot, BAD_CAST "MsgBody");
	return *pnode;
}

xmlNodePtr get_response_node(xmlNodePtr *pnode, xmlNodePtr recserver_info_node, const char *string)
{
	*pnode = get_children_node(recserver_info_node, BAD_CAST string);
	return *pnode;
}

//解析用户登录xml信令。

int parse_xml_login_msg(parse_xml_t *parse_xml_info, user_login_info *login_info)
{
	xmlNodePtr pnode_username_xml = NULL;
	xmlNodePtr pnode_password_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	int ret = 0;
	int len = 0;


	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_username_xml, pnode_msg_body_xml, "UserName") == NULL) {
		PRINTF("pnode_username_xml=%p\n", pnode_username_xml);
		return -1;
	}

	len = sizeof(login_info->username);
	ret = get_current_node_value(login_info->username, len, parse_xml_info->pdoc,  pnode_username_xml);

	if(ret < 0) {
		PRINTF("get username failed\n");
		return -1;
	}

	if(get_response_node(&pnode_password_xml, pnode_msg_body_xml, "Password") == NULL) {
		PRINTF("pnode_password_xml=%p\n", pnode_password_xml);
		return -1;
	}

	len = sizeof(login_info->password);
	ret = get_current_node_value(login_info->password, len, parse_xml_info->pdoc,  pnode_password_xml);

	if(ret < 0) {
		PRINTF("get password failed\n");
		return -1;
	}

	PRINTF("username=%s,password=%s\n", login_info->username, login_info->password);
	return 0;
}

//解析UserIDxml信令

int parse_xml_user_id_msg(int pos, parse_xml_t *parse_xml_info, char *user_id)
{
	xmlNodePtr userid_node = NULL;
	char tmpbuf[16] = {0};
	userid_node = get_children_node(parse_xml_info->proot, BAD_CAST "UserID");

	if(userid_node == NULL) {
		sprintf(user_id, "%c%c", '-', '1');
		PRINTF("get UserID node failed user_id=%s\n", user_id);
		return -1;
	}

	if(get_current_node_value(tmpbuf, 16, parse_xml_info->pdoc, userid_node) <  0) {
		sprintf(user_id, "%c%c", '-', '1');
		PRINTF("get UserID node  value  failed.user_id=%s\n", user_id);
		return -1;
	}

	strcpy(user_id, tmpbuf);

	return 0;
}

//解析请求码流xml信令。
int parse_xml_request_rate_msg(int input, int pos, parse_xml_t *parse_xml_info, char *user_id, int *roomId, char *passkey)
{
	xmlNodePtr pnode_strm_req_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_quality_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr tmp_pnode_xml = NULL;
	int ret = 0;
	char roomid[8] = {0};
	char quality[8] = {0};
	int len = 0;

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_strm_req_xml, pnode_msg_body_xml, "StrmReq") == NULL) {
		PRINTF("get strmreq failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_strm_req_xml, "RoomID") == NULL) {
		PRINTF("get ROOMID failed\n");
		return -1;
	}

	len  = sizeof(roomid);
	ret = get_current_node_value(roomid, len, parse_xml_info->pdoc,  pnode_room_id_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_quality_xml, pnode_strm_req_xml, "Quality") == NULL) {
		PRINTF("get Quality failed\n");
		return -1;
	}

	len = sizeof(quality);
	ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  pnode_quality_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	PRINTF("pos=%d,roomid=%s,quality=%s\n", pos, roomid, quality);

	if(atoi(quality) == 0 || atoi(quality) == 1) {
		SET_PARSE_LOW_RATE_FLAG(input, pos, atoi(quality));
	}

#ifdef SUPPORT_IP_MATRIX

	if(strcmp(passkey, "Decode") == 0) {
		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "EncodeIndex") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "Ipaddr") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "Port") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "StreamType") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}

		if(get_response_node(&tmp_pnode_xml, pnode_strm_req_xml, "Name") == NULL) {
			PRINTF("get Quality failed\n");
			return -1;
		}

		ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  tmp_pnode_xml);

		if(ret < 0) {
			PRINTF("get roomid value failed\n");
			return -1;
		}
	}

#endif

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

//解析远遥xml信令。

int parse_xml_remote_msg(int input, int pos, parse_xml_t *parse_xml_info, REMOTE_INFO *info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_remote_ctrl_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_type_xml = NULL;
	xmlNodePtr pnode_speed_xml = NULL;
	xmlNodePtr pnode_addr_xml = NULL;
	xmlNodePtr pnode_num_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_room_info_xml = NULL;
	xmlNodePtr pnode_enc_info_xml = NULL;
	char roomid[8] = {0};
	char tmpbuf[16] = {0};
	REMOTE_INFO *remote = &info[input];
	int  tmp_len = sizeof(tmpbuf);

	int  len = 0;

	get_sys_remote_info(input, remote);

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_remote_ctrl_xml, pnode_msg_body_xml, "RemoteCtrlReq") == NULL) {
		PRINTF("RemoteCtrlReq failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_info_xml, pnode_remote_ctrl_xml, "RoomInfo") == NULL) {
		PRINTF("RoomInfo failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_room_info_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed\n");
		return -1;
	}

	len  = sizeof(roomid);

	if(get_current_node_value(roomid, len, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("set RoomID val failed\n");
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_enc_info_xml, pnode_room_info_xml, "EncInfo") == NULL) {
		PRINTF("RoomID  failed\n");
		return -1;
	}

	if(get_response_node(&pnode_type_xml, pnode_enc_info_xml, "ID") == NULL) {
		PRINTF("Type failed\n");
		return -1;
	}

	len  = sizeof(roomid);

	if(get_current_node_value(roomid, len, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("set RoomID val failed\n");
		return -1;
	}

	if(get_response_node(&pnode_type_xml, pnode_enc_info_xml, "Type") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, tmp_len, parse_xml_info->pdoc, pnode_type_xml) < 0) {
			PRINTF("set Type failed\n");
			//return -1;
		}

		remote->type = atoi(tmpbuf);
	}

	if(get_response_node(&pnode_addr_xml, pnode_enc_info_xml, "Addr") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, tmp_len, parse_xml_info->pdoc, pnode_addr_xml) < 0) {
			PRINTF("set Addr failed\n");
			//return -1;
		}

		remote->caddr = atoi(tmpbuf);
	}

	if(get_response_node(&pnode_speed_xml, pnode_enc_info_xml, "Speed") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, tmp_len, parse_xml_info->pdoc, pnode_speed_xml) < 0) {
			PRINTF("set Speed failed\n");
			//return -1;
		}

		remote->speed = atoi(tmpbuf);
	}

	if(get_response_node(&pnode_num_xml, pnode_enc_info_xml, "Num") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, tmp_len, parse_xml_info->pdoc, pnode_num_xml) < 0) {
			PRINTF("set Num failed\n");
			//return -1;
		}

		remote->prezition = atoi(tmpbuf);
	}

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	PRINTF("pos=%d,type=%d,speed=%d,addr=%d,pre=%d\n",
	       pos, remote->type, remote->speed, remote->caddr, remote->prezition);
	return 0;
}

//get high and low quality  video info
static int get_sys_video_rate_info(int index, RATE_INFO *info)
{
	int channel1 = 0, channel2 = 0;
	MMPVideoParam video1;
	MMPVideoParam video2;

	if(index == SIGNAL_INPUT_1) {
		channel1 = CHANNEL_INPUT_1_LOW;
		channel2 = CHANNEL_INPUT_1;
	} else if(index == SIGNAL_INPUT_2) {
		channel1 = CHANNEL_INPUT_2_LOW;
		channel2 = CHANNEL_INPUT_2;
	}

	MMP_video_info_get(channel1, &video1);
	MMP_video_info_get(channel2, &video2);
	info[0].nBitrate = video1.sBitrate;
	info[0].nFrame = video1.nFrameRate;
	info[0].nHeight = video1.nHight;
	info[0].nWidth = video1.nWidth;

	info[1].nBitrate = video2.sBitrate;
	info[1].nFrame = video2.nFrameRate;
	info[1].nHeight = video2.nHight;
	info[1].nWidth = video2.nWidth;

	return 0;
}


//解析设置高低质量xml信令

int parse_xml_set_quality_msg(int index , int pos, parse_xml_t *parse_xml_info, RATE_INFO *rate_info, int *roomId, char *user_id)
{
	//PRINTF("rate_info=%p\n",tmp_rate_info);
	xmlNodePtr pnode_room_info_req_xml = NULL;
	xmlNodePtr pnode_room_info_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_enc_info_xml = NULL;
	xmlNodePtr pnode_enc_id_xml = NULL;
	xmlNodePtr pnode_quality_info_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_tmp_xml = NULL;
	//RATE_INFO rate_info;

	int  quality_num = 0;
	char roomid[8] = {0};
	char tmpbuf[64] = {0};
	int  idx = 0;

	get_sys_video_rate_info(index, rate_info);

	PRINTF("old high rate info:\n num=%d,type=%d,bitrate=%d,width=%d,height=%d,frame=%d\n", rate_info[0].rateInfoNum, rate_info[0].rateType
	       , rate_info[0].nBitrate, rate_info[0].nWidth, rate_info[0].nHeight, rate_info[0].nFrame);
	PRINTF("old low rate info:\n num=%d,type=%d,bitrate=%d,width=%d,height=%d,frame=%d\n", rate_info[0].rateInfoNum, rate_info[1].rateType
	       , rate_info[1].nBitrate, rate_info[1].nWidth, rate_info[1].nHeight, rate_info[1].nFrame);

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_info_req_xml, pnode_msg_body_xml, "SetRoomInfoReq") == NULL) {
		PRINTF("set room info req failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_info_xml, pnode_room_info_req_xml, "RoomInfo") == NULL) {
		PRINTF("RoomInfo  failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_room_info_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed\n");
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  value  failed\n");
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_enc_info_xml, pnode_room_info_xml, "EncInfo") == NULL) {
		PRINTF("EncInfo  failed\n");
		return -1;
	}

	if(get_response_node(&pnode_enc_id_xml, pnode_enc_info_xml, "ID") == NULL) {
		PRINTF("ID failed\n");
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc, pnode_enc_id_xml) < 0) {
		PRINTF("ID   value failed\n");
		return -1;
	}

	if(get_response_node(&pnode_quality_info_xml, pnode_enc_info_xml, "QualityInfo") == NULL) {
		PRINTF("QualityInfo failed\n");
		return -1;
	}

	quality_num = get_current_samename_node_nums(pnode_quality_info_xml);
	PRINTF("quality =%d\n", quality_num);

	for(idx = 0; idx < quality_num; idx++) {
#if 1
		rate_info[idx].rateInfoNum = quality_num;

		//memset(&rate_info,0,sizeof(RATE_INFO));
		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "RateType") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("RateType value failed\n");
				//return -1;
			}

			if(tmpbuf[0] == '0' || tmpbuf[0] == '1') {
				rate_info[idx].rateType = atoi(tmpbuf);
			}
		}

		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncBitrate") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncBitrate value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nBitrate = atoi(tmpbuf);
			}
		}


		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncWidth") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncWidth value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nWidth = atoi(tmpbuf);
			}
		}


		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncHeight") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncHeight value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nHeight = atoi(tmpbuf);
			}
		}


		if(get_response_node(&pnode_tmp_xml, pnode_quality_info_xml, "EncFrameRate") != NULL) {
			memset(tmpbuf, 0, sizeof(tmpbuf));

			if(get_current_node_value(tmpbuf, 64, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
				PRINTF("EncFrameRate value failed\n");
				//return -1;
			}

			if(strlen(tmpbuf) != 0) {
				rate_info[idx].nFrame = atoi(tmpbuf);
			}
		}

#endif

		PRINTF("idx =%d,type=%d,bitrate=%d,width=%d,height=%d,frame=%d\n", idx, rate_info[idx].rateType
		       , rate_info[idx].nBitrate, rate_info[idx].nWidth, rate_info[idx].nHeight, rate_info[idx].nFrame);
		pnode_quality_info_xml = upper_msg_get_next_samename_node(pnode_quality_info_xml);
		PRINTF("pnode_quality_info_xml = %p\n", pnode_quality_info_xml);
	}

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

int get_sys_audio_param(int index, AUDIO_PARAM *audio_info)
{
	AudioEncodeParam info;
	int channel = input_get_high_channel(index);
	memset(&info, 0, sizeof(AudioEncodeParam));
	app_web_get_ainfo(channel, &info);
	audio_info->BitRate = info.BitRate;
	audio_info->InputMode = info.InputMode;
	audio_info->LVolume = info.LVolume;
	audio_info->RVolume = info.RVolume;
	audio_info->SampleRate = info.SampleBit;
	return 0;
}


//解析设置音频xml信令

int parse_xml_set_audio_info_msg(int index, int pos, parse_xml_t *parse_xml_info, AUDIO_PARAM *audio_info, int *roomId, char *user_id)
{

	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_room_audio_xml = NULL;
	xmlNodePtr pnode_room_info_xml = NULL;
	xmlNodePtr pnode_audio_info_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_tmp_xml = NULL;

	char roomid[8] = {0};
	char tmpbuf[8] = {0};

	get_sys_audio_param(index, audio_info);

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_audio_xml, pnode_msg_body_xml, "SetRoomAudioReq") == NULL) {
		PRINTF("SetRoomAudioReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_info_xml, pnode_room_audio_xml, "RoomInfo") == NULL) {
		PRINTF("RoomInfo failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_room_info_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_audio_info_xml, pnode_room_info_xml, "AudioInfo") == NULL) {
		PRINTF("AudioInfo failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		//return -1;
	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "InputMode") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get InputMode value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) == 0 || atoi(tmpbuf) == 1) {
			audio_info->InputMode = atoi(tmpbuf);
		}
	}


	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "SampleRate") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get SampleRate value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) <= 3 &&  atoi(tmpbuf) >= 0) {
			audio_info->SampleRate = atoi(tmpbuf);
		}
	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "Bitrate") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get Bitrate value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) <= 128000 && atoi(tmpbuf) >= 0) {
			audio_info->BitRate = atoi(tmpbuf);
		}

	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "Lvolume") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get Lvolume value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) >= 0 && atoi(tmpbuf) <= 100) {
			audio_info->LVolume = atoi(tmpbuf);
		}
	}

	if(get_response_node(&pnode_tmp_xml, pnode_audio_info_xml, "Rvolume") != NULL) {
		memset(tmpbuf, 0, sizeof(tmpbuf));

		if(get_current_node_value(tmpbuf, 8, parse_xml_info->pdoc, pnode_tmp_xml) < 0) {
			PRINTF("get Rvolume value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(atoi(tmpbuf) >= 0 && atoi(tmpbuf) <= 100) {
			audio_info->RVolume = atoi(tmpbuf);
		}

	}

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	PRINTF("client[%d] mode=%d,sample=%d,bitrate=%d,lv=%d,rv=%d\n", pos, audio_info->InputMode,
	       audio_info->SampleRate, audio_info->BitRate, audio_info->LVolume, audio_info->RVolume);
	return 0;
}


//解析获取教室信息xml信令
int parse_xml_get_room_info_msg(int input, int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_room_info_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}


	if(get_response_node(&pnode_room_info_xml, pnode_msg_body_xml, "RoomInfoReq") == NULL) {
		PRINTF("RoomInfoReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_room_info_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}


int parse_xml_sys_update_msg(parse_xml_t *parse_xml_info, xmlNodePtr pnode_msg_body_xml)
{
	xmlNodePtr pnode_operate_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};
	get_response_node(&pnode_operate_xml, pnode_msg_body_xml, "Operate");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	get_response_node(&pnode_room_id_xml, pnode_operate_xml, "Length");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	get_response_node(&pnode_room_id_xml, pnode_operate_xml, "Context");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	return 0;
}


int parse_xml_upload_logo_msg(parse_xml_t *parse_xml_info, xmlNodePtr pnode_msg_body_xml)
{
	xmlNodePtr pnode_upload_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};
	get_response_node(&pnode_upload_xml, pnode_msg_body_xml, "UploadLogReq");

	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "RoomID");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "EncodeIndex");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Format");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Transparency");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "LogoLength");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	return 0;
}


int parse_xml_add_title_msg(parse_xml_t *parse_xml_info, xmlNodePtr pnode_msg_body_xml)
{
	xmlNodePtr pnode_upload_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};
	get_response_node(&pnode_upload_xml, pnode_msg_body_xml, "UploadLogReq");

	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "RoomID");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "EncodeIndex");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Title");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);
	get_response_node(&pnode_room_id_xml, pnode_upload_xml, "Time");
	get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml);

	return 0;
}

//解析获取音量大小xml
int parse_xml_get_vol_msg(int input, int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_volume_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_volume_xml, pnode_msg_body_xml, "GetVolumeReq") == NULL) {
		PRINTF("GetVolumeReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_volume_xml, "RoomID") == NULL) {
		PRINTF("RoomID failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

//解析静音xml
int parse_xml_mute_msg(int input, int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_volume_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_volume_xml, pnode_msg_body_xml, "MuteReq") == NULL) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_volume_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("get RoomID value failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

int parse_xml_pic_rev_msg(int input, int pos, parse_xml_t *parse_xml_info, PICTURE_ADJUST *pic_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "RoomID") == NULL) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "EncodeIndex") == NULL) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "Hporch") != NULL) {
		memset(roomid, 0, sizeof(roomid));

		if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
			PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(strlen(roomid) != 0) {
			pic_info->hporch = atoi(roomid);
		}
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "Vporch") != NULL) {
		memset(roomid, 0, sizeof(roomid));

		if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
			PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(strlen(roomid) != 0) {
			pic_info->vporch = atoi(roomid);
		}
	}

	if(get_response_node(&pnode_room_id_xml, pnode_msg_body_xml, "ColorTrans") != NULL) {
		memset(roomid, 0, sizeof(roomid));

		if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
			PRINTF("MuteReq failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
			//return -1;
		}

		if(strlen(roomid) != 0) {
			pic_info->color_trans = atoi(roomid);
		}
	}

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	PRINTF("h=%d,v=%d,col=%d\n", pic_info->hporch, pic_info->vporch, pic_info->color_trans);

	return 0;
}

//解析请求I帧xml
int parse_xml_request_Iframe_msg(int input, int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_IFrame_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[8] = {0};


	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}


	if(get_response_node(&pnode_IFrame_xml, pnode_msg_body_xml, "IFrameReq") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_IFrame_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_room_id_xml, pnode_IFrame_xml, "EncodeIndex") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}


//新版海外录播解析锁定分辨率xml
int parse_fix_resolution_msg(int index, int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_rec_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	char roomid[32] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_rec_xml, pnode_msg_body_xml, "RecCtrlReq") == NULL) {
		PRINTF("RecCtrlReq  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "OptType") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if((atoi(roomid) == 0) || (atoi(roomid) == 1)) {
		SET_FIX_RESOLUTION_FLAG(index, pos, atoi(roomid));
	}

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}

#ifdef SUPPORT_IP_MATRIX

//解析停止码流xml

int parse_xml_stop_rate_msg(int input, int pos, parse_xml_t *parse_xml_info, char *user_id, int *roomId, char *passkey)
{
	xmlNodePtr pnode_strm_req_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	xmlNodePtr pnode_quality_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	int ret = 0;
	char roomid[8] = {0};
	char quality[8] = {0};
	int len = 0;

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_strm_req_xml, pnode_msg_body_xml, "StrmReq") == NULL) {
		PRINTF("get strmreq failed\n");
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_strm_req_xml, "RoomID") == NULL) {
		PRINTF("get ROOMID failed\n");
		return -1;
	}

	len  = sizeof(roomid);
	ret = get_current_node_value(roomid, len, parse_xml_info->pdoc,  pnode_room_id_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	*roomId = atoi(roomid);

	if(get_response_node(&pnode_quality_xml, pnode_strm_req_xml, "Quality") == NULL) {
		PRINTF("get Quality failed\n");
		return -1;
	}

	len = sizeof(quality);
	ret = get_current_node_value(quality, len, parse_xml_info->pdoc,  pnode_quality_xml);

	if(ret < 0) {
		PRINTF("get roomid value failed\n");
		return -1;
	}

	PRINTF("pos=%d,roomid=%s,quality=%s\n", pos, roomid, quality);

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	return 0;
}


//解析获取系统信息xml
int parse_xml_get_system_msg(parse_xml_t *parse_xml_info, user_login_info *login_info)
{
	xmlNodePtr pnode_username_xml = NULL;
	xmlNodePtr pnode_password_xml = NULL;
	xmlNodePtr pnode_msg_body_xml = NULL;
	int ret = 0;
	int len = 0;


	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_username_xml, pnode_msg_body_xml, "UserName") == NULL) {
		PRINTF("pnode_username_xml=%p\n", pnode_username_xml);
		return -1;
	}

	len = sizeof(login_info->username);
	ret = get_current_node_value(login_info->username, len, parse_xml_info->pdoc,  pnode_username_xml);

	if(ret < 0) {
		PRINTF("get username failed\n");
		return -1;
	}

	if(get_response_node(&pnode_password_xml, pnode_msg_body_xml, "Password") == NULL) {
		PRINTF("pnode_password_xml=%p\n", pnode_password_xml);
		return -1;
	}

	len = sizeof(login_info->password);
	ret = get_current_node_value(login_info->password, len, parse_xml_info->pdoc,  pnode_password_xml);

	if(ret < 0) {
		PRINTF("get password failed\n");
		return -1;
	}

	PRINTF("username=%s,password=%s\n", login_info->username, login_info->password);
	return 0;
}

//解析音视频，分辨率参数


int parse_lock_av_param_msg(int pos, parse_xml_t *parse_xml_info, int *roomId, char *user_id, LOCK_PARAM *lock)
{
	xmlNodePtr pnode_msg_body_xml = NULL;
	xmlNodePtr pnode_rec_xml = NULL;
	xmlNodePtr pnode_room_id_xml = NULL;
	int  lock_count = 0;
	char roomid[32] = {0};

	if(get_xml_msgbody_node(&pnode_msg_body_xml, parse_xml_info->proot) == NULL) {
		PRINTF("Msgbody failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_rec_xml, pnode_msg_body_xml, "RecCtrlReq") == NULL) {
		PRINTF("RecCtrlReq  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "RoomID") == NULL) {
		PRINTF("RoomID  failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("RoomID  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "VideoLock") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(atoi(roomid) == 0 || atoi(roomid) == 1) {
		lock->video_lock = atoi(roomid);
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "AudioLock") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(atoi(roomid) == 0 || atoi(roomid) == 1) {
		lock->audio_lock = atoi(roomid);
	}

	if(get_response_node(&pnode_room_id_xml, pnode_rec_xml, "RelutionLock") == NULL) {
		PRINTF("OptType   failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	memset(roomid, 0, sizeof(roomid));

	if(get_current_node_value(roomid, 8, parse_xml_info->pdoc,  pnode_room_id_xml) < 0) {
		PRINTF("OptType  val failed,pnode_msg_body_xml = %p\n", pnode_msg_body_xml);
		return -1;
	}

	if(atoi(roomid) == 0 || atoi(roomid) == 1) {
		lock->resulotion_lock = atoi(roomid);
	}

	if(parse_xml_user_id_msg(pos, parse_xml_info, user_id) < 0) {
		PRINTF("parse user id failed\n");
		//return -1;
	}

	PRINTF("audio_lock=%d,video_lock=%d,resolution_lock=%d\n", lock->audio_lock, lock->video_lock, lock->resulotion_lock);

	if(lock->audio_lock == 1 && lock->video_lock == 1 && lock->resulotion_lock == 1) {
		lock_count = get_g_lock_count(pos);
		lock_count += 1;
		set_g_lock_count(pos, lock_count);

	} else if(lock->audio_lock == 0 && lock->video_lock == 0 && lock->resulotion_lock == 0) {
		lock_count = get_g_lock_count(pos);
		lock_count -= 1;

		if(lock_count < 0) {
			lock_count = 0;
		}

		set_g_lock_count(pos, lock_count);

	} else
		;

	PRINTF("cli [%d] lock %d counts \n", pos, get_g_lock_count(pos));

	return 0;
}

#endif

//解析并处理xml信令中MsgBody中的内容

int parse_xml_msgbody_msg(int input, int pos, parse_xml_t *parse_xml_info, int msg_code, char *passkey, char *user_id)
{
	PRINTF("input = %d,pos=%d,msgcode=%d,passkey=%s\n", input, pos, msg_code, passkey);
	xmlNodePtr pnode_msg_body_xml = NULL;
	user_login_info login_info;
	REMOTE_INFO remote[2];
	RATE_INFO  rate_info[2];
	AUDIO_PARAM audio_info;
	PICTURE_ADJUST pic_info;
	char send_buf[1024] = {0};
	char tmpbuf[512] = {0};
	int roomid = 0;
	int sockfd = GETSOCK_NEW(input, pos);
	int count = 0;

	switch(msg_code) {
			//login info
		case NEW_MSG_TCP_LOGIN: {
			PRINTF("enter parse login cmd\n");
			memset(&login_info, 0, sizeof(login_info));

			//t1 = get_run_time();
			if(parse_xml_login_msg(parse_xml_info, &login_info) < 0) {
				PRINTF("parse login failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_TCP_LOGIN, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			//t2 = get_run_time();
			//PRINTF(" NEW_MSG_TCP_LOGIN t2 - t1 =%d\n",t2 - t1);
			process_xml_login_msg(input, pos, send_buf, &login_info, NEW_MSG_TCP_LOGIN, passkey, user_id);
			//t3 = get_run_time();
			//PRINTF(" NEW_MSG_TCP_LOGIN  t3 - t2 =%d\n",t3- t2);
		}
		break;

		case NEW_MSG_REQUEST_RATE: {
			PRINTF("pos=%d,parse request rate msg %p\n", pos, pnode_msg_body_xml);

			//t1 = get_run_time();
			if(parse_xml_request_rate_msg(input, pos, parse_xml_info, user_id, &roomid, passkey) < 0) {
				PRINTF("parse request rate  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_REQUEST_RATE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			//t2 = get_run_time();
			//PRINTF(" NEW_MSG_REQUEST_RATE  t2 - t1 =%d\n",t2 - t1);
			process_xml_request_rate_msg(input, pos, send_buf, NEW_MSG_REQUEST_RATE, passkey, user_id, &roomid);
			//t3 = get_run_time();
			//PRINTF(" NEW_MSG_REQUEST_RATE  t3 - t2 =%d\n",t3- t2);
		}
		break;

		case NEW_MSG_REMOTE: {
			PRINTF("client[%d]parse REMOTE CAMERA  info msg\n", pos);
			memset(&remote, 0, sizeof(REMOTE_INFO) * 2);

			if(parse_xml_remote_msg(input, pos, parse_xml_info, &remote, &roomid, user_id) < 0) {
				PRINTF("parse NEW_MSG_REMOTE  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_REMOTE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_xml_remote_ctrl_msg(input, pos, send_buf, &remote, NEW_MSG_REMOTE, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_SET_QUALITY_INFO: {
			PRINTF("client[%d] parse set quality info msg rate_info=%p,&rate_info=%p\n", pos, rate_info, &rate_info);
			memset(rate_info, 0, sizeof(RATE_INFO) * 2);

			//t1 = get_run_time();
			if(parse_xml_set_quality_msg(input, pos, parse_xml_info, rate_info, &roomid, user_id) < 0) {
				PRINTF("parse quality rate  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_SET_QUALITY_INFO, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			//t2 = get_run_time();
			//PRINTF(" NEW_MSG_SET_QUALITY_INFO  t2 - t1 =%d\n",t2 - t1);
			process_set_quality_cmd(input, pos, send_buf, rate_info, NEW_MSG_SET_QUALITY_INFO, passkey, user_id, &roomid);
			//t3 = get_run_time();
			//PRINTF(" NEW_MSG_SET_QUALITY_INFO  t3 - t2 =%d\n",t3- t2);
		}
		break;

		case NEW_MSG_SET_AUDIO: {
			PRINTF("parse set audio info msg\n");
			memset(&audio_info, 0, sizeof(AUDIO_PARAM));

			if(parse_xml_set_audio_info_msg(input, pos, parse_xml_info, &audio_info, &roomid, user_id) < 0) {
				PRINTF("parse NEW_MSG_SET_AUDIO  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_SET_AUDIO, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_set_audio_cmd(input, pos, send_buf, &audio_info, NEW_MSG_SET_AUDIO, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_GET_ROOM_INFO: {
			PRINTF("parse get room info msg\n");

			if(parse_xml_get_room_info_msg(input, pos, parse_xml_info, &roomid, user_id) < 0) {
				PRINTF("parse NEW_MSG_GET_ROOM_INFO  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_GET_ROOM_INFO, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_get_room_info_cmd(input, pos, send_buf, NEW_MSG_GET_ROOM_INFO, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_SYS_UPDATE: {
			parse_xml_sys_update_msg(parse_xml_info, pnode_msg_body_xml);

		}
		break;

		case NEW_MSG_UPLOAD_LOGO: {
			parse_xml_upload_logo_msg(parse_xml_info, pnode_msg_body_xml);

		}
		break;

		case NEW_MSG_ADD_TITLE: {
			parse_xml_add_title_msg(parse_xml_info, pnode_msg_body_xml);

		}
		break;

		case NEW_MSG_GET_VOLUME: {
			//PRINTF("parse get VOLUME  msg\n");
			if(parse_xml_get_vol_msg(input, pos, parse_xml_info, &roomid, user_id) < 0) {
				PRINTF("parse get_vol_msg  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_GET_VOLUME, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_xml_get_volume_msg(input, pos, send_buf, NEW_MSG_GET_VOLUME, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_MUTE: {
			if(parse_xml_mute_msg(input, pos, parse_xml_info, &roomid, user_id) < 0) {
				PRINTF("parse get_vol_msg  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_MUTE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_mute_cmd(input, pos, send_buf, NEW_MSG_MUTE, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_PIC_REVISE: {
			memset(&pic_info, 0, sizeof(PICTURE_ADJUST));

			if(parse_xml_pic_rev_msg(input, pos, parse_xml_info, &pic_info, &roomid, user_id) < 0) {
				PRINTF("parse get_vol_msg  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_MSG_PIC_REVISE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_pic_adjust_cmd(input, pos, send_buf, &pic_info, NEW_MSG_PIC_REVISE, passkey, user_id, &roomid);
		}
		break;

		case NEW_REQUEST_IFRAME: {
			if(parse_xml_request_Iframe_msg(input, pos, parse_xml_info, &roomid, user_id) < 0) {
				PRINTF("parse REQUEST_IFRAME  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_REQUEST_IFRAME, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_xml_request_IFrame_msg(input, pos, send_buf, NEW_REQUEST_IFRAME, passkey, user_id, &roomid);
		}
		break;

		case NEW_FIX_RESOLUTION: {
			if(parse_fix_resolution_msg(input, pos, parse_xml_info, &roomid, user_id) < 0) {
				PRINTF("parse REQUEST_IFRAME  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_FIX_RESOLUTION, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_fix_resolution_msg(input, pos, send_buf, NEW_FIX_RESOLUTION, passkey, user_id, &roomid);
		}
		break;

		case NEW_MSG_STATUS_REPORT: {
			PRINTF("parse NEW_MSG_STATUS_REPORT msg\n");
			count = cut_heart_count(input, pos);
			PRINTF("input  = %d,client [%d] parse NEW_MSG_STATUS_REPORT cut heart count = %d\n", input, pos, count);

		}
		break;

#ifdef SUPPORT_IP_MATRIX

		case NEW_MSG_SYS_RESTART: {
			process_sys_restart_msg(pos, send_buf, NEW_MSG_TCP_LOGIN, passkey, user_id, &roomid);
		}
		break;

		case NEW_ENCODE_ENABLE: {
			process_encode_enable_msg(pos, send_buf, NEW_ENCODE_ENABLE, passkey, user_id, &roomid);
		}
		break;

		case NEW_STOP_RATE: {
			if(parse_xml_stop_rate_msg(input, pos, parse_xml_info, user_id, &roomid, passkey) < 0) {
				PRINTF("parse request rate  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_STOP_RATE, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_xml_stop_rate_msg(input, pos, send_buf, NEW_STOP_RATE, passkey, user_id, &roomid);

		}
		break;

		case NEW_LOCK_AV_PARAM: {
			LOCK_PARAM  lock_param;

			if(parse_lock_av_param_msg(pos, parse_xml_info, &roomid, user_id, &lock_param) < 0) {
				PRINTF("parse REQUEST_IFRAME  failed\n");
				memset(tmpbuf, 0, sizeof(tmpbuf));
				package_head_msg(tmpbuf, NEW_LOCK_AV_PARAM, passkey, "0", user_id);
				tcp_send_data(sockfd, tmpbuf);
				return -1;
			}

			process_lock_av_param_msg(pos, send_buf, NEW_LOCK_AV_PARAM, passkey, user_id, &roomid, &lock_param);
		}
		break;

		case NEW_GET_SYS_PARAM: {
			process_get_system_msg(pos, send_buf, NEW_GET_SYS_PARAM, passkey, user_id);
		}
		break;
#endif

		default: {
			PRINTF("unknown xml  msg\n");
		}
		break;


	}

	return 0;
}


int parse_cli_xml_msg(int input, int pos, char *buffer, int *msgCode, char *passkey, char *user_id)
{
	//	PRINTF("buffer=\n%s\nbuf=%p\n",buffer,buffer);
	parse_xml_t *parse_xml_info = (parse_xml_t *)malloc(sizeof(parse_xml_t));
	xmlNodePtr pnode_msgcode_xml = NULL;
	xmlNodePtr pnode_passkey_xml = NULL;
	char tmpbuf[64] = {0};
	int ret = 0;
	int len = 0;

	if(init_dom_tree(parse_xml_info, buffer) == NULL) {
		PRINTF("[init_dom_tree] is error!\n");
		ret = -1;
		goto EXIT;
	}

	if(is_request_msg(parse_xml_info->proot) != 1) {
		PRINTF("request_msg != 1\n");
		ret = -1;
		goto EXIT;
	}

	if(get_xml_msgcode(&pnode_msgcode_xml, parse_xml_info->proot) == NULL) {
		PRINTF("get xml msgcode failed\n");
		ret = -1;
		goto EXIT;
	}

	len = sizeof(tmpbuf);

	if(get_current_node_value(tmpbuf, len, parse_xml_info->pdoc,  pnode_msgcode_xml) != 0) {
		PRINTF("get xml msgcode value failed\n");
		ret = -1;
		goto EXIT;
	}

	if(strlen(tmpbuf) != 0) {
		*msgCode = atoi(tmpbuf);
	}

	if(get_xml_passkey(&pnode_passkey_xml, parse_xml_info->proot) == NULL) {
		PRINTF("get xml passkey failed\n");
		ret = -1;
		goto EXIT;
	}

	len = sizeof(tmpbuf);
	memset(tmpbuf, 0, sizeof(tmpbuf));

	if(get_current_node_value(tmpbuf, len, parse_xml_info->pdoc,  pnode_passkey_xml) != 0) {
		PRINTF("get xml passkey value failed\n");
		ret = -1;
		goto EXIT;
	}

	if(strlen(tmpbuf) != 0) {
		strcpy(passkey, tmpbuf);
	}

#ifdef SUPPORT_IP_MATRIX

	if(strcmp(passkey, "IPMTSServer") == 0) {
		SET_PASSKEY_FLAG(input, pos, 1);
	}

#endif

	//t1 = get_run_time();
	if(parse_xml_msgbody_msg(input, pos, parse_xml_info, *msgCode, passkey, user_id) < 0) {
		PRINTF("parse xml msgbody failed\n");
		ret = -1;
		goto EXIT;
	}

	//t2 = get_run_time();
	//PRINTF("***********************t2-t1=%d\n",t2-t1);
EXIT:

	if(parse_xml_info->pdoc != NULL) {
		release_dom_tree(parse_xml_info->pdoc);
		free(parse_xml_info);
		parse_xml_info = NULL;
	}

	//t3 = get_run_time();
	//PRINTF("***********************t2-t1=%d\n",t3-t2);
	return ret;

}

void  recv_client_msg_thread(void *pParams)
{
	PRINTF("pid=%d,thresd_id = %d\n", getpid(), pthread_self());
	int  lock_count = 0;
	//char *szData = NULL;
	char szData[2048] = {0};
	unsigned  int nLen;
	int 	total_len;
	int sSocket;
	int nPos;
	MsgHead  msg_info;
	int  input = 0;
	enc2000_xml_cli_info  *cli_info = (enc2000_xml_cli_info *)pParams;
	PRINTF("enter recv_client_msg_thread() function!!\n");
	char user_id[16] = {0};
	int  msg_code = 0;
	char  passkey[64] = {0};

	struct timeval timeout ;
	int ret = 0;
	int old_status = get_mp_status();
	int new_status = 0;

	//szData = (char *)malloc(sizeof(char) * 2048);
	nPos = cli_info->npos;
	input = cli_info->input;
	sSocket = GETSOCK_NEW(input, nPos);
	PRINTF("input = %d,sockfd =%d,pos =%d\n", input, sSocket, nPos);
	timeout.tv_sec = 3 ; //3
	timeout.tv_usec = 0;

	ret = setsockopt(sSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

	if(ret == -1) {
		ERR_PRN("setsockopt() Set Send Time Failed\n");
	}

	sem_post(&g_new_tcp_lock);
	PRINTF("recv_client_msg_thread     sem_post!!!!\n");

	while(!gblGetQuit()) {
		new_status = get_mp_status();

		if(old_status != new_status) {
			PRINTF("status is change ,new status =%d,old_status=%d\n", new_status, old_status);
			goto ExitClientMsg;
		}

		memset(szData, 0, sizeof(szData));

		if(sSocket <= 0) {
			goto ExitClientMsg;
		}

		total_len = tcp_recv(sSocket, (char *)&msg_info, sizeof(msg_info), 0);


		if(total_len == -1) {
			ERR_PRN("error:%d,error msg: = %s\n", errno, strerror(errno));
			goto ExitClientMsg;
		}

		if(ntohs(msg_info.sVer) != 2012) {
			ERR_PRN(" sver error:%d,error msg: = %s\n", errno, strerror(errno));
			goto ExitClientMsg;
		}

		//PRINTF( "receive length:%d,len=%d,ver = %d,%d,buf=%p\n", total_len,ntohs(msg_info.sLen),msg_info.sVer,ntohs(msg_info.sVer),szData);
		//message length
		//total_len = ntohs(msg_info.sLen);
		total_len = ntohs(msg_info.sLen) - sizeof(msg_info);

		if(total_len <= 0) {
			PRINTF("nMsgLen  < HEAD_LEN\n");
			goto ExitClientMsg;
		}

		if(total_len > 0) {
			//t1 = get_run_time();
			nLen = tcp_recv(sSocket, szData, total_len, 0);

			if(nLen == -1 || nLen < total_len) {
				ERR_PRN("nLen < nMsgLen -HEAD_LEN\n");

				goto ExitClientMsg;
			}

			//t2 = get_run_time();
			//PRINTF("@@@@@@@@@@@@@@@@t2 - t1 = %d@@@@@@@@@@@@@@@\n",t2-t1);


			if(parse_cli_xml_msg(input, nPos, szData, &msg_code, passkey, user_id) < 0) {
				ERR_PRN("parse xml msg failed\n");
				//goto ExitClientMsg;
			}

			//t3 = get_run_time();
			//PRINTF("###############t3 - t2 = %d######################\n",t3-t2);
		}

#if 0

		if(process_cli_xml_msg(nPos, msg_code, passkey, user_id) < 0) {
			PRINTF("process xml msg failed\n");
			//goto ExitClientMsg;
		}

#endif

		//PRINTF( "Switch End!\n");
	}

ExitClientMsg:
	//cli_pthread_id[nPos] = 0;
	PRINTF("Exit Client Msg thread:%d--->input=%d nPos = %d sSocket = %d\n", pthread_self(), input,nPos, sSocket);

	if(sSocket == GETSOCK_NEW(input, nPos)) {
		PRINTF("Exit Client Msg  nPos = %d sSocket = %d,lock_count=%d\n", nPos, sSocket, get_g_lock_count(nPos));
		SETCLIUSED_NEW(input, nPos, FALSE);
		SET_SEND_AUDIO_NEW(input, nPos, FALSE);
		SETCLILOGIN_NEW(input, nPos, FALSE);
		SETLOWRATEFLAG_NEW(input, nPos, 0);
		SET_SEND_VIDEO_NEW(input, nPos, FALSE);
		SET_PARSE_LOW_RATE_FLAG(input, nPos, INVALID_SOCKET);
		SET_FIX_RESOLUTION_FLAG(input, nPos, FALSE);
		//IISCloseLowRate_new();
		set_heart_count(input, nPos, FALSE);
		close(sSocket);
		SETSOCK_NEW(input, nPos, INVALID_SOCKET);
#ifdef SUPPORT_IP_MATRIX

		if(get_audio_lock_resolution() > 0  && (GET_PASSKEY_FLAG(input, nPos) == 1)) {
			cut_audio_lock_resolution();
		}

		if(get_video_lock_resolution() > 0 && (GET_PASSKEY_FLAG(input, nPos) == 1)) {
			cut_video_lock_resolution();
		}

		if(get_ipmatrix_lock_resolution() > 0 && (GET_PASSKEY_FLAG(input, nPos) == 1)) {
			cut_ipmatrix_lock_resolution();
		}

		SET_PASSKEY_FLAG(input, nPos, 0);
#endif
		lock_count = get_g_lock_count(nPos);

		if(lock_count > 0) {
			lock_count = lock_count - 1;
			set_g_lock_count(nPos, lock_count);
		}
	} else {

		int cli ;

		for(cli = 0; cli < MAX_CLIENT; cli++) {
			PRINTF("[%s %d],socket = %d,,blogin=%d,bused=%d,sendAudioFlag=%d\n", __FUNCTION__, __LINE__, g_client_para[input].cliDATA[cli].sSocket,
			       g_client_para[input].cliDATA[cli].bLogIn,
			       g_client_para[input].cliDATA[cli].bUsed,
			       g_client_para[input].cliDATA[cli].sendAudioFlag);
		}

		PRINTF("Warnning,this client is also cleared socket =%d,pos=%d,s=%d\n", sSocket, nPos, GETSOCK_NEW(input, nPos));
	}

	//if(szData != NULL)
	//	free(szData);
	pthread_detach(pthread_self());
	pthread_exit(NULL);
}

int create_signal_cli_thread(int sockfd, struct sockaddr_in *cli_addr, int input)
{
	char newipconnect[20] = {0};
	//打印客户端ip//
	inet_ntop(AF_INET, (void *) & (cli_addr->sin_addr), newipconnect, 16);
	PRINTF("input  = %d,sockfd =%d ip =%s\n", input, sockfd, newipconnect);
	int nPos = 0;
	int nLen = 0;
	int cli_num = 0;
	char send_buf[256] = {0};
	char user_id[8] = {0};
	char dtype[16] = {0};
	unsigned int cur_time = 0;
	static enc2000_xml_cli_info  cli_info[SIGNAL_INPUT_MAX];
	GetDeviceType(dtype);
	ClearLostClient_new(input);
	nPos = GetNullClientData_new(input);
	cli_num = read_client_num(input);
	PRINTF("============================== input = %d,GetNullClientData_new = %d==================\n", input, nPos);
	nLen = sizeof(struct sockaddr_in);

	if(-1 == nPos || cli_num >= 6) 	{
		//需要告警上报
		package_head_msg(send_buf, 30087, dtype, "0", user_id);
		tcp_send_data(sockfd, send_buf);
		ERR_PRN("ERROR: max client error\n");
		close(sockfd);
		sockfd = -1;
	} else {
		int nSize = 0;
		int result;
		int *pnPos = malloc(sizeof(int));

		/* set client used */

		PRINTF("input = %d,pos =%d,sockfd =%d\n", input, nPos, sockfd);
		SETCLIUSED_NEW(input, nPos, TRUE);
		//		SETCLILOGIN_NEW(input,nPos, TRUE);
		SETSOCK_NEW(input, nPos, sockfd);
		cur_time = getCurrentTime();
		SETCONNECTTIME_NEW(input, nPos, cur_time);
		nSize = 1;

		if((setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (void *)&nSize,
		               sizeof(nSize))) == -1) {
			perror("setsockopt failed");
		}

		nSize = 0;
		nLen = sizeof(nLen);
		result = getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &nSize , (socklen_t *)&nLen);

		if(result) {
			ERR_PRN("getsockopt() errno:%d socket:%d  result:%d\n", errno, sockfd, result);
		}

		nSize = 1;
		PRINTF("Clent:%s connected,nPos:%d socket:%d!\n", newipconnect, nPos, sockfd);

		//Create ClientMsg task!
		*pnPos = nPos;
		cli_info[input].npos = *pnPos;
		cli_info[input].input = input;
		result = pthread_create(&cli_pthread_id[input][nPos], NULL, (void *)recv_client_msg_thread, &(cli_info[input]));


		if(result) {
			close(sockfd);
			sockfd = -1;
			ERR_PRN("creat pthread ClientMsg error  = %d!\n" , errno);
			return 0;
		}

		sem_wait(&g_new_tcp_lock);
	}

	return 0;
}
void open_new_tcp_task(void *param)
{
	int serv_sockfd = -1, cli_sockfd;
	struct sockaddr_in serv_addr, cli_addr;
	int len = 0;
	int opt = 1;
	int ipos = 0;
	int clientsocket = -1;
	void  *ret = NULL;
	unsigned int ip1 = 0, ip2 = 0;
	unsigned int gateway1 = 0, gateway2 = 0;
	unsigned int netmask1 = 0, netmask2 = 0;
	int ret1 = 0;
	int *tmp = (int *)param;
	int input = *tmp;
	struct in_addr addr1, addr2;
	sem_init((sem_t *)&g_new_tcp_lock, 0, 0);

	InitClientData_new();
	PRINTF("input  = %d,open_new_tcp_task is start\n", input);

	if(SIGNAL_INPUT_1 == input) {
		ret1 = sys_get_network(ETH0, &ip1, &gateway1, &netmask1);

		if(ret1 == -1) {
			PRINTF("Error,get ip is error\n");
			sleep(5);
			//goto SERVERSTARTRUN;
		}

		memcpy(&addr1, &ip1, 4);
		PRINTF("input = %d, ip  = %s\n", input, inet_ntoa(addr1));
	} else if(SIGNAL_INPUT_2 == input) {
		ret1 = sys_get_network(ETH0_1, &ip2, &gateway2, &netmask2);

		if(ret1 == -1) {
			PRINTF("Error,get ip is error\n");
			sleep(5);
			//goto SERVERSTARTRUN;
		}

		memcpy(&addr2, &ip2, 4);
		PRINTF("input = %d, ip  = %s\n", input, inet_ntoa(addr2));
	} else {
		ret1 = sys_get_network(ETH0, &ip1, &gateway1, &netmask1);

		if(ret1 == -1) {
			PRINTF("Error,get ip is error\n");
			sleep(5);
			//goto SERVERSTARTRUN;
		}

		memcpy(&addr1, &ip1, 4);
	}

	//serv_sockfd = create_new_tcp_sock();
	bzero(&serv_addr, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	//serv_addr.sin_port = htons(NEW_TCP_SOCK_PORT);
	serv_addr.sin_port = htons(NEW_TCP_SOCK_PORT);

	if(SIGNAL_INPUT_1 == input) {
		serv_addr.sin_addr.s_addr = ip1;
	} else if(SIGNAL_INPUT_2 == input) {
		serv_addr.sin_addr.s_addr = ip2;
	} else {
		serv_addr.sin_addr.s_addr = ip1;
	}

	//inet_pton(AF_INET, "0.0.0.0", &serv_addr.sin_addr);
	serv_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(serv_sockfd < 0)  {
		ERR_PRN("ListenTask create error:%d,error msg: = %s\n", errno, strerror(errno));
		gblSetQuit();
		return;
	}

	setsockopt(serv_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if(bind(serv_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  {
		ERR_PRN("bind terror:%d,error msg: = %s\n", errno, strerror(errno));

		gblSetQuit();
		return;
	}

	if(listen(serv_sockfd, 10) < 0) {
		ERR_PRN("listen error:%d,error msg: = %s", errno, strerror(errno));
		gblSetQuit();
		return ;
	}

#if 0

	if((file_flag = fcntl(serv_sockfd, F_GETFL, 0)) == -1) {
		ERR_PRN("fcntl F_GETFL error:%d,error msg: = %s\n", errno, strerror(errno));

		//gblSetQuit();
		return;
	}

	if(fcntl(serv_sockfd, F_SETFL, file_flag | O_NONBLOCK) == -1) {
		ERR_PRN("fcntl F_SETFL error:%d,error msg: = %s\n", errno, strerror(errno));

		//gblSetQuit();
		return;
	}

#endif

	//PRINTF("input  = %d,ip1 = %s,ip2 = %s,sockfd=%d\n",input,inet_ntoa(addr1),inet_ntoa(addr2),serv_sockfd);
	while(!gblGetQuit()) {
		memset(&cli_addr, 0, sizeof(struct sockaddr_in));
		len = sizeof(struct sockaddr_in);
		cli_sockfd = accept(serv_sockfd , (void *)&cli_addr, (socklen_t *) &len);

		if(cli_sockfd > 0) {
			PRINTF("input = %d,serv_sockfd=%d,cli_sockfd=%d\n", input, serv_sockfd, cli_sockfd);
			create_signal_cli_thread(cli_sockfd, &cli_addr, input);
			PRINTF("sem_wait() semphone inval!!!\n");
		} else {
			//ERR_PRN("client connect failed errno =%d %s\n",errno,strerror(errno));
			if(errno == ECONNABORTED || errno == EAGAIN) { //软件原因中断
				//ERR_PRN("errno =%d program again start!!!\n",errno);
				usleep(100000);
				continue;
			}

			//if(serv_sockfd> 0)	{
			//close(serv_sockfd);
			//}
			usleep(3000000);
			//goto RECREATE_TCP_SOCK;
		}
	}

	for(input = 0; input < 2; input++) {
		for(ipos = 0; ipos < MAX_CLIENT_NUM; ipos++) {
			if(cli_pthread_id[input][ipos]) {
				clientsocket = GETSOCK_NEW(input, ipos);

				if(clientsocket != INVALID_SOCKET) {
					close(clientsocket);
					SETSOCK_NEW(input, ipos, INVALID_SOCKET);
				}

				if(pthread_join(cli_pthread_id[input][ipos], &ret) == 0) {
					if(ret == (void *) - 1) {
						ERR_PRN("drawtimethread ret == THREAD_FAILURE \n");
					}
				}
			}
		}
	}
}


void init_new_tcp_module(void)
{
	int ret = 0;
	pthread_t new_tcp_thread_id[SIGNAL_INPUT_MAX];
	static int index1 = SIGNAL_INPUT_1, index2 = SIGNAL_INPUT_2;
	pthread_t st_report_id[SIGNAL_INPUT_MAX];

	ret = pthread_create(&new_tcp_thread_id[index1], NULL, open_new_tcp_task, (void *)&index1);

	if(ret < 0) {
		ERR_PRN("create DSP1TCPTask() failed\n");
		return 0;
	}

	ret = pthread_create(&st_report_id[index1], NULL, report_pthread_fxn, (void *)&index1);

	if(ret < 0) {
		ERR_PRN("create DSP1TCPTask() failed\n");
		return 0;
	}

	ret = pthread_create(&new_tcp_thread_id[index2], NULL, open_new_tcp_task, (void *)&index2);

	if(ret < 0) {
		ERR_PRN("create DSP2TCPTask() failed\n");
		return 0;
	}

	ret = pthread_create(&st_report_id[index2], NULL, report_pthread_fxn, (void *)&index2);

	if(ret < 0) {
		ERR_PRN("create DSP2TCPTask() failed\n");
		return 0;

	}
}

#endif
