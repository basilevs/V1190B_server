#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <arpa/inet.h>
#include "V1190B.h"
#include "libvmedirect.h"
#include "signal.h"

#define BUSFILE "/dev/vme0"

char getch(void)
{
	char ch;
	struct termios old_tc,new_tc;

	tcgetattr(fileno(stdin),&old_tc);
	new_tc = old_tc;
	new_tc.c_lflag &= ~ICANON;
	tcsetattr(fileno(stdin),TCSANOW,&new_tc);
	ch = getc(stdin);
	tcsetattr(fileno(stdin),TCSANOW,&old_tc);
	return(ch);
}

#define IRQFILE "/dev/vmei%d"

struct IRQHandler {
	int line; //< IRQ level
	int descriptor; //< file descriptor
};

struct IRQHandler * openIrq() {
	int i, fdi;
	char str[40];
	for(i=1; i<=7; i++) {
		sprintf(str, IRQFILE, i);
		fdi = open(str, O_RDONLY);
		if (fdi >= 0) {
			struct IRQHandler * handler=(struct IRQHandler *)malloc(sizeof(struct IRQHandler));
			assert(handler);
			if (!handler) return 0;
			handler->line=i;
			handler->descriptor=fdi;
			eprintf("IRQ line: %d, irq fd: %d.\n", i, fdi);
			return handler;
		}
	}
	eprintf("Can't lock any IRQ line\n");
	return 0;
}

void closeIrq(struct IRQHandler * handler) {
	assert(handler);
	if (!handler) return;
	assert(handler->descriptor>=0);
	if (!handler->descriptor>=0) return;
	close(handler->descriptor);
	handler->line=-1;
	handler->descriptor=-1;
	free(handler);
}

int catch_irq(struct IRQHandler * handler) {
	assert(handler);
	if (!handler) return -1;
	int fdi=handler->descriptor;
	int line=handler->line;
	int r;
	unsigned buf;
	fd_set fdset;
	struct timeval ktimeout;
	ktimeout.tv_sec = 1;
	ktimeout.tv_usec = 0;

	FD_ZERO(&fdset);
	FD_SET(fdi, &fdset);
	if (select(fdi+1, 0, 0, &fdset, &ktimeout) != 0){
		r = read(fdi, &buf, sizeof(unsigned));
		if (r==0) {
			//			eprintf("Caught interrupt %d\n", line);
		} else {
			//			eprintf("Caught interrupt %d, vector %x\n",line, buf);
		}
	}
	else{
		eprintf("No IRQ caught on line %d, fd %d\n", line, fdi);
		return -1;
	}
	return 0;
}

Error tdc_printEvent(Event * event) {
	assert(event);
	int i;
	uint32_t tempHost;
	eprintf("Event %d, length %d:\n", event->count, event->length);
	for (i=0; i < event->length; i++) {
		tempHost=event->words[i];
		eprintf("%0.8x ", tempHost);
	}
	eprintf("\n");
	return OK;
}

#define MAX_LINE_LENGTH 200
#define MAX_BUFFER_SIZE 201

int doDAQ = 0;

Error processCommand(BusAddr tdcAddr, char * line) {
	char* position=0;
	const char * delimeters=" \t";
	assert(strlen(line)<MAX_LINE_LENGTH);
	eprintf("Processing command: |%s|\n", line);
	char *command=strtok_r(line, delimeters, &position);
	if (!command) return NO_DATA;
	if (strcmp(command, "window")==0) {
		char *argument=strtok_r(0, delimeters, &position);
		if (!argument) return BAD_ARGUMENT;
		int offset=atoi(argument);
		argument=strtok_r(0, delimeters, &position);
		if (!argument) return BAD_ARGUMENT;
		int width = atoi(argument);
		return tdc_setWindow(tdcAddr, offset, width);
	} else if (strcmp(command, "clear")==0) {
		return tdc_softwareClear(tdcAddr);
	} else if (strcmp(command, "noop")==0) {
		return OK;
	} else if (strcmp(command, "start")==0) {
		doDAQ = 1;
		return tdc_softwareClear(tdcAddr);
	} else if (strcmp(command, "stop")==0) {
		doDAQ = 0;
		return OK;
	} else if (strcmp(command, "lowSpeedCoreClock")==0) {
		return tdc_disableHighSpeedCoreClock(tdcAddr);
	} else if (strcmp(command, "deadTime")==0) {
		char *argument=strtok_r(0, delimeters, &position);
		if (!argument) return BAD_ARGUMENT;
		int timeMode=atoi(argument);
		return tdc_setDeadTime(tdcAddr, timeMode);
	} else if (strcmp(command, "dllClock")==0) {
		char *argument=strtok_r(0, delimeters, &position);
		if (!argument) return BAD_ARGUMENT;
		int mode=atoi(argument);
		return tdc_setDllClock(tdcAddr, mode);
	} else {
		return BAD_COMMAND;
	}
}

