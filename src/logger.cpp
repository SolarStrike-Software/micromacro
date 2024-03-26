/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "logger.h"
#include "error.h"

#include <time.h>
#include <string.h>
#include <new>

CLogger *CLogger::pinstance = 0;
CLogger *CLogger::instance()
{
	try {
	if( pinstance == 0 )
		pinstance = new CLogger;
	}
	catch( std::bad_alloc &ba ) { badAllocation(); }

	return pinstance;
}

CLogger::CLogger()
{

}

CLogger::~CLogger()
{
	if( outfile.is_open() )
	{
		add("Logging finished. Cleaning up.");
		outfile.close();
	}
}

/* Opens the file for logging with given name.
   Returns zero on error, nonzero on success */
int CLogger::open(const char *filename)
{
	if( outfile.is_open() )
		outfile.close();

	openedFilename = "";
	outfile.open(filename, std::ios::out);
	if( outfile.is_open() )
	{
		time_t rawtime;
		struct tm * timeinfo;
		time( &rawtime );
		timeinfo = localtime ( &rawtime );
		char szTime[256];
		strftime(szTime, sizeof(szTime) - 1, "%Y-%m-%d %H:%M:%S", timeinfo);

		openedFilename = filename;
		outfile.flush();
		return true;
	}

	return false;
}

/* Close the currently open file */
void CLogger::close()
{
	if( outfile.is_open() )
		outfile.close();
}

/* If you guessed that his tells us if the log file is open or not...
	You guessed correctly.
*/
bool CLogger::is_open()
{
	return outfile.is_open();
}

/* Write formatted output to the file. add() prepends
   output with the date and time, and appends a trailing newline (\n) */
void CLogger::add(const char *fmt, ...)
{
	if( !outfile.is_open() )
		return;

	if( fmt == NULL )
		return;

	time_t rawtime;
	struct tm * timeinfo;
	time( &rawtime );
	timeinfo = localtime ( &rawtime );
	char szTime[256];
	strftime(szTime, sizeof(szTime)-1, "%Y-%m-%d %H:%M:%S", timeinfo);
	outfile << szTime << " : ";

	va_list va_alist;
	char logbuf[2048] = {0};
	va_start(va_alist, fmt);
	_vsnprintf(logbuf, sizeof(logbuf), fmt, va_alist);
	va_end(va_alist);

	outfile << logbuf;
	outfile.flush();
}

/* Write unformatted output to the file. add_raw() does not prepend
   output with the date and time, and does not append a trailing
   newline (\n) */
void CLogger::add_raw(const char *outstr)
{
	if( !outfile.is_open() )
		return;

	if( outstr == NULL )
		return;

	outfile << outstr;
	outfile.flush();
}

std::string CLogger::get_filename()
{
	return openedFilename;
}

bool CLogger::meetsLogLevel(LogLevel l) {
    if (l == LogLevel::emergency) {
        return true;
    }

    return l <= this->level;
}

void CLogger::setLevel(LogLevel l) {
    if( l < LogLevel::emergency ) {
        l = LogLevel::emergency;
	}

    this->level = l;
}

const char *CLogger::getLevelName(LogLevel level) {
    switch(level) {
        case LogLevel::emergency: return "EMERG";
        case LogLevel::alert: return "ALERT";
        case LogLevel::critical: return "CRIT";
        case LogLevel::error: return "ERROR";
        case LogLevel::warning: return "WARN";
        case LogLevel::notice: return "NOTICE";
        case LogLevel::info: return "INFO";
        case LogLevel::debug: return "DEBUG";
    }
}

void CLogger::emergency(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::emergency) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::emergency), msg);
}

void CLogger::alert(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::alert) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::alert), msg);
}

void CLogger::critical(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::critical) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::critical), msg);
}

void CLogger::error(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::error) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::error), msg);
}

void CLogger::warning(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::warning) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::warning), msg);
}


void CLogger::notice(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::notice) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::notice), msg);
}

void CLogger::info(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::info) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::info), msg);
}

void CLogger::debug(const char *msg) {
    if( !this->meetsLogLevel(LogLevel::debug) ) {
        return;
    }

    this->add("[%s] %s\n", this->getLevelName(LogLevel::debug), msg);
}
