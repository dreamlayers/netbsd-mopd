PROGS = mopa.out
OBJS = mopa.out.o
LIBS = ../common/libcommon.a

CPPFLAGS = -I..

all: $(PROGS)

mopa.out: $(OBJS) $(LIBS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o mopa.out $(OBJS) $(LIBS)

../common/libcommon.a:
	cd ../common && $(MAKE) libcommon.a

.c.o:
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f core $(PROGS) *.o
