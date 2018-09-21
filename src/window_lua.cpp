/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "window_lua.h"
#include "types.h"
#include "error.h"
#include "strl.h"
#include "event.h"
#include "macro.h"
#include "luatypes.h"
#include "types.h"
#include "mathl.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

//using MicroMacro::EnumWindowPair;
using MicroMacro::EnumWindowListInfo;
using MicroMacro::WindowInfo;

const char *windowThumbnailClassName = "ThumbnailClass";

unsigned char Window_lua::getR(int color)
{
	return (unsigned char)(color >> 16);
}

unsigned char Window_lua::getG(int color)
{
	return (unsigned char)((color >> 8) & 0xFF);
}

unsigned char Window_lua::getB(int color)
{
	return (unsigned char)(color & 0xFF);
}

// Helper function to Window_lua::find()
BOOL CALLBACK Window_lua::_findProc(HWND hwnd, LPARAM lparam)
{
	//EnumWindowPair *winpair = (EnumWindowPair *)lparam;
	WindowInfo *wi = (WindowInfo *)lparam;
	char namestring[2048];
	char classname[256];
	GetWindowText(hwnd, (char *)&namestring, sizeof(namestring)-1);

	sztolower(namestring, namestring, strlen(namestring));
	int match = wildfind(wi->name, namestring);

	if( match )
	{
		// Looking for just the window, not a specific classname
		if( strcmp(wi->classname.c_str(), "") == 0 )
		{
			// Ensure that this isn't a window preview/overlay

			GetClassName(hwnd, (char*)&classname, sizeof(classname)-1);

			if( !strcmp((char*)&classname, windowThumbnailClassName))
			return true;

			wi->hwnd = hwnd;
			return false;
		}
		else
		{
			// Check if this window is valid itself
			GetClassName(hwnd, (char*)&classname, sizeof(classname)-1);

			if( strcmp(classname, wi->classname.c_str()) == 0 )
			{
				// We have a match
				wi->hwnd = hwnd;
				return false;
			}

			// If not, scan it's children
			HWND controlHwnd = FindWindowEx(hwnd, NULL, wi->classname.c_str(), NULL);

			if( controlHwnd == NULL )
				return true;

			// We have a match
			wi->hwnd = controlHwnd;
			return false;
		}
	}
	else
		return true;
}

// Helper function to Window_lua::findList()
BOOL CALLBACK Window_lua::_findListProc(HWND hwnd, LPARAM lparam)
{
	EnumWindowListInfo *wli = (EnumWindowListInfo *)lparam;
	char namestring[2048];
	char classname[256];
	GetWindowText(hwnd, (char *)&namestring, sizeof(namestring)-1);

	WindowInfo wi;
	wi.hwnd = hwnd;
	wi.name = namestring;	// Copy it before we modify it

	sztolower(namestring, namestring, strlen(namestring));
	int match = wildfind(wli->name, namestring);

	if( match )
	{
		// Looking for just the window, not a specific classname
		if( strcmp(wli->classname.c_str(), "") == 0 )
		{
			// Ensure that this isn't a window preview/overlay
			GetClassName(hwnd, (char*)&classname, sizeof(classname)-1);

			wi.classname = classname;	// Copy it before we modify it

			if( !strcmp((char*)&classname, windowThumbnailClassName))
			return true;

			wli->windows.push_back(wi);
		}
		else
		{
			// Check if this window is valid itself
			GetClassName(hwnd, (char*)&classname, sizeof(classname)-1);

			wi.classname = classname;	// Copy it before we modify it

			if( strcmp(classname, wli->classname.c_str()) == 0 )
			{
				// We have a match
				WindowInfo wi;
				wi.hwnd = hwnd;
				wi.name = namestring;
				wi.classname = classname;

				wli->windows.push_back(wi);
			}

			// If not, scan it's children
			HWND controlHwnd = FindWindowEx(hwnd, NULL, wli->classname.c_str(), NULL);

			if( controlHwnd != NULL )
			{
				GetWindowText(controlHwnd, (char *)&namestring, sizeof(namestring)-1);
				GetClassName(controlHwnd, (char*)&classname, sizeof(classname)-1);

				WindowInfo wi;
				wi.hwnd = controlHwnd;
				wi.name = namestring;
				wi.classname = classname;

				wli->windows.push_back(wi);
			}
		}
	}

	return true;
}


