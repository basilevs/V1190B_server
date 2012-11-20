/*
 * socketbuf.cpp
 *
 *  Created on: 20.10.2009
 *      Author: gulevich
 */

#include "socketbuf.h"
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
using namespace std;

socketwrapper::socketwrapper(int socket, bool own):
	_socket(socket),
	_own(own)
{}

socketwrapper::~socketwrapper() {
	assert(_socket!=-1);
	if (!_own)
		return;
	shutdown(_socket, SHUT_RDWR);
	close(_socket);
}

socketwrapper * socketwrapper::connect(const Host & host, int port) {
	struct hostent *hp;
	struct sockaddr_in addr;
	typedef socketwrapper Err;
	if((hp = gethostbyname(host.c_str())) == NULL){
		throw HErrno();
	}
	bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	int _socket = ::socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_socket < 0) {
		throw Errno();
	}
	assert(ENOTCONN==107);
	assert(_socket>=0);
	errno=0;

	if(::connect(_socket, (const struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1){
		throw Errno();
	}

	if (errno == EINPROGRESS)	{
		//fd_set wset;
		//FD_ZERO(&wset);
		//assert(_socket>=0);
		//FD_SET(_socket,&wset);
		//timeval timeout;
		//timeout.tv_sec=10;
		//timeout.tv_usec=0;

		///* Wait for write bit to be set */
		//if (select(1,0,&wset,0,&timeout) > 0) {
		//	return OK;
		//} else {
		//	return TIMEOUT;
		//}
	}
	return new socketwrapper(_socket, true);
}
inline void dump(ostream & str, char * data, int size, const string & title) {
	return;
	if (size < 0)  {
		str << "Socket returned negative value" << endl;
		return;
	}
	str << title << ":";
	for (int i = 0; i < size; i++) 
		str << hex << (unsigned(0xFF) & unsigned(data[i])) << " ";
	str << dec;
	str << endl;
}


int socketwrapper::read(char * oBuffer, int length) {
	int rv = ::read(socket(), oBuffer, length);
	dump(cerr, oBuffer, length, "low level read");
	return rv;
}

int socketwrapper::write(const char * iBuffer, int length) {
//    cerr << "Writing " << length << " bytes in socket " << socket() << endl;
	return ::write(socket(), iBuffer, length);
}

typedef socketbuf::int_type int_type;

int_type socketbuf::writeChars(size_t toWriteCount)
{
    assert(toWriteCount <= size_t(BUFFER_SIZE));
    assert(toWriteCount>0);
    cerr << "Writing " << toWriteCount << " bytes " << endl;
    int byteCount = _socket.write(_oBuffer, toWriteCount * sizeof (char_type));
    cerr << "Wrote " << byteCount << " bytes " << endl;
    if(byteCount <= 0) {
    	cerr << "Connection closed" << endl;
        return traits_type::eof();
    }

    char_type lastWrittenCharacter = _oBuffer[byteCount - 1];
    int bytesLeft = toWriteCount - byteCount;
    assert(bytesLeft < BUFFER_SIZE);
    memmove(_oBuffer, _oBuffer + byteCount, bytesLeft);
    this->setp(_oBuffer + bytesLeft, _oBuffer + BUFFER_SIZE - 1);
    return traits_type::to_int_type(lastWrittenCharacter);
}

int_type socketbuf::overflow(int_type c)
{
    char_type *current = this->pptr();
    assert(current <= this->epptr());
    assert(current <= _oBuffer+BUFFER_SIZE-1);
    *current = traits_type::to_char_type(c); //We can write there as current is still not greater than _oBuffer+BUFFER_SIZE-1
    return writeChars(current - _oBuffer + 1);
}

int_type socketbuf::underflow()
{
    char_type *begin = this->eback(), * current = this->gptr(), *end = this->egptr();
    assert(begin==_iBuffer);
    assert(end<=_iBuffer+BUFFER_SIZE);
    assert(current>=begin);
    assert(current<=end);
    // Move non-read characters to the beginning of the buffer
    int length = end - current;
    memmove(_iBuffer, current, length);
    int byteCount = _socket.read(_iBuffer + length, (BUFFER_SIZE - length) * sizeof (char_type));
    if(byteCount <= 0)
        return traits_type::eof();

    this->setg(_iBuffer, _iBuffer, _iBuffer + byteCount + length);
    return traits_type::to_int_type(_iBuffer[0]);
}

inline void socketbuf::memmove(char_type *to, char_type *from, int size)
{
    assert(to <= from);
    if(to == from)
        return;

    for(int i = 0;i < size;++i){
        to[i] = from[i];
    }
}


int socketwrapper::CodeError::code() const
{
	return _code;
}

socketwrapper::Errno::Errno():
	CodeError(errno, strerror(errno))
{
}

socketwrapper::HErrno::HErrno():
	CodeError(h_errno, strerror(h_errno))
{
}







