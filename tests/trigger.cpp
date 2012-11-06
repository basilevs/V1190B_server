#include "V1190B2.h"
#include "LibVME.h"
#include <getopt.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>


using namespace std;

struct Fd {
	int fd;
	Fd(int fdi): fd(fdi) {}
	~Fd() {
		if (fd != -1)
			close(fd);
	}
};

int main(int argc, char * const argv[])
{
	int opt;
	int level = 1;
	uint32_t address = 0xEE000000;
	while((opt = getopt(argc, argv, "hl:a:")) != -1) {
		switch (opt) {
		case 'l':
			level = atoi(optarg);
			break;
		case 'a':
			address = atol(optarg);
			break;
		case 'h':
			cout <<
			"trigger [-h] [-l level] [-a address] \n"
			"-l level - sets the interrupt level to use\n"
			"-a address - sets the address of V1190B card being tested\n";
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
		Fd fd(LibVME::instance().openIrqLevel());
		tdc.reset();
		if (!tdc.test()) {
			cerr << "Test failed" << endl;
			return 8;
		}
		tdc.triggerMode(true);

		if (!tdc.triggerMode()) {
			cerr << "Failed to set trigger mode" <<endl;
			return 9;
		}

		tdc.configureInterrupt(fd.fd, 123);

		struct timeval timeout = {10, 0};
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(fd.fd, &fdset);
		int selectRv = select(fd.fd+1, &fdset, &fdset, &fdset, &timeout);
		if (selectRv > 0)
		{
			int vector;
			int bytes = read(fd.fd, &vector, 4);
			if (bytes != 4) {
				cerr << "Failed to read interrupt vector" << endl;
				return 2;
			} else {
				cout << "Caught interrupt " << endl;
				return 0;
			}
		} else if (selectRv == 0) {
			cerr << "Select timeout" << endl;
			cout << "Event count: " << tdc.bufferedEventsCount() << endl;
			return 7;
		} else {
			perror("Select failed");
			return 3;
		}
	} catch (V1190B2::Error & e) {
		cerr << "V1190B fail: " << e.what() << endl;
		return 6;
	} catch (VmeError & e) {
		cerr << "Vme bus error: " << e.what() << endl;
		return 5;
	}
}
