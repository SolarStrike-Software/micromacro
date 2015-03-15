/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef EVENT_H
#define EVENT_H

	#include <string>

	#include "types.h"

	enum EventType
	{
		EVENT_UNKNOWN,
		EVENT_ERROR,
		EVENT_WARNING,
		EVENT_KEYPRESSED,
		EVENT_KEYRELEASED,
		EVENT_MOUSEPRESSED,
		EVENT_MOUSERELEASED,
		EVENT_GAMEPADPRESSED,
		EVENT_GAMEPADRELEASED,
		EVENT_GAMEPADPOVCHANGED,
		EVENT_GAMEPADAXISCHANGED,
		EVENT_FOCUSCHANGED,
		EVENT_CONSOLERESIZED,
		EVENT_SOCKETCONNECTED,
		EVENT_SOCKETDISCONNECTED,
		EVENT_SOCKETRECEIVED,
		EVENT_SOCKETERROR,
		EVENT_QUIT
	};

	class Event
	{
		protected:
		public:
			enum EventType type;
			std::string msg;
			//char *msg;
			union {
				size_t idata1;
				double fdata1;
				Socket *pSocket;
			};
			union {
				size_t idata2;
				double fdata2;
			};
			union {
				size_t idata3;
				double fdata3;
			};

			Event() : type(EVENT_UNKNOWN), msg(""), idata1(0), idata2(0), idata3(0) { };
			Event &operator=(const Event &);
	};

#endif
