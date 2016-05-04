#ifndef __SETTINGVALUE_H
#define __SETTINGVALUE_CPP

#include <string>

using namespace std;

class SettingValue {
  public:
	SettingValue();
	SettingValue( string newName, string newValue );
	~SettingValue();
	
	int asInt();
	string asString();

  protected:
	string name;
	string value;
};

#endif
