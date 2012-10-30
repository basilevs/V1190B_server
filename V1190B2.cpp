#include "V1190B2.h"

const int tries = 100;

void V1190B2::reset()
{
	_interface.write_a32d16(_address+0x1014, 0xffff);
}

void V1190B2::triggerMode(bool isTrigger)
{
	writeOpcode(isTrigger ? 0 : 0x0100);
	assert(triggerMode() == isTrigger);
}

void V1190B2::waitOpcode(bool write)
{
	for (int i = 0 ; i < tries; i++) {
		uint16_t value = _interface.read_a32d16(_address+0x1030);
		bool ready = false;
		if (write) {
			ready = (value & 1) ? true : false;
		} else {
			ready = (value & 2) ? true : false;
		}
		if (ready)
			return;
	}
	throw VmeError("Timeout while waiting for opcode");
}

void V1190B2::writeOpcode(uint16_t opcode)
{
	waitOpcode(true);
	_interface.write_a32d16(_address+0x102E, opcode);
}

uint16_t V1190B2::readOpcode()
{
	waitOpcode(false);
	return _interface.read_a32d16(_address+0x102E);
}

bool V1190B2::triggerMode()
{
	writeOpcode(0x0200);
	uint16_t data = readOpcode();
	return (data & 1) ? true : false;
}

void V1190B2::configureInterrupt(uint8_t level, uint8_t vector)
{
	assert((level & 7) == level);
	_interface.write_a32d16(_address+0x100A, level);
	_interface.write_a32d16(_address+0x100C, vector);
}




