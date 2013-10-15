FILESYS=backfs/usr/local/reach/
FILESYS_CGI=backfs/usr/local/reach/var/www/html/cgi-bin/
#CC=/opt/arm-2009q1/bin/arm-none-linux-gnueabi-gcc
CC=/opt/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-linux-gnueabi-gcc
LIBS := -lpthread
INCLUDE := -fpic -I./
TARGETS=backapp
DATE=$(shell date +%Y%m%d)
SRC := $(wildcard ./*.c)
CFLAGS := -g
OBJS_DIR = objs
$(shell mkdir -p $(OBJS_DIR))
OBJS_DEST= $(wildcard $(OBJS_DIR)/*.o)
OBJS = $(SRC:.c=.o)
$(TARGETS):$(OBJS)
	$(CC) -o $(TARGETS) $(OBJS_DEST) $(LIBS)
	cp $(TARGETS) /home/reach/workdir/backfs/usr/local/reach
.c.o:
	$(CC) $(INCLUDE) $(CFLAGS) -c $<
	mv *.o $(OBJS_DIR)
.PHONY:cgi
cgi:
	$(MAKE) -C cgi -f Makefile
cgi_clean:
	$(MAKE) -C cgi -f Makefile clean
pkg:
	mkdir -p backfs
	tar xfvz backfs_enc2000_*.tgz -C backfs
	rm -rf backfs_enc2000_*.tgz
	cp -rf S90ReachEth0Config backfs/etc/rc3.d/
	cp -rf $(TARGETS) $(FILESYS)
#	cp -rf cgi/encoder.cgi $(FILESYS_CGI)
	cd backfs;tar cfvz ../backfs_enc2000_$(DATE).tgz .
	cd ..
	rm -rf backfs
clean:
	rm -rf $(OBJS_DIR)
	rm  -rf ./*.o
	rm -rf $(TARGETS)
