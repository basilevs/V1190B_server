#ifndef V1190B2_H_
#define V1190B2_H_
#include "IVmeInterface.h"

class V1190B2 {
	IVmeInterface & _interface;
	unsigned _address;
	void writeOpcode(uint16_t opcode);
	uint16_t readOpcode();
	void waitOpcode(bool write);
	uint16_t scanPathWord(unsigned index);
	void scanPathWord(unsigned index, uint16_t word);
	void setScanPathSetupBit(unsigned index, bool isSet);
	void loadScanPathSetup();
	void readTriggerConfiguration(uint16_t * data);
	uint16_t control();
	void control(uint16_t value);
public:
	struct Error: public std::runtime_error {
		Error(const std::string & m): runtime_error(m) {}
	};
	V1190B2(IVmeInterface & interface, unsigned address):
		_interface(interface),
		_address(address)
	{}
	void reset();
	bool triggerMode();
	void triggerMode(bool);
	void configureInterrupt(uint8_t level, uint8_t vector);
	bool test();
	unsigned bufferedEventsCount();
	//Next word from output buffer.
	uint32_t read();
	/** Time of which edges to write in event:
	* 0 - pair mode
	* 1 - trailing only
	* 2 - leading only
	* 4 - both
	* */
	void edgeDetectionMode(unsigned mode);
	void extendedTriggerTimeTag();
	void setWindow(int offset, int width);
	void highSpeedSerialization();
	void highSpeedCoreClock();
	void clear();
};

#endif /* V1190B2_H_ */
