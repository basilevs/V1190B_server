#include "V1190B.h"
#include "libvmedirect.h"

#define TDC_MHS_TIMEOUT		1
#define TDC_MHS_WRITE_OK	1
#define TDC_MHS_READ_OK		2

Error tdc_softwareReset(BusAddr tdcBase) {
	if (libvme_write_a32_word(tdcBase+0x1014, 0xffff)!=0) {
		return WRITE_ERROR;
	}
	return OK;
}

Error tdc_softwareClear(BusAddr tdcBase) {
	if (libvme_write_a32_word(tdcBase+0x1016, 0)!=0) {
		eprintf("Tdc software clear failed\n");
		return WRITE_ERROR;
	}
	return OK;
}

Error tdc_readControlRegister(BusAddr tdcBase, unsigned short * oControl) {
	assert(oControl);
	if (libvme_read_a32_word(tdcBase+0x1000, oControl)!=0)
		return READ_ERROR;
	return OK;
}

Error tdc_writeControlRegister(BusAddr tdcBase, unsigned short iControl) {
	if (libvme_write_a32_word(tdcBase+0x1000, iControl)!=0)
		return WRITE_ERROR;
	return OK;
}

Error tdc_enableExtendedTriggerTimeTag(BusAddr tdcBase, unsigned short doEnable) {
	unsigned short word;
	Error rc = tdc_readControlRegister(tdcBase, &word);
	if (rc !=OK)
		return rc;
	if (doEnable) {
		word |= (1 << 9);
	} else {
		word &= (1 << 9);
	}
	return tdc_writeControlRegister(tdcBase, word);
}

void eprintf(char * fmt, ...) {
	va_list arg_ptr;
	va_start(arg_ptr, fmt);
	vfprintf(stderr, fmt, arg_ptr);
	va_end(arg_ptr);
}

char * tdc_errorToString(Error error) {
#define item(x) case x: return #x
	switch(error) {
		item(OK);
		item(TIMEOUT);
		item(READ_ERROR);
		item(WRITE_ERROR);
		item(NO_DATA);
		item(BAD_COMMAND);
		item(BAD_ARGUMENT);
		item(DATA_END);
		default: return "UNKNOWN_ERROR";
	}
#undef item
}

Error tdc_isDataReady(BusAddr tdcBase, int * isReady) {
	uint16_t count;
	Error rc;
	if ((rc=tdc_storedEventsCount(tdcBase, &count))!=OK) {
		return rc;
	}
	*isReady=0;
	if (count>0) {
		*isReady=1;
	}
	return OK;
//	unsigned short word;
//	if (libvme_read_a32_word(tdcBase+TDC_STATUS_REGISTER, &word)!=0) {
//		eprintf("tdc_isDataReady - unable to read %x.\n", tdcBase+TDC_STATUS_REGISTER);
//		return READ_ERROR;
//	}
//	if (word&TDC_STATUS_DREADY) {
//		*isReady=1;
//	} else {
//		*isReady=0;
//	}
	return OK;
}

Error waitForBitInWord(BusAddr a32, unsigned short mask, int delay, WaitFor waitFor) {
	unsigned short word;
	int tries=0;
	word=0;
	assert(waitFor==WAIT_FOR_ZERO||waitFor==WAIT_FOR_ONE);
	delay*=100;
	do {
		tries++;
		if (libvme_read_a32_word(a32, &word)!=0) {
			eprintf("waitForBitInWord - unable to read word %x.\n", a32);
			return READ_ERROR; //addressing or bus error
		}
		if (waitFor==WAIT_FOR_ONE && (word&mask)) return OK;
		if (waitFor==WAIT_FOR_ZERO && !(word&mask)) return OK;
		usleep(10);
		delay--;
	} while (delay>0);
	eprintf("Timeout while waiting for mask %x, from %x, usually the word is %x. %d tries.\n", mask, a32, word, tries);
	return TIMEOUT;
}

Error tdc_getReadyForWriteOpcode(BusAddr tdcBase) {
	Error error;
	error=waitForBitInWord(tdcBase+TDC_MICRO_HANDSHAKE, TDC_MHS_WRITE_OK, TDC_MHS_TIMEOUT, WAIT_FOR_ONE);
	switch (error) {
		case OK: return OK;
		case TIMEOUT: eprintf("Timeout while wating for WRITE_OK bit in TDCs micro handshaker register.\n"); return TIMEOUT;
		case READ_ERROR: eprintf("Read error while wating for WRITE_OK bit in TDCs micro handshaker register.\n"); return READ_ERROR;
		default: eprintf("Unknown error %s:%d.\n", __FILE__, __LINE__); return error;
	}
}