int Window_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"find", Window_lua::find},
		{"findList", Window_lua::findList},
		{"getParent", Window_lua::getParent},
		{"getTitle", Window_lua::getTitle},
        {"setTitle", Window_lua::setTitle},
		{"getClassName", Window_lua::getClassName},
		{"valid", Window_lua::valid},
		{"getRect", Window_lua::getRect},
		{"setRect", Window_lua::setRect},
		{"getClientRect", Window_lua::getClientRect},
		{"setClientRect", Window_lua::setClientRect},
		{"show", Window_lua::show},
		//{"openDC", Window_lua::openDC},
		//{"closeDC", Window_lua::closeDC},
		{"flash", Window_lua::flash},
		{"getPixel", Window_lua::getPixel},
		{"pixelSearch", Window_lua::pixelSearch},
		{"saveScreenshot", Window_lua::saveScreenshot},
		{"getAppHwnd", Window_lua::getAppHwnd},
		{"getFocusHwnd", Window_lua::getFocusHwnd},
		{"makeColor", Window_lua::makeColor},
		{"drawLine", Window_lua::drawLine},
		{"drawRect", Window_lua::drawRect},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, WINDOW_MODULE_NAME);

	lua_pushinteger(L,	SW_FORCEMINIMIZE);
	lua_setglobal(L,	"SW_FORCEMINIMIZE");
	lua_pushinteger(L,	SW_HIDE);
	lua_setglobal(L,	"SW_HIDE");
	lua_pushinteger(L,	SW_MAXIMIZE);
	lua_setglobal(L,	"SW_MAXIMIZE");
	lua_pushinteger(L,	SW_MINIMIZE);
	lua_setglobal(L,	"SW_MINIMIZE");
	lua_pushinteger(L,	SW_RESTORE);
	lua_setglobal(L,	"SW_RESTORE");
	lua_pushinteger(L,	SW_SHOW);
	lua_setglobal(L,	"SW_SHOW");
	lua_pushinteger(L,	SW_SHOWDEFAULT);
	lua_setglobal(L,	"SW_SHOWDEFAULT");
	lua_pushinteger(L,	SW_SHOWMAXIMIZED);
	lua_setglobal(L,	"SW_SHOWMAXIMIZED");
	lua_pushinteger(L,	SW_SHOWMINIMIZED);
	lua_setglobal(L,	"SW_SHOWMINIMIZED");
	lua_pushinteger(L,	SW_SHOWMINNOACTIVE);
	lua_setglobal(L,	"SW_SHOWMINNOACTIVE");
	lua_pushinteger(L,	SW_SHOWNA);
	lua_setglobal(L,	"SW_SHOWNA");
	lua_pushinteger(L,	SW_SHOWNOACTIVATE);
	lua_setglobal(L,	"SW_SHOWNOACTIVATE");
	lua_pushinteger(L,	SW_SHOWNORMAL);
	lua_setglobal(L,	"SW_SHOWNORMAL");
	return MicroMacro::ERR_OK;
}

/*	window.find(string title [, string classname])
	Returns (on success):	number hwnd
	Returns (on failure):	nil

	Finds a window's HWND based on its title (not case-sensitive),
	and (optionally) its classname (case-sensitive).
	'title' and 'classname' can contain wildcards * and ?.

	Returns the first match found.
	If no match was found, returns nil.
*/
int Window_lua::find(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	if( top == 2 )
		checkType(L, LT_STRING, 2);

	size_t nameLen = 0;
	size_t classnameLen = 0;
	const char *name = lua_tolstring(L, 1, &nameLen);
	const char *classname = "";
	if( top == 2 )
		classname = lua_tolstring(L, 2, &classnameLen);

	char *name_lower = 0;
	//char *classname_lower;
	try {
		name_lower = new char[nameLen+1];
		//classname_lower = new char[classnameLen+1];
	} catch( std::bad_alloc &ba ) { badAllocation(); }

	// Convert the search name to lowercase for matching
	sztolower(name_lower, name, nameLen);

//	EnumWindowPair searchpair;
	WindowInfo searchWindowInfo;
	searchWindowInfo.name = name_lower;
	searchWindowInfo.classname = (char*)classname;//classname_lower;
	searchWindowInfo.hwnd = 0;

	EnumWindows(_findProc, (LPARAM)&searchWindowInfo);

	// Free allocated memory
	delete []name_lower;
	//delete []classname_lower;

	if( searchWindowInfo.hwnd == 0 )
	{ // Throw warning
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failure to find window. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());
		return 0;
	}

	lua_pushinteger(L, (lua_Integer)searchWindowInfo.hwnd);
	return 1;
}

