/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifdef NETWORKING_ENABLED

#include "socket_lua.h"
#include "error.h"
#include "macro.h"
#include "types.h"
#include "strl.h"
#include "logger.h"
#include "debugmessages.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

#include <ws2tcpip.h>
#include "macro.h"
#include "settings.h"

#define DEFAULT_LOCK_TIMEOUT		1000

using MicroMacro::Socket;
using MicroMacro::Event;
using MicroMacro::Mutex;

const char *LuaType::metatable_socket = "socket";

Mutex Socket_lua::socketListLock;
std::vector<Socket *> Socket_lua::socketList;

DWORD WINAPI Socket_lua::socketThread(Socket *pSocket)
{
	size_t buffSize = Macro::instance()->getSettings()->getInt(CONFVAR_NETWORK_BUFFER_SIZE);
	char *readBuff = new char[buffSize+1];

	size_t maxRecvQueueSize = Macro::instance()->getSettings()->getInt(CONFVAR_RECV_QUEUE_SIZE);
	while(true)
	{
		int result = ::recv(pSocket->socket, readBuff, buffSize, 0);

		if( result > 0 )
		{ // Data received
			// Copy the received data into an event; push it.
			std::string msg = std::string(readBuff, result);
			readBuff[result] = 0; // Enforce NULL-terminator
			Event e;
			e.idata1 = (int)pSocket->socket;
			e.type = MicroMacro::EVENT_SOCKETRECEIVED;
			e.msg = msg;

			if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
			{
				while( pSocket->recvQueue.size() > (maxRecvQueueSize + 1) )
					pSocket->recvQueue.pop();

				pSocket->eventQueue.push(e);
				pSocket->recvQueue.push(msg);
				pSocket->mutex.unlock(__FUNCTION__);
			}
		}
		else if( result == 0 )
		{ // Connection closed (probably by remote)
			Event e;
			e.idata1 = (int)pSocket->socket;
			e.type = MicroMacro::EVENT_SOCKETDISCONNECTED;

			if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
			{
				pSocket->eventQueue.push(e);
				pSocket->mutex.unlock(__FUNCTION__);
			}
			break;
		}
		else
		{ // Error occurred
			if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
			{
				int errCode = WSAGetLastError();

				switch(errCode)
				{
					case WSAENOTSOCK: // "Not a socket"; we closed the socket in the main thread, so this signals we should shut down this thread
					case WSAECONNRESET:
					{
						Event e;
						e.idata1 = (int)pSocket->socket;
						e.type = MicroMacro::EVENT_SOCKETDISCONNECTED;

						if( pSocket->open )
						{
							pSocket->eventQueue.push(e);

						}
					}
					break;

					case WSAECONNABORTED:	// Software caused connection abort. (your machine); we should close the socket
					{
						Event e;
						e.idata1 = (int)pSocket->socket;
						e.idata2 = errCode;
						e.type = MicroMacro::EVENT_SOCKETERROR;

						if( pSocket->open )
						{
							pSocket->eventQueue.push(e);
						}
					}
					break;

					default:
					#ifdef DISPLAY_DEBUG_MESSAGES
						fprintf(stderr, "Socket error occurred. Code: %d, socket: 0x%p\n", errCode, pSocket->socket);
					#endif
					{
						Event e;
						e.idata1 = (int)pSocket->socket;
						e.idata2 = errCode;
						e.type = MicroMacro::EVENT_SOCKETERROR;
						if( pSocket->open )
						{
							pSocket->eventQueue.push(e);
						}
					}
					break;
				}

				//printf("Set delete flag for 0x%x (%d)\n", pSocket, pSocket->socket);
				closesocket(pSocket->socket);
				pSocket->open		=	false;
				//pSocket->hThread	=	NULL;
				pSocket->connected	=	false;
				pSocket->socket		=	INVALID_SOCKET;
				pSocket->mutex.unlock(__FUNCTION__);
				break; // Break from while
			} // End of: ( pSocket->mutex.lock() )
		} // End of: else
	} // End of while(true)

	delete []readBuff;

	if( pSocket->mutex.lock(INFINITE, __FUNCTION__) )
	{
		if( pSocket->socket != INVALID_SOCKET )
			closesocket(pSocket->socket);
		pSocket->socket		=	INVALID_SOCKET;
		pSocket->hThread	=	NULL;
		//pSocket->deleteMe	=	true;
		pSocket->mutex.unlock(__FUNCTION__);
	}

	return 0;
}

