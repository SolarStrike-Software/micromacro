/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef EVENT_H
#define EVENT_H

	#include <string>
	#include <vector>
	#include "customEvent.h"

	// How many pieces of data can be passed to custom events
	#define CUSTOM_EVENT_DATA_SLOTS			16

	namespace MicroMacro
	{
		struct Socket;

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
			EVENT_QUIT,
			EVENT_CUSTOM,
		};

		class Event
		{
			protected:
			public:
				enum MicroMacro::EventType type;
				std::string msg;

				union {
					size_t idata1;
					double fdata1;
					MicroMacro::Socket *pSocket;
				};
				union {
					size_t idata2;
					double fdata2;
				};
				union {
					size_t idata3;
					double fdata3;
				};


				std::vector<CustomEventData> customEventDatas;
				Event() : type(EVENT_UNKNOWN), msg(""), idata1(0), idata2(0), idata3(0) { };
				Event &operator=(const Event &);
		};
	}

#endif