/*	window.findList(string title [, string classname])
	Returns (on success):	table (of numbers)
	Returns (on failure):	nil

	Finds a list of windows based on title (not case-sensitive),
	and (optionally) classname (case-sensitive).
	'title' and 'classname' can contain wildcards * and ?.

	Returns a table of HWNDs (numbers).
	If no match was found, returns nil.
*/
int Window_lua::findList(lua_State *L)
{
	int top = lua_gettop(L);
	if( top != 1 && top != 2 )
		wrongArgs(L);
	checkType(L, LT_STRING, 1);
	if( top == 2 )
		checkType(L, LT_STRING, 2);

	size_t nameLen = 0;
	size_t classnameLen = 0;
	const char *name = lua_tolstring(L, 1, &nameLen);
	const char *classname = "";
	if( top == 2 )
		classname = lua_tolstring(L, 2, &classnameLen);

	char *name_lower = 0;
	try {
		name_lower = new char[nameLen+1];
	} catch( std::bad_alloc &ba ) { badAllocation(); }

	// Convert the search name to lowercase for matching
	sztolower(name_lower, name, nameLen);

	EnumWindowListInfo searchWinListInfo;
	searchWinListInfo.name = name_lower;
	searchWinListInfo.classname = (char*)classname;

	EnumWindows(_findListProc, (LPARAM)&searchWinListInfo);

	// Free allocated memory
	delete []name_lower;

	if( searchWinListInfo.windows.empty() )
		return 0;

	lua_newtable(L);
	for(unsigned int i = 0; i < searchWinListInfo.windows.size(); i++)
	{
		lua_pushinteger(L, i+1); // Push key
		lua_newtable(L); // Push value (table)
			lua_pushstring(L, "hwnd");
			lua_pushinteger(L, (lua_Integer)searchWinListInfo.windows.at(i).hwnd);
			lua_settable(L, -3);

			lua_pushstring(L, "name");
			lua_pushstring(L, searchWinListInfo.windows.at(i).name.c_str());
			lua_settable(L, -3);

			lua_pushstring(L, "class");
			lua_pushstring(L, searchWinListInfo.windows.at(i).classname.c_str());
			lua_settable(L, -3);
		lua_settable(L, -3); // Set it
	}

	return 1;
}

/*	window.getParent(number hwnd)
	Returns (on success):	number hwnd
	Returns (on failure):	nil

	Returns a window's parent, or nil on error.
*/
int Window_lua::getParent(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

	HWND parent = ::GetParent(hwnd);

	if( parent == NULL ) // No parent or error occurred
		return 0;

	lua_pushinteger(L, (lua_Integer)parent);
	return 1;
}

/*	window.getTitle(number hwnd)
	Returns (on success):	string
	Returns (on failure):	nil

	Returns a window's title.
*/
int Window_lua::getTitle(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

	char buffer[2048];
	int success = ::GetWindowText(hwnd, buffer, sizeof(buffer)-1);

	if( !success )
	{ // Throw error
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failure to get window title. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());

		lua_pushnil(L);
		return 1;
	}

	lua_pushstring(L, buffer);
	return 1;
}

/*	window.setTitle(number hwnd, string title)
	Returns:	boolean

	Sets a window's title.
	Returns true on success, false on failure.
*/
int Window_lua::setTitle(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_STRING, 2);
	HWND hwnd = (HWND)lua_tointeger(L, 1);
	const char *title = lua_tostring(L, 2);

	int success = true;
	// If setting the title for this window, use SetConsoleTitle instead
	if( hwnd == Macro::instance()->getAppHwnd() )
		success = SetConsoleTitle(title);
	else
		success = SendMessage(hwnd, WM_SETTEXT, (WPARAM)0, (LPARAM)title);

	if( !success )
	{ // Throw error
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failure to set window title. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());
	}

	lua_pushboolean(L, success);
	return 1;
}

/*	window.getClassName(number hwnd)
	Returns (on success):	string
	Returns (on failure):	nil

	Returns a window's class name.
*/
int Window_lua::getClassName(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

	char buffer[2048];
	int success = ::GetClassName(hwnd, buffer, sizeof(buffer)-1);

	if( !success )
	{ // Throw error
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failure to get window class. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());

		lua_pushnil(L);
		return 1;
	}

	lua_pushstring(L, buffer);
	return 1;
}

/*	window.valid(number hwnd)
	Returns:	boolean

	Returns true if hwnd is a valid window handle, or false otherwise.
*/
int Window_lua::valid(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

	lua_pushboolean(L, ::IsWindow(hwnd));
	return 1;
}

/*	window.getRect(number hwnd)
	Returns (on success):	number x
							number y
							number width
							number height

	Returns (on failure):	nil

	Returns the position and size of a window.
*/
int Window_lua::getRect(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

    WINDOWPLACEMENT wp;
    memset(&wp, 0, sizeof(WINDOWPLACEMENT));
    wp.length = sizeof(WINDOWPLACEMENT);
    bool success = GetWindowPlacement(hwnd, &wp);

	if( !success )
	{
		lua_pushnil(L);
		return 1;
	}

	// Push results

    lua_pushinteger(L, wp.rcNormalPosition.left);
    lua_pushinteger(L, wp.rcNormalPosition.top);
    lua_pushinteger(L, wp.rcNormalPosition.right - wp.rcNormalPosition.left);
    lua_pushinteger(L, wp.rcNormalPosition.bottom - wp.rcNormalPosition.top);
    lua_pushinteger(L, wp.showCmd);

	return 5;
}

