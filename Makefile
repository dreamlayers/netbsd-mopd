AR = ar
CC = gcc
RANLIB = ranlib

CFLAGS = -O2 -g
LDFLAGS =
LIBELF = -lelf

SUBDIRS = common mopd mopchk mopprobe moptrace

all clean: 
	@for dir in $(SUBDIRS); do \
		(cd $$dir && \
		 $(MAKE) "AR=$(AR)" "CC=$(CC)" "RANLIB=$(RANLIB)" \
			 "CFLAGS=$(CFLAGS)" "LDFLAGS=$(LDFLAGS)" \
			 "LIBELF=$(LIBELF)" $@) || \
		 exit 1; \
	done
