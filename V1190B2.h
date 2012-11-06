#ifndef V1190B2_H_
#define V1190B2_H_
#include "IVmeInterface.h"

class V1190B2 {
	IVmeInterface & _interface;
	unsigned _address;
	void writeOpcode(uint16_t opcode);
	uint16_t readOpcode();
	void waitOpcode(bool write);
public:
	struct Error: public std::runtime_error {
		Error(const std::string & m): runtime_error(m) {}
	};
	V1190B2(IVmeInterface & interface, unsigned address):
		_interface(interface),
		_address(address)
	{}
	void reset();
	void triggerMode(bool);
	bool triggerMode();
	void configureInterrupt(uint8_t level, uint8_t vector);
	bool test();
	unsigned bufferedEventsCount();
};

#endif /* V1190B2_H_ */
