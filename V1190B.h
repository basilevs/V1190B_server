#ifndef V1190B_h_
#define V1190B_h_
#include "assert.h"
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>


#define TDC_OUTPUT_BUFFER	0x0
#define TDC_MICRO		0x102E
#define TDC_MICRO_HANDSHAKE	0x1030
#define TDC_STATUS_REGISTER	0x1002
#define TDC_SOFTRIGGER_REGISTER 0x101A
#define TDC_STATUS_DREADY	1

enum {MAX_EVENT_LENGTH=1000};

#define BusAddr unsigned int

typedef uint16_t TdcWord;

typedef enum {OK, TIMEOUT, READ_ERROR, WRITE_ERROR, UNKNOWN_ERROR, NO_DATA, BAD_COMMAND, BAD_ARGUMENT, DATA_END} Error;
typedef enum WaitFor {WAIT_FOR_ZERO, WAIT_FOR_ONE} WaitFor;

typedef struct {
	unsigned int count;
	int length;
	uint32_t words[MAX_EVENT_LENGTH];
} Event;

Error tdc_readEvent(BusAddr tdcBase, Event * event);

void eprintf(char * fmt, ...);
char * tdc_errorToString(Error error);
Error tdc_isDataReady(BusAddr tdcBase, int * isReady);

Error waitForBitInWord(BusAddr a32, unsigned short mask, int delay, WaitFor waitFor);

Error tdc_getReadyForWriteOpcode(BusAddr tdcBase);

Error tdc_writeWholeOpcode(BusAddr tdcBase, unsigned short opcode);

Error tdc_writeOpcode(BusAddr tdcBase, unsigned char command, unsigned char object);

Error tdc_readOpcode(BusAddr tdcBase, uint16_t * opcode);

Error tdc_softwareClear(BusAddr tdcBase);

//
Error tdc_softwareReset(BusAddr tdcBase);

Error tdc_readControlRegister(BusAddr tdcBase, unsigned short * oControl);
Error tdc_writeControlRegister(BusAddr tdcBase, unsigned short iControl);
Error tdc_enableExtendedTriggerTimeTag(BusAddr tdcBase, unsigned short doEnable);
Error tdc_readTriggerConfiguation(BusAddr tdcBase, int16_t * configuration);
Error tdc_getWindowWidth(BusAddr tdcBase, unsigned short * width);

Error tdc_getWindowOffset(BusAddr tdcBase, short * offset);

Error tdc_isTriggerMode(BusAddr tdcBase, int * triggerMode);

Error tdc_setTriggerMode(BusAddr tdcBase);

// width in [1, 4095]
// offset in [-2048, 40]
// width+offset <= 40
Error tdc_setWindow(BusAddr tdcBase, int offset, unsigned short width);

// Sets offset in cycles. The greater offset means earlier window opening.

Error tdc_test(BusAddr tdcBase);

// line in [1, 7], 0 - disable
Error tdc_setInterruptLine(BusAddr tdcBase, int line, uint16_t vector);

Error tdc_storedEventsCount(BusAddr tdcBase, uint16_t * count);

Error tdc_getStatus(BusAddr tdcBase, uint16_t * status);

int tdc_errorPrint(BusAddr tdcBase);

Error tdc_setEdgeDetectionMode(BusAddr tdcBase, uint16_t mode);

Error tdc_setJtagSetupBit(BusAddr tdcBase, unsigned bitIndex, unsigned isSet);

Error tdc_enableHighSpeedSerialization(BusAddr tdcBase);
Error tdc_enableHighSpeedCoreClock(BusAddr tdcBase);
Error tdc_disableHighSpeedCoreClock(BusAddr tdcBase);

Error tdc_loadScanPathSetup(BusAddr tdcBase);

// reads dead time (ns)
Error tdc_setDeadTime(BusAddr tdcBase, TdcWord timeMode);

//0 - 4
Error tdc_setDllClock(BusAddr tdcBase, TdcWord mode);


#endif
