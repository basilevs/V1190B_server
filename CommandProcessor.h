#ifndef COMMANDPROCESSOR_H_
#define COMMANDPROCESSOR_H_

#include <string>
#include <V1190B2.h>
#include "Thread.h"

class CommandProcessor {
	class ProcessingThread: public Thread {
		V1190B2 & _device;
		int _socket;
		int _interruptFd;
	public:
		ProcessingThread(V1190B2 & device, int port, int interruptFd);
		~ProcessingThread();
		void run();
	};
	V1190B2 & _device;
	std::auto_ptr<ProcessingThread> _thread;
	int _interruptFd;
public:
	struct Error: public std::runtime_error {
		Error(const std::string & message): runtime_error(message){}
	};
	CommandProcessor(V1190B2 & device, int interruptFd);
	void process(const std::string & line);
	virtual ~CommandProcessor(){}
};

#endif /* COMMANDPROCESSOR_H_ */
