#include <iostream>
#include <vector>
#include <memory>
#include <sstream>
#include <pthread.h>
#include <cstdlib>
#include <csignal>
#include <unistd.h>
using namespace std;

#include "logger.h"
#include "settings.h"
#include "TelnetServerSocket.h"
#include "libsocket++/SocketException.h"

int startServer();
void sigHandler( int sigNum );
void* handleConnection( void* );
void incomingConnection( TelnetServerSocket* sock );
void shutdownThread();

//Make the following info global, so handleSigterm can shutdown stuff gracefully
vector<pthread_t> activeThreads;
auto_ptr<TelnetServerSocket> server;
pthread_mutex_t activeThreadsMutex = PTHREAD_MUTEX_INITIALIZER;

//main() calls the startServer func
int main( int argc, char* argv[] ) {
	if( argc > 1 ) {
		//Load the default settings
		try {
			//Load the settings from the specified file
			Settings::load( string(argv[1]) );
		} catch( string & s ) {
			cerr << "Could not read settings from /etc/faketelnetd.conf: " << endl;
			cerr << string(argv[1]) << endl;
			return 127;
		}
	} else {
		//Load the default settings
		try {
			Settings::load( "/etc/faketelnetd.conf" );
		} catch( string & s ) {
			cerr << "Could not read settings from /etc/faketelnetd.conf: " << endl;
			cerr << s << endl;
			return 127;
		}
	}
		
	//Make sure all the settings we need to be defined are there now as
	//	these calls will throw an exception if the values aren't defined
	try {
		Settings::getValue( "logfile" );
		Settings::getValue( "listen" );
		Settings::getValue( "fumsg" );
		Settings::getValue( "valid_user" );
		Settings::getValue( "valid_pass" );
		Settings::getValue( "max_login_attempts" );
		Settings::getValue( "max_thread_count" );
	} catch( string s ) {
		cerr << "Error loading settings from file /etc/faketelnetd.conf: " << endl
				<< "\t" << s << endl;
		return 127;
	}

	//Connect the SIGTERM handler
	signal( SIGTERM, sigHandler );
	signal( SIGINT, sigHandler );
	signal( SIGQUIT, sigHandler );
	signal( SIGABRT, sigHandler );

	return startServer(); //Start the server
}

int startServer() {
	try {
		//Init the logging mechanism
		if( Settings::getValue("debug",0).asInt() == 1 ) {
			Logger::init( Settings::getValue("logfile").asString(), Logger::Debug );
		} else {
			Logger::init( Settings::getValue("logfile").asString() );
		}
			
		//Create the socket, this will start listening
		int listenPort = Settings::getValue("listen").asInt();
		try {
			//Setup a new socket and assign it to the auto_ptr which will delete it at the end of this function
			server.reset( new TelnetServerSocket(listenPort) );
		} catch(...) {
			stringstream ss;
			ss << "Could not bind to port " << listenPort;
			throw ss.str();
		}
		//Log this message to stdout as well and then fork so we become daemonized
		Logger::info() << "bound to port " << listenPort << ", server started" << endl;
		if( Settings::getValue("interactive",0).asInt() == 0 ) {
			cout << "bound to port " << listenPort << ", server started" << endl;
			if( fork() != 0 ) {
				exit( 0 );
			}
		}
			
		//Start accepting connections on the socket
		int maxThreadCount = Settings::getValue("max_thread_count").asInt();
		activeThreads.reserve( maxThreadCount );
		while( true ) {
			//Block until the number of threads is below the minimum
			while( true ) {
				pthread_mutex_lock( &activeThreadsMutex );
				int numThreads = activeThreads.size();
				pthread_mutex_unlock( &activeThreadsMutex );
				
				if( numThreads < maxThreadCount ) {
					break;
				} else {
					Logger::debug() << "Maximum thread count " << maxThreadCount << " reached, blocking connecting till threads finish" << endl;
					sleep( 1 );
				}
			}
		
			//Accept the incoming connection
			incomingConnection( server->accept() );
		}
		
	//Catch any expceptions, try to log them then print them to stderr as likely these are errors trying to start
	} catch( std::exception & e ) {
		try {
			Logger::info() << "Fatal Error: " << e.what() << endl;
		} catch(...) {}
		cerr << "Fatal Error: " << e.what() << endl;
	} catch( std::string & s ) {
		try {
			Logger::info() << "Fatal Error: " << s << endl;
		} catch(...) {}
		cerr << "Fatal Error: " << s << endl;
	} catch(...) {
		try {
			Logger::info() << "Unhandled unknown exception in startServer()" << endl;
		} catch(...) {}
		cerr << "Unhandled unknown exception in startServer()" << endl;
	}
}

