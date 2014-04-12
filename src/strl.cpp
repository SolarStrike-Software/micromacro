/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "strl.h"
#include <string.h>
#include <cstdarg>
#include <stdio.h>

// Just like strncpy, except ensures we always NULL-terminate
size_t strlcpy(char *dest, const char* src, size_t max_len)
{
	size_t src_len = strlen(src);
	size_t cpy_len = 0;

	if( src_len > max_len )
		cpy_len = max_len;
	else
		cpy_len = src_len;

	strncpy(dest, src, cpy_len);
	dest[cpy_len] = 0; // Ensure NULL-terminator

	return strlen(dest);
}

// Just like strncat, except ensures we always NULL-terminate
size_t strlcat(char *dest, const char *src, size_t max_len)
{
	size_t start_len = strlen(dest);
	size_t src_len = strlen(src);
	size_t cpy_len = max_len - src_len;

	if( cpy_len > max_len )
		cpy_len = max_len;
	else
		cpy_len = src_len;

	strncat(dest, src, cpy_len);
	dest[start_len + cpy_len] = 0; // Ensure NULL-terminator

	return strlen(dest) - start_len;
}

// I think you get the pattern by now. Like snprintf(), ensures NULL-terminator
int slprintf(char *dest, size_t size, const char *fmt, ...)
{
	// Forward to "normal" snprintf
	int ret;
	va_list args;
	va_start(args, fmt);
	ret = vsnprintf(dest, size, fmt, args);
	va_end(args);

	// Ensure NULL terminator
	dest[size] = 0;
	return ret;
}

// Converts 'src' to lowercase, stores in 'dest'.
void sztolower(char *dest, const char *src, size_t max_len)
{
	for(size_t i = 0; i <= max_len; i++)
	{
		char c = src[i];
		if( c >= 'A' && c <= 'Z' )
			c = c + 32; // A-a has a 32 char difference
		dest[i] = c;
		if( c == 0 )
			break;
	}
}

// Find with wildcards * and ?
int wildfind(const std::string &format, const std::string &checkstring)
{
	if( checkstring.length() == 0 || format.length() == 0 )
		return 0;

	unsigned int format_pos = 0;
	unsigned int checkstring_pos = 0;
	unsigned int mp = 0;
	unsigned int cp = 0;

	while( format.at(format_pos) != '*' && format.at(format_pos) != '?' )
	{
		if( format.at(format_pos) != checkstring.at(checkstring_pos) &&
		format.at(format_pos) != '?' && format.at(format_pos) != '*' )
			return 0;

		checkstring_pos++;
		format_pos++;

		if( checkstring_pos >= checkstring.length() || format_pos >= format.length() )
			break;
	}

	while( format_pos < format.length() && checkstring_pos < checkstring.length() )
	{
		if( format.at(format_pos) == '*' )
		{
			if( (format_pos < format.length()-1 && format.at(format_pos+1) != '*') ||
			format_pos == format.length()-1 )
			{
				format_pos++;
				if( format_pos >= format.length() )
				return 1;

				mp = format_pos;
				cp = checkstring_pos + 1;
			}
			else if( format.at(format_pos) == checkstring.at(checkstring_pos) )
			{
				format_pos++;
				checkstring_pos++;
			}
			else
			{
				format_pos = mp;
				checkstring_pos = cp++;
			}
		}
		else if( format.at(format_pos) == '?' )
		{
			if( format_pos < format.length()-1 && format.at(format_pos+1) == '?' )
			{
				if( format.at(format_pos) == checkstring.at(checkstring_pos) )
				{
					format_pos+=2;
					checkstring_pos++;
				}
				else
				{
					format_pos = mp;
					checkstring_pos = cp++;
				}
			}
			else
			{
				format_pos++;
				checkstring_pos++;
			}
		}
		else if( format.at(format_pos) == checkstring.at(checkstring_pos) )
		{
			format_pos++;
			checkstring_pos++;
		}
		else
		{
			format_pos = mp;
			checkstring_pos = cp++;
		}
	}

	while( format_pos < format.length() && format.at(format_pos) == '*' )
		format_pos++;

	if( format_pos >= format.length() )
		return true;
	else
		return false;
}
