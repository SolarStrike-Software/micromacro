#include "filesystem_lua.h"
#include "filesystem.h"
#include "error.h"
#include "wininclude.h"
#include "strl.h"
#include "macro.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

int Filesystem_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"directoryExists", Filesystem_lua::directoryExists},
		{"fileExists", Filesystem_lua::fileExists},
		{"getDirectory", Filesystem_lua::getDirectory},
		{"fixSlashes", Filesystem_lua::fixSlashes},
		{"getOpenFileName", Filesystem_lua::getOpenFileName},
		{"getSaveFileName", Filesystem_lua::getSaveFileName},
		{"getCWD", Filesystem_lua::getCWD},
		{"setCWD", Filesystem_lua::setCWD},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, FILESYSTEM_MODULE_NAME);

	return MicroMacro::ERR_OK;
}

int Filesystem_lua::directoryExists(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	const char *path = lua_tostring(L, 1);
	bool exists = ::directoryExists(path);

	lua_pushboolean(L, exists);
	return 1;
}

int Filesystem_lua::fileExists(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	const char *path = lua_tostring(L, 1);
	bool exists = ::fileExists(path);

	lua_pushboolean(L, exists);
	return 1;
}

int Filesystem_lua::getDirectory(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);

	const char *path = lua_tostring(L, 1);
	std::vector<std::string> files = ::getDirectory(path);

	if( files.empty() )
		return 0;

	lua_newtable(L);
	for(unsigned int i = 0; i < files.size(); i++)
	{
		lua_pushnumber(L, i+1); // Key
		lua_pushstring(L, files.at(i).c_str()); // Value
		lua_settable(L, -3);
	}
	return 1;
}

int Filesystem_lua::fixSlashes(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	std::string path = (char *)lua_tostring(L, 1);
	bool posix = true;

	if( top == 2 )
	{
		checkType(L, LT_BOOLEAN, 2);
		posix = lua_toboolean(L, 2);
	}

	int slashes = SLASHES_TO_STANDARD;
	if( !posix )
		slashes = SLASHES_TO_WINDOWS;

	lua_pushstring(L, ::fixSlashes(path, slashes).c_str());
	return 1;
}

int Filesystem_lua::getOpenFileName(lua_State *L)
{
	int top = lua_gettop(L);
	if( top > 2 )
		wrongArgs(L);

	size_t defaultFilenameLen = 0;
	size_t filterLen = 0;
	char *defaultFilename = (char *)"";
	char *filter = NULL;

	if( top >= 1 )
		checkType(L, LT_STRING | LT_NIL, 1);
	if( top >= 2 )
		checkType(L, LT_STRING | LT_NIL, 2);

	if( top >= 1 && lua_isstring(L, 1) )
		defaultFilename = (char *)lua_tolstring(L, 1, &defaultFilenameLen);

	if( top >= 2 && lua_isstring(L, 2) )
		filter = (char *)lua_tolstring(L, 2, &filterLen);
	else
		filter = (char *)"All Files\0*.*\0Lua files\0*.lua\0\0";

	// Make sure to use backslashes!
	defaultFilename = (char *)::fixSlashes(defaultFilename, SLASHES_TO_WINDOWS).c_str();

	// We will use these buffers to store some data later.
	char cwdBuffer[MAX_PATH+1];
	char fileBuffer[MAX_PATH+1];
	char pathBuffer[MAX_PATH+1];

    /* NOTE: The dialog will modify the CWD, so we must restore it when done. */
    GetCurrentDirectory(MAX_PATH,(LPTSTR)&cwdBuffer);

	// Copy some default data into the buffers, prep OFN struct
	strlcpy(fileBuffer, ::getFileName(defaultFilename).c_str(), MAX_PATH);
	if( ::getFilePath(defaultFilename, false) == "" )
	{ // Assume scripts directory
		std::string buff = cwdBuffer;
		buff += "/";
		buff += Macro::instance()->getSettings()->getString(CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY);
		strlcpy(pathBuffer, buff.c_str(), MAX_PATH);
	}
	else
		strlcpy(pathBuffer, ::fixSlashes(::getFilePath(defaultFilename, true), SLASHES_TO_WINDOWS).c_str(), MAX_PATH);

    OPENFILENAME ofn;
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = Macro::instance()->getAppHwnd();
    ofn.hInstance = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = (LPSTR)&fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = pathBuffer;
    ofn.lpstrTitle = NULL;
    ofn.Flags = (DWORD)(OFN_ENABLESIZING);
    ofn.nFileOffset = (WORD)NULL;
    ofn.nFileExtension = (WORD)NULL;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = (WORD)NULL;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;
    ofn.pvReserved = NULL;
    ofn.dwReserved = (WORD)NULL;
    ofn.FlagsEx = (DWORD)0;

	// Show dialog
	int retCount = 0;
    int success = GetOpenFileName(&ofn);
    if( success )
    { // User clicked OK
		lua_pushstring(L, ofn.lpstrFile);
		retCount = 1;
    }
    else
    { // User clicked cancel
		SetCurrentDirectory((LPCTSTR)&cwdBuffer);
		retCount = 0;
    }

	// Reset CWD
    SetCurrentDirectory((LPCTSTR)&cwdBuffer);

	/* Attempt to un-fuck message queue
		A Windows bug causes GetOpenFileName to screw up the input queue
		which prevents GetKeyboardState() from functioning properly
	*/
	MSG msg;
	while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0 )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return retCount;
}

