#ifndef _PNG_2_YUV_H
#define _PNG_2_YUV_H



 int write_yuv4xx_file(unsigned char *buf,int buf_len,char* filename);
 int rgb_2_yuv422(unsigned char * out,unsigned char * * row_pointers,int height,int width);
 int rgb_2_yuyv422(unsigned char *out, unsigned char **row_pointers, int height, int width);
 void yuv422_2_yuv420(unsigned char * yuv420,unsigned char * yuv422,int width,int height);
 void yuv420p_2_yuv420sp(unsigned char * yuv420sp,unsigned char * yuv420p,int width,int height);
 void write_yuv420sp_file(unsigned char *yuv420sp, int width, int height,char* filename);
 void yuv422Y16_2_yuv422yuyv( unsigned char * yuyv,unsigned char *y16, int width, int height);
 void yuvuv420_2_yuyv422(unsigned char* out,unsigned char* buf,int width,int height);


#endif
