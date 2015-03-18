/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "settings.h"


const char *CONFIG_FILENAME = 							"config.lua";
const char *CONFVAR_MEMORY_STRING_BUFFER_SIZE =			"memoryStringBufferSize";
const char *CONFVAR_LOG_DIRECTORY =						"logDirectory";
const char *CONFVAR_LOG_REMOVAL_DAYS =					"logRemovalDays";
const char *CONFVAR_SCRIPT_DIRECTORY =					"scriptDirectory";
const char *CONFVAR_AUDIO_ENABLED =						"audioEnabled";
const char *CONFVAR_YIELD_TIME_SLICE =					"yieldTimeSlice";
const char *CONFVAR_NETWORK_BUFFER_SIZE =				"networkBufferSize";
const char *CONFVAR_RECV_QUEUE_SIZE = 					"recvQueueSize";

const int CONFDEFAULT_MEMORY_STRING_BUFFER_SIZE =		128;
const int CONFDEFAULT_LOG_REMOVAL_DAYS =				7;
const char *CONFDEFAULT_LOG_DIRECTORY =					"logs";
const char *CONFDEFAULT_SCRIPT_DIRECTORY =				"scripts";
const int CONFDEFAULT_AUDIO_ENABLED =					1;
const int CONFDEFAULT_YIELD_TIME_SLICE =				1;
const int CONFDEFAULT_NETWORK_BUFFER_SIZE =				10240;
const int CONFDEFAULT_RECV_QUEUE_SIZE = 				100;

/* Setting value stuff */
CSettingValue::CSettingValue()
{
	iValue = 0;
	type = ST_NIL;
}

CSettingValue::CSettingValue(double d)
{
	setFloat(d);
}

CSettingValue::CSettingValue(int d)
{
	setInt(d);
}

CSettingValue::CSettingValue(std::string d)
{
	setString(d);
}

SettingType CSettingValue::getType()
{
	return type;
}

double CSettingValue::getFloat()
{
	return fValue;
}

void CSettingValue::setFloat(double d)
{
	fValue = d;
	type = ST_FLOAT;
}

int CSettingValue::getInt()
{
	return iValue;
}

void CSettingValue::setInt(int d)
{
	iValue = d;
	type = ST_INT;
}

std::string CSettingValue::getString()
{
	return szValue;
}

void CSettingValue::setString(std::string d)
{
	szValue = d;
	type = ST_INT;
}

void CSettingValue::setNil()
{
	szValue = ""; // Might as well get rid of unnecessary memory
	type = ST_NIL;
}


/* Our actual setting container */
CSettings::CSettings()
{
}

CSettings::~CSettings()
{
}

void CSettings::setFloat(std::string key, double nv)
{
	settingsmap[key].setFloat(nv);
}

void CSettings::setInt(std::string key, int nv)
{
	settingsmap[key].setInt(nv);
}

void CSettings::setString(std::string key, std::string nv)
{
	settingsmap[key].setString(nv);
}

double CSettings::getFloat(std::string key, double defaultValue)
{
	t_settingsmap::iterator foundpos = settingsmap.find(key);
	if( foundpos != settingsmap.end() )
		return foundpos->second.getFloat();

	return defaultValue;
}

int CSettings::getInt(std::string key, int defaultValue)
{
	t_settingsmap::iterator foundpos = settingsmap.find(key);
	if( foundpos != settingsmap.end() )
		return foundpos->second.getInt();

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