void incomingConnection( TelnetServerSocket* conn ) {
	try {
		//Log the incoming connection
		Logger::info() << "Incoming connection from " << conn->addressAsString() << endl;
		
		//Lock the activeThreadsMutex - we will be inserting a new thread id into the vector
		//	We lock the mutex before the tread is created as there is a possibility that
		//	the thread will try to remove itself from the vector should a connection
		//	problem occur. In this case the following sequence would happen:
		//	
		//	1. thread starts
		//	2. thread has connection problem and dies, attempting to remove itself from the vector
		//		but failing as it hasn't been inserted yet
		//	3. we insert the thread into the activeThreads vector assuming it is running
		//	4. various problems occur down the line as the threadId we just inserted into
		//		the vector never gets removed
		pthread_mutex_lock( &activeThreadsMutex );
		
		//Create the thread
		activeThreads.push_back( (pthread_t)(-1) );
		int retVal = pthread_create( &(activeThreads.back()), NULL, &handleConnection, (void*)conn );
		
		//Check the retval
		if( retVal == 0 ) {
			//Detach the thread, meaning it will free the resources immediately after it exits, rather than waiting for pthread_join
			pthread_detach( activeThreads.back() );
			
			Logger::debug() << "Started thread " << activeThreads.back() << " to handle connection" << endl;
		} else {
			//Delete the connection as it couldn't be processed
			activeThreads.erase( activeThreads.end() );
			delete conn;
			
			//Log the event
			Logger::info() << "Couldn't start thread to handle connection, pthread_create returned " << retVal << ", disconnecting socket" << endl;
		}
		
		//Unlock activeThreads
		pthread_mutex_unlock( &activeThreadsMutex );
	} catch( SocketException & e ) {
		Logger::info() << "Exception occurred in handleConnection:" << endl;
		Logger::info() << "\t" << e.description() << endl;
	} catch( std::exception & e ) {
		Logger::info() << "Exception occurred in handleConnection:" << endl;
		Logger::info() << "\t" << e.what() << endl;
	}
}