DWORD WINAPI Socket_lua::listenThread(Socket *pSocket)
{
	// Set the socket to listen mode
	::listen(pSocket->socket, LISTEN_BUFFER);

	SOCKET new_socket;
	struct sockaddr_in client;

	while(true)
	{
		int addrlen = sizeof(struct sockaddr_in);
		new_socket = accept(pSocket->socket, (struct sockaddr *)&client, &addrlen);

		if( new_socket == INVALID_SOCKET )
		{
			int errCode = WSAGetLastError();
			pSocket->connected = false;
			switch(errCode)
			{
				case WSAENOTSOCK: // "Not a socket"; we closed the socket in the main thread, so this signals we should shut down this thread
				case WSAECONNRESET: // CONNRESET; remote side closed the connection forcibly
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.type = MicroMacro::EVENT_SOCKETDISCONNECTED;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
					{
						pSocket->eventQueue.push(e);
						pSocket->mutex.unlock(__FUNCTION__);
					}
				}
				break;

				case WSAECONNABORTED:	// Software caused connection abort. (your machine); we should close the socket
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.idata2 = errCode;
					e.type = MicroMacro::EVENT_SOCKETERROR;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
					{
						pSocket->eventQueue.push(e);
						closesocket(pSocket->socket);
						pSocket->socket		=	INVALID_SOCKET;
						pSocket->connected	=	true;
						pSocket->mutex.unlock(__FUNCTION__);
					}
				}
				break;

				case WSAEINTR: // Interrupted blocking function call (probably terminated a listen thread on script end)
				break;

				default:
				#ifdef DISPLAY_DEBUG_MESSAGES
					fprintf(stderr, "Socket error occurred. Code: %d, listen socket: 0x%p\n", errCode, pSocket->socket);
				#endif
				{
					Event e;
					e.idata1 = (int)pSocket->socket;
					e.idata2 = errCode;
					e.type = MicroMacro::EVENT_SOCKETERROR;

					if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
					{
						pSocket->eventQueue.push(e);
						pSocket->mutex.unlock(__FUNCTION__);
					}
				}
				break;
			}

			pSocket->socket		=	INVALID_SOCKET;
			pSocket->open		=	false;
			pSocket->hThread	=	NULL;
			pSocket->connected	=	false;

			break; // Break while(true)
		}
		else
		{ // Successfully accepted new client
			Socket *npSocket = new Socket;

			// Push the event
			Event e;
			e.type		=	MicroMacro::EVENT_SOCKETCONNECTED;
			e.pSocket	=	npSocket;
			e.idata2	=	pSocket->socket;
			npSocket->eventQueue.push(e);

			/* Now we can record some more info and start the new thread
				Note: We don't need to mutex this because it cannot be accessed
				until after we create the thread.
			*/
			npSocket->socket	=	new_socket;
			npSocket->protocol	=	AF_INET;
			npSocket->connected	=	true;
			npSocket->open		=	true;
			npSocket->deleteMe	=	false;

			npSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socketThread, (PVOID)npSocket, 0, NULL);

			if( socketListLock.lock(INFINITE, __FUNCTION__) )
			{
				socketList.push_back(npSocket);
				socketListLock.unlock(__FUNCTION__);
			}
		}
	} // End of: while(true)

	return 0;
}

int Socket_lua::regmod(lua_State *L)
{
	const luaL_Reg meta[] = {
		{"__gc", gc},
		{"__tostring", tostring},
		{NULL, NULL}
	};

	const luaL_Reg methods[] = {
		{"connect", connect},
		{"listen", listen},
		{"send", send},
		{"recv", recv},
		{"flushRecvQueue", flushRecvQueue},
		{"getRecvQueueSize", getRecvQueueSize},
		{"close", close},
		{"id", id},
		{"ip", ip},
		{"remoteIp", remoteIp},
		{NULL, NULL}
	};

	luaL_newmetatable(L, LuaType::metatable_socket);
	luaL_setfuncs(L, meta, 0);
	luaL_newlib(L, methods);
	lua_setfield(L, -2, "__index");

	lua_pop(L, 1); // Pop table

	return MicroMacro::ERR_OK;
}

