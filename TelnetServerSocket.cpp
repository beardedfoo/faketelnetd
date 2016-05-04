#include "TelnetServerSocket.h"

#include <map>
#include <sstream>
#include <string>
#include <deque>
using namespace std;

#include "logger.h"
#include "TelnetOptions.h"
#include "TelnetCommands.h"

TelnetServerSocket::TelnetServerSocket( int port ) : ServerSocket( port ) {
	localEcho = -1;
	peerEcho = -1;
}

TelnetServerSocket::~TelnetServerSocket() {
}

unsigned char TelnetServerSocket::getChar() {
	unsigned char c; //Return value
	deque<unsigned char> history; //We will store a log of characters received here
	deque<unsigned char> rawHistory; //We will store a log of characters received here, even control codes
	
	while( true ) {
		//Read a character from the stream
		(*this) >> c;
		rawHistory.push_front( c );
		
		//Handle telnet IAC's
		if( c == TELNET_COMMAND_IAC ) {
			//Build a debug string for this telnet command
			stringstream debugStr;
			debugStr << "received control code: IAC ";
			
			//Read the command byte
			unsigned char cmd;
			(*this) >> cmd;
			debugStr << telnetCommandAsStr(cmd);
			
			//These commands will send a 3rd byte, the argument
			unsigned char arg = 0;
			if( cmd == TELNET_COMMAND_DO || cmd == TELNET_COMMAND_DONT || cmd == TELNET_COMMAND_WILL || cmd == TELNET_COMMAND_WONT ) {
				//Read the arg
				(*this) >> arg;
				debugStr << " " << telnetOptionAsStr(arg);
				
				//Handle echo negociation
				if( arg == TELNET_OPTION_ECHO ) {
					switch( cmd ) {
						case TELNET_COMMAND_WONT:
							setPeerEcho( false );
							break;
						case TELNET_COMMAND_WILL:
							setPeerEcho( true );
							break;
						
						case TELNET_COMMAND_DO:
							setLocalEcho( true );
							break;
						
						case TELNET_COMMAND_DONT:
							setLocalEcho( false );
							break;
					}
				}
				
				if( arg == TELNET_OPTION_LINEMODE && cmd == TELNET_COMMAND_WILL ) {
				}
			}

			//Handle subparam negociations
			if( cmd == TELNET_COMMAND_SB ) {
				vector<unsigned char> sbSequence;
				while( true ) {
					unsigned char sbParam;
					*this >> sbParam;
					sbSequence.push_back( sbParam );
					rawHistory.push_front( sbParam );

					//When we see 'IAC SE' the negociation sequence is over
					if( rawHistory[1] == TELNET_COMMAND_IAC && rawHistory[0] == TELNET_COMMAND_SE ) {
						//Pull 'IAC SE' out of the sbSequence, our handlers don't care about this
						sbSequence.erase( sbSequence.end()-1 );
						sbSequence.erase( sbSequence.end()-2 );
						break;
					}
				}

				if( sbSequence[0] == TELNET_OPTION_LINEMODE ) {
					sbSequence.erase( sbSequence.begin() );
					handleSbLinemode( sbSequence );
				}
			}
			
			//Send the debug message to the log
			Logger::debug() << debugStr.str() << endl;
		} else {
			//Put this character into the history log
			history.push_front( c );
			
			//Handle escape sequences
			if( c == 27 ) {
				continue;
			}
			
			if( history[1] == 27 ) {
				continue;
			}
			
			if( history[1] == '[' && history[2] == 27 ) {
				continue;
			}
			
			if( history[1] == '1' && history[2] == '[' && history[3] == 27 ) {
				continue;
			}
			
			if( history[2] == '1' && history[3] == '[' && history[4] == 27 ) {
				continue;
			}
			
			if( history[1] == '3' && history[2] == '[' && history[3] == 27 ) {
				continue;
			}
			
			if( history[1] == 'O' && history[2] == 27 ) {
				continue;
			}
			//End handling escape sequences
			
			//Return the character
			return c;
		}
	}
}

