PROGS = mopd
OBJS = mopd.o process.o
LIBS = ../common/libcommon.a

CPPFLAGS = -I..

all: $(PROGS)

mopd: $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o mopd $(OBJS) $(LIBS)

../common/libcommon.a:
	cd ../common && $(MAKE) libcommon.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f core $(PROGS) *.o