Error tdc_writeWholeOpcode(BusAddr tdcBase, unsigned short opcode) {
	Error error;
	error=tdc_getReadyForWriteOpcode(tdcBase);
	if (error!=OK) return error;
	if (libvme_write_a32_word(tdcBase+TDC_MICRO, opcode)!=0) {
		eprintf("Write error while writing opcode\n");
		return WRITE_ERROR;
	}
	return OK;
}

Error tdc_writeOpcode(BusAddr tdcBase, unsigned char command, unsigned char object) {
	unsigned short opcode;
	opcode=command;
	opcode=opcode<<8|object;
	return tdc_writeWholeOpcode(tdcBase, opcode);
}

Error tdc_readOpcode(BusAddr tdcBase, uint16_t * opcode) {
	// A VME address of micro handshake register
	Error error;
	error=waitForBitInWord(tdcBase+TDC_MICRO_HANDSHAKE, TDC_MHS_READ_OK, TDC_MHS_TIMEOUT, WAIT_FOR_ONE);
	switch (error) {
		case OK: break;
		case TIMEOUT: eprintf("Timeout while wating for READ_OK bit in TDCs micro handshaker register.\n"); return TIMEOUT;
		case READ_ERROR: eprintf("Read error while wating for READ_OK bit in TDCs micro handshaker register.\n"); return READ_ERROR;
		default: eprintf("Unknown error %s:%d.\n", __FILE__, __LINE__); return error;
	}
	if (libvme_read_a32_word(tdcBase+TDC_MICRO, opcode)!=0) {
		eprintf("Read error while reading opcode\n");
		return READ_ERROR;
	}
	return OK;
}

Error tdc_writeOpcodeArgument(BusAddr tdcBase, TdcWord opcode, TdcWord argument) {
	Error rc;
	rc=tdc_writeWholeOpcode(tdcBase, opcode);
	if (rc!=OK) {
		eprintf("Can't write opcode %x, to write argument %x: %s, ", opcode, argument, tdc_errorToString(rc));
		return rc;
	}
	rc=tdc_writeWholeOpcode(tdcBase, argument);
	if (rc!=OK) {
		eprintf("Can't write opcode %x argument %x: %s, ", opcode, argument, tdc_errorToString(rc));
	}
	return rc;
}

Error tdc_readOpcodeArgument(BusAddr tdcBase, TdcWord opcode, TdcWord * argument) {
	Error rc;
	rc=tdc_writeWholeOpcode(tdcBase, opcode);
	if (rc!=OK) {
		eprintf("Can't write opcode %x, to read argument: %s, ", opcode, tdc_errorToString(rc));
		return rc;
	}
	rc=tdc_readOpcode(tdcBase, argument);
	if (rc!=OK) {
		eprintf("Can't read opcode %x argument: %s, ", opcode, tdc_errorToString(rc));
	}
	return rc;
}

//See page 94 of V1190B Technical Information Manual
inline Error tdc_readJtagSetupWord(BusAddr tdcBase, unsigned wordIndex, TdcWord * setupWord) {
	TdcWord opcode=(0x71<<8) + wordIndex;
	assert(wordIndex<=0x28);
	Error rc=tdc_readOpcodeArgument(tdcBase, opcode, setupWord);
//	eprintf("JtagSetupWord %d==%x\n", wordIndex, *setupWord);
	return rc;
}

inline Error tdc_writeJtagSetupWord(BusAddr tdcBase, unsigned wordIndex, TdcWord setupWord) {
	TdcWord opcode=(0x70<<8) + wordIndex, temp=0;
	assert(wordIndex<=0x28);
//	eprintf("JtagSetupWord %d=%x\n", wordIndex, setupWord);
	Error rc=tdc_writeOpcodeArgument(tdcBase, opcode, setupWord);
	if (rc!=OK) return rc;
	rc=tdc_readJtagSetupWord(tdcBase, wordIndex, &temp);
	if (rc!=OK) return rc;
	assert(temp==setupWord);
	return rc;
}

Error tdc_setJtagSetupBit(BusAddr tdcBase, unsigned bitIndex, unsigned isSet) {
	Error rc;
	unsigned wordIndex=bitIndex/16, wordBit=bitIndex%16;
	TdcWord setupWord;
	assert(bitIndex<646);
	rc=tdc_readJtagSetupWord(tdcBase, wordIndex, &setupWord);
	if (rc!=OK)
		return rc;
	eprintf("%s %d setup scan path bit\n", isSet ? "Setting" : "Unsetting", bitIndex);
//	eprintf("Word was: %x \n", setupWord);
	if (isSet) {
		setupWord|=(1<<wordBit);
	} else {
		setupWord&=~(1<<wordBit);
	}
//	eprintf("Word became: %x\n", setupWord);
	rc=tdc_writeJtagSetupWord(tdcBase, wordIndex, setupWord);
	return rc;
}

