PROGS = mopchk
OBJS = mopchk.o
LIBS = ../common/libcommon.a
LIBELF = -lelf

CPPFLAGS = -I..

all: $(PROGS)

mopchk: $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o mopchk $(OBJS) $(LIBS) $(LIBELF)

../common/libcommon.a:
	cd ../common && $(MAKE) libcommon.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f core $(PROGS) *.o
