/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef LOGGER_H
#define LOGGER_H

	#include <fstream>
	#include <string>
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdarg.h>

    // As per RFC-5424      https://datatracker.ietf.org/doc/html/rfc5424
	enum LogLevel {
	    emergency = 0,
	    alert,
	    critical,
	    error,
	    warning,
	    notice,
	    info,
	    debug,
	};

	class CLogger;
	typedef CLogger Logger;

	class CLogger
	{
		private:
			static CLogger *pinstance;
			std::ofstream outfile;
			std::string openedFilename;
			LogLevel level = LogLevel::info;

			bool meetsLogLevel(LogLevel);
			const char *getLevelName(LogLevel);

		protected:
			CLogger();
			~CLogger();
			CLogger(const CLogger &);
			CLogger &operator=(const CLogger &);

		public:
			static CLogger *instance();
			int open(const char *);
			void close();
			bool is_open();
			void add(const char *, ...);
			void add_raw(const char *);
			void emergency(const char *);
			void alert(const char *);
			void critical(const char *);
			void error(const char *);
			void warning(const char *);
			void notice(const char *);
			void info(const char *);
			void debug(const char *);
			std::string get_filename();

			void setLevel(LogLevel);
	};

#endif
