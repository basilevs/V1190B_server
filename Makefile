V1190B_MAKEFILE:=$(lastword $(MAKEFILE_LIST))
include $(PPC860_BASE)/linux/apps/Makeconfig

V1190B_BASE=$(dir $(V1190B_MAKEFILE))

VPATH+=$(V1190B_BASE)

DIRECT_LIB = $(PPC860_BASE)/linux/apps/bivme2_direct_lib
INCLUDE += -I$(DIRECT_LIB)
LDFLAGS += -L$(DIRECT_LIB) -lvmedirect
CPPFLAGS += $(INCLUDE) 

.PHONY: Debug Release tests

all: V1190B_server2

tests:
	$(MAKE) -f $(V1190B_BASE)tests/Makefile 

report:
	@echo V1190B_MAKEFILE=$(V1190B_MAKEFILE)
	@echo VPATH=$(VPATH)

clean:
	rm -f *.o $(TARGET)
	rm -rf Debug Release 

V1190B_server.o: V1190B.h 
V1190B_server.o: CPPFLAGS+=-DBUILDDATE="\"$(shell date "+%F %H:%M:%S")\""
V1190B.o: V1190B.h
V1190B_trigger.o: V1190B.h
 
V1190B_trigger: V1190B_trigger.o V1190B.o
	$(CC) -o $@ $^ $(LDFLAGS)


V1190B_server: V1190B_server.o V1190B.o
	$(CC) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@
	
DEPGEN=$(CC) -M $(CPPFLAGS) $< | sed 's,\($*\)\[ :]*,\1 $@ : ,g' > $@
%.cpp.d: %.cpp
	@$(DEPGEN)

UNITS=LibVME V1190B2
DEPS=$(addsuffix .cpp.d, $(UNITS))
ifeq "$(filter clean,$(MAKECMDGOALS))" ""
	-include $(DEPS)
endif

V1190B_server2: LibVME.o V1190B2.o
	$(CXX) -o $@ $^ $(LDFLAGS)

	
	
%.d: %.c
	@set -e; rm -f $@; \
	$(CC) -M $(INCLUDE) $(CPPFLAGS) $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$
	
upload: 
	scp Debug/V1190B_server beam-daq:~daq/bin
	

