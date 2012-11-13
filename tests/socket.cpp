#include "socketbuf.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <iterator>
#include <iostream>
#include <Thread.h>

using namespace std;

int port = 1066;

ostream & printString(ostream & ostr, const string & value) {
	return ostr << value << ", length: " << value.size();
}

string acceptAll(int socket) {
	 int connection = ::accept(socket, 0, 0);
	 if (connection < 0)
		throw runtime_error(strerror(errno));
	 socketwrapper sw(connection, true);
	 socketbuf sb(sw);
	 return string(istreambuf_iterator<char>(&sb), istreambuf_iterator<char>());
}

struct CheckReceived: public Thread {
	int _socket;
	string _pattern;
	CheckReceived(int socket, string pattern): _socket(socket), _pattern(pattern) {}
	void run() {
		string received = acceptAll(_socket);
		cout << "received: ";
		printString(cout, received) << endl;
		assert(_pattern == received);
	}
};

int main() {
	sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(s, (sockaddr*)(&serv_addr),  sizeof(serv_addr)) < 0) {
		close(s);
		throw runtime_error(strerror(errno));
	}
	listen(s,3);
	{
		string pattern = "pattern";
		CheckReceived rc(s, pattern+pattern);
		auto_ptr<socketwrapper> sw(socketwrapper::connect("localhost", port));
		rc.start();
		sw->write(pattern.c_str(), pattern.size());
		cout << "Sent: ";
		printString(cout, pattern) << endl;
		sleep(1);
		sw->write(pattern.c_str(), pattern.size());
		cout << "Sent: ";
		printString(cout, pattern) << endl;
	}

}
