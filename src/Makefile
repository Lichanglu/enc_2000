
include $(dvr_rdk_PATH)/makerules/common_header_a8.mk

INCLUDE+= $(MCFW_INC) $(OSA_INC) $(SYSLINK_INC) $(CODEC_INC) $(LINUXDEVKIT_INC) -I./mp4/include/ -I./text -I./text/freetype2 

SOURCE_DIR = $(shell pwd)
LIBS= -L/home/reach/workdir/rfs/usr/lib/ -lpng -lfreetype -lz  -lm -lrt 

#FILES+=$(subst ./xml/, , $(foreach dir,.,$(wildcard $(dir)/*.c)) )
#FILES+=./xml/*.c
##add new subdir
INCLUDE += -I./modules/include \
 -I./modules/porting \
 -I./stream_output \
 -I./middle\
 -I./text \
 -I./xml \
 -I./extern_update_header\
 -I./front_panel_ctrl\
 -I./ip_matrix
EXTERN_FILE := $(wildcard middle/*.c)\
 $(wildcard stream_output/*.c)\
 $(wildcard modules/porting/*.c)\
 $(wildcard text/*.c)\
 $(wildcard xml/*.c)\
 $(wildcard extern_update_header/*.c)\
 $(wildcard front_panel_ctrl/*.c)\
 $(wildcard ip_matrix/*.c)


EXTERN_DEFINE := -DHAVE_RTSP_MODULE
EXTERN_DEFINE += -DHAVE_OSD_MODULE


LIBS +=  '-Wl,--start-group' 
LIBS += $(MCFW_LIBS) $(AUDIO_LIBS) $(RPE_LIBS) 
LIBS +=  $(LIB_DIR)/reach_enc2000.a
LIBS += $(dvr_rdk_PATH)/reach_enc2000/lib/libxml2.a $(dvr_rdk_PATH)/reach_enc2000/lib/libz.a
LIBS += ./modules/libs/libcurlarm.a
LIBS += ./modules/libs/libcurl_portarm.a
LIBS += ./modules/libs/liblog.a
LIBS += ./modules/libs/librtp_build.a
LIBS += ./modules/libs/librtsp.a
LIBS += ./modules/libs/libts_build.a
LIBS += '-Wl,--end-group'

LIBS += $(LIBS_OLD)

include $(dvr_rdk_PATH)/makerules/common_footer_a8.mk


