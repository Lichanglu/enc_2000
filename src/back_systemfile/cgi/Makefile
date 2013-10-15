CFLAGS=-g -Wall
CC=/opt/arm-2009q1/bin/arm-none-linux-gnueabi-gcc
AR=ar
RANLIB=ranlib
LIBS=-L./ -lcgic
LIBOBJS = cgi.o cgic.o htmllib.o Tools.o weblib.o webtcpcom.o
REACHDLL=encoder.cgi
LDFLAGS=-L./ 

all:$(REACHDLL)
%.o : %.c
	@echo -- Compiling "-$(CC) $(CFLAGS) -c  $< -o $@"
	-$(CC) $(CFLAGS) -c  $< -o $@

$(REACHDLL) :$(LIBOBJS)
	@echo -- Linking [ $< ]
	@echo "-$(CC) $(LDFLAGS) $(LIBOBJS) -o $@ "
	-$(CC) $(LDFLAGS) $(LIBOBJS) -lrt -o $@ 
#	cp -rf encoder.cgi ../backfs/usr/local/reach/var/www/html/cgi-bin/.
	rm -rf build_info.h
	
$(LIBOBJS):weblib.h
$(LIBOBJS):webmiddle.h
$(LIBOBJS):webtcpcom.h

install: libcgic.a
	cp libcgic.a /usr/local/lib
	cp cgic.h /usr/local/include
	@echo libcgic.a is in /usr/local/lib. cgic.h is in /usr/local/include.

libcgic.a: cgic.o cgic.h
	rm -f libcgic.a
	$(AR) rc libcgic.a cgic.o
	$(RANLIB) libcgic.a

#mingw32 and cygwin users: replace .cgi with .exe

clean:
	rm -f *.o *.a encoder.cgi

