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

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

const char *windowThumbnailClassName = "ThumbnailClass";

BOOL CALLBACK Window_lua::_findProc(HWND hwnd, LPARAM lparam)
{
	EnumWindowPair *winpair = (EnumWindowPair *)lparam;
	char namestring[2048];
	GetWindowText(hwnd, (char *)&namestring, sizeof(namestring)-1);

	sztolower(namestring, namestring, strlen(namestring));
	int match = wildfind(winpair->windowname, namestring);

	if( match )
	{
		// Looking for just the window, not a specific classname
		if( strcmp(winpair->classname, "") == 0 )
		{
			// Ensure that this isn't a window preview/overlay
			char tmpBuf[256];
			GetClassName(hwnd, (char*)&tmpBuf, sizeof(tmpBuf)-1);

			if( !strcmp((char*)&tmpBuf, windowThumbnailClassName))
			return true;

			winpair->hwnd = hwnd;
			return false;
		}
		else
		{
			// Check if this window is valid itself
			char tmpBuf[256];
			GetClassName(hwnd, (char*)&tmpBuf, sizeof(tmpBuf)-1);

			if( strcmp(tmpBuf, winpair->classname) == 0 )
			{
				// We have a match
				winpair->hwnd = hwnd;
				return false;
			}

			// If not, scan it's children
			HWND controlHwnd = FindWindowEx(hwnd, NULL, winpair->classname, NULL);

			if( controlHwnd == NULL )
				return true;

			// We have a match
			winpair->hwnd = controlHwnd;
			return false;
		}
	}
	else
		return true;
}

