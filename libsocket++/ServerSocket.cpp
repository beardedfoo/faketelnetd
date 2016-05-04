// Implementation of the ServerSocket class

#include "ServerSocket.h"
#include "SocketException.h"


ServerSocket::ServerSocket ( int port ) {
	if( port != -1 ) {
		if ( !Socket::create() ) {
			throw SocketException ( "Could not create server socket." );
		}
		
		if ( !Socket::bind ( port ) ) {
			throw SocketException ( "Could not bind to port." );
		}
		
		if ( !Socket::listen() ) {
			throw SocketException ( "Could not listen to socket." );
		}
	}
}

ServerSocket::~ServerSocket()
{
}