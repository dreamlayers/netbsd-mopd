PROGS = mopprobe
OBJS = mopprobe.o
LIBS = ../common/libcommon.a

CPPFLAGS = -I..

all: $(PROGS)

mopprobe: $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o mopprobe $(OBJS) $(LIBS)

../common/libcommon.a:
	cd ../common && $(MAKE) libcommon.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f core $(PROGS) *.o