/*	window.setRect(number hwnd, number x, number y, number width, number height)
	Returns:	nil

	Change the position and size of a window with handle 'hwnd'.
*/
int Window_lua::setRect(lua_State *L)
{
	int top = lua_gettop(L);
	if( top < 3 || top > 5 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);
	if( top >= 4 )
		checkType(L, LT_NUMBER | LT_NIL, 4);
	if( top >= 5 )
		checkType(L, LT_NUMBER | LT_NIL, 5);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	//RECT winrect;
	//GetWindowRect(hwnd, &winrect);
    WINDOWPLACEMENT wp;
    memset(&wp, 0, sizeof(WINDOWPLACEMENT));
    wp.length = sizeof(WINDOWPLACEMENT);
    GetWindowPlacement(hwnd, &wp);

	int origLeft = wp.rcNormalPosition.left;
	int origTop = wp.rcNormalPosition.top;

	wp.rcNormalPosition.left = lua_tointeger(L, 2);
	wp.rcNormalPosition.top = lua_tointeger(L, 3);
	if( lua_isnumber(L, 4) )
		wp.rcNormalPosition.right = lua_tointeger(L, 4);
	else
		wp.rcNormalPosition.right = wp.rcNormalPosition.right - origLeft;
	if( lua_isnumber(L, 5) )
		wp.rcNormalPosition.bottom = lua_tointeger(L, 5);
	else
		wp.rcNormalPosition.bottom = wp.rcNormalPosition.bottom - origTop;

	MoveWindow(hwnd, wp.rcNormalPosition.left, wp.rcNormalPosition.top, wp.rcNormalPosition.right, wp.rcNormalPosition.bottom, false);
	return 0;
}

/*	window.getClientRect(number hwnd)
	Returns (on success):	number x
							number y
							number width
							number height

	Returns (on failure):	nil

	Returns the position and size of a window's client area.
*/
int Window_lua::getClientRect(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);
	//RECT rect;
	POINT point;
	point.x = 0; point.y = 0;

	// Translate coords, get rect
	ClientToScreen(hwnd, &point);
	//GetClientRect(hwnd, &rect);
    WINDOWPLACEMENT wp;
    memset(&wp, 0, sizeof(WINDOWPLACEMENT));
    wp.length = sizeof(WINDOWPLACEMENT);
    bool success = GetWindowPlacement(hwnd, &wp);

	if( !success )
	{
		lua_pushnil(L);
		return 1;
	}

	// Push results
	lua_pushnumber(L, point.x);
	lua_pushnumber(L, point.y);
	lua_pushnumber(L, wp.rcNormalPosition.right - point.x);
	lua_pushnumber(L, wp.rcNormalPosition.bottom - point.y);
    lua_pushinteger(L, wp.showCmd);

	return 5;
}

/*	window.setClientRect(number hwnd, number x, number y, number width, number height)
	Returns:	nil

	Change the position and size of a window's client area with handle 'hwnd'.
*/
int Window_lua::setClientRect(lua_State *L)
{
	int top = lua_gettop(L);
	if( top < 3 || top > 5 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);
	if( top >= 4 )
		checkType(L, LT_NUMBER | LT_NIL, 4);
	if( top >= 5 )
		checkType(L, LT_NUMBER | LT_NIL, 5);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	RECT winrect;
	GetClientRect(hwnd, &winrect);
	unsigned int flags = SWP_ASYNCWINDOWPOS|SWP_SHOWWINDOW;

	winrect.left = 0; winrect.top = 0;
	int origLeft = winrect.left;
	//int origTop = winrect.top;
	int newLeft = lua_tointeger(L, 2);
	int newTop = lua_tointeger(L, 3);
	if( lua_isnumber(L, 4) )
		winrect.right = lua_tointeger(L, 4);
	else
		winrect.right = winrect.right + (newLeft-origLeft);
	if( lua_isnumber(L, 5) )
		winrect.bottom = lua_tointeger(L, 5);
	else
		winrect.bottom = winrect.bottom;


	// Adjust from client coordinates to screen
    DWORD dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    DWORD dwExStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    HMENU menu = GetMenu(hwnd);
	AdjustWindowRectEx(&winrect, dwStyle, menu != NULL, dwExStyle);

	// Re-add borders
	winrect.right = winrect.right - winrect.left;
	winrect.bottom = winrect.bottom - winrect.top;

	// Set it
	SetWindowPos(hwnd, HWND_NOTOPMOST, newLeft + winrect.left, newTop + winrect.top, winrect.right, winrect.bottom, flags);

	return 0;
}


