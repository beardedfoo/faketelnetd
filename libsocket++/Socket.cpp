// Implementation of the Socket class.


#include "Socket.h"
#include "SocketException.h"
#include "string.h"
#include "../logger.h"
#include <iostream>
#include <string.h>
#include <errno.h>
#include <fcntl.h>



Socket::Socket() {
	m_sock = -1;
	memset ( &m_addr, 0, sizeof(m_addr) );

}

Socket::~Socket() {
	if ( is_valid() ) {
		Logger::debug() << "Closing socket " << (int)m_sock << endl;
		::close ( m_sock );
	}
}

bool Socket::create()
{
	m_sock = socket ( AF_INET, SOCK_STREAM, 0 );

	if ( !is_valid() ) {
		return false;
	}


	// TIME_WAIT - argh
	int on = 1;
	if ( setsockopt ( m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on) ) == -1 ) {
		return false;
	}

	return true;
}

bool Socket::is_valid() const {
	return static_cast<bool>( m_sock != -1 );
}

bool Socket::bind ( const int port )
{

  if ( ! is_valid() )
    {
      return false;
    }



  m_addr.sin_family = AF_INET;
  m_addr.sin_addr.s_addr = INADDR_ANY;
  m_addr.sin_port = htons ( port );

  int bind_return = ::bind ( m_sock,
			     ( struct sockaddr * ) &m_addr,
			     sizeof ( m_addr ) );


  if ( bind_return == -1 )
    {
      return false;
    }

  return true;
}


bool Socket::listen() const {
	//We can't listen to an invalid socket
	if ( !is_valid() ) {
		return false;
	}

	int listen_return = ::listen ( m_sock, MAXCONNECTIONS );
	
	if ( listen_return == -1 ) {
		return false;
	}
	
	return true;
}

std::string Socket::addressAsString() {
	char tmp[512];
	
	sockaddr_in* sa = &m_addr;
	inet_ntop( AF_INET, (void*)&(m_addr.sin_addr), tmp, sizeof tmp);
	return std::string(tmp);
}

Socket* Socket::accept ( Socket* alreadyCreated ) const {
	Socket* retVal = NULL;
	if( alreadyCreated == NULL ) {
		retVal = new Socket();
	} else {
		retVal = dynamic_cast<Socket*>( alreadyCreated );
	}
	
	int addr_length = sizeof( m_addr );
	retVal->m_sock = ::accept ( m_sock, (sockaddr*) &(retVal->m_addr), (socklen_t*) &addr_length );
	
	if ( retVal->m_sock <= 0 ) {
		throw std::string("Failed to accept() socket because: m_sock <= 0");
	}
	
	return retVal;
}


bool Socket::send ( const std::string& s ) const {
	int status = ::send( m_sock, s.c_str(), s.size(), MSG_NOSIGNAL );
	if ( status == -1 ) {
		return false;
	} else {
		return true;
	}
}

bool Socket::send ( const unsigned char& c ) const
{
	std::string s;
	s += c;
	send( s );
}

const Socket& Socket::operator << ( const std::string& s ) const {
	send( s );
	
	return *this;
}

const Socket& Socket::operator << ( const unsigned char& c ) const {
	send( c );
	Logger::debug() << "Sent character " << c << " (" << (int)c << ")" << endl;
	
	return *this;
}

const Socket& Socket::operator >> ( std::string& s ) const {
	if ( !recv(s) ) {
		throw SocketException ( "Could not read from socket." );
	}

	return *this;
}

const Socket& Socket::operator >> ( unsigned char& c ) const {
	std::string s;
	if ( !recv(s,1) ) {
		throw SocketException ( "Could not read from socket." );
	}


	c = s[0];
	Logger::debug() << "Read character (" << (int)c << ")" << endl;
	return *this;
}

int Socket::recv( std::string& s, const int& max ) const {
	char buf[max+1];
	memset( buf, 0, max+1 );
	s = "";

	
	int status = ::recv( m_sock, buf, max, 0 );
	if ( status == -1 ) {
		Logger::info() << "status == -1   errno == " << errno << "  in Socket::recv\n";
		return 0;
	} else if( status == 0 ) {
		return 0;
	} else {
		s = buf;
		return status;
	}
}



bool Socket::connect ( const std::string host, const int port ) {
	if ( !is_valid() ) {
		return false;
	}
	
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons( port );

	int status = inet_pton( AF_INET, host.c_str(), &m_addr.sin_addr );

	if ( errno == EAFNOSUPPORT ) {
		return false;
	}

	status = ::connect( m_sock, (sockaddr*) &m_addr, sizeof(m_addr) );
	if ( status == 0 ) {
		return true;
	} else {
		return false;
	}
}

void Socket::set_non_blocking ( const bool b ) {
	int opts;
	opts = fcntl ( m_sock, F_GETFL );

	if ( opts < 0 ) {
		return;
	}

	if ( b ) {
		opts = ( opts | O_NONBLOCK );
	} else {
		opts = ( opts & ~O_NONBLOCK );
	}
	
	fcntl( m_sock, F_SETFL, opts );
}
