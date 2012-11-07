#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>

class Thread {
	pthread_t _pthread;
	static void * pbody(void *);
	volatile bool _interrupt;
public:
	struct Error: public std::runtime_error {
		Error(const std::string & message): runtime_error(message) {}
	};
	class Fail: public Error {
		int _code;
	public:
		Fail(int code, const std::string & message): Error(message), _code(code) {}
		int code() const {return _code;}
	};
	struct Interrupted: public Error {
		Interrupted(const std::string & message): Error(message) {}
	};
	Thread();
	void start();
	void interrupt();
	void join();
	//Throws Interrupted exception if thread is being interrupted
	static void interruption_point(const std::string & message ="Thread interrupted.");
	static bool isInterrupted();
	virtual void run() = 0;
	virtual ~Thread();
};

#endif /* THREAD_H_ */
