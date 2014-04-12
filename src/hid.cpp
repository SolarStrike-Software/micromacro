#include "hid.h"
#include "error.h"
#include "settings.h"
#include "macro.h"

#include "wininclude.h"
#include <stdio.h>

#define POSTMESSAGE_NORMAL		  	0x00000001 /* for normal messages */
#define POSTMESSAGE_EXTENDED		0x01000001 /* for extended messages */
#define POSTMESSAGE_UP				0xC0000000 /* for key up messages */

bool Hid::keyIsExtended(int vk)
{
	static int extList[] = {
		VK_INSERT, VK_DELETE, VK_HOME, VK_END, VK_DIVIDE, VK_LWIN,
		VK_RWIN, VK_PRIOR, VK_NEXT, VK_LEFT, VK_RIGHT, VK_UP,
		VK_DOWN, VK_CONTROL, VK_LCONTROL, VK_RCONTROL, VK_MENU,
		VK_LMENU, VK_RMENU, 0
	};

	for(unsigned int i = 0; extList[i] != 0; i++)
	{
		if( extList[i] == vk )
			return true;
	}

	return false;
}

int Hid::init()
{
	try {
		ks = new BYTE[KS_SIZE];
		lastks = new BYTE[KS_SIZE];
		joyinfo = new JOYINFOEX[GAMEPADS];
		lastjoyinfo = new JOYINFOEX[GAMEPADS];
	}
	catch( std::bad_alloc &ba ) { badAllocation(); }

	memset(lastks, 0, sizeof(lastks) - 1);
	memset(joyinfo, 0, sizeof(joyinfo) - 1);
	memset(lastjoyinfo, 0, sizeof(lastjoyinfo) - 1);

	// Initial polling
	int unused = 0;
	GetKeyState(unused); // To get around a Windows bug
	GetKeyboardState(ks);

	unsigned int gamepadsPolled =  0;
	for(unsigned int gamepad = 0; gamepad < GAMEPADS; gamepad++)
	{
		joyinfo[gamepad].dwSize = sizeof(JOYINFOEX);
		lastjoyinfo[gamepad].dwSize = sizeof(JOYINFOEX);
		joyinfo[gamepad].dwFlags = JOY_RETURNALL;

		int success = joyGetPosEx(gamepad, &joyinfo[gamepad]);

		if( success != JOYERR_NOERROR )
			memset(&joyinfo[gamepad], 0, sizeof(JOYINFOEX)-1); // zero out
		else
			++gamepadsPolled;
	}
	gamepadCount = gamepadsPolled;

	// Various other stuff
	keyHoldDelayMs = 50;
	vMouseX = 0;
	vMouseY = 0;

	return MicroMacro::ERR_OK;
}

void Hid::poll()
{
	// Swap buffers
	BYTE *tmp = lastks;
	lastks = ks;
	ks = tmp;

	JOYINFOEX *joytmp = lastjoyinfo;
	lastjoyinfo = joyinfo;
	joyinfo = joytmp;

	// Windows bug workaround... again.
	int unused = 0;
	GetKeyState(unused); // Do NOT use GetAsyncKeyState here!

	// Poll keyboard
	GetKeyboardState(ks);

	// Poll gamepad
	unsigned int gamepadsPolled =  0;
	for(unsigned int gamepad = 0; gamepad < GAMEPADS; gamepad++)
	{
		joyinfo[gamepad].dwSize = sizeof(JOYINFOEX);
		lastjoyinfo[gamepad].dwSize = sizeof(JOYINFOEX);
		joyinfo[gamepad].dwFlags = JOY_RETURNALL;

		int success = joyGetPosEx(gamepad, &joyinfo[gamepad]);

		if( success != JOYERR_NOERROR )
			memset(&joyinfo[gamepad], 0, sizeof(JOYINFOEX)-1); // zero out
		else
			++gamepadsPolled;
	}
	gamepadCount = gamepadsPolled;
}

BYTE *Hid::getState()
{
	return ks;
}

BYTE *Hid::getLastState()
{
	return lastks;
}



bool Hid::pressed(int vk)
{
	return ( (ks[vk]&128) && !(lastks[vk]&128) );
}

bool Hid::released(int vk)
{
	return ( !(ks[vk]&128) && (lastks[vk]&128) );
}

bool Hid::isDown(int vk)
{
	return (ks[vk]&128);
}

bool Hid::getToggleState(int vk)
{
	return (ks[vk]&1);
}