int Filesystem_lua::getSaveFileName(lua_State *L)
{
	int top = lua_gettop(L);
	if( top > 2 )
		wrongArgs(L);

	size_t defaultFilenameLen = 0;
	size_t filterLen = 0;
	char *defaultFilename = (char *)"";
	char *filter = NULL;

	if( top >= 1 )
		checkType(L, LT_STRING | LT_NIL, 1);
	if( top >= 2 )
		checkType(L, LT_STRING | LT_NIL, 2);

	if( top >= 1 && lua_isstring(L, 1) )
		defaultFilename = (char *)lua_tolstring(L, 1, &defaultFilenameLen);

	if( top >= 2 && lua_isstring(L, 2) )
		filter = (char *)lua_tolstring(L, 2, &filterLen);
	else
		filter = (char *)"All Files\0*.*\0Lua files\0*.lua\0\0";

	// Make sure to use backslashes!
	defaultFilename = (char *)::fixSlashes(defaultFilename, SLASHES_TO_WINDOWS).c_str();

	// We will use these buffers to store some data later.
	char cwdBuffer[MAX_PATH+1];
	char fileBuffer[MAX_PATH+1];
	char pathBuffer[MAX_PATH+1];

    /* NOTE: The dialog will modify the CWD, so we must restore it when done. */
    GetCurrentDirectory(MAX_PATH,(LPTSTR)&cwdBuffer);

	// Copy some default data into the buffers, prep OFN struct
	strlcpy(fileBuffer, ::getFileName(defaultFilename).c_str(), MAX_PATH);
	if( ::getFilePath(defaultFilename, false) == "" )
	{ // Assume scripts directory
		std::string buff = cwdBuffer;
		buff += "/";
		buff += Macro::instance()->getSettings()->getString(CONFVAR_SCRIPT_DIRECTORY, CONFDEFAULT_SCRIPT_DIRECTORY);
		strlcpy(pathBuffer, buff.c_str(), MAX_PATH);
	}
	else
		strlcpy(pathBuffer, ::fixSlashes(::getFilePath(defaultFilename, true), SLASHES_TO_WINDOWS).c_str(), MAX_PATH);

    OPENFILENAME ofn;
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = Macro::instance()->getAppHwnd();
    ofn.hInstance = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter = 0;
    ofn.nFilterIndex = 1;
    ofn.lpstrFile = (LPSTR)&fileBuffer;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = pathBuffer;
    ofn.lpstrTitle = NULL;
    ofn.Flags = (DWORD)(OFN_ENABLESIZING);
    ofn.nFileOffset = (WORD)NULL;
    ofn.nFileExtension = (WORD)NULL;
    ofn.lpstrDefExt = NULL;
    ofn.lCustData = (WORD)NULL;
    ofn.lpfnHook = NULL;
    ofn.lpTemplateName = NULL;
    ofn.pvReserved = NULL;
    ofn.dwReserved = (WORD)NULL;
    ofn.FlagsEx = (DWORD)0;

	// Show dialog
	int retCount = 0;
    int success = GetSaveFileName(&ofn);
    if( success )
    { // User clicked OK
		lua_pushstring(L, ofn.lpstrFile);
		retCount = 1;
    }
    else
    { // User clicked cancel
		SetCurrentDirectory((LPCTSTR)&cwdBuffer);
		retCount = 0;
    }

	// Reset CWD
    SetCurrentDirectory((LPCTSTR)&cwdBuffer);

	/* Attempt to un-fuck message queue
		A Windows bug causes GetOpenFileName to screw up the input queue
		which prevents GetKeyboardState() from functioning properly
	*/
	MSG msg;
	while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0 )
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return retCount;
}

int Filesystem_lua::getCWD(lua_State *L)
{
	if( lua_gettop(L) != 0 )
		wrongArgs(L);

	char cwdBuffer[MAX_PATH+1];
	GetCurrentDirectory(MAX_PATH,(LPTSTR)&cwdBuffer);

	std::string fixed = ::fixSlashes(cwdBuffer, SLASHES_TO_STANDARD);

	lua_pushstring(L, fixed.c_str());
	return 1;
}

int Filesystem_lua::setCWD(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	std::string newCWD = lua_tostring(L, 1);

	newCWD = ::fixSlashes(newCWD, SLASHES_TO_WINDOWS);

	bool success = SetCurrentDirectory(newCWD.c_str());
	lua_pushboolean(L, success);
	return 1;
}