BOOL CALLBACK Window_lua::_findListProc(HWND hwnd, LPARAM lparam)
{
	EnumWindowListPair *winpair = (EnumWindowListPair *)lparam;
	char namestring[2048];
	GetWindowText(hwnd, (char *)&namestring, sizeof(namestring)-1);

	sztolower(namestring, namestring, strlen(namestring));
	int match = wildfind(winpair->windowname, namestring);

	if( match )
	{
		// Looking for just the window, not a specific classname
		if( strcmp(winpair->classname, "") == 0 )
		{
			// Ensure that this isn't a window preview/overlay
			char tmpBuf[256];
			GetClassName(hwnd, (char*)&tmpBuf, sizeof(tmpBuf)-1);

			if( !strcmp((char*)&tmpBuf, windowThumbnailClassName))
			return true;

			winpair->hwndList.push_back(hwnd);
		}
		else
		{
			// Check if this window is valid itself
			char tmpBuf[256];
			GetClassName(hwnd, (char*)&tmpBuf, sizeof(tmpBuf)-1);

			if( strcmp(tmpBuf, winpair->classname) == 0 )
			{
				// We have a match
				winpair->hwndList.push_back(hwnd);
			}

			// If not, scan it's children
			HWND controlHwnd = FindWindowEx(hwnd, NULL, winpair->classname, NULL);

			if( controlHwnd != NULL )
				winpair->hwndList.push_back(controlHwnd);
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
		{"getClassName", Window_lua::getClassName},
		{"valid", Window_lua::valid},
		{"getRect", Window_lua::getRect},
		{"setRect", Window_lua::setRect},
		{"show", Window_lua::show},
		//{"openDC", Window_lua::openDC},
		//{"closeDC", Window_lua::closeDC},
		{"flash", Window_lua::flash},
		{"getPixel", Window_lua::getPixel},
		{"pixelSearch", Window_lua::pixelSearch},
		{"saveScreenshot", Window_lua::saveScreenshot},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, WINDOW_MODULE_NAME);

	lua_pushinteger(L, SW_FORCEMINIMIZE);
	lua_setglobal(L, "SW_FORCEMINIMIZE");
	lua_pushinteger(L, SW_HIDE);
	lua_setglobal(L, "SW_HIDE");
	lua_pushinteger(L, SW_MAXIMIZE);
	lua_setglobal(L, "SW_MAXIMIZE");
	lua_pushinteger(L, SW_MINIMIZE);
	lua_setglobal(L, "SW_MINIMIZE");
	lua_pushinteger(L, SW_RESTORE);
	lua_setglobal(L, "SW_RESTORE");
	lua_pushinteger(L, SW_SHOW);
	lua_setglobal(L, "SW_SHOW");
	lua_pushinteger(L, SW_SHOWDEFAULT);
	lua_setglobal(L, "SW_SHOWDEFAULT");
	lua_pushinteger(L, SW_SHOWMAXIMIZED);
	lua_setglobal(L, "SW_SHOWMAXIMIZED");
	lua_pushinteger(L, SW_SHOWMINIMIZED);
	lua_setglobal(L, "SW_SHOWMINIMIZED");
	lua_pushinteger(L, SW_SHOWMINNOACTIVE);
	lua_setglobal(L, "SW_SHOWMINNOACTIVE");
	lua_pushinteger(L, SW_SHOWNA);
	lua_setglobal(L, "SW_SHOWNA");
	lua_pushinteger(L, SW_SHOWNOACTIVATE);
	lua_setglobal(L, "SW_SHOWNOACTIVATE");
	lua_pushinteger(L, SW_SHOWNORMAL);
	lua_setglobal(L, "SW_SHOWNORMAL");
	return MicroMacro::ERR_OK;
}

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
	//sztolower(classname_lower, classname, classnameLen);

	EnumWindowPair searchpair;
	searchpair.windowname = name_lower;
	searchpair.classname = (char*)classname;//classname_lower;
	searchpair.hwnd = 0;

	EnumWindows(_findProc, (LPARAM)&searchpair);

	// Free allocated memory
	delete []name_lower;
	//delete []classname_lower;

	if( searchpair.hwnd == 0 )
	{ // Throw warning
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Failure to find window. %s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_WARNING;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
	}

	lua_pushinteger(L, (unsigned int)searchpair.hwnd);
	return 1;
}

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

	EnumWindowListPair searchpair;
	searchpair.windowname = name_lower;
	searchpair.classname = (char*)classname;

	EnumWindows(_findListProc, (LPARAM)&searchpair);

	// Free allocated memory
	delete []name_lower;

	if( searchpair.hwndList.empty() )
		return 0;

	lua_newtable(L);
	for(unsigned int i = 0; i < searchpair.hwndList.size(); i++)
	{
		lua_pushinteger(L, i+1); // Push key
		lua_pushinteger(L, (unsigned int)searchpair.hwndList.at(i)); // Push value
		lua_settable(L, -3); // Set it
	}

	return 1;
}

int Window_lua::getParent(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);
	lua_pushinteger(L, (int)::GetParent(hwnd));
	return 1;
}

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
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Failure to get window title. %s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
		return 0;
	}

	lua_pushstring(L, buffer);
	return 1;
}

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
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Failure to set window title. %s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
	}

	lua_pushboolean(L, success);
	return 1;
}

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
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Failure to get window class. %s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
		return 0;
	}

	lua_pushstring(L, buffer);
	return 1;
}

int Window_lua::valid(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

	lua_pushboolean(L, ::IsWindow(hwnd));
	return 1;
}

int Window_lua::getRect(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);
	RECT rect;
	POINT point;
	point.x = 0; point.y = 0;

	// Translate coords, get rect
	ClientToScreen(hwnd, &point);
	GetClientRect(hwnd, &rect);

	// Push results
	lua_pushnumber(L, point.x);
	lua_pushnumber(L, point.y);
	lua_pushnumber(L, rect.right);
	lua_pushnumber(L, rect.bottom);

	return 4;
}