Error tdc_loadScanPathSetup(BusAddr tdcBase) {
	return tdc_writeOpcode(tdcBase, 0x72, 0);
}

Error tdc_enableHighSpeedSerialization(BusAddr tdcBase) {
	Error rc;
	rc=tdc_setJtagSetupBit(tdcBase, 26, 1);
	if (rc!=OK) return rc;
	rc=tdc_loadScanPathSetup(tdcBase);
	if (rc!=OK) return rc;
	return rc;
}

Error tdc_enableHighSpeedCoreClock(BusAddr tdcBase) {
	Error rc;
	rc=tdc_setJtagSetupBit(tdcBase, 622, 1);
	if (rc!=OK) return rc;
	rc=tdc_setJtagSetupBit(tdcBase, 623, 0);
	if (rc!=OK) return rc;
	rc=tdc_loadScanPathSetup(tdcBase);
	if (rc!=OK) return rc;
	return rc;
}

Error tdc_disableHighSpeedCoreClock(BusAddr tdcBase) {
	Error rc;
	rc=tdc_setJtagSetupBit(tdcBase, 622, 0);
	if (rc!=OK) return rc;
	rc=tdc_setJtagSetupBit(tdcBase, 623, 0);
	if (rc!=OK) return rc;
	rc=tdc_loadScanPathSetup(tdcBase);
	if (rc!=OK) return rc;
	return rc;
}

//This function don't work. I don't know why. Gulevich
Error tdc_readTriggerConfiguation(BusAddr tdcBase, int16_t * configuration) {
	Error rc;
	unsigned i=0;
	int16_t temp;
	assert(!tdc_errorPrint(tdcBase));
	rc=tdc_writeWholeOpcode(tdcBase, 0x1600);
	assert(!tdc_errorPrint(tdcBase));
	if (rc!=OK) return rc;
	for(i=0; i<5; i++) {
		assert(!tdc_errorPrint(tdcBase));
		if (i==0||1) {
			rc=tdc_readOpcode(tdcBase, &temp);
			if (rc!=OK)
				eprintf("%s while reading %d word of trigger configuration\n", tdc_errorToString(rc), i);
		} else {
			rc=(libvme_read_a32_word(tdcBase+TDC_MICRO, &temp)==0)?OK:READ_ERROR;
		}
		assert(!tdc_errorPrint(tdcBase));
		if (rc!=OK) return rc;
		configuration[i]=temp;//&0xfff; // only 12 bit are significant
	}
	eprintf("Trigger configuration: %x, %x, %x, %x, %x\n", configuration[0], configuration[1], configuration[2], configuration[3], configuration[4]);
	return OK;
}

Error tdc_getWindowWidth(BusAddr tdcBase, unsigned short * width) {
	Error rc;
	uint16_t configuration[5];
	assert(width);
	rc=tdc_readTriggerConfiguation(tdcBase, configuration);
	if (rc!=OK) return rc;
	*width=configuration[0];
	return OK;
}

Error tdc_getWindowOffset(BusAddr tdcBase, short * offset) {
	Error rc;
	int16_t configuration[5];
	assert(offset);
	assert(!tdc_errorPrint(tdcBase));
	rc=tdc_readTriggerConfiguation(tdcBase, configuration);
	if (rc!=OK) return rc;
	uint16_t raw = configuration[1];
	eprintf("Raw offset: 0x%hx\n", raw);
	uint16_t temp = raw + 2048;
	*offset =  (int)temp - 2048;
	assert(!tdc_errorPrint(tdcBase));
	return OK;
}

Error tdc_isTriggerMode(BusAddr tdcBase, int * triggerMode) {
	Error rc;
	unsigned short word;
	*triggerMode=0;
	/*Checking if trigger mode is set now*/
	rc=tdc_writeOpcode(tdcBase, 0x02, 0x00);
	if (rc != OK) {
		eprintf("Failed to get trigger mode state\n");
		return rc;
	}
	rc=tdc_readOpcode(tdcBase, &word);
	if (rc != OK) {
		eprintf("%s while reading trigger mode\n", tdc_errorToString(rc));
		return rc;
	}
	if ((word&1)==1) {
		*triggerMode=1;
	} else {
		*triggerMode=0;
	}
	return OK;
}

