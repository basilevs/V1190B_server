#ifndef LIBVME_H_
#define LIBVME_H_

#include "IVmeInterface.h"

class LibVME: public IVmeInterface {
	int ctl(int);
	void ctl(int, int);
	LibVME();
public:
	static LibVME & instance() {
		static LibVME instance;
		return instance;
	}
	//Sets 4 LSB bits of VME AM: AM3..AM0; PAM is always 0
	void addressModifier(int);
	int addressModifier();
	virtual void write_a32d16(uint32_t address, uint16_t value);
	virtual uint16_t read_a32d16(uint32_t address);
	virtual ~LibVME();
};

#endif /* LIBVME_H_ */