void Hid::press(int vk, bool async)
{
	// Hold it
	hold(vk);
	if( async ) // In async mode, continue processing but queue release
	{
		// Queue its release
		KeyTimePair ktp;
		ktp.timestamp = ::getNow();
		ktp.type = KTP_KEYBOARD;
		ktp.vk = vk;
		ktp.hwnd = 0;
		keyHeldQueue.push(ktp);
	}
	else // In blocking mode, wait, then release
	{
		Sleep(keyHoldDelayMs);
		release(vk);
	}
}

void Hid::hold(int vk)
{
	INPUT inp;
	LPARAM lparam;
	unsigned int scancode = vk;
	unsigned int sc_test = MapVirtualKey(vk, 0);
	bool extended = keyIsExtended(vk);

	if( sc_test != 0 )
		scancode = sc_test;

	if( vk <= VK_XBUTTON2 )
	{ // Extra mouse button 2 is the highest recognized
		memset(&inp, 0, sizeof(INPUT));
		inp.type = INPUT_MOUSE;
		switch(vk)
		{
			case VK_LBUTTON:
				inp.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
			break;
			case VK_RBUTTON:
				inp.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
			break;
			case VK_MBUTTON:
				inp.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
			break;
			case VK_XBUTTON1:
				inp.mi.dwFlags = MOUSEEVENTF_XDOWN;
				inp.mi.mouseData = XBUTTON1;
			break;
			case VK_XBUTTON2:
				inp.mi.dwFlags = MOUSEEVENTF_XDOWN;
				inp.mi.mouseData = XBUTTON2;
			break;
		}
	}
	else
	{
		if( extended )
		{
			lparam = (scancode << 16) | POSTMESSAGE_EXTENDED;
			inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY;
		}
		else
		{
			lparam = (scancode << 16) | POSTMESSAGE_NORMAL;
			inp.ki.dwFlags = KEYEVENTF_SCANCODE;
		}
		inp.type = INPUT_KEYBOARD;
		inp.ki.wScan = scancode;
		inp.ki.dwExtraInfo = lparam;
	}

	SendInput(1, &inp, sizeof(INPUT));
}

void Hid::release(int vk)
{
	INPUT inp;
	LPARAM lparam;
	unsigned int scancode = vk;
	unsigned int sc_test = MapVirtualKey(vk, 0);
	bool extended = keyIsExtended(vk);

	if( sc_test != 0 )
		scancode = sc_test;

	if( vk <= VK_XBUTTON2 )
	{ // Extra mouse button 2 is the highest recognized
		memset(&inp, 0, sizeof(INPUT));
		inp.type = INPUT_MOUSE;
		switch(vk)
		{
			case VK_LBUTTON:
				inp.mi.dwFlags = MOUSEEVENTF_LEFTUP;
			break;
			case VK_RBUTTON:
				inp.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
			break;
			case VK_MBUTTON:
				inp.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
			break;
			case VK_XBUTTON1:
				inp.mi.dwFlags = MOUSEEVENTF_XUP;
				inp.mi.mouseData = XBUTTON1;
			break;
			case VK_XBUTTON2:
				inp.mi.dwFlags = MOUSEEVENTF_XUP;
				inp.mi.mouseData = XBUTTON2;
			break;
		}
	}
	else
	{
		if( extended )
		{
			lparam = (scancode << 16) | POSTMESSAGE_EXTENDED;
			inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
		}
		else
		{
			lparam = (scancode << 16) | POSTMESSAGE_NORMAL;
			inp.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
		}
		inp.type = INPUT_KEYBOARD;
		inp.ki.wScan = scancode;
		inp.ki.dwExtraInfo = lparam;
	}

	SendInput(1, &inp, sizeof(INPUT));
}

void Hid::virtualPress(HWND hwnd, int vk, bool async)
{
	// Hold it
	virtualHold(hwnd, vk);

	if( async ) // In async mode, continue processing but queue release
	{
		// Queue its release
		KeyTimePair ktp;
		ktp.timestamp = ::getNow();
		ktp.type = KTP_KEYBOARD;
		ktp.vk = vk;
		ktp.hwnd = hwnd;
		keyHeldQueue.push(ktp);
	}
	else // In blocking mode, wait, then release
	{
		Sleep(keyHoldDelayMs);
		virtualRelease(hwnd, vk);
	}

}