/*	window.show(number hwnd, number cmd)
	Returns:	nil

	Show/hide/minimize/maximize/whatever with the window.
	See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms633548%28v=vs.85%29.aspx
*/
int Window_lua::show(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	int cmd = lua_tointeger(L, 2);

	if( hwnd == 0 )
		hwnd = GetDesktopWindow();

	ShowWindowAsync(hwnd, cmd);

	if( cmd == SW_SHOW || cmd == SW_SHOWNORMAL || cmd == SW_RESTORE )
	{
		SetForegroundWindow(hwnd);

		WINDOWPLACEMENT wp;
		GetWindowPlacement(hwnd, &wp);

		// Prevent blocking if necessary
		if( !(wp.flags & WPF_ASYNCWINDOWPLACEMENT) && hwnd != Macro::instance()->getAppHwnd() )
		{
			wp.flags |= WPF_ASYNCWINDOWPLACEMENT;
		}

		if( cmd == SW_RESTORE )
		{
			if( wp.flags & WPF_RESTORETOMAXIMIZED )
			{
				wp.showCmd = SW_SHOWMAXIMIZED;
				SetWindowPlacement(hwnd, &wp);
			}
		}
	}

	return 0;
}

/*	window.flash(number hwnd, number flashCount)
	Returns:	nil

	"Flash" the window to attempt to grab the user's attention.
	if 'flashCount' is < 0, stop flashing
	if 'flashCount' is 0, flash until user focuses the window
	if 'flashCount' is > 0, flash this many times
*/
int Window_lua::flash(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	int count = lua_tointeger(L, 2);

	FLASHWINFO fwi;
	fwi.hwnd = hwnd;
	fwi.dwFlags = FLASHW_ALL;

	if( count < 0 )
		fwi.dwFlags = FLASHW_STOP;
	else if( count == 0 )
		fwi.dwFlags |= FLASHW_TIMERNOFG;
	else
	{
		fwi.dwFlags |= FLASHW_TIMERNOFG;
		fwi.uCount = count;
	}

	fwi.cbSize = sizeof(FLASHWINFO);
	fwi.dwTimeout = 0;
	FlashWindowEx(&fwi);

	return 0;
}

/* NOTE: We don't need thse functions right now
int Window_lua::openDC(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

	HDC hdc = ::GetWindowDC(hwnd);
	if( hdc == NULL )
	{ // An error occurred
		pushLuaErrorEvent(L, "Failure to open window device context. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());
		return 0;
	}

	WindowDCPair *pWinDC = static_cast<WindowDCPair *>(lua_newuserdata(L, sizeof(WindowDCPair)));
	pWinDC->hwnd = hwnd;
	pWinDC->hdc = hdc;

	// Set metatable
	luaL_getmetatable(L, LuaType::metatable_windowDC);
	lua_setmetatable(L, -2);

	return 1;
}

int Window_lua::closeDC(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	WindowDCPair *pWinDC = static_cast<WindowDCPair *>(lua_touserdata(L, 1));
	ReleaseDC(pWinDC->hwnd, pWinDC->hdc);

	pWinDC->hwnd = NULL; pWinDC->hdc = NULL;
	return 0;
}
*/

/*	window.getPixel(number hwnd, number x, number y)
	Returns (on success):	number r
							number g
							number b

	Returns (on failure):	nil

	Get the color of a pixel at (x,y) inside window 'hwnd',
	and return the color split into red, green, and blue channels.
	r, g, and b results will be between 0 and 255.
*/
int Window_lua::getPixel(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);

	POINT point;
	HWND hwnd = (HWND)lua_tointeger(L, 1);
	point.x = lua_tointeger(L, 2);
	point.y = lua_tointeger(L, 3);

	HDC hdc = GetDC(NULL); // Open DC to desktop (not specified window)
	if( !hdc )
	{ // Throw error
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Failure to open DC to desktop. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());
		return 0;
	}

	ClientToScreen(hwnd, &point);
	COLORREF ref = GetPixel(hdc, point.x, point.y);
	ReleaseDC(hwnd, hdc);

	lua_pushinteger(L, GetRValue(ref));
	lua_pushinteger(L, GetGValue(ref));
	lua_pushinteger(L, GetBValue(ref));
	return 3;
}