int setBlocking(int fd, int doBlock) {
	int flags;

	/* If they have O_NONBLOCK, use the Posix way to do it */
#if defined(O_NONBLOCK)
	/* Fixme: O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
	if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
		flags = 0;
	if (!doBlock) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}
	return fcntl(fd, F_SETFL, flags);
#else
	/* Otherwise, use the old way of doing it */
	flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
}

Error processInput(BusAddr tdcAddr) {
	static char buffer[MAX_BUFFER_SIZE];
	static int position=0;
	fd_set fdset;
	struct timeval ktimeout;
	int maxFd=STDIN_FILENO+1;
	int flags = 0;
	setBlocking(STDIN_FILENO, 0);
	FD_ZERO(&fdset);
	FD_SET(STDIN_FILENO, &fdset);
	ktimeout.tv_sec = 0;
	ktimeout.tv_usec = 1;
	int selectRv=-1;
	assert(position<MAX_BUFFER_SIZE);

	while ((selectRv=select(maxFd, &fdset, NULL, NULL, &ktimeout)) > 0) {
		eprintf("Input detected\n");
		int left=MAX_BUFFER_SIZE-position-1;
		assert(left>0);
		int bytesCount = fread(buffer+position, 1, left, stdin);
		eprintf("Input bytes: %d\n", bytesCount);
		assert(bytesCount<=left);
		position+=bytesCount;
		if (bytesCount==0) {
			eprintf("End of input\n");
			return DATA_END;
		}
		assert(position<MAX_BUFFER_SIZE);
		while (1) {
			buffer[position]=0;
//			eprintf("Buffer (%d): %s\n", position, buffer);
			char* endOfLine = index(buffer, '\n');
			if (!endOfLine || endOfLine - buffer > position) {
//				eprintf("No end of line\n");
				break;
			}
			int length = endOfLine - buffer;
			assert(endOfLine>=buffer);

			*endOfLine=0;
			assert(length<MAX_LINE_LENGTH);
//			eprintf("Line: %s\n", buffer);
			//Saving command line before sending it to modifying parser
			if (length>0) {
				char originalCommandLine[MAX_LINE_LENGTH];
				strncpy(originalCommandLine, buffer, length);
				int i = 0;
				for (i =0; i < 3; i++) {
					if (buffer[length-i]=='\r')
						buffer[length-i] = 0;
				}
				int rc=processCommand(tdcAddr, buffer);
				if (rc!=OK) {
					eprintf("%s while processing command: %s\n", tdc_errorToString(rc), originalCommandLine);
				}
				position-=(length+1);
				assert(position>=0);
				memmove(buffer, endOfLine+1, position);
			}
		}
	}
	return OK;
}

inline Error tdc_sendEvent(const Event * event) {
	assert(event);
	int i;
	uint32_t tempNet, tempHost;
	setBlocking(STDOUT_FILENO, 1);
	//	eprintf("Event %d:\n", event->count);
	for (i=0; i < event->length; i++) {
		tempHost=event->words[i];
		tempNet=htonl(tempHost);
		if (fwrite(&tempNet, 4, 1,  stdout)!=1) {
			eprintf("Event: %d, word index: %d, word value: %d (%d). Unable to send.\n", event->count, i, tempHost, tempNet);
			if (feof(stdout)) {
				eprintf("End of stdout\n");
			} else {
				eprintf("Stdout error: %d\n", ferror(stdout));
				perror("Stdout error:");
			}

			return WRITE_ERROR;
		}
	}
	//	eprintf("Sent event of length %d\n", event->length);
	return OK;
}


int pass = 0;
int loopmode = 0;


void deinit() {
	eprintf("Closing...\n");
	libvme_close();
}

void sigIntHandler(int sig) {
	deinit();
	exit(1);
}