void Hid::virtualHold(HWND hwnd, int vk)
{
	LPARAM lparam;
	unsigned int scancode = vk;
	unsigned int sc_test = MapVirtualKey(vk, 0);
	bool extended = keyIsExtended(vk);

	if( sc_test != 0 )
		scancode = sc_test;

	if( vk <= VK_XBUTTON2 )
	{ // Extra mouse button 2 is the highest recognized
		lparam = MAKELPARAM(vMouseX, vMouseY);
		UINT msg = 0;
		WPARAM wparam = 0;

		switch(vk)
		{
			case VK_LBUTTON:
				msg = WM_LBUTTONDOWN;
				wparam = MK_LBUTTON;
			break;
			case VK_RBUTTON:
				msg = WM_RBUTTONDOWN;
				wparam = MK_RBUTTON;
			break;
			case VK_MBUTTON:
				msg = WM_MBUTTONDOWN;
				wparam = MK_MBUTTON;
			break;
			case VK_XBUTTON1:
				msg = WM_XBUTTONDOWN;
				wparam = MK_XBUTTON1 | (XBUTTON1 << 16);
			break;
			case VK_XBUTTON2:
				msg = WM_XBUTTONDOWN;
				wparam = MK_XBUTTON2 | (XBUTTON2 << 16);
			break;
		}

		PostMessage(hwnd, msg, wparam, lparam);
	}
	else
	{
		if( extended )
			lparam = (scancode << 16) | POSTMESSAGE_EXTENDED;
		else
			lparam = (scancode << 16) | POSTMESSAGE_NORMAL;

		PostMessage(hwnd, WM_KEYDOWN, vk, lparam);
	}
}

void Hid::virtualRelease(HWND hwnd, int vk)
{
	LPARAM lparam;
	unsigned int scancode = vk;
	unsigned int sc_test = MapVirtualKey(vk, 0);
	bool extended = keyIsExtended(vk);

	if( sc_test != 0 )
		scancode = sc_test;

	if( vk <= VK_XBUTTON2 )
	{ // Extra mouse button 2 is the highest recognized
		lparam = MAKELPARAM(vMouseX, vMouseY);
		UINT msg = 0;
		WPARAM wparam = 0;

		switch(vk)
		{
			case VK_LBUTTON:
				msg = WM_LBUTTONUP;
				wparam = MK_LBUTTON;
			break;
			case VK_RBUTTON:
				msg = WM_RBUTTONUP;
				wparam = MK_RBUTTON;
			break;
			case VK_MBUTTON:
				msg = WM_MBUTTONUP;
				wparam = MK_MBUTTON;
			break;
			case VK_XBUTTON1:
				msg = WM_XBUTTONUP;
				wparam = MK_XBUTTON1 | (XBUTTON1 << 16);
			break;
			case VK_XBUTTON2:
				msg = WM_XBUTTONUP;
				wparam = MK_XBUTTON2 | (XBUTTON2 << 16);
			break;
		}

		PostMessage(hwnd, msg, wparam, lparam);
	}
	else
	{
		if( extended )
			lparam = (scancode << 16) | POSTMESSAGE_UP | POSTMESSAGE_EXTENDED;
		else
			lparam = (scancode << 16) | POSTMESSAGE_UP | POSTMESSAGE_NORMAL;

		PostMessage(hwnd, WM_KEYUP, vk, lparam);
	}
}

void Hid::getVirtualMousePos(int &_x, int &_y)
{
	_x = vMouseX;
	_y = vMouseY;
}

void Hid::setVirtualMousePos(int _x, int _y)
{
	vMouseX = _x;
	vMouseY = _y;
}

bool Hid::joyPressed(int joyId,int button)
{
	if( joyId < 0 || joyId >= GAMEPADS )
		return false;

	return ( (joyinfo[joyId].dwButtons & (1<<button)) &&
			!(lastjoyinfo[joyId].dwButtons & (1<<button)) );
}

bool Hid::joyReleased(int joyId, int button)
{
	if( joyId < 0 || joyId >= GAMEPADS )
		return false;

	return ( (lastjoyinfo[joyId].dwButtons & (1<<button)) &&
			!(joyinfo[joyId].dwButtons & (1<<button)) );
}

bool Hid::joyIsDown(int joyId, int button)
{
	if( joyId < 0 || joyId >= GAMEPADS )
		return false;

	return (joyinfo[joyId].dwButtons & (1<<button));
}

DWORD Hid::joyPOV(int joyId)
{
	if( joyId < 0 || joyId >= GAMEPADS )
		return (DWORD)0;

	return (joyinfo[joyId].dwPOV);
}