int Socket_lua::cleanup()
{
	if( socketListLock.lock(INFINITE, __FUNCTION__) )
	{
		// Step 1: Close the sockets; give the threads time to terminate; Do *not* delete yet!
		for(SocketListIterator i = socketList.begin(); i != socketList.end(); ++i)
		{
			Socket *pSocket = *i;
			if( pSocket->mutex.lock(INFINITE, __FUNCTION__) )
			{
				// Close the socket if needed
				if( pSocket->open )
					closesocket(pSocket->socket);

				// Just in case anything still tries to access it (waiting for Lua GC to kick in?)
				pSocket->open = false;
				pSocket->connected = false;

				pSocket->mutex.unlock(__FUNCTION__); // Let the thread do its part
			}
		}

		// Step 2: Now we can actually delete the data.
		while( !socketList.empty() )
		{
			Socket *pSocket = socketList.front();

			if( (pSocket->inLua && !pSocket->deleteMe) || pSocket->hThread )
			{ // Make sure we aren't trying to erase it before Lua has shut down
				Sleep(1);
				continue;
			}
			socketList.erase(socketList.begin());	// Erase from the list
			delete pSocket;							// Free memory
			pSocket = NULL;
		}

		socketListLock.unlock(__FUNCTION__);
	}

	return MicroMacro::ERR_OK;
}

// TODO: This is completely broken...
bool Socket_lua::isIP(const char *ip)
{
	struct addrinfo *result;
	int success = getaddrinfo(ip, NULL, NULL, &result);

	return (success != 0);
}

int Socket_lua::connect(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 3 )
		wrongArgs(L);
	checkType(L, LT_STRING, 2);
	checkType(L, LT_NUMBER, 3);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	const char *host = lua_tostring(L, 2);
	int port = lua_tointeger(L, 3);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

/*	if( isIP(host) )
	{
		printf("Use raw IP\n");
		server.sin_addr.s_addr = inet_addr(host);
	}
	else*/
	{
		HOSTENT *pHostent;
		pHostent = gethostbyname(host);
		server.sin_addr = *((LPIN_ADDR)*pHostent->h_addr_list);
	}

	if( !pSocket->mutex.lock(INFINITE, __FUNCTION__) )
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, "Could not lock socket mutex.");
		return 2;
	}

	if( pSocket->connected || pSocket->open )
	{ // Socket already in use; cannot do this.
		pSocket->mutex.unlock(__FUNCTION__);
		lua_pushboolean(L, false);
		lua_pushstring(L, "Socket already connected; cannot open a new connection. Use a new socket.\n");
		return 2;
	}


	int success = (::connect(pSocket->socket, (struct sockaddr *)&server, sizeof(struct sockaddr)) >= 0);

	if( !success )
	{
		pSocket->mutex.unlock(__FUNCTION__);
		char errbuff[2048];
		slprintf(errbuff, sizeof(errbuff), "Connection failed. Err code %d\n", WSAGetLastError());
		lua_pushboolean(L, false);
		lua_pushstring(L, errbuff);
		return 2;
	}

	// Start a thread for this socket
	pSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)socketThread, (PVOID)pSocket, 0, NULL);
	pSocket->connected = true;
	pSocket->open = true;

	pSocket->mutex.unlock(__FUNCTION__);

	// Lets make a record of it in our list
	if( socketListLock.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
	{
		socketList.push_back(pSocket);
		socketListLock.unlock(__FUNCTION__);
	}

	lua_pushboolean(L, true);
	return 1;
}

int Socket_lua::listen(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 3 )
		wrongArgs(L);
	checkType(L, LT_STRING, 2);
	checkType(L, LT_NUMBER, 3);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	const char *host = lua_tostring(L, 2);
	int port = lua_tointeger(L, 3);

	struct sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	/*if( isIP(host) )
	{
		printf("Use raw IP\n");
		server.sin_addr.s_addr = inet_addr(host);
	}*/
	if( strlen(host) == 0 )
	{
		server.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		HOSTENT *pHostent;
		pHostent = gethostbyname(host);
		server.sin_addr = *((LPIN_ADDR)*pHostent->h_addr_list);
	}

	if( !pSocket->mutex.lock(INFINITE, __FUNCTION__) )
	{
		lua_pushboolean(L, false);
		lua_pushstring(L, "Could not lock socket mutex.");
		return 2;
	}

	if( pSocket->connected || pSocket->open )
	{ // Socket already in use; cannot do this.
		pSocket->mutex.unlock(__FUNCTION__);
		lua_pushboolean(L, false);
		lua_pushstring(L, "Socket already connected; cannot set to listen. Use a new socket.\n");
		return 2;
	}

	int success = (::bind(pSocket->socket, (struct sockaddr *)&server, sizeof(struct sockaddr)) >= 0);

	if( !success )
	{
		pSocket->mutex.unlock(__FUNCTION__);
		char errbuff[2048];
		slprintf(errbuff, sizeof(errbuff), "Bind failed. Err code %d\n", WSAGetLastError());
		lua_pushboolean(L, false);
		lua_pushstring(L, errbuff);
		return 2;
	}

	// Start a thread for this socket
	pSocket->connected = true;
	pSocket->open = true;
	pSocket->hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)listenThread, (PVOID)pSocket, 0, NULL);

	pSocket->mutex.unlock(__FUNCTION__);

	// Lets make a record of it in our list
	if( socketListLock.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
	{
		socketList.push_back(pSocket);
		socketListLock.unlock(__FUNCTION__);
	}

	lua_pushboolean(L, true);
	return 1;
}

