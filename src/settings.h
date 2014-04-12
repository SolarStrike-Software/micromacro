#ifndef SETTINGS_H
#define SETTINGS_H

	#include "types.h"

	#include <string>
	#include <map>


	#define CONFIG_FILENAME								"config.lua"
	#define CONFVAR_MEMORY_STRING_BUFFER_SIZE			"memoryStringBufferSize"
	#define CONFVAR_LOG_DIRECTORY						"logDirectory"
	#define CONFVAR_LOG_REMOVAL_DAYS					"logRemovalDays"
	#define CONFVAR_SCRIPT_DIRECTORY					"scriptDirectory"

	#define CONFDEFAULT_MEMORY_STRING_BUFFER_SIZE		128
	#define CONFDEFAULT_LOG_REMOVAL_DAYS				7
	#define CONFDEFAULT_LOG_DIRECTORY					"logs"
	#define CONFDEFAULT_SCRIPT_DIRECTORY				"scripts"

	class CSettings;
	typedef CSettings Settings;
	typedef std::map<std::string, Multivar> t_settingsmap;

	class CSettings
	{
		protected:
			t_settingsmap settingsmap;

		public:
			CSettings();
			~CSettings();

			void setNumber(std::string, double);
			void setString(std::string, std::string);

			double getNumber(std::string, double = 0.0);
			std::string getString(std::string, std::string = "");

			void clear(std::string);
			void clearAll();
	};


#endif
