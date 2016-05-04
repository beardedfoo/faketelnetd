#ifndef __LOGGER_H
#define __LOGGER_H

#include <string>
#include <fstream>
using namespace std;

class Logger {
    public:
	enum LogLevel { Info, Debug };
	static void init( string filename, LogLevel setLevel=Info );
	static void shutdown();
	static ostream& info();
	static ostream& debug();

    protected:
	static bool hasInited;
	static LogLevel logLevel;
	static ofstream logFile;
	static ofstream blackhole;

    public:
};

#endif
