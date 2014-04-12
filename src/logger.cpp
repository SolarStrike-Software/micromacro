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
	outfile << (char*)&szTime << " : ";

	va_list va_alist;
	char logbuf[1024] = {0};
	va_start(va_alist, fmt);
	_vsnprintf(logbuf + strlen(logbuf), sizeof(logbuf) - strlen(logbuf),
	fmt, va_alist);
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
