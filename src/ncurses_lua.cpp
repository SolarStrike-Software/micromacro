/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "ncurses_lua.h"
#include "event.h"
#include "macro.h"
#include "strl.h"
#include "luatypes.h"
#include <algorithm>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

bool Ncurses_lua::initialized = false;
const char *Ncurses_lua::stdscr_name = "STDSCR";

int Ncurses_lua::is_initialized()
{
	return initialized;
}

int Ncurses_lua::regmod(lua_State *L)
{
	static const luaL_Reg _funcs[] = {
		{"init", Ncurses_lua::init},
		{"cleanup", Ncurses_lua::cleanup},
		{"print", Ncurses_lua::print},
		{"refresh", Ncurses_lua::refresh},
		{"scrollok", Ncurses_lua::scrollok},
		{"clear", Ncurses_lua::clear},
		{"move", Ncurses_lua::move},
		{"createWindow", Ncurses_lua::createWindow},
		{"getString", Ncurses_lua::getString},
		{"setPair", Ncurses_lua::setPair},
		{"getPair", Ncurses_lua::getPair},
		{"attributeOn", Ncurses_lua::attributeOn},
		{"attributeOff", Ncurses_lua::attributeOff},
		{"setAttribute", Ncurses_lua::setAttribute},
		{"setBackground", Ncurses_lua::setBackground},
		{"getWindowSize", Ncurses_lua::getWindowSize},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);
	lua_setglobal(L, NCURSES_MODULE_NAME);

	/* Set global vars */
	// Colors
	lua_pushnumber(L, COLOR_BLACK);
	lua_setglobal(L, "COLOR_BLACK");
	lua_pushnumber(L, COLOR_RED);
	lua_setglobal(L, "COLOR_RED");
	lua_pushnumber(L, COLOR_GREEN);
	lua_setglobal(L, "COLOR_GREEN");
	lua_pushnumber(L, COLOR_YELLOW);
	lua_setglobal(L, "COLOR_YELLOW");
	lua_pushnumber(L, COLOR_BLUE);
	lua_setglobal(L, "COLOR_BLUE");
	lua_pushnumber(L, COLOR_MAGENTA);
	lua_setglobal(L, "COLOR_MAGENTA");
	lua_pushnumber(L, COLOR_CYAN);
	lua_setglobal(L, "COLOR_CYAN");
	lua_pushnumber(L, COLOR_WHITE);
	lua_setglobal(L, "COLOR_WHITE");

	// Styles
	lua_pushnumber(L, A_NORMAL);
	lua_setglobal(L, "A_NORMAL");
	lua_pushnumber(L, A_STANDOUT);
	lua_setglobal(L, "A_STANDOUT");
	lua_pushnumber(L, A_UNDERLINE);
	lua_setglobal(L, "A_UNDERLINE");
	lua_pushnumber(L, A_REVERSE);
	lua_setglobal(L, "A_REVERSE");
	lua_pushnumber(L, A_BLINK);
	lua_setglobal(L, "A_BLINK");
	lua_pushnumber(L, A_DIM);
	lua_setglobal(L, "A_DIM");
	lua_pushnumber(L, A_BOLD);
	lua_setglobal(L, "A_BOLD");
	lua_pushnumber(L, A_PROTECT);
	lua_setglobal(L, "A_PROTECT");
	lua_pushnumber(L, A_INVIS);
	lua_setglobal(L, "A_INVIS");
	lua_pushnumber(L, A_ALTCHARSET);
	lua_setglobal(L, "A_ALTCHARSET");
	lua_pushnumber(L, A_CHARTEXT);
	lua_setglobal(L, "A_CHARTEXT");

	return MicroMacro::ERR_OK;
}

