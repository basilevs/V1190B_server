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
#include "socketbuf.h"

using namespace std;

CommandProcessor::CommandProcessor(V1190B2 & device, int interruptFd):_device(device), _interruptFd(interruptFd) {
}

template<class T>
void getArgument(istream & istr, T & arg, const string & errorMessage) {
	istr >> arg;
	if (istr.fail())
		throw CommandProcessor::Error(errorMessage);
}

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
		_thread.reset(new ProcessingThread(_device, port, _interruptFd));
		_thread->start();
	} else {
		return throw Error(string("Invalid command: ")+command);
	}
}

CommandProcessor::ProcessingThread::ProcessingThread(V1190B2 & device, int port, int interruptFd): _device(device), _interruptFd(interruptFd) {
	_socket = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	if (bind(_socket, (sockaddr*)(&serv_addr),  sizeof(serv_addr)) < 0) {
		close(_socket);
		throw runtime_error(strerror(errno));
	}
	listen(_socket,1);
	start();
}
CommandProcessor::ProcessingThread::~ProcessingThread() {
	close(_socket);
}
void CommandProcessor::ProcessingThread::run() {
	 int connection = ::accept(_socket, 0, 0);
	 if (connection < 0)
		throw runtime_error(strerror(errno));
	 socketwrapper sw(connection, true);
	 socketbuf sb(sw);
	 ostream output(&sb);
	 while (true) {
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
				cout << "Caught interrupt " << endl;
				cout << "Vector " << vector << endl;
				cout << "Event count: " << eventCount << endl;
			}
		}
		if (eventCount < 0)
			continue;
		try {
			while (true) {
				uint32_t word = _device.read();
				output.write((const char *)(&word), sizeof(word));
			}
		} catch (VmeReadError &) {
		}
		output.flush();
	 }
}