/*	window.pixelSearch(number hwnd, number r, number g, number b,
						number x1, number y1, number x2, number y2,
						number accuracy, number step)

	Returns (on success):	number x
							number y

	Returns (on failure):	nil

	Search the given window for a pixel that matches r,g,b
	within the rectangle outlined by (x1,y1) -> (x2,y2)

	'accuracy' is how many units each channel must be within
	the given color to generate a match. Default: 1

	'step' is the step size (distance between pixels to search).
	Default: 1
*/
int Window_lua::pixelSearch(lua_State *L)
{
	int top = lua_gettop(L);
	if( top < 8 || top > 10 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1); // HWND
	checkType(L, LT_NUMBER, 2); // R
	checkType(L, LT_NUMBER, 3); // G
	checkType(L, LT_NUMBER, 4); // B
	checkType(L, LT_NUMBER, 5); // x1
	checkType(L, LT_NUMBER, 6); // y1
	checkType(L, LT_NUMBER, 7); // x2
	checkType(L, LT_NUMBER, 8); // y2

	if( top >= 9 )
		checkType(L, LT_NUMBER, 9); // accuracy

	if( top >= 10 )
		checkType(L, LT_NUMBER, 10); // step

	POINT retval;
	POINT offset;
	RECT winRect;
	RECT clientRect;
	int r1, g1, b1, r2, g2, b2;
	int x1, y1, x2, y2;
	int step = 1;
	int accuracy = 1;
	HWND hwnd;
	bool reversex;
	bool reversey;

	hwnd = (HWND)lua_tointeger(L, 1);
	r1 = lua_tointeger(L, 2);
	g1 = lua_tointeger(L, 3);
	b1 = lua_tointeger(L, 4);
	x1 = lua_tointeger(L, 5);
	y1 = lua_tointeger(L, 6);
	x2 = lua_tointeger(L, 7);
	y2 = lua_tointeger(L, 8);

	if( top >= 9 )
		accuracy = lua_tointeger(L, 9);

	if( top >= 10 )
		step = lua_tointeger(L, 10);

	if( step < 1 ) step = 1;
	reversex = (x2 < x1);
	reversey = (y2 < y1);
	int steps_x = abs(x2-x1)/step;
	int steps_y = abs(y2-y1)/step;

	// The number of steps across each axis
	int width = abs(x2-x1);
	int height = abs(y2-y1);

	// Figure out the client offset
	winRect.left = 0; winRect.top = 0;
	offset.x = 0; offset.y = 0;

	GetWindowRect(hwnd, &winRect);
	ClientToScreen(hwnd, &offset);
	offset.x = offset.x - winRect.left;
	offset.y = offset.y - winRect.top;
	GetClientRect(hwnd, &clientRect);

	// Ensure we're not attempting to work outside the client rect
	x1 = clamp(x1, 0, (int)clientRect.right);
	y1 = clamp(y1, 0, (int)clientRect.bottom);
	x2 = clamp(x2, 0, (int)clientRect.right);
	y2 = clamp(y2, 0, (int)clientRect.bottom);

	// Grab a copy of the target window as a bitmap
	HDC hdcScreen = GetDC(NULL);
	HDC tmpHdc = CreateCompatibleDC(hdcScreen);
	HBITMAP hBmp = CreateCompatibleBitmap(hdcScreen, clientRect.right-clientRect.left,
		clientRect.bottom-clientRect.top);

	// Make sure we will even be able to handle it
	if( IsIconic(hwnd) || hdcScreen == NULL || tmpHdc == NULL || hBmp == NULL )
	{ // Throw an error
		DeleteDC(tmpHdc);
		DeleteObject(hBmp);
		ReleaseDC(NULL, hdcScreen);
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Unable to grab screenshot of target window. Error code %i (%s)",
			errCode, getWindowsErrorString(errCode).c_str());
		return 0;
	}

	// Get info from the screenshot
	HGDIOBJ origSelectedObject = SelectObject(tmpHdc, hBmp);
	int pw = PrintWindow(hwnd, tmpHdc, PW_CLIENTONLY);
	int biWidth = clientRect.right;// - clientRect.left;
	int biHeight = clientRect.bottom;// - clientRect.top;
	BITMAPINFO bmpInfo;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = biWidth;
	bmpInfo.bmiHeader.biHeight = biHeight;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biSizeImage = 0;

	BITMAP obmp;
	int bytes = GetObject(hBmp, sizeof(obmp), &obmp);

	// Switch back to original object before we call GetDIBits! (important)
	SelectObject(tmpHdc, origSelectedObject);


	int pixelsBound = (biHeight+1)*biWidth;
	RGBQUAD *_pixels = new RGBQUAD[pixelsBound];
	int scanlines = GetDIBits(tmpHdc, hBmp, 0, biHeight, _pixels, &bmpInfo, DIB_RGB_COLORS);

	if( pw == 0 || scanlines == 0 || scanlines == ERROR_INVALID_PARAMETER )
	{ // Throw error
		DeleteDC(tmpHdc);
		DeleteObject(hBmp);
		ReleaseDC(NULL, hdcScreen);
		delete []_pixels;
		int errCode = GetLastError();
		pushLuaErrorEvent(L, "Received invalid data or unable to allocate memory while reading image data. "
			"Error code %i (%s)", errCode, getWindowsErrorString(errCode).c_str());
		return 0;
	}

	if( height > scanlines ) // Re-adjust scanlines if we didn't read enough
	{
		height = scanlines;
		steps_y = height/step;
	}

	// Iterate through, check for matches
	int x, y;
	retval.x = 0; retval.y = 0;
	bool found = false;
	for(int i = 0; i <= steps_y; i++)
	{
		for(int v = 0; v <= steps_x; v++)
		{
			if( !reversex )
				x = x1 + v*step;
			else
				x = x2 + width - v*step;
			if( !reversey )
				y = y1 + i*step;
			else
				y = y2 + height - i*step;

			if( x > (offset.x + clientRect.right/*-clientRect.left*/) )
			{
				x = 0;
				y++;
				continue;
			}
			if( y > (clientRect.bottom/*-clientRect.top*/) )
			{
				i = steps_y;
				break;
			}

			int index = (biHeight-y)*biWidth + x;
			RGBQUAD rgba = _pixels[index];

			r2 = rgba.rgbRed;
			g2 = rgba.rgbGreen;
			b2 = rgba.rgbBlue;

			if( abs(r2 - r1) <= accuracy &&
				abs(g2 - g1) <= accuracy &&
				abs(b2 - b1) <= accuracy )
			{
				retval.x = x;
				retval.y = y;
				found = true;
				i = steps_y; // To break from Y loop
				break;
			}
		}
	}

	// Make sure to free resources
	delete []_pixels;
	DeleteDC(tmpHdc);
	DeleteObject(hBmp);
	ReleaseDC(NULL, hdcScreen);

	if( !found )
		return 0;

	lua_pushinteger(L, retval.x);
	lua_pushinteger(L, retval.y);
	return 2;
}

