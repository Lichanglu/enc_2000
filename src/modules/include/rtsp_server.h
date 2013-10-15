#ifndef _RTSP_SERVER_H__
#define _RTSP_SERVER_H__
//#define PRINT_DEUBG

//enum{
//	INVALID_CLIENT = -1,
//	VLC_CLIENT = 0,
//	QT_CLIENT,
//	STB_TS_CLIENT,
//	DEFAULT_CLIENT
//};


//typedef struct _DataHeader110E
//{
//	unsigned int id;
//	unsigned int biWidth;							//VGA£ºWidth		Audio£ºSampleRate
//	unsigned int biHeight;							//VGA£ºHeight	Audio£ºBitRate
//	unsigned int biBitCount;							//VGA£ºBitCount	Audio£ºChannel
//	unsigned int fccCompressedHandler;				//VGA£ºmmioFOURCC (¡®H¡¯,¡¯2¡¯,¡¯6¡¯,¡¯4¡¯)
//											    //Audio£ºmmioFOURCC(¡®A¡¯,¡¯D¡¯,¡¯T¡¯,¡¯S¡¯)
//	unsigned int  fccHandler;	   						//Audio£ºSampleBit
//	unsigned int dwCompressedDataLength;
//	unsigned int dwFlags;
//	unsigned int  dwSegment;
//	unsigned int  dwPacketNumber;
//}DataHeader_110E;

int RtspClientIsNull(void);

//int RtspVideoPack(int nLen, unsigned char * pData, int nFlag,unsigned int timetick,int *seq,int roomid,int idr_flag);
//int RtspAudioPack(int nLen,	unsigned char *pData,unsigned int timetick, int roomid,unsigned int samplerate);

//void RtspTask(void);
void rtsp_module_init(void);
//int RtspGetlocalip(unsigned int local_ip);
//char *RtspGetVersion(void);
//int RtspGetPort(void);
void rtsp_change_sdp_info( int roomid);

int rtsp_get_local_ip(char *buff,int len);
void rtsp_close_all_client(void );
//int RtspAdudioSeqReset(void);

//int rtsp_stream_rebuild_audio_frame(unsigned char *buff,unsigned int len ,int id);
//int rtsp_stream_rebuild_video_frame(unsigned char *buff,unsigned int len ,int id);
//int rtsp_stream_rebuild_video_frame_110e(unsigned char *buff,unsigned int len ,int id,DataHeader_110E *param);
//int rtsp_stream_rebuild_audio_frame_110e(unsigned char *buff,unsigned int len ,int id,DataHeader_110E *param);
//
//


int RtspSetPort(unsigned short port);

int rtsp_set_local_ip(unsigned int localipaddr);
int rtsp_set_keepactive_time(int s);


int rtsp_ts_get_client_num(void);




#endif
