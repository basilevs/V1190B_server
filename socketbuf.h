#ifndef SOCKETBUF_H_
#define SOCKETBUF_H_

#include <streambuf>
#include <assert.h>
#include <stdexcept>

class ISocketWrapper {
public:
	ssize_t fs;
	struct Error: public std::runtime_error {
		Error(const std::string & message): runtime_error(message) {}
	};
	//Blocks until completion
	virtual int read(char * oBuffer, int length) = 0;
	//Blocks until completion
	virtual int write(const char * iBuffer, int length) = 0;
	//Returns immediately
	virtual int recv(char * oBuffer, int length) {
		return 0;
	}
	//Returns immediately
	virtual int send(char * iBuffer, int length) {
		return 0;
	}
	virtual ~ISocketWrapper() {}
};

class socketwrapper: public ISocketWrapper {
public:
	typedef std::string Host;
private:
	int _socket;
	bool _own;
public:
	int socket() {return _socket;}
	int read(char * oBuffer, int length);
	int write(const char * iBuffer, int length);
public:
	class CodeError: public ISocketWrapper::Error {
		int _code;
	public:
		int code() const;
		CodeError(int code, const std::string & message): Error(message), _code(code) {}
	};
	struct Errno: public CodeError {
		Errno();
	};
	struct HErrno: public CodeError {
		HErrno();
	};

	socketwrapper(int socket, bool own);
	virtual ~socketwrapper();
	static socketwrapper * connect(const Host & host, int port);
};

class socketbuf: public std::basic_streambuf<char> {
	static const int BUFFER_SIZE=500;
public:
	typedef std::basic_streambuf<char> Parent;
	typedef Parent::char_type char_type;
	typedef Parent::int_type int_type;
	typedef Parent::traits_type traits_type;
private:
	char_type _iBuffer[BUFFER_SIZE], _oBuffer[BUFFER_SIZE];
	ISocketWrapper & _socket;
public:
	socketbuf(ISocketWrapper & socket): _socket(socket) {
		this->setg(_iBuffer, _iBuffer+BUFFER_SIZE, _iBuffer+BUFFER_SIZE);
		this->setp(_oBuffer, _oBuffer+BUFFER_SIZE-1);
	}
	virtual ~socketbuf() {}

private:

	void memmove(char_type* to, char_type* from, int size);

	int_type underflow();

	int_type writeChars(size_t toWriteCount);

	int_type overflow(int_type c);
	int sync() {
		int length=this->pptr() - _oBuffer;
		assert(length <= BUFFER_SIZE);
		if (length>0) {
			return writeChars(length)==traits_type::eof() ? -1 : 0;
		} else {
			return 0;
		}
	}
};






#endif /* SOCKETBUF_H_ */