/*	window.saveScreenshot(number hwnd, string filename)
	Returns:	nil

	Save a screenshot of window 'hwnd' to 'filename'.
	If 'hwnd' is 0, this screenshots the whole desktop.
*/
int Window_lua::saveScreenshot(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_STRING, 2);

	RECT winRect;
	HDC hdc;
	HDC tmpDc;
	HBITMAP hBmp;

	size_t filenameLen;
	HWND hwnd = (HWND)lua_tointeger(L, 1);
	const char *filename = lua_tolstring(L, 2, &filenameLen);

	if( hwnd == 0 )
		hwnd = GetDesktopWindow();

	// Get the window rect, convert
	GetWindowRect(hwnd, &winRect);
	winRect.right = winRect.right - winRect.left;
	winRect.bottom = winRect.bottom - winRect.top;

	// Copy it
	hdc = GetDC(NULL);
	tmpDc = CreateCompatibleDC(hdc);
	hBmp = CreateCompatibleBitmap(hdc, winRect.right, winRect.bottom);
	SelectObject(tmpDc, hBmp);
	BitBlt(tmpDc, 0, 0, winRect.right, winRect.bottom, hdc, winRect.left, winRect.top, SRCCOPY);

	// Gather info
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = winRect.right;
	bmi.bmiHeader.biHeight = winRect.bottom;
	bmi.bmiHeader.biBitCount = 24;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 32 * winRect.right * winRect.bottom / 8;

	// Time to do some saving
	BYTE *pbBits = 0;
	try {
		pbBits = new BYTE[bmi.bmiHeader.biSizeImage];
	} catch( std::bad_alloc &ba ) { badAllocation(); }

	GetDIBits(tmpDc, hBmp, 0, bmi.bmiHeader.biHeight, pbBits, &bmi, DIB_RGB_COLORS);

	BITMAPFILEHEADER bfh;
	bfh.bfType = ('M' << 8) + 'B'; // BM header for bitmaps, always
	bfh.bfSize = sizeof(BITMAPFILEHEADER) + bmi.bmiHeader.biSizeImage +
		sizeof(BITMAPINFOHEADER);
	bfh.bfReserved1 = 0;
	bfh.bfReserved2 = 0;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	HANDLE file = CreateFile(filename, GENERIC_WRITE, 0, 0,
		OPEN_ALWAYS, 0, 0);
	DWORD dwWritten;
	WriteFile(file, &bfh, sizeof(bfh), &dwWritten, NULL);
	WriteFile(file, &bmi.bmiHeader, sizeof(BITMAPINFOHEADER), &dwWritten, NULL);
	WriteFile(file, pbBits, bmi.bmiHeader.biSizeImage, &dwWritten, NULL);
	CloseHandle(file);

	// Release resources
	DeleteDC(tmpDc);
	ReleaseDC(hwnd, hdc);
	DeleteObject(hBmp);
	delete []pbBits;

	return 0;
}

// Return the main application's HWND
int Window_lua::getAppHwnd(lua_State *L)
{
	lua_pushinteger(L, (size_t)Macro::instance()->getAppHwnd());
	return 1;
}

// Return the main focused HWND
int Window_lua::getFocusHwnd(lua_State *L)
{
	lua_pushinteger(L, (size_t)Macro::instance()->getForegroundWindow());
	return 1;
}

int Window_lua::makeColor(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);

	int cr	=	(int)lua_tointeger(L, 1);
	int cg	=	(int)lua_tointeger(L, 2);
	int cb	=	(int)lua_tointeger(L, 3);

	int col = (cr << 16) | (cg << 8) | cb;
	lua_pushinteger(L, col);
	return 1;
}

