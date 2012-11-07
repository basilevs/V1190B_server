#include "Thread.h"
#include <iostream>

using namespace std;

Thread::Thread() {
	_pthread = 0;
	_interrupt = false;
}
static __thread Thread *currentThread = 0;

void * Thread::pbody(void* d)
{
	Thread * thread = static_cast<Thread *>(d);
	currentThread = thread;
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
	if (!currentThread)
		return false;
	return currentThread->_interrupt;
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

