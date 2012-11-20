#include "Thread.h"
#include <iostream>
#include <assert.h>

using namespace std;

template<class T>
class ThreadKey {
	pthread_key_t _threadKey;
	T * getSpecificPointer() const {
		void * stored = pthread_getspecific(_threadKey);
		return static_cast<T*>(stored);
	}
	static void destructor(void * value) {
		T * stored = static_cast<T*>(value);
//		clog << "Deleting " << stored << endl;
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
//			clog << "Created " << stored << endl;
			if (pthread_setspecific(_threadKey, stored) != 0) {
				throw std::runtime_error("pthread_setspecific fail.");
			}
		} else {
//			clog << "Changed " << stored << endl;
			*stored = value;
		}	}
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
		cerr << "Thread " << thread->_pthread << " start" << endl;
		interruption_point();
		thread->_action->run();
		cerr << "Thread " << thread->_pthread  << " ended without errors" << endl;
	} catch (Interrupted &) {
		cerr << "Thread " << thread->_pthread  << " interrupted " << endl;
	} catch (exception & e) {
		cerr << "Unhandled exception: " << e.what() << endl;
	}
	return 0;
}

void Thread::start()
{
	join();
	_interrupt = false;
	int rv = pthread_create(&_pthread, 0, pbody, this);
	if (rv != 0) {
		_pthread = 0;
		throw Fail(rv, "Failed to create thread");
	}
	assert(_pthread!=0);
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
	cerr << "Thread " << _pthread  << " joined" << endl;
	_pthread = 0;
}

void Thread::interruption_point(const std::string & message)
{
	if (isInterrupted())
		throw Interrupted(message);
}

Thread::~Thread() {
	interrupt();
	join();
	assert(_pthread==0&&"Unjoined thread destructed.");
}

