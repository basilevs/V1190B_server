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
#include <memory>
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

struct CheckReceived {
	int _socket;
	string _pattern;
	CheckReceived(int socket, string pattern): _socket(socket), _pattern(pattern) {}
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
		 cerr <<"Connection closing" << endl;
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
	if (false)
	{
		string pattern = "pattern";
		CheckReceived t(s, pattern+pattern);
		Thread rc(t);
		{
			auto_ptr<socketwrapper> swr(socketwrapper::connect("localhost", port));
			rc.start();
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
	{
		string pattern = "pattern";
		AcceptAndClose t(s);
		Thread rc(t);
		{
			auto_ptr<socketwrapper> swr(socketwrapper::connect("localhost", port));
			socketbuf sw(*swr);
			rc.start();
			ostream ostr(&sw);
			ostr << pattern << flush;
			sleep(1);
			while (ostr) {
				cerr << "Sending" << endl;
				ostr << pattern << flush;
				cerr << "Sent" << endl;
			}
		}
		cout << "Connection close handled properly" << endl;
		rc.join();
	}
	{
		string pattern = "pattern";
		CheckReceived t(s, pattern*1000);
		Thread rc(t);
		{
			auto_ptr<socketwrapper> sw(socketwrapper::connect("localhost", port));
			rc.start();
			socketbuf sb(*sw);
			for (int i = 0; i < 1000; i++)
				sb.sputn(pattern.c_str(), pattern.size());
			sb.pubsync();
			cout << "Sent: ";
			printString(cout, pattern) << endl;
		}
		rc.join();
	}

}
