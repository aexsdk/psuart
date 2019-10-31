OBJS = ini.o psuservice.o utils.o xLogMessage.o

CFLAGS += -DHAVE_CONFIG_H -DLINUX -I../inc

all: $(OBJS) 

romfs:
	$(ROMFSINST) psuservice.sh

clean:
	rm -f *.o

