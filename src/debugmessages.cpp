/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include "debugmessages.h"

#ifdef DISPLAY_DEBUG_MESSAGES
#include "strl.h"
#include "logger.h"


#include <string.h>

void debugMessageReal(const char *file, int line, const char *fmt, ...)
{
    va_list va_alist;
    char logbuf[1024] = {0};
    va_start(va_alist, fmt);
    _vsnprintf(logbuf, sizeof(logbuf) - 1, fmt, va_alist);
    va_end(va_alist);

    char logbuf2[2048];
    slprintf(logbuf2, sizeof(logbuf2) - 1, "Debug: %s:%d: %s\n", file, line, logbuf);

    // Print to standard output
    fprintf(stdout, logbuf2);

    // Log to file
    Logger::instance()->add("%s", logbuf2);
}
#endif
