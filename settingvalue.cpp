#include "settingvalue.h"

#include <sstream>

using namespace std;

SettingValue::SettingValue() {}

SettingValue::SettingValue( string newName, string newValue ) {
	name = newName;
	value = newValue;
}

SettingValue::~SettingValue() {}

int SettingValue::asInt() {
	stringstream ss;
	ss << value;
	int retVal;
	ss >> retVal;
	return retVal;
}

string SettingValue::asString() {
	return value;
}
