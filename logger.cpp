#include "logger.h"

#include <fstream>
#include <string>
using namespace std;

bool Logger::hasInited = false;
ofstream Logger::logFile;
Logger::LogLevel Logger::logLevel;
ofstream Logger::blackhole;

void Logger::init( string filename, LogLevel setLevel ) {
	if( hasInited ) {
		return ;
	}
	
	//Set the log level
	logLevel = setLevel;
	
	//
	logFile.open( filename.c_str(), ofstream::app | ofstream::out );
	if( !logFile.is_open() || !logFile.good() ) {
		throw string("Could not open logfile ") + filename;
	}
	
	hasInited = true;
}

void Logger::shutdown() {
	if( hasInited ) {
		logFile.close();
	}
}

ostream& Logger::info() {
	if( !hasInited ) {
		throw string("Please run Logger::init()");
	}
	
	logFile << "INFO: ";
	ostream& retVal = logFile;
	return retVal;
}

ostream& Logger::debug() {
	if( !hasInited ) {
		throw string("Please run Logger::init()");
	}
	
	if( logLevel == Info ) {
		ostream& retVal = blackhole;
		return blackhole;
	}
	
	logFile << "DEBUG: ";
	ostream& retVal = logFile;
	return retVal;
}

