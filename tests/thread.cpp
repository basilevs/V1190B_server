#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iterator>
#include <iostream>
#include <memory>
#include <Thread.h>
#include <assert.h>
#include <iostream>


class FlagSetter {
	bool & _flag;
public:
	FlagSetter(bool & flag): _flag(flag) {}
	void operator()() {
		_flag = true;
	}
};

class CycleWaiter: public FlagSetter {
public:
	CycleWaiter(bool & flag): FlagSetter(flag) {}
	void operator()() {
		FlagSetter::operator()();
		for(int i = 0 ; i < 10; ++i) {
			Thread::interruption_point();
			sleep(1);
		}
		assert(false);
	}
};

using namespace std;

int main() {
	bool flag = false;
	{
		FlagSetter t(flag);
		Thread s(t);
		flag = false;
		s.start();
		sleep(1);
		if (!flag)
			cerr << "Simple thread failed" << endl;
		s.join();
	}
	{
		CycleWaiter t(flag);
		Thread s(t);
		flag=false;
		s.start();
		sleep(2);
		s.interrupt();
		if (!flag)
			cerr << "Interruption failed" << endl;
		s.join();
	}
	{
		FlagSetter t(flag);
		Thread s(t);
		flag = false;
		s.start();
	}
	cerr << "Immediate interruption success" << endl;

	return 0;
}
