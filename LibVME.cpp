extern "C" {
	#include "libvmedirect.h"
}
#include "LibVME.h"
#include <stdexcept>
#include <assert.h>
#include <sstream>
#include <iomanip>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h> //close


using namespace std;

LibVME::LibVME() {
	//Library init
	if (libvme_init() < 0)
		throw runtime_error("libvme init failed");
	//Setting system controller mode
	ctl(VME_SYSTEM_W, 1);
	//Setting VME address modifier "A32 supervisory data access" 0xd
	ctl(VME_AM_W, 0xd);
}

struct ModeAccess {
	int mode;
	string name;
	bool write;
};

const ModeAccess & getModeAccess(int mode)
{
#define ADDREAD(x) \
	case VME_##x##_R: {static const ModeAccess a = {VME_##x##_R, #x, false}; return a;} break;
#define ADD(x) \
	case VME_##x##_W: {static const ModeAccess a = {VME_##x##_R, #x, true}; return a;} break;
	switch(mode) {
		ADD(AM);
		ADD(BUSL);
		ADD(SYSTEM);
		ADD(ARB);
		ADD(SYSFAIL);
		ADDREAD(ACFAIL);
	default:
		throw VmeError("Invalid controller mode");
	}
#undef ADD
#undef ADDREAD
}

int LibVME::ctl(int mode)
{
	assert(!getModeAccess(mode).write);
	int rv;
	if (libvme_ctl(mode, &rv) != 0) {
		ostringstream error;
		error << "Failed to get controller mode 0x" << hex;
		throw VmeError(error.str());
	}
	return rv;
}

void LibVME::ctl(int mode, int value)
{
	assert(getModeAccess(mode).write);
	if (libvme_ctl(mode, &value) != 0) {
		ostringstream error;
		error << "Failed to set controller mode 0x" << hex << " to value 0x" << hex << value;
		throw VmeError(error.str());
	}
}

void LibVME::write_a32d16(uint32_t address, uint16_t value)
{
	if (libvme_write_a32_word(address, value)!=0)
		throw VmeWriteError("Failed to write to VME a32 d16");
}

uint16_t LibVME::read_a32d16(uint32_t address)
{
	uint16_t rv = 0;
	if (libvme_read_a32_word(address, &rv)!=0)
		throw VmeReadError("Failed to write to VME a32 d16");
	return rv;
}

void LibVME::write_a32d32(uint32_t address, uint32_t value)
{
	if (libvme_write_a32_dword(address, value)!=0)
		throw VmeWriteError("Failed to write to VME a32 d32");
}

uint32_t LibVME::read_a32d32(uint32_t address)
{
	uint32_t rv = 0;
	if (libvme_read_a32_dword(address, &rv)!=0)
		throw VmeReadError("Failed to write to VME a32 d16");
	return rv;
}

#define IRQFILE "/dev/vmei%d"

typedef LibVME::InterruptLine InterruptLine;

InterruptLine::InterruptLine(int fd, unsigned level): _fd(fd), _level(level) {}

InterruptLine::~InterruptLine() {
	close(_fd);
}

int32_t InterruptLine::readVector() const {
	int32_t vector;
	int bytes = read(fd(), &vector, 4);
	if (bytes != 4) {
		ostringstream tmp;
		tmp << "Failed to read interrupt vector";
		throw VmeError(tmp.str());
	}
	return vector;

}
auto_ptr<InterruptLine> LibVME::openIrqLevel()
{
	int i, fdi;
	char str[40];
	for(i=1; i<=7; i++) {
		sprintf(str, IRQFILE, i);
		fdi = open(str, O_RDONLY);
		if (fdi >= 0) {
			return auto_ptr<InterruptLine>(new InterruptLine(fdi, i));
		}
	}
	throw VmeError("Can't lock IRQ level");
}

LibVME::~LibVME() {
	libvme_close();
}

void LibVME::addressModifier(int am)
{
	assert(am & 0xf == am);
	ctl(VME_AM_W, am);
}

int LibVME::addressModifier()
{
	int rv = 0;
	return ctl(VME_AM_R);
}



