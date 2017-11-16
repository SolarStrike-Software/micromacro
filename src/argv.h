/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef ARGV_H
#define ARGV_H

	// This aids in providing a compatibility when not using standard C++ main
	#include <vector>

	class Argv
	{
		protected:
			std::vector<char *> cmdArgs;

		public:
			Argv();
			Argv(char *);
			~Argv();
			void add(char *);
			void parse(char *);
			unsigned int size();
			void flush();
			const char *operator[](unsigned int);
	};

#endif
