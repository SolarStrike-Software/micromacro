/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef SETTINGS_H
#define SETTINGS_H

	#include "types.h"

	#include <string>
	#include <map>


	extern const char *CONFIG_FILENAME;
	extern const char *CONFIG_DEFAULT_FILENAME;
	extern const char *CONFVAR_MEMORY_STRING_BUFFER_SIZE;
	extern const char *CONFVAR_LOG_DIRECTORY;
	extern const char *CONFVAR_LOG_REMOVAL_DAYS;
	extern const char *CONFVAR_SCRIPT_DIRECTORY;
	extern const char *CONFVAR_YIELD_TIME_SLICE;
	extern const char *CONFVAR_NETWORK_ENABLED;
	extern const char *CONFVAR_NETWORK_BUFFER_SIZE;
	extern const char *CONFVAR_RECV_QUEUE_SIZE;
	extern const char *CONFVAR_STYLE_ERRORS;
	extern const char *CONFVAR_FILE_STYLE;
	extern const char *CONFVAR_LINE_NUMBER_STYLE;
	extern const char *CONFVAR_MESSAGE_STYLE;

	extern const int CONFDEFAULT_MEMORY_STRING_BUFFER_SIZE;
	extern const int CONFDEFAULT_LOG_REMOVAL_DAYS;
	extern const char *CONFDEFAULT_LOG_DIRECTORY;
	extern const char *CONFDEFAULT_SCRIPT_DIRECTORY;
	extern const int CONFDEFAULT_YIELD_TIME_SLICE;
	extern const int CONFDEFAULT_NETWORK_ENABLED;
	extern const int CONFDEFAULT_NETWORK_BUFFER_SIZE;
	extern const int CONFDEFAULT_RECV_QUEUE_SIZE;
	extern const int CONFDEFAULT_STYLE_ERRORS;
	extern const char *CONFDEFAULT_FILE_STYLE;
	extern const char *CONFDEFAULT_LINE_NUMBER_STYLE;
	extern const char *CONFDEFAULT_MESSAGE_STYLE;

	class CSettings;
	class CSettingValue;
	typedef CSettings Settings;
	typedef std::map<std::string, CSettingValue> t_settingsmap;

	enum SettingType {ST_FLOAT, ST_INT, ST_STRING, ST_NIL};

	class CSettingValue
	{
		protected:
			SettingType type;
			union
			{
				double fValue;
				int iValue;
			};
			std::string szValue;

		public:
			CSettingValue();
			CSettingValue(double);
			CSettingValue(int);
			CSettingValue(std::string);
			SettingType getType();
			double getFloat();
			void setFloat(double);
			int getInt();
			void setInt(int);
			std::string getString();
			void setString(std::string);
			void setNil();
	};

	class CSettings
	{
		protected:
			t_settingsmap settingsmap;

		public:
			CSettings();
			~CSettings();

			void setFloat(std::string, double);
			void setInt(std::string, int);
			void setString(std::string, std::string);

			double getFloat(std::string, double = 0.0);
			int getInt(std::string, int = 0);
			std::string getString(std::string, std::string = "");

			void clear(std::string);
			void clearAll();
	};


#endif
