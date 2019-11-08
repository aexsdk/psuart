EXEC = psuart
OBJS = ini.o psuservice.o utils.o xLogMessage.o ringBuffer.o

CFLAGS += -DHAVE_CONFIG_H -DLINUX -I../inc

all: $(EXEC)

$(EXEC): $(OBJS) $(CONF_H)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

romfs:
	$(ROMFSINST) /bin/$(EXEC)
	$(ROMFSINST) /bin/psuservice.sh

clean:
	rm -f $(EXEC) *.o

