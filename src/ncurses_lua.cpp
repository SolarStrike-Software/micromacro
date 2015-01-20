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

#include "global_addon.h"

#include <algorithm>

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

bool Ncurses_lua::initialized = false;
const char *Ncurses_lua::stdscr_name = "STDSCR";
unsigned int Ncurses_lua::historyIndex = 0;
std::vector<std::string> Ncurses_lua::history;

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
		{"resizeWindow", Ncurses_lua::resizeWindow},
		{"moveWindow", Ncurses_lua::moveWindow},
		{"getString", Ncurses_lua::getString},
		{"setPair", Ncurses_lua::setPair},
		{"getPair", Ncurses_lua::getPair},
		{"attributeOn", Ncurses_lua::attributeOn},
		{"attributeOff", Ncurses_lua::attributeOff},
		{"setAttribute", Ncurses_lua::setAttribute},
		{"getAttribute", Ncurses_lua::getAttribute},
		{"setBackground", Ncurses_lua::setBackground},
		{"getWindowSize", Ncurses_lua::getWindowSize},
		{NULL, NULL}
	};

	luaL_newlib(L, _funcs);

	/* Set global vars */
	// Colors
	lua_pushinteger(L, COLOR_BLACK);
	lua_setfield(L, -2, "BLACK");
	lua_pushinteger(L, COLOR_RED);
	lua_setfield(L, -2, "RED");
	lua_pushinteger(L, COLOR_GREEN);
	lua_setfield(L, -2, "GREEN");
	lua_pushinteger(L, COLOR_YELLOW);
	lua_setfield(L, -2, "YELLOW");
	lua_pushinteger(L, COLOR_BLUE);
	lua_setfield(L, -2, "BLUE");
	lua_pushinteger(L, COLOR_MAGENTA);
	lua_setfield(L, -2, "MAGENTA");
	lua_pushinteger(L, COLOR_CYAN);
	lua_setfield(L, -2, "CYAN");
	lua_pushinteger(L, COLOR_WHITE);
	lua_setfield(L, -2, "WHITE");

	// Styles
	lua_pushinteger(L, A_NORMAL);
	lua_setfield(L, -2, "NORMAL");
	lua_pushinteger(L, A_STANDOUT);
	lua_setfield(L, -2, "STANDOUT");
	lua_pushinteger(L, A_UNDERLINE);
	lua_setfield(L, -2, "UNDERLINE");
	lua_pushinteger(L, A_REVERSE);
	lua_setfield(L, -2, "REVERSE");
	lua_pushinteger(L, A_BLINK);
	lua_setfield(L, -2, "BLINK");
	lua_pushinteger(L, A_DIM);
	lua_setfield(L, -2, "DIM");
	lua_pushinteger(L, A_BOLD);
	lua_setfield(L, -2, "BOLD");
	lua_pushinteger(L, A_PROTECT);
	lua_setfield(L, -2, "PROTECT");
	lua_pushinteger(L, A_INVIS);
	lua_setfield(L, -2, "INVIS");
	lua_pushinteger(L, A_ALTCHARSET);
	lua_setfield(L, -2, "ALTCHARSET");
	lua_pushinteger(L, A_CHARTEXT);
	lua_setfield(L, -2, "CHARTEXT");

	lua_setglobal(L, NCURSES_MODULE_NAME);

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

	historyIndex = 0;

	/*	Certain mouse events cause blocking issues in non-blocking mode.
		To avoid this, lets only listen to a single event that doesn't
		cause any issues. Note that using 0 for the mask also doesn't help.
	*/
	mousemask(BUTTON1_PRESSED, NULL);

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
	// Clear history
	history.clear();
	historyIndex = 0;

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
	int top = lua_gettop(L);
	if( top < 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_STRING, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	if( top == 2 )
		wprintw(*pw, lua_tostring(L, 2));
	else
	{
		lua_remove(L, 1); // Toss out the Window pointer
		Global_addon::sprintf(L); // Format it, put result on top of stack
		if( lua_isstring(L, -1) ) // If we can, print it
			wprintw(*pw, lua_tostring(L, -1));
	}

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

	int y = lua_tointeger(L, 2);
	int x = lua_tointeger(L, 3);
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
	starty = lua_tointeger(L, 1);
	startx = lua_tointeger(L, 2);
	height = lua_tointeger(L, 3);
	width = lua_tointeger(L, 4);

	WINDOW *nWin = newwin(height, width, starty, startx);
	if( nWin == NULL )
	{ // Throw an error
		pushLuaErrorEvent(L, "Error creating Ncurses window.");
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

/*	ncurses.resizeWindow(window win, number lines, number columns)
	Returns:	boolean

	Resize a window to the given columns/lines.
	Returns true on success, false on failure.
*/
int Ncurses_lua::resizeWindow(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	int lines = lua_tointeger(L, 2);
	int columns = lua_tointeger(L, 3);
	int success = wresize(*pw, lines, columns);

	lua_pushboolean(L, success != ERR);
	return 1;
}

/*	ncurses.moveWindow(window win, number y, number x)
	Returns:	boolean

	Move a window to the given position.
	Returns true on success, false on failure.
*/
int Ncurses_lua::moveWindow(lua_State *L)
{
	if( lua_gettop(L) != 3 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);
	checkType(L, LT_NUMBER, 3);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	int y = lua_tointeger(L, 2);
	int x = lua_tointeger(L, 3);
	int success = mvwin(*pw, y, x);

	lua_pushboolean(L, success != ERR);
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
	FlushConsoleInputBuffer(Macro::instance()->getAppHandle()); // Flush Windows' input
	::flushinp(); // Flush Ncurses input

	/*	Because for some reason there's still crap in the buffers...
		We manually read it till it's empty.
		Set to nodelay so we can catch its emptiness instead of freeze.
	*/
	nodelay(pw, true);
	int c = wgetch(pw);
	while( c != ERR )
	{
		if( c == KEY_MOUSE )
		{
			MEVENT e;
			getmouse(&e);
		}
		c = wgetch(pw);
	}

	nodelay(pw, false);
}

/* NOTE: Not exposed to Lua
	This is essentially a non-broken getline() for a given
	Ncurses window.
*/
void Ncurses_lua::readline(WINDOW *pw, char *buffer, size_t bufflen)
{
	historyIndex = history.size();

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
		else if( c == KEY_UP && historyIndex > 0 )
		{
			--historyIndex;
			std::string str = history.at(historyIndex).c_str();
			strlcpy(buffer, str.c_str(), bufflen);
			len = str.length();
			pos = len;
		}
		else if( c == KEY_DOWN && historyIndex < (history.size() - 1)  )
		{
			++historyIndex;
			std::string str = history.at(historyIndex).c_str();
			strlcpy(buffer, str.c_str(), bufflen);
			len = str.length();
			pos = len;
		}
		else if( c == KEY_HOME )
			pos = 0;
		else if( c == KEY_END )
			pos = len;
		else if( c == VK_BACK || c == KEY_BACKSPACE ) // Remove left char
			if( pos > 0 ) {
				memmove(buffer+pos-1, buffer+pos, len-pos);
				buffer[len] = '\0';
				--pos; --len;
			}
			else beep();
		else if( c == KEY_DC ) // Remove pos char
			if( pos <= len && len > 0 ) {
				if( pos < len )
				{
					memmove(buffer+pos, buffer+pos+1, len-pos);
					buffer[len] = '\0';
					--len;
				}
				if( pos > len )
					pos = len;
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

	// Re-print it, in case the user repositioned the pointer, Ncurses might clear to EOL
	mvwaddnstr(pw, y, x, buffer + outstart, (len < outwidth ? len : outwidth));
	wrefresh(pw);

	if( scrolling ) // Re-enable
		::scrollok(pw, true);

	buffer[len] = 0; // Ensure NULL terminator

	// Remove dropped history
	if( history.size() >= MAX_NCURSES_HISTORY_LINES ) // Don't keep too many
		history.erase(history.begin(), history.begin()+1);

	// Insert history entry
	history.push_back(buffer);
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
	int colR = lua_tointeger(L, 2);
	int colG = lua_tointeger(L, 3);
	int colB = lua_tointeger(L, 4);

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

	int pairIndex = lua_tointeger(L, 1);
	int foreground = lua_tointeger(L, 2);
	int background = lua_tointeger(L, 3);

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

	int pairIndex = lua_tointeger(L, 1);

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
	int attribValue = lua_tointeger(L, 2);
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
	int attribValue = lua_tointeger(L, 2);
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
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);
	int attribValue = lua_tointeger(L, 2);
	wattrset(*pw, attribValue);

	return 0;
}

/*	ncurses.getAttribute(window win)
	Returns (on success):	number attributes
							number pair

	Returns (on failure):	nil

	Returns the attribute mask and color pair for a Ncurses window.
*/
int Ncurses_lua::getAttribute(lua_State *L)
{
	if( lua_gettop(L) != 1 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	attr_t attribs;
	short pair;
	int success = wattr_get(*pw, &attribs, &pair, NULL);

	if( success == ERR )
		return 0;

	lua_pushinteger(L, attribs); // An attr_t is basically just an unsigned int
	lua_pushinteger(L, pair); // Pair is a short, but same deal
	return 2;
}

/*	ncurses.setBackground(window win, number attrib)
	Returns:	nil

	Sets a background on the given window to the attribute mask.
*/
int Ncurses_lua::setBackground(lua_State *L)
{
	if( lua_gettop(L) != 2 )
		wrongArgs(L);
	checkType(L, LT_USERDATA, 1);
	checkType(L, LT_NUMBER, 2);

	WINDOW **pw = NULL;
	pw = (WINDOW **)lua_touserdata(L, 1);

	int style = lua_tointeger(L, 2);
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

void Ncurses_lua::safeDestroy(WINDOW *pWin)
{
	if( pWin != ::stdscr )
		delwin(pWin);
}
