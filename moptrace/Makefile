PROGS = moptrace
OBJS = moptrace.o
LIBS = ../common/libcommon.a

CPPFLAGS = -I..

all: $(PROGS)

moptrace: $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o moptrace $(OBJS) $(LIBS)

../common/libcommon.a:
	cd ../common && $(MAKE) libcommon.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f core $(PROGS) *.o
