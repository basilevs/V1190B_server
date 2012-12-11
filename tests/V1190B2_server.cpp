#include "V1190B2.h"
#include "LibVME.h"
#include <getopt.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>//read
#include "CommandProcessor.h"
#include <socketbuf/socketbuf.h>

using namespace std;

#define xstr(x) astr(x)
#define astr(x) #x



int main(int argc, char * const argv[])
{
	int opt;
	int level = 1;
	uint32_t address = 0xEE000000;
	while((opt = getopt(argc, argv, "hl:a:v")) != -1) {
		switch (opt) {
		case 'l':
			level = atoi(optarg);
			break;
		case 'a':
			address = atol(optarg);
			break;
		case 'v':
			cout << "Built on " << xstr(BUILDDATE) << endl;
			return 0;
		case 'h':
			cout <<
			"server [-h] [-l level] [-a address] \n"
			"-a address - sets the address of V1190B card being tested\n"
			"-v         - shows version information\n"
			"-h         - shows this help\n";
			return 0;
		default:
			cerr << "Invalid option. Use -h for help." << endl;
			return 1;
		}
	}

	try {
		IVmeInterface & interface(LibVME::instance());
		V1190B2 tdc(interface, address);

		tdc.reset();
		if (!tdc.test()) {
			cerr << "Test failed" << endl;
			return 8;
		}

		tdc.triggerMode(true);
		tdc.highSpeedCoreClock();
		//tdc.highSpeedSerialization();
		tdc.edgeDetectionMode(2);

		tdc.extendedTriggerTimeTag();

		if (!tdc.triggerMode()) {
			cerr << "Failed to set trigger mode" <<endl;
			return 9;
		}

		auto_ptr<LibVME::InterruptLine> irq = LibVME::instance().openIrqLevel();
		tdc.configureInterrupt(irq->level(), 123);

		CommandProcessor processor(tdc, irq->fd());

		while (cin) {
			string line;
			getline(cin, line);
			try {
				processor.process(line);
				cout << "#Success" << endl;
			} catch (CommandProcessor::Error & e) {
				cout << "#Error: " << e.what() << endl;
			} catch (socketwrapper::Errno & e) {
				cout << "#Error: " << e.what() << "("<< e.code() <<")" << endl;
			}
		}
		cout << "Exiting" << endl;

	} catch (V1190B2::Error & e) {
		cerr << "V1190B fail: " << e.what() << endl;
		return 6;
	} catch (VmeError & e) {
		cerr << "Vme bus error: " << e.what() << endl;
		return 5;
	} catch (exception & e) {
		cerr << "Common error: " << e.what() << endl;
		return 11;
	}
}
