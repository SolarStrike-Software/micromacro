#include "key_lua.h"
#include "error.h"
#include "macro.h"

extern "C"
{
	#include <lua.h>
	#include <lauxlib.h>
	#include <lualib.h>
}

typedef struct VK_NAME_PAIR
{
	const char *name;
	int vk;
} VK_NAME_PAIR;

int Key_lua::regmod(lua_State *L)
{
	// We don't really have any functions to put here right now
	static const luaL_Reg _funcs[] = {
		{NULL, NULL}
	};

	// VK list: http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731%28v=vs.85%29.aspx
	static const VK_NAME_PAIR _vks[] = {
		{"VK_LMOUSE", VK_LBUTTON},
		{"VK_RMOUSE", VK_RBUTTON},
		{"VK_CANCEL", VK_CANCEL},
		{"VK_MMOUSE", VK_MBUTTON},
		{"VK_XMOUSE1", VK_XBUTTON1},
		{"VK_XMOUSE2", VK_XBUTTON2},
		{"VK_BACKSPACE", VK_BACK},
		{"VK_TAB", VK_TAB},
		{"VK_RETURN", VK_RETURN},
		{"VK_ENTER", VK_RETURN},
		{"VK_SHIFT", VK_SHIFT},
		{"VK_CONTROL", VK_CONTROL},
		{"VK_ALT", VK_MENU},
		{"VK_MENU", VK_MENU},
		{"VK_PAUSE", VK_PAUSE},
		{"VK_CAPITAL", VK_CAPITAL},
		{"VK_CAPSLOCK", VK_CAPITAL},
		{"VK_ESCAPE", VK_ESCAPE},
		{"VK_SPACE", VK_SPACE},
		{"VK_PRIOR", VK_PRIOR},
		{"VK_PAGEUP", VK_PRIOR},
		{"VK_NEXT", VK_NEXT},
		{"VK_PAGEDOWN", VK_NEXT},
		{"VK_END", VK_END},
		{"VK_HOME", VK_HOME},
		{"VK_LEFT", VK_LEFT},
		{"VK_UP", VK_UP},
		{"VK_RIGHT", VK_RIGHT},
		{"VK_DOWN", VK_DOWN},
		{"VK_PRINT", VK_PRINT},
		{"VK_PRINTSCREEN", VK_SNAPSHOT},
		{"VK_SNAPSHOT", VK_SNAPSHOT},
		{"VK_INSERT", VK_INSERT},
		{"VK_DELETE", VK_DELETE},
		{"VK_HELP", VK_HELP},
		{"VK_0", 0x30}, // Number row; not numpad
		{"VK_1", 0x31},
		{"VK_2", 0x32},
		{"VK_3", 0x33},
		{"VK_4", 0x34},
		{"VK_5", 0x35},
		{"VK_6", 0x36},
		{"VK_7", 0x37},
		{"VK_8", 0x38},
		{"VK_9", 0x39},
		// 3A - 40 undefined
		{"VK_A", 0x41},
		{"VK_B", 0x42},
		{"VK_C", 0x43},
		{"VK_D", 0x44},
		{"VK_E", 0x45},
		{"VK_F", 0x46},
		{"VK_G", 0x47},
		{"VK_H", 0x48},
		{"VK_I", 0x49},
		{"VK_J", 0x4A},
		{"VK_K", 0x4B},
		{"VK_L", 0x4C},
		{"VK_M", 0x4D},
		{"VK_N", 0x4E},
		{"VK_O", 0x4F},
		{"VK_P", 0x50},
		{"VK_Q", 0x51},
		{"VK_R", 0x52},
		{"VK_S", 0x53},
		{"VK_T", 0x54},
		{"VK_U", 0x55},
		{"VK_V", 0x56},
		{"VK_W", 0x57},
		{"VK_X", 0x58},
		{"VK_Y", 0x59},
		{"VK_Z", 0x5A},
		{"VK_LWINDOWS", VK_LWIN},
		{"VK_RWINDOWS", VK_RWIN},
		{"VK_APPS", VK_APPS},
		{"VK_SLEEP", VK_SLEEP},
		{"VK_NUMPAD0", VK_NUMPAD0}, // Ok, NOW it's numpad time
		{"VK_NUMPAD1", VK_NUMPAD1},
		{"VK_NUMPAD2", VK_NUMPAD2},
		{"VK_NUMPAD3", VK_NUMPAD3},
		{"VK_NUMPAD4", VK_NUMPAD4},
		{"VK_NUMPAD5", VK_NUMPAD5},
		{"VK_NUMPAD6", VK_NUMPAD6},
		{"VK_NUMPAD7", VK_NUMPAD7},
		{"VK_NUMPAD8", VK_NUMPAD8},
		{"VK_NUMPAD9", VK_NUMPAD9},
		{"VK_MULTIPLY", VK_MULTIPLY},
		{"VK_ADD", VK_ADD},
		{"VK_SUBTRACT", VK_SUBTRACT},
		{"VK_DECIMAL", VK_DECIMAL},
		{"VK_DIVIDE", VK_DIVIDE},
		{"VK_F1", VK_F1},
		{"VK_F2", VK_F2},
		{"VK_F3", VK_F3},
		{"VK_F4", VK_F4},
		{"VK_F5", VK_F5},
		{"VK_F6", VK_F6},
		{"VK_F7", VK_F7},
		{"VK_F8", VK_F8},
		{"VK_F9", VK_F9},
		{"VK_F10", VK_F10},
		{"VK_F11", VK_F11},
		{"VK_F12", VK_F12},
		// Fuck VK_F13 - VK_F24, does this even exist?
		{"VK_NUMLOCK", VK_NUMLOCK},
		{"VK_SCROLL", VK_SCROLL},
		{"VK_LSHIFT", VK_LSHIFT},
		{"VK_RSHIFT", VK_RSHIFT},
		{"VK_LCONTROL", VK_LCONTROL},
		{"VK_RCONTROL", VK_RCONTROL},
		{"VK_LMENU", VK_LMENU},
		{"VK_LALT", VK_LMENU},
		{"VK_RMENU", VK_RMENU},
		{"VK_RALT", VK_RMENU},
		{"VK_PLUS", VK_OEM_PLUS},
		{"VK_COMMA", VK_OEM_COMMA},
		{"VK_PERIOD", VK_OEM_PERIOD},
		/*{"JOY_BUTTON1", JOY_BUTTON1},
		{"JOY_BUTTON2", JOY_BUTTON2},
		{"JOY_BUTTON3", JOY_BUTTON3},
		{"JOY_BUTTON4", JOY_BUTTON4},
		{"JOY_BUTTON5", JOY_BUTTON5},
		{"JOY_BUTTON6", JOY_BUTTON6},
		{"JOY_BUTTON7", JOY_BUTTON7},
		{"JOY_BUTTON8", JOY_BUTTON8},
		{"JOY_BUTTON9", JOY_BUTTON9},
		{"JOY_BUTTON10", JOY_BUTTON10},
		{"JOY_BUTTON11", JOY_BUTTON11},
		{"JOY_BUTTON12", JOY_BUTTON12},
		{"JOY_BUTTON13", JOY_BUTTON13},
		{"JOY_BUTTON14", JOY_BUTTON14},
		{"JOY_BUTTON15", JOY_BUTTON15},
		{"JOY_BUTTON16", JOY_BUTTON16},
		{"JOY_BUTTON17", JOY_BUTTON17},
		{"JOY_BUTTON18", JOY_BUTTON18},
		{"JOY_BUTTON19", JOY_BUTTON19},
		{"JOY_BUTTON20", JOY_BUTTON20},
		{"JOY_BUTTON21", JOY_BUTTON21},
		{"JOY_BUTTON22", JOY_BUTTON22},
		{"JOY_BUTTON23", JOY_BUTTON23},
		{"JOY_BUTTON24", JOY_BUTTON24},
		{"JOY_BUTTON25", JOY_BUTTON25},
		{"JOY_BUTTON26", JOY_BUTTON26},
		{"JOY_BUTTON27", JOY_BUTTON27},
		{"JOY_BUTTON28", JOY_BUTTON28},
		{"JOY_BUTTON29", JOY_BUTTON29},
		{"JOY_BUTTON30", JOY_BUTTON30},
		{"JOY_BUTTON31", JOY_BUTTON31},
		{"JOY_BUTTON32", JOY_BUTTON32},*/
		{NULL, 0} // NULL terminator
	};

	// Create module
	luaL_newlib(L, _funcs);

	// Push our load of VKs as variables
	int i = 0;
	while(_vks[i].name)
	{
		lua_pushnumber(L, _vks[i].vk);
		lua_setfield(L, -2, _vks[i].name);
		i++;
	}

	// Set the module
	lua_setglobal(L, KEY_MODULE_NAME);

	return MicroMacro::ERR_OK;
}
