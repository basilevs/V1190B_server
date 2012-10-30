V1190BT_MAKEFILE:=$(lastword $(MAKEFILE_LIST))
include $(PPC860_BASE)/linux/apps/Makeconfig
V1190BT_BASE=$(dir $(V1190BT_MAKEFILE))

VPATH+=$(V1190BT_BASE)/..:$(V1190BT_BASE)

DIRECT_LIB = $(PPC860_BASE)/linux/apps/bivme2_direct_lib
INCLUDE += -I$(DIRECT_LIB) -I$(V1190BT_BASE)/..
LDFLAGS += -L$(DIRECT_LIB) -lvmedirect
CPPFLAGS += $(INCLUDE) 


UNITS = interface LibVME V1190B2

DEPGEN=$(CC) -M $(CPPFLAGS) $< | sed 's,\($*\)\[ :]*,\1 $@ : ,g' > $@
%.cpp.d: %.cpp
	@$(DEPGEN)
DEPS=$(addsuffix .cpp.d, $(UNITS))
ifeq "$(filter clean,$(MAKECMDGOALS))" ""
	-include $(DEPS)
endif

all: interface

interface: interface.o LibVME.o V1190B2.o
	$(CXX) -o $@ $^ $(LDFLAGS)

upload: all
	scp interface meson:/opt/daq/bin