bool Hid::joyPOVChanged(int joyId)
{
	if( joyId < 0 || joyId >= GAMEPADS )
		return false;

	return (joyinfo[joyId].dwPOV != lastjoyinfo[joyId].dwPOV);
}

DWORD Hid::joyAxis(int joyId, int axisId)
{
	if( joyId < 0 || joyId >= GAMEPADS )
		return (DWORD)0;

	DWORD axis;
	switch(axisId)
	{
		case 1:
			axis = joyinfo[joyId].dwXpos;
		break;

		case 2:
			axis = joyinfo[joyId].dwYpos;
		break;

		case 3:
			axis = joyinfo[joyId].dwZpos;
		break;

		case 4:
			axis = joyinfo[joyId].dwRpos;
		break;

		case 5:
			axis = joyinfo[joyId].dwUpos;
		break;

		case 6:
			axis = joyinfo[joyId].dwVpos;
		break;

		default:
			return (DWORD)0;
		break;
	}
	return axis;
}

bool Hid::joyAxisChanged(int joyId, int axisId)
{
	if( joyId < 0 || joyId >= GAMEPADS )
		return false;

	bool changed = false;
	switch(axisId)
	{
		case 1:
			changed = joyinfo[joyId].dwXpos != lastjoyinfo[joyId].dwXpos;
		break;

		case 2:
			changed = joyinfo[joyId].dwYpos != lastjoyinfo[joyId].dwYpos;
		break;

		case 3:
			changed = joyinfo[joyId].dwZpos != lastjoyinfo[joyId].dwZpos;
		break;

		case 4:
			changed = joyinfo[joyId].dwRpos != lastjoyinfo[joyId].dwRpos;
		break;

		case 5:
			changed = joyinfo[joyId].dwUpos != lastjoyinfo[joyId].dwUpos;
		break;

		case 6:
			changed = joyinfo[joyId].dwVpos != lastjoyinfo[joyId].dwVpos;
		break;

		default:
			changed = false;
		break;
	}
	return changed;
}

/*
void Hid::joyPress(int gamepad, int button, bool async)
{
	// Hold it
	joyHold(gamepad, button);

	if( async ) // In async mode, continue processing but queue release
	{
		// Queue its release
		KeyTimePair ktp;
		ktp.timestamp = ::getNow();
		ktp.type = KTP_GAMEPAD;
		ktp.gamepad = gamepad;
		ktp.vk = button;
		ktp.hwnd = 0;
		keyHeldQueue.push(ktp);
	}
	else // In blocking mode, wait, then release
	{
		Sleep(keyHoldDelayMs);
		joyRelease(gamepad, button);
	}
}

void Hid::joyHold(int gamepad, int button)
{
	INPUT inp;
	inp.type = INPUT_HARDWARE;

	SendInput(1, &inp, sizeof(INPUT));
}

void Hid::joyRelease(int gamepad, int button)
{
	INPUT inp;
	inp.type = INPUT_HARDWARE;

	SendInput(1, &inp, sizeof(INPUT));
}

void Hid::virtualJoyPress(HWND hwnd, int gamepad, int button, bool async)
{
}

void Hid::virtualJoyHold(HWND hwnd, int gamepad, int button)
{
}

void Hid::virtualJoyRelease(HWND hwnd, int gamepad, int button)
{
}
*/

unsigned int Hid::getGamepadCount()
{
	return gamepadCount;
}

void Hid::handleKeyHeldQueue()
{
	TimeType now = getNow();
	KeyTimePair *pktp;
	while(!keyHeldQueue.empty()) // While there's still things in queue
	{
		// Check if enough time has elapsed
		pktp = &keyHeldQueue.front();
		if( deltaTime(now, pktp->timestamp)*1000 >= keyHoldDelayMs )
		{ // Release this key, pop it
			if( pktp->hwnd == 0 )
			{
				if( pktp->type == KTP_KEYBOARD )
					release(pktp->vk);
				/*else
					joyRelease(pktp->gamepad, pktp->vk);*/
			}
			else
			{
				if( pktp->type == KTP_KEYBOARD )
					virtualRelease(pktp->hwnd, pktp->vk);
				/*else
					virtualJoyRelease(pktp->hwnd, pktp->gamepad, pktp->vk);*/
			}
			keyHeldQueue.pop();
		}
		else
			break; // All done! Time to go home.
	}
}
