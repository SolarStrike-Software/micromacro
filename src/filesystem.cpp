/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "filesystem.h"
#include "strl.h"
#include "wininclude.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>


bool directoryExists(const char *path)
{
	DWORD dwAttrib = GetFileAttributes(path);

	if( (dwAttrib&FILE_ATTRIBUTE_DIRECTORY) && (dwAttrib!=INVALID_FILE_ATTRIBUTES) )
		return true;
	else
		return false;
}

bool fileExists(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if( file )
	{
		fclose(file);
		return true;
	}
	else
		return false;
}

std::vector<std::string> getDirectory(std::string path, std::string extension)
{
	std::vector<std::string> files;
	bool checkExt = (extension.size() > 0);
	DIR *dir = opendir(path.c_str());

	if( dir == NULL ) // If we can't open the directory,
		return files; // return early.

	dirent *dirp;
	char ext[128];
	while( (dirp = readdir(dir)) != NULL )
	{
		if( checkExt ) // Only check the extension if one is given
		{
			char *foundpos = strrchr(dirp->d_name, '.');
			if( strlen(foundpos) > (sizeof(ext) - 1) )
			{ // Make sure the "extension" isn't ridiculous and overflows our buffer
				continue;
			}

			if( foundpos != NULL )
				strlcpy((char*)&ext, foundpos+1, sizeof(ext) - 1);
			else
				strlcpy((char*)&ext, "", sizeof(ext) - 1);
		}

		if( strcmp(dirp->d_name, ".") != 0 &&
			strcmp(dirp->d_name, "..") != 0 )
		{
			if( !checkExt || extension.compare(ext) == 0 )
				files.push_back(std::string(dirp->d_name));
		}
	}
	closedir(dir);

	return files;
}

unsigned int filetimeDelta(FILETIME *t2, FILETIME *t1)
{
	LARGE_INTEGER tt_f;
	tt_f.LowPart = t2->dwLowDateTime - t1->dwLowDateTime;
	tt_f.HighPart = t2->dwHighDateTime - t1->dwHighDateTime;

	// Because Windows' FILETIME goes by 100ns increments,
	// convert it to seconds.
	return tt_f.QuadPart / 10000000;
}

std::string fixSlashes(std::string instr, int type)
{
	size_t i = SLASHES_TO_STANDARD;

	if( type == SLASHES_TO_STANDARD )
		i = instr.find("\\");
	else if( type == SLASHES_TO_WINDOWS )
		i = instr.find("/");

	while( i != std::string::npos )
	{
		if( type == SLASHES_TO_STANDARD ) {
			instr.replace(i, 1, "/");
			i = instr.find("\\", i+1); }
		else if( type == SLASHES_TO_WINDOWS ) {
			instr.replace(i, 1, "\\");
			i = instr.find("/", i+1); }
		else // Invalid conversion
			break;
	}

	return instr;
}

std::string getFileName(std::string fullpath)
{
	fullpath = fixSlashes(fullpath, SLASHES_TO_STANDARD);
	fullpath = fullpath.substr(fullpath.rfind("/")+1);
	return fullpath;
}

std::string getFilePath(std::string fullpath, bool includeTrailingSlash)
{
	//fullpath = fixFileRelatives(fullpath); // Not necessary... call it manually if needed
	fullpath = fixSlashes(fullpath, SLASHES_TO_STANDARD);

	size_t foundpos = fullpath.rfind("/");

	if( foundpos != std::string::npos )
		fullpath = fullpath.substr(0, fullpath.rfind("/"));
	else
		fullpath = ""; // resides in "this" directory

	if( includeTrailingSlash )
		fullpath += "/";

	return fullpath;
}

std::string fixFileRelatives(std::string instr)
{
	instr = fixSlashes(instr, SLASHES_TO_STANDARD);

	/* Keep any leading ../ and ./ */
	std::string prefix;
	for(size_t i = 0; i < instr.length(); i++)
	{
		char c = instr.at(i);
		if( c == '.' || c == '/' )
		{
			prefix.append(1, c);
		}
		else
			break;
	}
	if( prefix.length() )
		instr.erase(0, prefix.length());

	/* Strip out any "../", and their previous directory (if sane) */
	size_t foundpos = instr.find("../");
	while( foundpos != std::string::npos )
	{
		size_t prevslash = instr.substr(0, foundpos - 1).rfind("/");

		if( prevslash != std::string::npos )
			instr.erase(prevslash+1, foundpos - prevslash + 2);
			// +2 because of length of "../" - length of "/"
		else
		{
			instr.erase(foundpos - 1, 3); // 3 = length of "../"
		}

		foundpos = instr.find("../");
	}

	// now strip out any "./"
	foundpos = instr.find("./");
	while( foundpos != std::string::npos)
	{
		instr.erase(foundpos, 2); // 2, length of "./"

		foundpos = instr.find("./");
	}

	return prefix + instr;
}

