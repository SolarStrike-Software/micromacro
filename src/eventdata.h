/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef EVENT_DATA_H
#define EVENT_DATA_H

	#include <cstddef>
	#include <string>
	#include <Winsock2.h>

	namespace MicroMacro
	{
		struct Socket;

		enum EventDataType
		{
			ED_NIL,
			ED_INTEGER,
			ED_64INTEGER,
			ED_NUMBER,
			ED_STRING,
			ED_SOCKET,
			ED_SOCKADDR,
		};

		class EventData
		{
			public:
				EventDataType	type;
				size_t			length;

				union {
					int iNumber;
					unsigned long long i64Number;
					double fNumber;
					struct sockaddr_in remoteAddr;
				};
				std::string str;

				MicroMacro::Socket *pSocket;

				EventData();
				~EventData();
				void setValue();						// NIL
				void setValue(int);						// Integer
				void setValue(unsigned long long);		// 64-bit Ints
				void setValue(double);					// Number
				void setValue(std::string, size_t = 0);	// String
				void setValue(char *, size_t);			// String
				void setValue(MicroMacro::Socket *);	// Socket
				void setValue(struct sockaddr_in *);	// SockAddr
				EventData &operator=(const EventData &);
		};
	}

#endif
