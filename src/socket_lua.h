/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef SOCKET_LUA_H
#define SOCKET_LUA_H
	#ifdef NETWORKING_ENABLED

	#include "wininclude.h"
	#include <winsock2.h>
	#include <vector>

	#define LISTEN_BUFFER		10

	typedef struct lua_State lua_State;

	namespace LuaType
	{
		extern const char *metatable_socket;
	}

	class Socket;
	class Socket_lua
	{
		protected:
			static int gc(lua_State *);
			static int tostring(lua_State *);

			static int connect(lua_State *);
			static int listen(lua_State *);
			static int send(lua_State *);
			static int close(lua_State *);

			static int id(lua_State *);

			static HANDLE socketListLock;
			static std::vector<Socket *> socketList;
			static DWORD WINAPI socketThread(SOCKET);
			static DWORD WINAPI listenThread(SOCKET);

			static bool isIP(const char *);

		public:
			static int regmod(lua_State *);
			static int cleanup();
	};
	#endif
#endif
