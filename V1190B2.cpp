#include "V1190B2.h"
#include <unistd.h>

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
		usleep(10);
	}
	throw Error("Timeout while waiting for opcode");
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

bool V1190B2::test()
{
	uint32_t data = 10000003;
	_interface.write_a32d32(_address+0x1200, data);
	return _interface.read_a32d32(_address+0x1200) == data;
}

unsigned V1190B2::bufferedEventsCount()
{
	return _interface.read_a32d16(_address+0x1020);
}

void V1190B2::setScanPathSetupBit(unsigned  index, bool isSet)
{
	unsigned wordIndex=index/16, wordBit=index%16;
	uint16_t setupWord = scanPathWord(wordIndex);
	if (isSet) {
		setupWord|=(1<<wordBit);
	} else {
		setupWord&=~(1<<wordBit);
	}
//	eprintf("Word became: %x\n", setupWord);
	scanPathWord(wordIndex, setupWord);
}

void V1190B2::loadScanPathSetup()
{
	writeOpcode(0x7200);
}

void V1190B2::edgeDetectionMode(unsigned  mode)
{
	if (mode>=4)
		throw Error("Invalid edge detection mode.");
	writeOpcode(0x2200);
	writeOpcode(mode);
}

void V1190B2::setWindow(int offset, int width)
{
	assert(width>=1);
	assert(width<=4095);
	assert(offset>=-2048);
	assert(offset<=40);
	assert(offset+width<=40);
	writeOpcode(0x1100);
	writeOpcode(offset);
	writeOpcode(0x1000);
	writeOpcode(width);
	uint16_t data[5];
	readTriggerConfiguration(data);
	assert(data[0] == width);
#ifndef NDEBUG
	assert(int(data[1]) == offset);
#endif
}





void V1190B2::highSpeedSerialization()
{
	setScanPathSetupBit(26, 1);
	loadScanPathSetup();
}



void V1190B2::highSpeedCoreClock()
{
	setScanPathSetupBit(622, true);
	setScanPathSetupBit(623, false);
	loadScanPathSetup();
}




uint16_t V1190B2::scanPathWord(unsigned index)
{
	assert(index<=0x28);
	writeOpcode(0x7100 + index);
	return readOpcode();
}

void V1190B2::scanPathWord(unsigned index, uint16_t word)
{
	assert(index<=0x28);
	writeOpcode(0x7000 + index);
	writeOpcode(word);
	assert(scanPathWord(index) == word);
}

void V1190B2::readTriggerConfiguration(uint16_t *data)
{
	writeOpcode(0x1600);
	for(int i=0; i<5; i++) {
		data[i] = readOpcode();
	}
}

uint32_t V1190B2::read()
{
	return _interface.read_a32d32(_address);
}

uint16_t V1190B2::control()
{
	return _interface.read_a32d16(_address+0x1000);
}

void V1190B2::control(uint16_t value)
{
	assert(value & 0x3F == 0);
	_interface.write_a32d16(_address+0x1000, value);
	assert(value == control());
}

void V1190B2::extendedTriggerTimeTag()
{
	uint16_t word = control();
	if (true) {
		word |= (1 << 9);
	} else {
		word &= ~(1 << 9);
	}
	control(word);
}

void V1190B2::clear()
{
	_interface.write_a32d16(_address+0x1016, 0);
}