void* handleConnection( void* param ) {
	try {	
		//Setup an auto_ptr to delete the socket when this function ends
		auto_ptr<TelnetServerSocket> sock( (TelnetServerSocket*)param );
		
		//Setup some vars
		string fumsg = Settings::getValue("fumsg").asString();
		string remoteHost = sock->addressAsString();
		string validUser = Settings::getValue("valid_user").asString();
		string validPass = Settings::getValue("valid_pass").asString();
		string username;
		string password;
	
		sock->init();
		
		//Run the connect_exec as configured
		string connect_exec = Settings::getValue("connect_exec","").asString();
		if( !connect_exec.empty() ) {
			//Do string replacement on the parameter
			if( connect_exec.find("%ip") != string::npos ) {
				connect_exec.replace( connect_exec.find("%ip"), 3, remoteHost );
			}
			
			//Log and run the command
			Logger::info() << "Running connect_exec '" << connect_exec << "'" << endl;
			int exitCode = system( connect_exec.c_str() );
			Logger::info() << "connect_exec finished with exit code " << exitCode << endl;
		}
		
		//Get the username
		(*sock) << "Telnet server could not log you in using NTML authentication.\r\n"
			<< "Your password may have expired.\r\n"
			<< "Login using username and password\r\n"
			<< "\r\n"
			<< "Welcome to Microsoft Telnet Service\r\n"
			<< "\r\n";
		
		//Let the user try go "log in"
		int maxTries = Settings::getValue("max_login_attempts").asInt();
		bool loggedin = false;
		for( int tries = 0; tries <= maxTries; tries++ ) {
			(*sock) << "login: ";
			username = sock->getLine();
			Logger::debug() << "Received username " << username << endl;
			
			//Get the password
			(*sock) << "password: ";
			password = sock->getLine( true );
			(*sock) << "\r\n";
			
			//Check the username and password we received
			if( username == validUser && password == validPass ) {
				//Mark that we had a successful log
				loggedin = true;
				
				//Send a message to the log
				Logger::info() << "Successful login from " << sock->addressAsString() << " with credentials " << username << ":" << password << endl;
				
				//Run the successful login cmd as configured
				string login_exec = Settings::getValue("login_exec","").asString();
				if( !login_exec.empty() ) {
					//Do string replacement on the parameters
					if( login_exec.find("%ip") != string::npos ) {
						login_exec.replace( login_exec.find("%ip"), 3, remoteHost );
					}
			
					if( login_exec.find("%user") != string::npos ) {
						login_exec.replace( login_exec.find("%user"), 5, username );
					}
				
					if( login_exec.find("%pass") != string::npos ) {
						login_exec.replace( login_exec.find("%pass"), 5, password );
					}
			
					//Do some logging
					Logger::info() << "Running login_exec '" << login_exec << "'" << endl;
					int exitCode = system( login_exec.c_str() );
					Logger::info() << "login_exec finished with exit code " << exitCode << endl;
				}
				
				//Leave the loop since the login was successful
				break;
			} else {
				//Provide that delay that most systems do when a bad password was entered
				sleep( 1 );
				
				//Send back a message that the login attempt failed
				(*sock) << "\r\n";
				Logger::info() << "Failed login from " << sock->addressAsString() << " with credentials " << username << ":" << password << endl;
				
				//Run the login_fail_exec as configured
				string login_fail_exec = Settings::getValue("login_fail_exec","").asString();
				if( !login_fail_exec.empty() ) {
					//Do string replacement on the parameters
					if( login_fail_exec.find("%ip") != string::npos ) {
						login_fail_exec.replace( login_fail_exec.find("%ip"), 3, remoteHost );
					}
			
					if( login_fail_exec.find("%user") != string::npos ) {
						login_fail_exec.replace( login_fail_exec.find("%user"), 5, username );
					}
				
					if( login_fail_exec.find("%pass") != string::npos ) {
						login_fail_exec.replace( login_fail_exec.find("%pass"), 5, password );
					}
			
					//Do some logging
					Logger::info() << "Running login_fail_exec '" << login_fail_exec << "'" << endl;
					int exitCode = system( login_fail_exec.c_str() );
					Logger::info() << "login_fail_exec finished with exit code " << exitCode << endl;
				}
				
				//Go back to the start of the loop, to let the user try to login again
				continue;
			}
		}
		
		//If we didn't see a good login that the user hit max login attempts
		if( !loggedin ) {
			Logger::info() << "Disconnecting " << remoteHost << " after max login attempts of " << maxTries << endl;
			
			shutdownThread();
		}
		
		//Start accepting commands into a fake shell
		string cmd_exec = Settings::getValue("cmd_exec","").asString();
		while( true ) {
			//Print the fake command prompt
			(*sock) << "C:\\Documents and Settings\\" << username << ">";
			
			//Read the command line and log it
			string line = sock->getLine();
			Logger::info() << username << "@" << remoteHost << " entered command: " << line << endl;
			
			//Run the cmd_exec as configured
			if( !cmd_exec.empty() ) {
				//Do string replacement on the parameters
				if( cmd_exec.find("%ip") != string::npos ) {
					cmd_exec.replace( cmd_exec.find("%ip"), 3, remoteHost );
				}
			
				if( cmd_exec.find("%user") != string::npos ) {
					cmd_exec.replace( cmd_exec.find("%user"), 5, username );
				}
				
				if( cmd_exec.find("%pass") != string::npos ) {
					cmd_exec.replace( cmd_exec.find("%pass"), 5, password );
				}
				
				if( cmd_exec.find("%cmd") != string::npos ) {
					cmd_exec.replace( cmd_exec.find("%cmd"), 4, line );
				}
			
				//Do some logging and run the command
				Logger::info() << "Running cmd_exec '" << cmd_exec << "'" << endl;
				int exitCode = system( cmd_exec.c_str() );
				Logger::info() << "cmd_exec finished with exit code " << exitCode << endl;
			}
			
			//implement a fake 'dir' output
			if( line == "dir" ) {
				(*sock) << "08/11/2008   12:30 PM        <DIR>          .\r\n";
				(*sock) << "08/11/2008   12:30 PM        <DIR>          ..\r\n";
				(*sock) << "08/11/2008   12:30 PM        <DIR>          ..\r\n";
				(*sock) << "08/11/2008   12:30 PM        <DIR>          Start Menu\r\n";
				(*sock) << "08/11/2008   12:30 PM        <DIR>          My Documents\r\n";
				(*sock) << "08/11/2008   12:30 PM        <DIR>          Favorites\r\n";
				(*sock) << "08/11/2008   12:30 PM        <DIR>          Desktop\r\n";
				
			//let the user logout
			} else if( line == "exit" || line == "logout" || line == "quit" ) {
				break;
				
			//default to printing the 'fu' message
			} else {
				(*sock) << fumsg << "\r\n";
				break;
			}
		}
		
		//Log that the user has been disconnected
		Logger::info() << "Ending session from " << sock->addressAsString() << endl;
		shutdownThread();
	} catch( std::exception & e ) {
		Logger::info() << "handleConnection: " << e.what() << endl;
	} catch( string & e ) {
		Logger::info() << "handleConnection: " << e << endl;
	} catch( SocketException & e ) {
		Logger::info() << "handleConnection: " << e.description() << endl;
	}
	
	shutdownThread();
}

