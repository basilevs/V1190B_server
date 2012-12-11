V1190BT_MAKEFILE:=$(lastword $(MAKEFILE_LIST))
include $(PPC860_BASE)/linux/apps/Makeconfig
V1190BT_BASE=$(dir $(V1190BT_MAKEFILE))

VPATH+=$(V1190BT_BASE)/..:$(V1190BT_BASE)/../socketbuf:$(V1190BT_BASE)

DIRECT_LIB = $(PPC860_BASE)/linux/apps/bivme2_direct_lib
INCLUDE += -I$(DIRECT_LIB) -I$(V1190BT_BASE)/..
LDFLAGS += -L$(DIRECT_LIB) -lvmedirect -pthread
CPPFLAGS += $(INCLUDE) -fmessage-length=0 "-DBUILDDATE=$(shell date "+%F %H:%M:%S")" -pthread


UNITS = interface trigger LibVME V1190B2 V1190B2_server socketbuf Thread

all: interface trigger V1190B2_server socket thread

DEPGEN=$(CXX) -M $(CPPFLAGS) $< | sed 's,\($*\)\[ :]*,\1 $@ : ,g' > $@

%.cpp.d: %.cpp
	@$(DEPGEN)
DEPS=$(addsuffix .cpp.d, $(UNITS))
ifeq "$(filter clean,$(MAKECMDGOALS))" ""
	-include $(DEPS)
endif

interface: interface.o LibVME.o V1190B2.o
	$(CXX) -static -o $@ $^ $(LDFLAGS)
CLEAN+=interface
	
trigger: trigger.o  LibVME.o V1190B2.o
	$(CXX) -o $@ $^ $(LDFLAGS)
CLEAN+=trigger

V1190B2_server: V1190B2_server.o LibVME.o V1190B2.o CommandProcessor.o Thread.o socketbuf.o
	$(CXX) -o $@ $^ $(LDFLAGS)
CLEAN+=V1190B2_server

socket: socket.o Thread.o socketbuf.o
	$(CXX) -o $@ $^ $(LDFLAGS)
CLEAN+=socket

thread: thread.o Thread.o 
	$(CXX) -o $@ $^ $(LDFLAGS)
CLEAN+=thread	

upload: socket V1190B2_server
	scp $^ meson:/opt/daq/bin
	
clean:
	rm -f $(addsuffix .o, $(UNITS)) $(DEPS) $(CLEAN)
	