int Ncurses_lua::init(lua_State *L)
{
	::initscr();
	::cbreak();
	::noecho();
	::keypad(::stdscr, true);
	::leaveok(::stdscr, true);
	::start_color();

	// Move/hide cursor
	int y,x;
	getmaxyx(::stdscr, y, x);
	mvcur(0, 0, y-1, x-1);
	curs_set(0);
	//hideCursor();

	// Forecefully clear the screen, reset style
	wbkgd(::stdscr, A_NORMAL);
	wattrset(::stdscr, A_NORMAL);
	wclear(::stdscr);
	wrefresh(::stdscr);

	// Create a new window, put it on the Lua stack
	WINDOW **w = (WINDOW **)lua_newuserdata(L, sizeof(WINDOW *));
	*w = ::stdscr;

	// Set metatable
	luaL_getmetatable(L, LuaType::metatable_ncursesWindow);
	lua_setmetatable(L, -2);

	// Set Lua var (global namespace)
	lua_setglobal(L, Ncurses_lua::stdscr_name);

	initialized = true;
	return 0;
}

int Ncurses_lua::cleanup(lua_State *L)
{
	// Reset text/BG style
	wbkgd(::stdscr, A_NORMAL);
	wattrset(::stdscr, A_NORMAL);

	// Clear stdscr
	werase(::stdscr);
	wrefresh(::stdscr);

	// Close ncurses
	endwin();

	// Unset vars
	lua_getglobal(L, NCURSES_MODULE_NAME);
	lua_pushnil(L);
	lua_setfield(L, -2, Ncurses_lua::stdscr_name);
	lua_pop(L, 1);

	initialized = false;
	return 0;
}