int Socket_lua::send(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER | LT_STRING, 2);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	size_t len;
	const char *msg = lua_tolstring(L, 2, &len);

	if( !pSocket->mutex.lock(INFINITE, __FUNCTION__) )
	{
		lua_pushboolean(L, false);
		return 1;
	}

	if( !pSocket->connected || !pSocket->open )
	{ // Cannot send on a closed socket.
		pSocket->mutex.unlock(__FUNCTION__);
		lua_pushboolean(L, false);
		return 1;
	}

	int success = ::send(pSocket->socket, msg, len, 0);
	pSocket->mutex.unlock(__FUNCTION__);
	if( success == SOCKET_ERROR )
	{
		int errCode = WSAGetLastError();
		lua_pushboolean(L, false);
		return 1;
	}

	lua_pushboolean(L, true);
	return 1;
}

int Socket_lua::recv(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	int retVal = 0;
	if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
	{
		if( !pSocket->recvQueue.empty() )
		{
			lua_pushlstring(L, pSocket->recvQueue.front().c_str(), pSocket->recvQueue.front().size());
			pSocket->recvQueue.pop();
			retVal = 1;
		}

		pSocket->mutex.unlock(__FUNCTION__);
	}

	return retVal;
}

int Socket_lua::flushRecvQueue(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
	{ // We use 'swap' instead of just popping elements for performance reasons
		std::queue<std::string> emptyQueue;
		swap(pSocket->recvQueue, emptyQueue);
		pSocket->mutex.unlock(__FUNCTION__);
	}
	return 0;
}

int Socket_lua::getRecvQueueSize(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	size_t size = 0;
	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	if( pSocket->mutex.lock(DEFAULT_LOCK_TIMEOUT, __FUNCTION__) )
	{
		size = pSocket->recvQueue.size();
		pSocket->mutex.unlock(__FUNCTION__);
	}

	lua_pushinteger(L, size);
	return 1;
}

int Socket_lua::id(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	lua_pushinteger(L, pSocket->socket);
	return 1;
}

int Socket_lua::ip(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));

	struct sockaddr_in name;
	int	namelen	=	sizeof(name);
	getsockname(pSocket->socket, (struct sockaddr *)&name, &namelen);

	char buffer[128];
	DWORD dwNamelen	=	sizeof(name);
	DWORD outputLen	=	sizeof(buffer); // Initialize with our original buffer's size
	WSAAddressToString((sockaddr*)&name, dwNamelen, NULL, buffer, &outputLen);

	lua_pushstring(L, buffer);
	return 1;
}

int Socket_lua::remoteIp(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));

	struct sockaddr_in name;
	int	namelen	=	sizeof(name);
	getpeername(pSocket->socket, (struct sockaddr *)&name, &namelen);

	char buffer[128];
	DWORD dwNamelen	=	sizeof(name);
	DWORD outputLen	=	sizeof(buffer); // Initialize with our original buffer's size
	WSAAddressToString((sockaddr*)&name, dwNamelen, NULL, buffer, &outputLen);

	lua_pushstring(L, buffer);
	return 1;
}

int Socket_lua::close(lua_State *L)
{
	int top = lua_gettop(L);
	if( top < 1 )
		wrongArgs(L);

	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	if( pSocket->hThread && pSocket->open && pSocket->mutex.lock(INFINITE, __FUNCTION__) )
	{
		closesocket(pSocket->socket);
		pSocket->socket		=	INVALID_SOCKET;
		pSocket->connected	=	false;
		pSocket->open		=	false;
		pSocket->mutex.unlock(__FUNCTION__);
	}


	return 0;
}

int Socket_lua::gc(lua_State *L)
{
	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));

	// Mark the socket for deletion.
	if( pSocket->mutex.lock(INFINITE, __FUNCTION__) )
	{
		if( pSocket->hThread )
			close(L); // We only need to close() it if our thread is alive

		pSocket->deleteMe	=	true;
		pSocket->mutex.unlock(__FUNCTION__);
	}

	return 0;
}

int Socket_lua::tostring(lua_State *L)
{
	Socket *pSocket = *static_cast<Socket **>(lua_touserdata(L, 1));
	char buffer[64];
	slprintf(buffer, sizeof(buffer), "Socket (0x%p)", pSocket->socket);

	lua_pushstring(L, buffer);
	return 1;
}

#endif
