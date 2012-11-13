V1190BT_MAKEFILE:=$(lastword $(MAKEFILE_LIST))
include $(PPC860_BASE)/linux/apps/Makeconfig
V1190BT_BASE=$(dir $(V1190BT_MAKEFILE))

VPATH+=$(V1190BT_BASE)/..:$(V1190BT_BASE)

DIRECT_LIB = $(PPC860_BASE)/linux/apps/bivme2_direct_lib
INCLUDE += -I$(DIRECT_LIB) -I$(V1190BT_BASE)/..
LDFLAGS += -L$(DIRECT_LIB) -lvmedirect
CPPFLAGS += $(INCLUDE) -fmessage-length=0 "-DBUILDDATE=$(shell date "+%F %H:%M:%S")" -pthread


UNITS = interface trigger LibVME V1190B2 V1190B2_server socketbuf Thread

all: interface trigger V1190B2_server socket

DEPGEN=$(CXX) -M $(CPPFLAGS) $< | sed 's,\($*\)\[ :]*,\1 $@ : ,g' > $@

%.cpp.d: %.cpp
	@$(DEPGEN)
DEPS=$(addsuffix .cpp.d, $(UNITS))
ifeq "$(filter clean,$(MAKECMDGOALS))" ""
	-include $(DEPS)
endif


interface: interface.o LibVME.o V1190B2.o
	$(CXX) -static -o $@ $^ $(LDFLAGS)
	
trigger: trigger.o  LibVME.o V1190B2.o
	$(CXX) -static -o $@ $^ $(LDFLAGS)

V1190B2_server: V1190B2_server.o LibVME.o V1190B2.o CommandProcessor.o Thread.o socketbuf.o
	$(CXX) -pthread -static -o $@ $^ $(LDFLAGS)

socket: socket.o Thread.o socketbuf.o
	$(CXX) -pthread -static -o $@ $^ $(LDFLAGS)
	
upload: trigger interface V1190B2_server socket
	scp $^ meson:/opt/daq/bin