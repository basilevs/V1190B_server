#ifndef LIBVME_H_
#define LIBVME_H_

#include "IVmeInterface.h"
#include <memory>

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
	virtual void write_a32d32(uint32_t address, uint32_t value);
	virtual uint32_t read_a32d32(uint32_t address);
	class InterruptLine {
		int _fd;
		unsigned _level;
		InterruptLine(const InterruptLine &);
		InterruptLine & operator=(const InterruptLine &);
	public:
		InterruptLine(int fd, unsigned level);
		~InterruptLine();
		int32_t readVector() const;
		int fd() const {return _fd;}
		unsigned level() const {return _level;}
	};
	std::auto_ptr<InterruptLine> openIrqLevel();
	virtual ~LibVME();
};

#endif /* LIBVME_H_ */

