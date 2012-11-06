#ifndef IVMEINTERFACE_H_
#define IVMEINTERFACE_H_
#include <inttypes.h>
#include <stdexcept>

struct VmeError: public std::runtime_error {
	VmeError(const std::string & message): runtime_error(message) {}
};

struct VmeReadError: public VmeError {
	VmeReadError(const std::string & message = "VME read failed"): VmeError(message) {}
};

struct VmeWriteError: public VmeError {
	VmeWriteError(const std::string & message = "VME write failed"): VmeError(message) {}
};


struct IVmeInterface {
	virtual void write_a32d16(uint32_t address, uint16_t value) = 0;
	virtual uint16_t read_a32d16(uint32_t address) = 0;
	virtual void write_a32d32(uint32_t address, uint32_t value) = 0;
	virtual uint32_t read_a32d32(uint32_t address) = 0;
	virtual ~IVmeInterface(){}
};


#endif /* IVMEINTERFACE_H_ */
