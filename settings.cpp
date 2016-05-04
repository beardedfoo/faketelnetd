#include "settings.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

SettingValueMap Settings::values;

void Settings::load( string filename ) {
	ifstream file;
	file.open( filename.c_str(), ifstream::in );
	if( !file.good() ) {
		throw string("Cannot read setting from ")+filename;
	}
	
	for( int linenum = 1; !file.eof(); linenum++ ) {
		string line;
		getline( file, line );
		
		if( line[0] == '#') {
			continue;
		}
		
		if( line.size() == 0 ) {
			continue;
		}
		
		size_t pos = line.find_first_of( '=' );
		if( pos == string::npos ) {
			stringstream ss;
			ss << "Syntax error reading settings file " << filename << " at line " << linenum;
			throw ss.str();
		}
		
		string name = line.substr( 0, pos );
		string value = line.substr( pos+1 );
		Settings::values[name] = SettingValue( name, value );
	}
	
	return;
}

SettingValueMap Settings::getAllValues() {
	return values;
}

SettingValue Settings::getValue( string name ) {
	return getValue( name, "__THROW_EXCEPTION__" );
}

SettingValue Settings::getValue( string name, string defValue ) {
	SettingValueMap::iterator it = values.find( name );
	if( it != values.end() ) {
		return it->second;
	} else {
		if( defValue == "__THROW_EXCEPTION__" ) {
			throw string("Setting ")+name+string(" is undefined.");
		}
		return SettingValue( name, defValue );
	}
}

SettingValue Settings::getValue( string name, int defValue ) {
	stringstream ss;
	ss << defValue;
	return getValue( name, ss.str() );
}