/*	ncurses.print(window win, string str)
	Returns:	nil

	Prints 'str' onto window 'win' at current cursor position.
	Remember to refresh the screen afterwards!
*/
int Ncurses_lua::print(lua_State *L)
{
	if( lua_gettop(L) < 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_STRING, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	wprintw(*pw, lua_tostring(L, 2));
	return 0;
}

/*	ncurses.refresh(window win)
	Returns:	nil

	Redraw the given window to commit our changes.
*/
int Ncurses_lua::refresh(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	wrefresh(*pw);
	return 0;
}

/*	ncurses.scrollok(window win, boolean scrolling)
	Returns:	nil

	Sets (or unsets) scrolling on the given window.
*/
int Ncurses_lua::scrollok(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_BOOLEAN, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	::scrollok(*pw, (bool)lua_toboolean(L, 2));
	return 0;
}

/*	ncurses.clear(window win)
	Returns:	nil

	Erase the contents of window 'win'.
*/
int Ncurses_lua::clear(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	werase(*pw);
	return 0;
}

/*	ncurses.move(window win, number y, number x)
	Returns:	nil

	Moves the position of the cursor to y,x in the given window.
*/
int Ncurses_lua::move(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	int y = (int)lua_tonumber(L, 2);
	int x = (int)lua_tonumber(L, 3);
	wmove(*pw, y, x);
	wrefresh(*pw);

	return 0;
}

/*	ncurses.createWindow(number sy, number sx, number width, number height)
	Returns (on success):	userdata (window)
	Returns (on failure):	nil

	Creates a new Ncurses window and returns it.
	'sy' and 'sx' indicate the top-left corner of the window (in characters).
	'width' and 'height' represent the window's width/height in characters.
*/
int Ncurses_lua::createWindow(lua_State *L)
{
	if( lua_gettop(L) != 4 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);
	checkType(L, LT_NUMBER, 4);

	int height, width, starty, startx;
	WINDOW **pw;
	starty = (int)lua_tonumber(L, 1);
	startx = (int)lua_tonumber(L, 2);
	height = (int)lua_tonumber(L, 3);
	width = (int)lua_tonumber(L, 4);

	WINDOW *nWin = newwin(height, width, starty, startx);
	if( nWin == NULL )
	{ // Throw an error
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		const char *script = ar.short_src;

		char buffer[2048];
		slprintf(buffer, sizeof(buffer)-1, "Error creating Ncurses window. %s:%d", script, line);

		Event e;
		e.type = EVENT_ERROR;
		e.msg = buffer;
		Macro::instance()->getEventQueue()->push(e);

		return 0;
	}

	// If it worked, we can create a userdata and set it up
	pw = (WINDOW **)lua_newuserdata(L, sizeof(WINDOW *));
	*pw = nWin;
	leaveok(*pw, true);
	keypad(*pw, true);

	// Set metatable
	luaL_getmetatable(L, LuaType::metatable_ncursesWindow);
	lua_setmetatable(L, -2);

	return 1;
}

/* NOTE: This seems broken in this version of Ncurses... disabled for now
void Ncurses_lua::hideCursor()
{
	CONSOLE_CURSOR_INFO cci;
	cci.dwSize = 50;
	cci.bVisible = false;

	int s = SetConsoleCursorInfo(Macro::instance()->getAppHandle(), &cci);
	curs_set(0);
}

void Ncurses_lua::showCursor()
{
	CONSOLE_CURSOR_INFO cci;
	cci.dwSize = 1;
	cci.bVisible = true;

	SetConsoleCursorInfo(Macro::instance()->getAppHandle(), &cci);
	curs_set(1);
}
*/

/*	NOTE: Not exposed to Lua
	Discards buffered keyboard input for given window
*/
void Ncurses_lua::flush(WINDOW *pw)
{
	::flushinp(); // Flush Ncurses input
	FlushConsoleInputBuffer(Macro::instance()->getAppHandle()); // Flush Windows' input

	// Because for some reason there's still crap in the buffers...
	// We manually read it till it's empty.
	// Set to nodelay so we can catch its emptiness instead of freeze
	nodelay(pw, true);
	while( wgetch(pw) != ERR ) { }
	nodelay(pw, false);
}

/* NOTE: Not exposed to Lua
	This is essentially a non-broken getline() for a given
	Ncurses window.
*/
void Ncurses_lua::readline(WINDOW *pw, char *buffer, size_t bufflen)
{
	unsigned int pos = 0;
	unsigned int len = 0;
	unsigned int outstart = 0;
	unsigned int outwidth = 0;
	int x, y, mx, my;

	memset(buffer, 0, bufflen);
	getyx(pw, y, x);
	getmaxyx(pw, my, mx);
	outwidth = mx - x - 1;

	bool scrolling = pw->_scroll;

	if( scrolling ) // Temp disable
		::scrollok(pw, false);

	while(true)
	{
		int c; // Input character

		if( pos < outstart )
			outstart = (pos >= 5 ? pos - 5 : 0); // Just a nice touch when we're moving left
		else if( pos > (outstart + outwidth) )
			outstart = pos - outwidth;
		if( outstart > len )
			outstart = len;

		for(unsigned int i = 0; i <= outwidth; i++)
			mvwaddch(pw, y, x+i, ' '); // Erase garbage
		mvwaddnstr(pw, y, x, buffer + outstart, (len < outwidth ? len : outwidth)); // len-outstart ?
		wmove(pw, y, x + (pos - outstart));

		wrefresh(pw);
		c = wgetch(pw);

		if( c == KEY_ENTER || c == '\n' || c == '\r' )
			break;
		else if( c == KEY_LEFT )
			if( pos > 0 ) --pos; else beep();
		else if( c == KEY_RIGHT )
			if( pos < len ) ++pos; else beep();
		else if( c == VK_BACK || c == KEY_BACKSPACE ) // Remove left char
			if( pos > 0 ) {
				memmove(buffer+pos-1, buffer+pos, len-pos);
				buffer[len] = '\0';
				--pos; --len;
			}
			else beep();
		else if( c == KEY_DC ) // Remove pos char
			if( pos <= len && len > 0 ) {
				memmove(buffer+pos, buffer+pos+1, len-pos);
				buffer[len] = '\0';
				--len;
			} else beep();
		else if( c >= ' ' && c <= '~' ) // Make sure it is even printable
		{
			if( pos + 2 < bufflen ) // Make sure we're not overflowing
			{
				if( pos == len ) // Append character
				{
					buffer[pos] = c;
					buffer[len+1] = 0; // NULL-terminator
					++pos; ++len;
				}
				else // Insert character
				{
					memmove(buffer+pos+1, buffer+pos, len-pos); // Shift characters right
					buffer[pos] = c; // Insert new character
					++pos; ++len;
				}
			}
			else beep();
		}
	}

	if( scrolling ) // Re-enable
		::scrollok(pw, true);

	buffer[len] = 0; // Ensure NULL terminator
}

/*	ncurses.getString(window win)
	Returns:	string

	Prompt the user for some input on an Ncurses window.
	Returns that input as a string value.

	NOTE: This function is blocking!
*/
int Ncurses_lua::getString(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	char buffer[1024];
	int sx, sy, nx, ny;

	// Flush input, prep window for input
	flush(*pw);

	leaveok(*pw, false);
	getbegyx(*pw, sy, sx);
	getyx(*pw, ny, nx);
	mvcur(0, 0, sy+ny, sx+nx);

	// Do input
	readline(*pw, buffer, sizeof(buffer)-1);

	// Output a newline, but we don't need to refresh
	wprintw(*pw, "\n");

	// Change back for output mode again
	int y,x;
	getmaxyx(::stdscr, y, x);
	mvcur(0, 0, y-1, x-1);
	leaveok(*pw, true);

	lua_pushstring(L, buffer);
	return 1;
}

/* NOTE: Not supported on Windows console
int Ncurses_lua::setColor(lua_State *L)
{
	if( lua_gettop(L) != 4 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);
	checkType(L, LT_NUMBER, 4);

	short colIndex = (short)lua_tonumber(L, 1);
	int colR = (int)lua_tonumber(L, 2);
	int colG = (int)lua_tonumber(L, 3);
	int colB = (int)lua_tonumber(L, 4);

	return 0;
}
*/

/*	ncurses.setPair(number pairIndex, number foregroundColor, number backgroundColor)
	Returns:	nil

	Modified the color (foreground/background) pair at a given index.
*/
int Ncurses_lua::setPair(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);

	int pairIndex = (int)lua_tonumber(L, 1);
	int foreground = (int)lua_tonumber(L, 2);
	int background = (int)lua_tonumber(L, 3);

	// Ensure index is not out of bounds
	if( pairIndex < 1 || pairIndex >= COLOR_PAIRS )
		return 0;

	// Constrain colors
	foreground = std::max(0, foreground);
	foreground = std::min(foreground, COLORS);
	background = std::max(0, background);
	background = std::min(background, COLORS);

	// Do it!
	init_pair(pairIndex, foreground, background);

	return 0;
}