int main(int argc, char **argv)
{
	unsigned int u32;
	int doSend=1, doPrint=0;
	unsigned edgeDetectionMode=2;
	Event event;
	Error rc=OK;

	const char * buildDate="unknown build date";
	#ifdef BUILDDATE
		buildDate=BUILDDATE;
	#endif
	eprintf("BEAM V1190B readout %s\n", buildDate);



	//    eprintf("*** Initializing VME interface ***\n");
	if (libvme_init() < 0) {
		eprintf("libvme_init failed\n");
		return 2;
	}

	//    eprintf("System controller ...");
	u32 = 1; libvme_ctl(VME_SYSTEM_W, &u32);
	//eprintf("ok\n");

	signal(SIGINT, sigIntHandler);
	eprintf("Address modificator 0xd ...");
	u32 = 0x0d; libvme_ctl(VME_AM_W, &u32); eprintf("ok\n");

	BusAddr tdcAddr=0xEE000000;

	eprintf("Resetting V1190B at %x ...", tdcAddr);
	if ((rc=tdc_softwareReset(tdcAddr))!=OK) {
		eprintf(" %s while resetting V1190B block at address %x\n", tdc_errorToString(rc), tdcAddr);
		deinit();
		return 3;
	}
	eprintf("Done\n", tdcAddr);

	rc=tdc_test(tdcAddr);
	if (rc!=OK) {
		eprintf("%s while testing V1190B block at address %x\n", tdc_errorToString(rc), tdcAddr);
		deinit();
		return 5;
	}

	if ((rc=tdc_setTriggerMode(tdcAddr))!=OK) {
		eprintf("Can't set trigger mode: %s\n", tdc_errorToString(rc));
		deinit();
		return 6;
	}

#if 1
	if ((rc=tdc_setEdgeDetectionMode(tdcAddr, edgeDetectionMode))!=OK) {
		eprintf("Can't set edge detection mode %u: %s\n", edgeDetectionMode, tdc_errorToString(rc));
	}
#endif

#if 0
	if ((rc=tdc_setWindow(tdcAddr, offset, width))!=OK) {
		eprintf("Can't set window offset: %d, width: %d : %s\n", offset, width, tdc_errorToString(rc));
		deinit();
		return 7;
	}
#endif

#if 0
	if ((rc=tdc_enableHighSpeedSerialization(tdcAddr))!=OK) {
		eprintf("Can't enable high speed  serialization: %s\n", tdc_errorToString(rc));
		deinit();
		return 13;
	}
#endif

#if 1
	if ((rc=tdc_enableHighSpeedCoreClock(tdcAddr))!=OK) {
		eprintf("Can't enable high speed for TDC core clock: %s\n", tdc_errorToString(rc));
		deinit();
		return 13;
	}
#else
	if ((rc=tdc_disableHighSpeedCoreClock(tdcAddr))!=OK) {
		eprintf("Can't disable high speed for TDC core clock: %s\n", tdc_errorToString(rc));
		deinit();
		return 13;
	}
#endif

	if ((rc=tdc_enableExtendedTriggerTimeTag(tdcAddr, 1))!=OK) {
		eprintf("Can't enable Extended Trigger Time Tag: %s\n", tdc_errorToString(rc));
		deinit();
		return 16;
	}
	

//	if ((rc=tdc_setJtagSetupBit(tdcAddr, 444, 1))!=OK) {
//		eprintf("Can't set test JTAG bit: %s\n", tdc_errorToString(rc));
//		return 15;
//	}
//	tdc_loadScanPathSetup(tdcAddr);


#if 0
	eprintf("Clearing V1190B at %x\n", tdcAddr);
	if ((rc=tdc_softwareClear(tdcAddr))!=OK) {
		eprintf("%s while clearing V1190B block at address %x\n", tdc_errorToString(rc), tdcAddr);
		deinit();
		return 4;
	}
#endif

#if 1
	struct IRQHandler * irqHandler=openIrq();
#else
	struct IRQHandler * irqHandler = 0;
#endif
	if (irqHandler)
		if ((rc=tdc_setInterruptLine(tdcAddr, irqHandler->line, 0))!=OK) {
			eprintf("Can't set interrupt line and vector: %s\n", tdc_errorToString(rc));
			deinit();
			return 9;
		}

	int prevEvent=-1;
	while (1) {
		if ((rc=processInput(tdcAddr))!=OK) {
			if (rc==DATA_END) {
				break;
			}
			eprintf("%s while processing input\n", tdc_errorToString(rc));
			break;
		}
		if (!doDAQ) continue;
		uint16_t count=0;
		if ((rc=tdc_storedEventsCount(tdcAddr, &count))!=OK) {
			eprintf("Can't get data ready state: %s\n", tdc_errorToString(rc));
			deinit();
			return 10;
		}

#if 1
		eprintf("Events in buffer: %d\n", count);
#endif
		for (;count>0; count--) {
			rc=tdc_readEvent(tdcAddr, &event);
			if (rc==NO_DATA) {
				eprintf("No data while reading event\n");
				break;
			}
			if (rc!=OK) {
				eprintf("%s while reading event\n", tdc_errorToString(rc));
				deinit();
				return 11;
			}
			if (doPrint && event.length != 6) {
				tdc_printEvent(&event);
			}
			if (doSend) {
				rc=tdc_sendEvent(&event);
				if (rc!=OK) {
					eprintf("%s while sending event\n", tdc_errorToString(rc));
					deinit();
					return 12;
				}
			}
//				if (event.count != prevEvent+1) {
//					eprintf("%d events were skipped\n\a", event.count-prevEvent-1);
//				}
//				prevEvent=event.count;
		}
		if (doSend) {
			fflush(stdout);
		}
		if ((rc=tdc_storedEventsCount(tdcAddr, &count))!=OK) {
			eprintf("Can't get data ready state: %s\n", tdc_errorToString(rc));
			deinit();
			return 10;
		}
		if (!count) {
			if (irqHandler) {
				uint16_t d;
				catch_irq(irqHandler);
				tdc_getStatus(tdcAddr, &d);
				eprintf("Status: %x\n", d);
			} else {
				sleep(1);
			}
		}
	}
	if (irqHandler) {
		closeIrq(irqHandler);
		irqHandler=0;
	}
	deinit();
	return 0;
}
