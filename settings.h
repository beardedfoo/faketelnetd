#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <map>
#include <string>
#include "settingvalue.h"

using namespace std;

typedef map<string,SettingValue> SettingValueMap;

class Settings {
  public:
	static void load( string filename );
	static SettingValue getValue( string name );
	static SettingValue getValue( string name, string defValue );
	static SettingValue getValue( string name, int defValue );
	
	static SettingValueMap getAllValues();
  protected:
	static SettingValueMap values;
};

#endif
