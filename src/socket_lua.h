/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef SOCKET_LUA_H
#define SOCKET_LUA_H
	#ifdef NETWORKING_ENABLED

	#include <winsock2.h>
	#include "wininclude.h"
	#include "mutex.h"
	#include <vector>

	#define LISTEN_BUFFER		10

	typedef struct lua_State lua_State;

	namespace LuaType
	{
		extern const char *metatable_socket;
	}

	namespace MicroMacro
	{
		struct Socket;
	}

	class Socket_lua
	{
		protected:
			static int gc(lua_State *);
			static int tostring(lua_State *);

			static int connect(lua_State *);
			static int listen(lua_State *);
			static int send(lua_State *);
			static int recv(lua_State *);
			static int flushRecvQueue(lua_State *);
			static int getRecvQueueSize(lua_State *);
			static int close(lua_State *);

			static int id(lua_State *);


			static DWORD WINAPI socketThread(MicroMacro::Socket *);
			static DWORD WINAPI listenThread(MicroMacro::Socket *);

			static bool isIP(const char *);

		public:
			static int regmod(lua_State *);
			static int cleanup();

			static Mutex socketListLock;
			static std::vector<MicroMacro::Socket *> socketList;
	};

	typedef std::vector<MicroMacro::Socket *>::iterator SocketListIterator;
	#endif
#endif
