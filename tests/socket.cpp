#include "socketbuf.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h> //system
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <iterator>
#include <iostream>
#include <memory>
#include <Thread.h>
#include <stdio.h> //perror
#include <sstream>

using namespace std;

int port = 1100;

ostream & printString(ostream & ostr, const string & value) {
	return ostr << value << ", length: " << value.size();
}

string acceptAll(int socket) {
	if (true) {
		if (true) { //waiting for connection
			fd_set fdset;
			FD_ZERO(&fdset);
			FD_SET(socket, &fdset);
			struct timeval timeout = {10, 0};
			int selectRv = select(socket+1, &fdset, 0, 0, &timeout);
			if (selectRv < 0) {
				perror("Accept wait failed");
				return "";
			} if (selectRv == 0) {
				cerr << "No client connection in 10 seconds. Port closed." << endl;
				return "";
			} else {
				cerr << "Accept select OK" << endl;
			}

		}
	}
	int connection = ::accept(socket, 0, 0);
	if (connection < 0)
		throw runtime_error(strerror(errno));
	socketwrapper sw(connection, true);
	socketbuf sb(sw);
	return string(istreambuf_iterator<char>(&sb), istreambuf_iterator<char>());
}

struct CheckReceived {
	int _socket;
	string _pattern;
	CheckReceived(int socket, const string & pattern):
		_socket(socket),
		_pattern(pattern)
	{}
	void operator()() {
		string received = acceptAll(_socket);
		cout << "received: ";
		printString(cout, received) << endl;
		assert(_pattern == received);
	}
};

struct AcceptAndClose {
	int _socket;
	AcceptAndClose(int socket): _socket(socket) {}
	void operator()() {
		 int connection = ::accept(_socket, 0, 0);
		 char x;
		 read(connection, &x, 1);
		 cerr <<"Connection closing" << endl;
		 shutdown(connection,SHUT_RDWR);
		 close(connection);
		 cerr <<"Connection closed" << endl;
	}
};

std::string operator*(std::string const &s, size_t n)
{
    std::string r;  // empty string
    r.reserve(n * s.size());
    for (size_t i=0; i<n; i++)
        r += s;
    return r;
}

void noop(){}

int main() {
	sockaddr_in serv_addr;
	const string pattern = "pattern";
	const string largePattern = pattern*1000;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(s, (sockaddr*)(&serv_addr),  sizeof(serv_addr)) < 0) {
		close(s);
		throw runtime_error(strerror(errno));
	}
	listen(s,1);
	if (true)
	{
		string a = pattern+pattern;
		CheckReceived t(s, a);
		Thread rc(t);
		rc.start();
		{
			auto_ptr<socketwrapper> swr(socketwrapper::connect("localhost", port));
			socketbuf sw(*swr);
			sw.sputn(pattern.c_str(), pattern.size());
			sw.pubsync();
			cout << "Sent: ";
			printString(cout, pattern) << endl;
			sleep(1);
			sw.sputn(pattern.c_str(), pattern.size());
			cout << "Sent: ";
			printString(cout, pattern) << endl;
			sw.pubsync();
		}
		cout << "Multisend success" << endl;
		rc.join();
	}
	if (true){
//		if (fork()) {
//			ostringstream command;
//			command << "Debug/listen -p " << port+10;
//			cout << command.str() << endl;
//			system(command.str().c_str());
//			return 0;
//		}
//		sleep(1);

		AcceptAndClose t(s);
		Thread rc(t);
		rc.start();
		sleep(1);
		{
			auto_ptr<socketwrapper> swr(socketwrapper::connect("localhost", port));
			{
				timeval timeout = {10, 0};
				setsockopt(swr->socket(), SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeval));
			}
			socketbuf sw(*swr);
			ostream ostr(&sw);
			ostr << pattern << flush;
			int i = 0;
			cerr << "Sending" << endl;
			errno=0;
			while (ostr) {
				ostr << pattern << flush;
				i++;
			}
			cerr << "Sent " << i*pattern.size() << " bytes" <<  endl;
		}
		rc.join();
		cout << "Connection close handled properly: "  << strerror(errno) << endl;
	}
	if (true){
		CheckReceived t(s, pattern*1000);
		Thread rc(t);
		{
			auto_ptr<socketwrapper> sw(socketwrapper::connect("localhost", port));
			rc.start();
			socketbuf sb(*sw);
			for (int i = 0; i < 1000; i++)
				sb.sputn(pattern.c_str(), pattern.size());
			sb.pubsync();
		}
		rc.join();

	}
}
