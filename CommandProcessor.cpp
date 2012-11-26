#include "CommandProcessor.h"
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <functional>
#include "socketbuf.h"

using namespace std;

CommandProcessor::CommandProcessor(V1190B2 & device, int interruptFd):
	_device(device),
	_interruptFd(interruptFd),
	_listenSocket(socket(AF_INET, SOCK_STREAM, 0))
{
	if( _listenSocket< 0)
		throw Error(strerror(errno));
}

CommandProcessor::~CommandProcessor() {
	close(_listenSocket);
}
template<class T>
void getArgument(istream & istr, T & arg, const string & errorMessage) {
	istr >> arg;
	if (istr.fail())
		throw CommandProcessor::Error(errorMessage);
}

struct CommandProcessor::Nullary {
	CommandProcessor & _processor;
	void operator()() {
		_processor.sendData();
		cerr << "All data sent." << endl;
	}
};

void CommandProcessor::process(const std::string & line)
{
	istringstream istr(line);
	string command;
	std::getline(istr, command, ' ');
	if (istr.fail())
		throw Error(string("Failed to read command from line: ")+line);
	if (command == "window") {
		int offset, width;
		getArgument(istr, offset, "Failed to read offset for window command");
		getArgument(istr, width, "Failed to read width for window command");
		_device.setWindow(offset, width);
	} else if (command == "clear") {
		_device.clear();
		return;
	} else if (command == "noop") {
		return;
	} else if (command == "stop") {
		_thread.reset();
	} else if (command == "accept") {
		int port;
		getArgument(istr, port, "Failed to read port argument for accept");
		_thread.reset(); // To close socket
		CommandProcessor::listen(port);
		Nullary n = {*this};
		_thread.reset(new Thread(n));
		_thread->start();
	} else {
		return throw Error(string("Invalid command: ")+command);
	}
}

void CommandProcessor::listen(int port) {
	_listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(_listenSocket, (sockaddr*)(&serv_addr),  sizeof(serv_addr)) < 0) {
		close(_listenSocket);
		_listenSocket=-1;
		throw Error(strerror(errno));
	}
	::listen(_listenSocket,1);
}

void CommandProcessor::sendData() {
	if (true) { //waiting for connection
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(_listenSocket, &fdset);
		struct timeval timeout = {10, 0};
		int selectRv = select(_listenSocket+1, &fdset, 0, 0, &timeout);
		if (selectRv < 0) {
			perror("Accept wait failed");
			return;
		} if (selectRv == 0) {
			cerr << "No client connection in 10 seconds. Port closed." << endl;
			return;
		}
	}

	int connection = ::accept(_listenSocket, 0, 0);
	 clog << "Connection accepted" << endl;
	 if (connection < 0)
		throw runtime_error(strerror(errno));
	 socketwrapper sw(connection, true);
	 socketbuf sb(sw);
	 ostream output(&sb);
	 try {
		 while (output) {
			Thread::interruption_point();
			struct timeval timeout = {1, 0};
			fd_set fdset;
			FD_ZERO(&fdset);
			FD_SET(_interruptFd, &fdset);
			int selectRv = select(_interruptFd+1, 0, 0, &fdset, &timeout);
			int eventCount = _device.bufferedEventsCount();
			if (selectRv < 0) {
				perror("Select failed");
			} else if (selectRv == 0) {

				if (eventCount > 2)
					cerr << "Select timeout. Event count: " << eventCount << endl;
			} else if (selectRv > 0) {
				int vector;
				int bytes = read(_interruptFd, &vector, 4);
				if (bytes != 4) {
					cerr << "Failed to read interrupt vector" << endl;
				} else {
					clog << "Caught interrupt: Vector " << vector << "\n";
					clog << "Event count: " << eventCount << "\n";
				}
			}
			if (eventCount < 0)
				continue;
			try {
				while (output) {
					uint32_t word = _device.read();
					if (word >> 27 == 0x18) //Filler. No more data in output buffer so far.
						break;
					output.write((const char *)(&word), sizeof(word));
				}
			} catch (VmeReadError &) {}
			cerr << "Event sent\n";
			output.flush();
		 }
	 } catch (...) {
		 cerr << "Error in sending thread" << endl;
	 }
}

