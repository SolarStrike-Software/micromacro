/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

	#include <string>
	#include <vector>

	#define SLASHES_TO_STANDARD		1
	#define SLASHES_TO_WINDOWS		2

	typedef struct _FILETIME FILETIME;

	bool directoryExists(const char *);
	bool fileExists(const char *);
	bool copyFile(const char *, const char *);
	std::vector<std::string> getDirectory(std::string, std::string = "");
	unsigned int filetimeDelta(FILETIME *, FILETIME *);
	std::string fixSlashes(std::string, int);
	std::string getFileName(std::string);
	std::string getFilePath(std::string, bool);
	std::string fixFileRelatives(std::string);

#endif
