PROGS = mopd
OBJS = mopd.o process.o
LIBS = ../common/libcommon.a
LIBELF = -lelf

CPPFLAGS = -I..

all: $(PROGS)

mopd: $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o mopd $(OBJS) $(LIBS) $(LIBELF)

../common/libcommon.a:
	cd ../common && $(MAKE) libcommon.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f core $(PROGS) *.o