Error tdc_setTriggerMode(BusAddr tdcBase) {
	Error rc;
	int isTrigger;
	rc=tdc_isTriggerMode(tdcBase, &isTrigger);
	if (rc!=OK) {
		eprintf("Unable to precheck trigger mode\n");
		return rc;
	}
	if (isTrigger) {
		return OK;
	}
	/*Setting*/
	rc=tdc_writeOpcode(tdcBase, 0x00, 0x00);
	if (rc != OK) return rc;
	rc=tdc_isTriggerMode(tdcBase, &isTrigger);
	if (rc != OK) {
		eprintf("Unable to postcheck trigger mode\n");
		return rc;
	}
	if (!isTrigger) {
		eprintf("Trigger mode was not set for unknown reason");
		return WRITE_ERROR;
	}
	rc=tdc_writeOpcode(tdcBase, 0x14, 0x00); // enable subtraction of trigger time
	if (rc != OK) {
		eprintf("Unable to enable subtraction of trigger time: %s\n", tdc_errorToString(rc));
		return rc;
	}
	return OK;
}

// width is in [1, 4095]
Error tdc_setWindowWidth(BusAddr tdcBase, unsigned short width) {
	Error rc;
	assert(width>=1);
	eprintf("Setting window width %d\n", width);
	assert(width<=4095);
//	assert(!tdc_errorPrint(tdcBase));
	rc=tdc_writeWholeOpcode(tdcBase, 0x1000);
	if (rc!=OK) return rc;
	rc=tdc_writeWholeOpcode(tdcBase, width);
	if (rc!=OK) return rc;
	assert(!tdc_errorPrint(tdcBase));
	return rc;
}

// Sets offset in cycles. The greater offset means earlier window opening.
// Offset should be in [-2048, 40]
Error tdc_setWindowOffset(BusAddr tdcBase, short offset) {
	Error rc;
	assert(offset>=-2048);
	assert(offset<=40);
	eprintf("Setting window offset %d\n", offset);
	short uoffset=offset;
	rc=tdc_writeOpcode(tdcBase, 0x11, 0);
	if (rc!=OK) return rc;
	rc=tdc_writeWholeOpcode(tdcBase, uoffset);
	if (rc!=OK) return rc;
	assert(!tdc_errorPrint(tdcBase));
	eprintf("Written raw offset %d\n", uoffset);
#if 0
	short temp;
	rc=tdc_getWindowOffset(tdcBase, &temp);
	if (rc!=OK) {
		eprintf("%s while checking setted offset\n", tdc_errorToString(rc));
		return rc;
	}
	if (temp!=offset) {
		eprintf("Written offset %d, read offset %d\n", offset, temp);
		return UNKNOWN_ERROR;
	}
#endif
	return OK;
}

// width in [1, 4095]
// offset in [-2048, 40]
// width+offset <= 40
Error tdc_setWindow(BusAddr tdcBase, int offset, unsigned short width) {
	Error rc;
	assert(width>=1);
	assert(width<=4095);
	assert(offset>=-2048);
	assert(offset<=40);
	assert(offset+width<=40);
	if ((rc=tdc_setWindowOffset(tdcBase, offset))!=OK) {
//		return rc;
	}
	if ((rc=tdc_setWindowWidth(tdcBase, width))!=OK) {
//		return rc;
	}
	return OK;
}

inline Error tdc_clearEvent(Event * event) {
	event->length=0;
	return OK;
}

Error tdc_readEvent(BusAddr tdcBase, Event * event) {
	Error rc;
	BusAddr curr;
	uint32_t word;
	curr=tdcBase+TDC_OUTPUT_BUFFER;
//	rc=tdc_isDataReady(tdcBase, &isReady);
//	if (rc!=OK) {
//		eprintf("%s while getting data ready status", tdc_errorToString(rc));
//		return rc;
//	}
//	if (!isReady) return NO_DATA;
	rc=tdc_clearEvent(event);
	if (rc!=OK) {
		eprintf("tdc_readEvent - unable to clear event because of %s\n", tdc_errorToString(rc));
		return rc;
	}
	while(1) {
		if (libvme_read_a32_dword(curr, &word)!=0) {
			eprintf("Unable to read VME address %x.\n", curr);
			return READ_ERROR;
		}
		if (word>>27==8) { // Global header
			if (event->length!=0) {
				eprintf("Duplicate event header\n");
				return READ_ERROR;
			}
			event->count=word>>5;//Skipping GEO information
			event->count&=0x1FFFFF;//Extracting 21 bit of event count
		}
		event->words[event->length]=word;
		event->length++;
		if (event->length>=MAX_EVENT_LENGTH) return READ_ERROR;
		if (word>>27==16) { //Global trailer
			return OK;
		}
	//	curr++; //Data is serialized. Always read zero address. There is no need to shift.
	}
	return OK:
}

