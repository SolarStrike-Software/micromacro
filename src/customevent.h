/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef CUSTOM_EVENT_H
#define CUSTOM_EVENT_H

	#include <cstddef>

	namespace MicroMacro
	{
		class CustomEventData
		{
			public:
				int		type;
				size_t	length;

				union {
					double number;
					char *str;
				};

				CustomEventData();
				~CustomEventData();
				void setValue(double);
				void setValue(char *, size_t);
				CustomEventData &operator=(const CustomEventData &);
		};
	}

#endif