int Window_lua::show(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);

	HWND hwnd = (HWND)lua_tointeger(L, 1);
	int cmd = lua_tointeger(L, 2);

	ShowWindowAsync(hwnd, cmd);
	SetForegroundWindow(hwnd);

	if( cmd == SW_SHOW || cmd == SW_SHOWNORMAL || cmd == SW_RESTORE )
	{
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
/*
int Window_lua::openDC(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	HWND hwnd = (HWND)lua_tointeger(L, 1);

	HDC hdc = ::GetWindowDC(hwnd);
	if( hdc == NULL )
	{ // An error occurred
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Failure to open window device context. %s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
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
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Failure to open DC to desktop. %s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
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

int Window_lua::pixelSearch(lua_State *L)
{
	if( lua_gettop(L) != 10 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1); // HWND
	checkType(L, LT_NUMBER, 2); // R
	checkType(L, LT_NUMBER, 3); // G
	checkType(L, LT_NUMBER, 4); // B
	checkType(L, LT_NUMBER, 5); // x1
	checkType(L, LT_NUMBER, 6); // y1
	checkType(L, LT_NUMBER, 7); // x2
	checkType(L, LT_NUMBER, 8); // y2
	checkType(L, LT_NUMBER, 9); // accuracy
	checkType(L, LT_NUMBER, 10); // step

	POINT retval;
	POINT offset;
	RECT winRect;
	RECT clientRect;
	int r1, g1, b1, r2, g2, b2;
	int x1, y1, x2, y2;
	int step;
	int accuracy;
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
	accuracy = lua_tointeger(L, 9);
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

		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Unable to grab screenshot of target window. %s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
		return 0;
	}

	// Get info from the screenshot
	SelectObject(tmpHdc, hBmp);
	int pw = PrintWindow(hwnd, tmpHdc, PW_CLIENTONLY);
	int biWidth = clientRect.right - clientRect.left;
	int biHeight = clientRect.bottom - clientRect.top;
	BITMAPINFO bmpInfo;
	bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpInfo.bmiHeader.biWidth = biWidth;
	bmpInfo.bmiHeader.biHeight = biHeight;
	bmpInfo.bmiHeader.biPlanes = 1;
	bmpInfo.bmiHeader.biBitCount = 32;
	bmpInfo.bmiHeader.biCompression = BI_RGB;
	bmpInfo.bmiHeader.biSizeImage = 0;

	BITMAP obmp;
	GetObject(hBmp, sizeof(obmp), &obmp);
	RGBQUAD *_pixels = new RGBQUAD[(biHeight+1)*biWidth];
	int scanlines = GetDIBits(tmpHdc, hBmp, 0, biHeight, _pixels, &bmpInfo, DIB_RGB_COLORS);

	if( pw == 0 || scanlines == 0 || scanlines == ERROR_INVALID_PARAMETER )
	{ // Throw error
		DeleteDC(tmpHdc);
		DeleteObject(hBmp);
		ReleaseDC(NULL, hdcScreen);
		delete []_pixels;

		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		int errCode = GetLastError();
		char buffer[4096];
		slprintf(buffer, sizeof(buffer)-1,
			"Received invalid data or unable to allocate memory while reading image data. "
			"%s:%d, Error code %i (%s)",
			script, line, errCode, getWindowsErrorString(errCode).c_str());

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);
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

			if( x > (offset.x + clientRect.right-clientRect.left) )
			{
				x = 0;
				y++;
				continue;
			}
			if( y > (clientRect.bottom-clientRect.top) )
			{
				i = steps_y;
				break;
			}

			RGBQUAD rgba = _pixels[(biHeight-(y+1))*biWidth + x];

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
	RECT winrect;
	GetWindowRect(hwnd, &winrect);
	unsigned int flags = SWP_ASYNCWINDOWPOS|SWP_SHOWWINDOW;
	//int x = winrect.left, y = winrect.top, w = winrect.right, h = winrect.bottom;

	int origLeft = winrect.left;
	int origTop = winrect.top;

	winrect.left = lua_tointeger(L, 2);
	winrect.top = lua_tointeger(L, 3);
	if( lua_isnumber(L, 4) )
		winrect.right = lua_tointeger(L, 4);
	else
		winrect.right = winrect.right - origLeft;
	if( lua_isnumber(L, 5) )
		winrect.bottom = lua_tointeger(L, 5);
	else
		winrect.bottom = winrect.bottom - origTop;

	SetWindowPos(hwnd, HWND_NOTOPMOST, winrect.left, winrect.top, winrect.right, winrect.bottom, flags);

	return 0;
}
