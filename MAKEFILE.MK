
CURRENT_DIR =  $(CURDIR)
MODULE_DIR =  $(notdir $(CURDIR))

check_version:
	./get_version.sh
	@if diff  pro_ver.h SDK_ver.h ; then \
		echo "check SDK version! *******OK*******"; \
	else \
		echo "Error check SDK version mismatch!!!   please check version.h and pro_ver.h files! " ; \
		exit 1;  \
	fi



demo:
	echo #$(MODULE_DIR)#
	echo #$(CURRENT_DIR)#
	@echo "const static char *g_make_build_date = \"`env LANG=en_US.UTF-8 date`\";" >./src/build_info.h

#	@echo "const static char *g_make_build_date = \"`date`\";" >./src/build_info.h
	@sed -e "s#<version>#$$(git describe --dirty --always)#g" <app_version.h.in> app_version.h.tmp
	@if diff -q app_version.h.tmp app_version.h >/dev/null 2>&1; then \
		rm app_version.h.tmp; \
	else \
		echo "app_version.h.in => ./src/app_version.h" ; \
	mv app_version.h.tmp ./src/app_version.h; \
	fi
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/ -p
#add subdir 	
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/extern_update_header -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/middle -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/stream_output -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/modules -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/modules/porting -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/text -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/xml -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/front_panel_ctrl -p
	mkdir $(dvr_rdk_PATH)/lib/ti816x-evm/obj/$(MODULE_DIR)/ip_matrix -p
	#$(MAKE) -fMAKEFILE.MK check_version
	$(MAKE) -fMAKEFILE.MK libs
	$(MAKE) -fMAKEFILE.MK apps TARGET=exe

libs:
	#$(MAKE) -f Makefile MODULE=$(MODULE_DIR) $(TARGET)
	$(MAKE) -f Makefile -C$(dvr_rdk_PATH)/$(MODULE_DIR)/src MODULE=$(MODULE_DIR) $(TARGET)

apps:
	#$(MAKE) -f Makefile MODULE=$(MODULE_DIR) $(TARGET)
	$(MAKE) -f Makefile -C$(dvr_rdk_PATH)/$(MODULE_DIR)/src MODULE=$(MODULE_DIR) $(TARGET)

clean:
	$(MAKE) -fMAKEFILE.MK libs TARGET=clean


format:
	$(MAKE) -f Makefile -C$(dvr_rdk_PATH)/$(MODULE_DIR)/src format
	
	
all: clean demo

