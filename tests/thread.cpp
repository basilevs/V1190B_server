#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iterator>
#include <iostream>
#include <memory>
#include <Thread.h>
#include <assert.h>
#include <iostream>


class FlagSetter: public Thread {
	bool & _flag;
public:
	FlagSetter(bool & flag): _flag(flag) {}
	void run() {
		_flag = true;
	}
};

class CycleWaiter: public FlagSetter {
public:
	CycleWaiter(bool & flag): FlagSetter(flag) {}
	void run() {
		FlagSetter::run();
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
		FlagSetter s(flag);
		flag = false;
		s.start();
		sleep(1);
		if (!flag)
			cerr << "Simple thread failed" << endl;
		s.join();
	}
	{
		CycleWaiter s(flag);
		flag=false;
		s.start();
		sleep(2);
		s.interrupt();
		if (!flag)
			cerr << "Interruption failed" << endl;
		s.join();
	}


	return 0;
}