void sigHandler( int sigNum ) {
	//Log a message that we caught a signal
	try {
		Logger::info() << "Caught signal " << sigNum << ", shutting down" << endl;
	} catch(...) {
		cerr << "Caught signal " << sigNum << ", shutting down" << endl;
	}
	
	//Kill any sub-threads we have going right now
	for( int i = 0; i < activeThreads.size(); i++ ) {
		if( pthread_cancel(activeThreads[i]) != 0 ) {
			cerr << "Could not pthread_cancel thread " << activeThreads[i] << endl;
		}
	}
	
	//Delete the primary listening socket
	server.release();
	
	//Shutdown the logging mechanism
	Logger::shutdown();
	
	//Exit the program
	exit( 0 );
}

void shutdownThread() {
	//Lock activeThreads
	pthread_mutex_lock( &activeThreadsMutex );
	
	//Find the specified thread and remove it from the list
	for( int i = 0; i <= activeThreads.size(); i++ ) {
		if( activeThreads[i] == pthread_self() ) {
			Logger::debug() << "Removing thread " << pthread_self() << " from active thread list" << endl;
			activeThreads.erase( activeThreads.begin()+i );
		}
	}
	
	//Unlock activeThreads
	pthread_mutex_unlock( &activeThreadsMutex );
	
	//Exit the thread
	pthread_exit( NULL );
}
