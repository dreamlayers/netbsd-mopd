LIBS = libcommon.a
OBJS = cmp.o device.o dl.o file.o get.o loop-bsd.o mopdef.o nma.o pf-linux.o \
	pf.o print.o put.o rc.o version.o

CPPFLAGS =

all: $(LIBS)

libcommon.a: $(OBJS)
	-rm -f libcommon.a
	$(AR) cru libcommon.a $(OBJS)
	$(RANLIB) libcommon.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
	
version.c: VERSION
	sed 's/.*/char version[] = "&";/' < VERSION > version.c
	
clean: 
	rm -f core *.a *.o version.c
