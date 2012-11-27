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
#include <sstream>
#include <memory>
#include <Thread.h>
#include <stdio.h> //perror


using namespace std;

template<class T>
T parse(const string & input) {
	istringstream temp(input);
	T rv;
	temp >> rv;
	if (temp.fail())
		throw runtime_error(string("Failed to parse string ") + input);
	return rv;
}

int main(int argc, char * argv[]) {
	int opt;
	int port = 1090;
	while((opt = getopt(argc, argv, "hp:")) != -1) {
		switch (opt) {
		case 'p': port = parse<int>(optarg); break;
		case 'h':
		default:
			cout << "Usage: listen -p port" << endl;
			return 1;
		}
	}
	sockaddr_in serv_addr;
	const string pattern = "pattern";
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(s, (sockaddr*)(&serv_addr),  sizeof(serv_addr)) < 0) {
		close(s);
		throw runtime_error(strerror(errno));
	}
	cerr << "Listening" << endl;
	listen(s,1);
	int connection = accept(s, 0, 0);
	char x;
	read(connection, &x, 1);
	cerr <<"Connection closing" << endl;
	shutdown(connection,SHUT_RDWR);
	close(connection);
	cerr <<"Connection closed" << endl;
	sleep(10);
	return 0;
}

