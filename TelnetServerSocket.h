#ifndef __TELNETSERVERSOCKET_H
#define __TELNETSERVERSOCKET_H

#include <string>
#include <map>
#include <vector>
using namespace std;

#include "libsocket++/ServerSocket.h"
#include "TelnetOptions.h"
#include "TelnetCommands.h"

class TelnetServerSocket : public ServerSocket {
	public:
		TelnetServerSocket( int port = 23 );
		virtual ~TelnetServerSocket();
		
		unsigned char getChar();
		string getLine( bool hidden=false );
		
		TelnetServerSocket* accept();
		
		static enum {
			BELL=7,
			BS=8,
			HT=9,
			LF=10,
			VT=11,
			FF=12,
			CR=13
		} TelnetCtrlCode;
		
		void setPeerEcho( bool val );
		void setLocalEcho( bool val );
		bool getLocalEcho();
		bool getPeerEcho();
		void init();

		void requestLineModeNegociation();
		void handleSbLinemode( const vector<unsigned char>& sbSequence );
	protected:
		int peerEcho;
		int localEcho;
};

#endif
