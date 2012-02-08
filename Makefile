V1190B_MAKEFILE:=$(lastword $(MAKEFILE_LIST))
include $(PPC860_BASE)/linux/apps/Makeconfig

V1190B_BASE=$(dir $(V1190B_MAKEFILE))

VPATH+=$(V1190B_BASE)

DIRECT_LIB = $(PPC860_BASE)/linux/apps/bivme2_direct_lib
INCLUDE += -I$(DIRECT_LIB)
LDFLAGS += -L$(DIRECT_LIB) -lvmedirect
CPPFLAGS += $(INCLUDE) 

all: V1190B_server V1190B_trigger

report:
	@echo V1190B_MAKEFILE=$(V1190B_MAKEFILE)
	@echo VPATH=$(VPATH)

clean:
	rm -f *.o $(TARGET)

V1190B_server.o: V1190B.h 
V1190B_server.o: CPPFLAGS+=-DBUILDDATE="\"$(shell date "+%F %H:%M:%S")\""
V1190B.o: V1190B.h
V1190B_trigger.o: V1190B.h
 
V1190B_trigger: V1190B_trigger.o V1190B.o
	$(CC) -o $@ $^ $(LDFLAGS)


V1190B_server: V1190B_server.o V1190B.o
	$(CC) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@
	
.PHONY: Debug Release

Debug: CPPFLAGS+=-O0
Release: CPPFLAGS+=-DNDEBUG=1 -O3
 
Debug Release:
	mkdir -p $@ && cd $@ && $(MAKE) -f $(abspath $(V1190B_MAKEFILE)) V1190B_server V1190B_trigger
	
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(INCLUDE) $(CPPFLAGS) $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
	
upload: Debug
	scp Debug/V1190B_server beam-daq:~daq/bin
	