int Window_lua::drawLine(lua_State *L)
{
	int top = lua_gettop(L);
	if( top < 5 || top > 7 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1); // hwnd
	checkType(L, LT_NUMBER, 2); // x1
	checkType(L, LT_NUMBER, 3); // y1
	checkType(L, LT_NUMBER, 4); // x2
	checkType(L, LT_NUMBER, 5); // y2

	if( top >= 6 )
		checkType(L, LT_NUMBER, 6); // color

	if( top >= 7 )
		checkType(L, LT_NUMBER, 7); // thickness

	RECT winRect;
	RECT clientRect;
	HDC tmpDc;
	HPEN pen;
	LOGBRUSH lb;
	POINT offset;

	HWND hwnd		=	(HWND)lua_tointeger(L, 1);
	int x1			=	(int)lua_tointeger(L, 2);
	int y1			=	(int)lua_tointeger(L, 3);
	int x2			=	(int)lua_tointeger(L, 4);
	int y2			=	(int)lua_tointeger(L, 5);
	int color		=	0;
	int thickness	=	1;

	if( top >= 6 )
		color		=	(int)lua_tointeger(L, 6);

	if( top >= 7 )
		thickness	=	(int)lua_tointeger(L, 7);


	// Figure out the client offset
	winRect.left = 0; winRect.top = 0;
	offset.x = 0; offset.y = 0;

	GetWindowRect(hwnd, &winRect);
	ClientToScreen(hwnd, &offset);
	GetClientRect(hwnd, &clientRect);

	tmpDc			=	GetDC(NULL); // Get desktop DC
	lb.lbStyle		=	BS_SOLID;
	lb.lbColor		=	RGB( getR(color), getG(color), getB(color) );
	lb.lbHatch		=	0;
	pen				=	ExtCreatePen(PS_GEOMETRIC | PS_SOLID, thickness, &lb, 0, NULL);

	SelectObject(tmpDc, pen);

	MoveToEx(tmpDc, offset.x + x1, offset.y + y1, (LPPOINT) NULL);
	LineTo(tmpDc, offset.x + x2, offset.y + y2);

	DeleteObject(pen);
	ReleaseDC(NULL, tmpDc);

	lua_pushboolean(L, true);
	return 1;
}

int Window_lua::drawRect(lua_State *L)
{
	int top = lua_gettop(L);
	if( top < 5 || top > 7 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1); // hwnd
	checkType(L, LT_NUMBER, 2); // x1
	checkType(L, LT_NUMBER, 3); // y1
	checkType(L, LT_NUMBER, 4); // x2
	checkType(L, LT_NUMBER, 5); // y2

	if( top >= 6 )
		checkType(L, LT_NUMBER, 6); // color

	if( top >= 7 )
		checkType(L, LT_NUMBER, 7); // thickness

	RECT winRect;
	RECT clientRect;
	HDC tmpDc;
	HPEN pen;
	LOGBRUSH lb;
	POINT offset;

	HWND hwnd		=	(HWND)lua_tointeger(L, 1);
	int x1			=	(int)lua_tointeger(L, 2);
	int y1			=	(int)lua_tointeger(L, 3);
	int x2			=	(int)lua_tointeger(L, 4);
	int y2			=	(int)lua_tointeger(L, 5);
	int color		=	0;
	int thickness	=	1;

	if( top >= 6 )
		color		=	(int)lua_tointeger(L, 6);

	if( top >= 7 )
		thickness	=	(int)lua_tointeger(L, 7);


	// Figure out the client offset
	winRect.left = 0; winRect.top = 0;
	offset.x = 0; offset.y = 0;

	GetWindowRect(hwnd, &winRect);
	ClientToScreen(hwnd, &offset);
	GetClientRect(hwnd, &clientRect);

	tmpDc			=	GetDC(NULL); // Get desktop DC
	lb.lbStyle		=	BS_SOLID;
	lb.lbColor		=	RGB( getR(color), getG(color), getB(color) );
	lb.lbHatch		=	0;
	pen				=	ExtCreatePen(PS_GEOMETRIC | PS_SOLID, thickness, &lb, 0, NULL);

	SelectObject(tmpDc, pen);

	MoveToEx(tmpDc, offset.x + x1, offset.y + y1, (LPPOINT) NULL);
	LineTo(tmpDc, offset.x + x1, offset.y + y2); // Draw down
	LineTo(tmpDc, offset.x + x2, offset.y + y2); // Draw across
	LineTo(tmpDc, offset.x + x2, offset.y + y1); // Draw up
	LineTo(tmpDc, offset.x + x1, offset.y + y1); // Draw across

	DeleteObject(pen);
	ReleaseDC(NULL, tmpDc);

	lua_pushboolean(L, true);
	return 1;
}
