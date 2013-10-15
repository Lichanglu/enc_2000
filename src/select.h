#ifndef _SELECT_H_
#define _SELECT_H_


#define Reach_floor(val, align)  (((val) / (align)) * (align))

#define Reach_align(val, align)  Reach_floor(((val) + (align)-1), (align))

extern int g_vp0_writeyuv_flag;
extern int g_vp1_writeyuv_flag;
extern int g_mp_writeyuv_flag;


Int32 reach_select_process(ENC2000_LINK_STRUCT *pstruct);

Int32 reach_MP_select_process(ENC2000_LINK_STRUCT *pstruct);

Int32 reach_writeyuv_process(ENC2000_LINK_STRUCT *pstruct);

Int32 set_resolution(Int32 VpNum, Int32 streamId, UInt32 outWidth, UInt32 outHeight,UInt32 inWidth,UInt32 inHeight);

#endif

