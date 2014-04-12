#include "settings.h"

CSettings::CSettings()
{

}

CSettings::~CSettings()
{

}

void CSettings::setNumber(std::string key, double nv)
{
	settingsmap[key].setNumber(nv);
}

void CSettings::setString(std::string key, std::string nv)
{
	settingsmap[key].setString(nv);
}

double CSettings::getNumber(std::string key, double defaultValue)
{
	t_settingsmap::iterator foundpos = settingsmap.find(key);
	if( foundpos != settingsmap.end() )
		return foundpos->second.getNumber();

	return defaultValue;
}

std::string CSettings::getString(std::string key, std::string defaultValue)
{
	t_settingsmap::iterator foundpos = settingsmap.find(key);
	if( foundpos != settingsmap.end() )
		return foundpos->second.getString();

	return defaultValue;
}

void CSettings::clear(std::string key)
{
	t_settingsmap::iterator foundpos = settingsmap.find(key);
	if( foundpos != settingsmap.end() )
		settingsmap.erase(foundpos);
}

void CSettings::clearAll()
{
	settingsmap.clear();
}