string TelnetServerSocket::getLine( bool hidden ) {
	string line; //The return value
	while( true ) {
		//Get a formatted character
		unsigned char c = getChar();
		
		//Remember whether or not we erased a character from the line
		bool erasedChar = false;
		
		//Handle line endings
		if( c == '\r' ) {
			//Read another character after we receive \r, because telnet always does either \r\n or \r\0
			(*this) >> c;
			
			//If we are supposed to be doing the echo'ing, send end-of-line to the client
			if( getLocalEcho() ) {
				(*this) << "\r\n";
			}
			
			//Since we received end-of-line, return the string as it is
			return line;
		} else {
			//Handle backspace and ^H
			if( c == 127 || c == 8 ) {
				//Replace the last character in the line with nothing
				if( !line.empty() ) {
					line.replace( line.size()-1, 1, "" );
					erasedChar = true;
				}
			} else {
				//Add the character to the end of line
				line += c;
			}
		}
		
		if( getLocalEcho() && !hidden ) {
			//Handle character erasing (backspace) by moving left, printing a space, and moving left again
			// This is either the way it's supposed to be done or a hack, I couldn't find any info on
			// "the right way" to do backspaces so I have no clue.
			if( erasedChar ) {
				(*this) << '\x08' << ' ' << '\x08';
			} else if( c != 127 && c != 8 ) {
				//If this has nothing to do with backspaces than echo the character
				(*this) << c;
			}
		}
	}
}

TelnetServerSocket* TelnetServerSocket::accept() {
	//Create an unbinded TelnetServerSocket
	TelnetServerSocket* sock = new TelnetServerSocket( -1 );
	
	//Accept a connection, using the socket we created
	ServerSocket::accept( sock );
	
	//Return the socket
	return sock;
}

void TelnetServerSocket::setPeerEcho( bool val ) {
	if( (int)val != peerEcho || peerEcho == -1 ) {
		if( val ) {
			(*this) << TELNET_COMMAND_IAC << TELNET_COMMAND_DO << TELNET_OPTION_ECHO;
		} else {
			(*this) << TELNET_COMMAND_IAC << TELNET_COMMAND_DONT << TELNET_OPTION_ECHO;
		}
		peerEcho = val;
	}
}

void TelnetServerSocket::setLocalEcho( bool val ) {
	if( (int)val != localEcho || localEcho == -1 ) {
		if( val ) {
			(*this) << TELNET_COMMAND_IAC << TELNET_COMMAND_WILL << TELNET_OPTION_ECHO;
		} else {
			(*this) << TELNET_COMMAND_IAC << TELNET_COMMAND_WONT << TELNET_OPTION_ECHO;
		}
		localEcho = val;
	}
}

bool TelnetServerSocket::getLocalEcho() {
	return (bool) localEcho;
}

bool TelnetServerSocket::getPeerEcho() {
	return (bool) peerEcho;
}

void TelnetServerSocket::requestLineModeNegociation() {
	*this << TELNET_COMMAND_IAC << TELNET_COMMAND_DO << TELNET_OPTION_LINEMODE;
}

void TelnetServerSocket::handleSbLinemode( const vector<unsigned char>& sbSequence ) {
	enum LineModeCommands {
		LINEMODE_MODE = 1,
		LINEMODE_FORWARDMASK = 2,
  		LINEMODE_SLC = 3,
	};
	
	enum LineModeModeOptions {
		MODE_EDIT = 1,
  		MODE_TRAPSIG = 2,
    		MODE_ACK = 4,
      		MODE_SOFT_TAB = 8,
		MODE_LIT_ECHO = 16,
	};
	
	//Request an empty mode mask, this way characters are sent as the user types them without allowing remote editing, which totally screws up local echo
	unsigned char requestedMode = 0;
	*this << TELNET_COMMAND_IAC << TELNET_COMMAND_SB << TELNET_OPTION_LINEMODE << LINEMODE_MODE << requestedMode << TELNET_COMMAND_IAC << TELNET_COMMAND_SE;
	return;
}

void TelnetServerSocket::init() {
	requestLineModeNegociation();
	setPeerEcho( false );
	setLocalEcho( true );
}

