#include "Thread.h"
#include <iostream>

using namespace std;

Thread::Thread() {
	_pthread = 0;
	_interrupt = false;
}

template<class T>
class ThreadKey {
	pthread_key_t _threadKey;
	T * getSpecificPointer() const {
		void * stored = pthread_getspecific(_threadKey);
		return static_cast<T*>(stored);
	}
	static void destructor(void * value) {
		T * stored = static_cast<T*>(stored);
		assert(stored);
		delete stored;
	}
public:
	ThreadKey() {
		_threadKey = 0;
		if (pthread_key_create(&_threadKey, destructor) != 0)
			throw std::runtime_error("pthread_key_create failed");
	}
	T & getSpecific() const {
		T * stored = getSpecificPointer();
		if (!stored)
			throw std::runtime_error("Nothing was stored in thread local storage under this key.");
		return *(static_cast<T*>(stored));
	}
	void setSpecific(const T & value) {
		T * stored = getSpecificPointer();
		if (!stored) {
			stored = new T(value);
			if (pthread_setspecific(_threadKey, stored) != 0) {
				throw std::runtime_error("pthread_setspecific fail.");
			}
		} else {
			*stored = value;
		}
	}
	~ThreadKey() {
		pthread_key_delete(_threadKey);
	}
};

ThreadKey<Thread*> currentThreadKey;



Thread & Thread::current() {
	return *currentThreadKey.getSpecific();
}

void * Thread::pbody(void* d)
{
	Thread * thread = static_cast<Thread *>(d);
	currentThreadKey.setSpecific(thread);
	try {
		thread->run();
	} catch (Interrupted &) {
	} catch (exception & e) {
		cerr << "Unhandled exception: " << e.what() << endl;
	}
	return 0;
}

void Thread::start()
{
	if (_pthread != 0)

	_interrupt = false;
	int rv = pthread_create(&_pthread, 0, pbody, this);
	if (rv != 0) {
		_pthread = 0;
		throw Fail(rv, "Failed to create thread");
	}
}

bool Thread::isInterrupted() {
	return current()._interrupt;
}

void Thread::interrupt() {
	_interrupt = true;
}

void Thread::join() {
	if (!_pthread)
		return;
	int rv = pthread_join(_pthread, 0);
	if (rv != 0)
		throw Fail(rv, "Failed to join thread");
	_pthread = 0;
}

void Thread::interruption_point(const std::string & message)
{
	if (isInterrupted())
		throw Interrupted(message);
}

Thread::~Thread() {
	interrupt();
	try {
		join();
	} catch(...) {}
}

