#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>
#include <stdexcept>
#include <memory>


class Thread {
	pthread_t _pthread;
	static void * pbody(void *);
	volatile bool _interrupt;
	Thread(const Thread &);
	Thread & operator=(const Thread &);
	struct Action {
		virtual void run() = 0;
	};
	std::auto_ptr<Action> _action;
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
	template<class T>
	Thread(const T & action):
		_pthread(0),
		_interrupt(false)
	{
		struct ActionImpl: public Action {
			T _action;
			virtual void run() {
				this->_action();
			}
			ActionImpl(const T & t): _action(t){}
		};
		_action.reset(new ActionImpl(action));
	}
	// Throws std::runtime_error() if called from a thread created bypassing this mechanism
	// (like main thread or raw posix threads)
	static Thread & current();
	void start();
	void interrupt();
	void join();
	//Throws Interrupted exception if thread is being interrupted
	static void interruption_point(const std::string & message ="Thread interrupted.");
	static bool isInterrupted();
	virtual ~Thread();
};

#endif /* THREAD_H_ */
