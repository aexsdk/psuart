DIRS = src

all: 
	for i in $(DIRS) ; do $(MAKE) -C $$i $@ || exit $?; done 
	
romfs:
	for i in $(DIRS) ; do $(MAKE) -C $$i $@ || exit $?; done
	#$(ROMFSINST) /bin/psuservice.sh

clean:
	for i in $(DIRS) ; do $(MAKE) -C $$i $@ || exit $?; done

