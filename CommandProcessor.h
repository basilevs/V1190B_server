#ifndef COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_H_

#include <string>
#include <V1190B2.h>
#include "Thread.h"

class CommandProcessor {
	struct Nullary;
	V1190B2 & _device;
	std::auto_ptr<Thread> _thread;
	int _interruptFd;
	int _listenSocket;
	void listen(int port);
	void sendData();
public:
	struct Error: public std::runtime_error {
		Error(const std::string & message): runtime_error(message){}
	};
	CommandProcessor(V1190B2 & device, int interruptFd);
	void process(const std::string & line);
	virtual ~CommandProcessor();
};

#endif /* COMMANDPROCESSOR_H_ */