/*	ncurses.getPair(number pairIndex)
	Returns:	number (foreground/background pair)

	Returns the attribute mask of a given color pair.
*/
int Ncurses_lua::getPair(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_NUMBER, 1);

	int pairIndex = (int)lua_tonumber(L, 1);

	// Ensure index is not out of bounds
	if( pairIndex < 0 || pairIndex >= COLOR_PAIRS )
		return 0;

	lua_pushnumber(L, COLOR_PAIR(pairIndex));
	return 1;
}

/*	ncurses.attributeOn(window win, number attrib)
	Returns:	nil

	Turns an attribute (identified by 'attrib') on for the given window.
*/
int Ncurses_lua::attributeOn(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);
	int attribValue = (int)lua_tonumber(L, 2);
	wattron(*pw, attribValue);

	return 0;
}


/*	ncurses.attributeOff(window win, number attrib)
	Returns:	nil

	Turns an attribute (identified by 'attrib') off for the given window.
*/
int Ncurses_lua::attributeOff(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);
	int attribValue = (int)lua_tonumber(L, 2);
	wattroff(*pw, attribValue);

	return 0;
}


/*	ncurses.setAttribute(window win, number attrib)
	Returns:	nil

	Sets an attribute (identified by 'attrib') on the given window.
*/
int Ncurses_lua::setAttribute(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, /*LT_NIL |*/ LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);
	int attribValue = (int)lua_tonumber(L, 2);
	wattrset(*pw, attribValue);

	return 0;
}


/*	ncurses.setBackground(window win, number attrib)
	Returns:	nil

	Sets an background on the given window to the attribute mask.
*/
int Ncurses_lua::setBackground(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	int style = (int)lua_tonumber(L, 2);
	attr_t attribs = style;
	wbkgd(*pw, attribs);
	return 0;
}


/*	ncurses.getWindowSize(window win)
	Returns:	number y
				number x

	Returns the size of a window in characters.
*/
int Ncurses_lua::getWindowSize(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	int y, x;
	getmaxyx(*pw, y, x);

	lua_pushnumber(L, y);
	lua_pushnumber(L, x);

	return 2;
}
