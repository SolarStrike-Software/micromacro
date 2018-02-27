/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifdef NETWORKING_ENABLED

#include "network_lua.h"
#include "error.h"
#include "macro.h"
#include "strl.h"
#include "types.h"
#include "logger.h"
#include "socket_lua.h"
#include "settings.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}


using MicroMacro::Socket;

int Network_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"socket", Network_lua::socket},
		{NULL, NULL}
	};

	if( Macro::instance()->getSettings()->getInt(CONFVAR_NETWORK_ENABLED, CONFDEFAULT_NETWORK_ENABLED) )
	{
		luaL_newlib(L, _funcs);
		lua_setglobal(L, NETWORK_MODULE_NAME);
	}

	return MicroMacro::ERR_OK;
}

int Network_lua::cleanup()
{
	// Make sure we shut down all sockets & threads first
	Socket_lua::cleanup();

	return MicroMacro::ERR_OK;
}

int Network_lua::socket(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 0 && top != 1 )
		wrongArgs(L);
	if( top >= 1 )
		checkType(L, LT_STRING, 1);

	int streamtype = SOCK_STREAM;
	int protocol = IPPROTO_TCP;
	if( top >= 1 )
	{
		std::string protoStr = lua_tostring(L, 1);
		if( protoStr.compare("tcp") == 0 )
		{
			protocol = IPPROTO_TCP;
			streamtype = SOCK_STREAM;
		}
		else if( protoStr.compare("udp") == 0 )
		{
			protocol = IPPROTO_UDP;
			streamtype = SOCK_DGRAM;
		}
		else
		{
			char errbuff[1024];
			slprintf(errbuff, sizeof(errbuff)-1, "Expected \'tcp\' or \'udp\'; got \'%s\'", protoStr.c_str());
			return luaL_argerror(L, 1, errbuff);
		}
	}

	Socket *pSocket		=	new Socket;
	pSocket->inLua		=	true;
	Socket **ppSocket	=	static_cast<Socket **>(lua_newuserdata(L, sizeof(Socket **)));
	*ppSocket = pSocket;

	pSocket->socket		=	::socket(AF_INET, streamtype, protocol);
	pSocket->protocol	=	protocol;
	pSocket->deleteMe	=	false;


	if( pSocket->socket == INVALID_SOCKET )
	{
		lua_pop(L, 1); // Pop the garbage socket off since it failed.
		lua_pushboolean(L, false);
		char errbuff[2048];
		slprintf(errbuff, sizeof(errbuff), "Failed to create socket. Err code: %d\n", WSAGetLastError());
		lua_pushstring(L, errbuff);
		return 2;
	}

	pSocket->connected = false;
	pSocket->open = false;

	// It was created, so give it a metatable
	luaL_getmetatable(L, LuaType::metatable_socket);
	lua_setmetatable(L, -2);

	return 1;
}

#endif