Error tdc_test(BusAddr tdcBase) {
	uint32_t x=56, y=0;
	if (libvme_write_a32_dword(tdcBase+0x1200, x)!=0) return WRITE_ERROR;
	if (libvme_read_a32_dword(tdcBase+0x1200, &y)!=0) return READ_ERROR;
	if (x!=y) {
		return UNKNOWN_ERROR;
	}
	return OK;
}

Error tdc_setInterruptLine(BusAddr tdcBase, int line, uint16_t vector) {
	unsigned short line1=line;
	assert(line>=0);
	assert(line<=7);
	//WARN: the following line disables Almost Full level! WTF?
	//if (libvme_write_a32_word(tdcBase+0x1022, -1)!=0) return WRITE_ERROR;
	if (libvme_write_a32_word(tdcBase+0x100A, line1)!=0) return WRITE_ERROR;
	line1=0;
	if (libvme_read_a32_word(tdcBase+0x100A, &line1)!=0)
		 return READ_ERROR;
	if (line1 != line) {
		return WRITE_ERROR;
	}
	if (libvme_write_a32_word(tdcBase+0x100C, vector)!=0) return WRITE_ERROR;
	return OK;
}

Error tdc_storedEventsCount(BusAddr tdcBase, uint16_t * count) {
	if (libvme_read_a32_word(tdcBase+0x1020, count)!=0) {
		return READ_ERROR;
	}
	return OK;
}

Error tdc_getStatus(BusAddr tdcBase, uint16_t * status) {
	assert(status!=0);
	if (libvme_read_a32_word(tdcBase+0x1002, status)!=0) {
		return READ_ERROR;
	}
	return OK;
}

inline int statusToError(uint32_t status) {
	return  (status >> 6) & 0xF;
}

int tdc_error(BusAddr tdcBase) {
	uint16_t status;
	if (tdc_getStatus(tdcBase, &status)!=OK) {
		return 0xff;
	}
	return statusToError(status);
}

int tdc_errorPrint(BusAddr tdcBase) {
	int error=tdc_error(tdcBase);
	if (error) {
		eprintf("V1190B error: %x\n", error);
	}
	return error;
}

Error tdc_readEdgeDetectionMode(BusAddr tdcBase, uint16_t * mode) {
	Error rc;
	assert(mode);
	rc=tdc_writeOpcode(tdcBase, 0x23, 0x00);
	if (rc!=OK) {
		eprintf("%s while reading edge detection mode\n", tdc_errorToString(rc));
		return rc;
	}
	rc=tdc_readOpcode(tdcBase, mode);
	if (rc!=OK) {
		eprintf("%s while reading edge detection mode\n", tdc_errorToString(rc));
	}
	return rc;
}

Error tdc_setEdgeDetectionMode(BusAddr tdcBase, uint16_t mode) {
	Error rc;
	assert(mode<4);
	if (mode>=4) {
		return BAD_ARGUMENT;
	}
	rc=tdc_writeOpcode(tdcBase, 0x22, 0x00);
	if (rc!=OK) {
		eprintf("%s while setting edge detection mode %d\n", tdc_errorToString(rc), mode);
		return rc;
	}
	rc=tdc_writeWholeOpcode(tdcBase, mode);
	if (rc!=OK) {
		eprintf("%s while setting edge detection mode %d\n", tdc_errorToString(rc), mode);
	}
#ifndef NDEBUG
	uint16_t newMode=5;
	rc=tdc_readEdgeDetectionMode(tdcBase, &newMode);
	assert(newMode==mode);
	if (rc!=OK) return rc;
#endif
	return rc;
}

Error tdc_setDeadTime(BusAddr tdcBase, TdcWord  time) {
	Error rcTotal, rc;
	TdcWord opcode;
	if (time > 3)
		return BAD_ARGUMENT;
	int last = 0x2840;
	last = 0x2801; // this command is global
	for (opcode = 0x2800; opcode < last; ++opcode) {
		rc = tdc_writeOpcodeArgument(tdcBase, opcode, time);
		if (rc!=OK) {
			eprintf("%s while setting dead time mode with 0x%x opcode\n", tdc_errorToString(rc), opcode);
		}
		if (rcTotal == OK)
			rcTotal = rc;
	}
	return rc;
}

Error tdc_setDllClock(BusAddr tdcBase, TdcWord mode) {
	Error rc;
	return tdc_writeOpcodeArgument(tdcBase, 0xC8000, mode);
}
