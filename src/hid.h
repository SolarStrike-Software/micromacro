/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#ifndef HID_H
#define HID_H

	#include <queue>
	#include "timer.h"
	#include "wininclude.h"

	#define KS_SIZE				256
	#define GAMEPADS			16
	#define GAMEPAD_BUTTONS		32
	#define GAMEPAD_AXIS_COUNT	6

	#define POSTMESSAGE_NORMAL		  	0x00000001 /* for normal messages */
	#define POSTMESSAGE_EXTENDED		0x01000001 /* for extended messages */
	#define POSTMESSAGE_UP				0xC0000000 /* for key up messages */

	// Key pair types; KTP_KEYBOARD is for keyboard AND mouse!
	enum KTP_TYPE{KTP_KEYBOARD, KTP_GAMEPAD};

	typedef struct lua_State lua_State;
	typedef unsigned char BYTE;
	struct KeyTimePair{TimeType timestamp; KTP_TYPE type; int vk; int gamepad; HWND hwnd;};
	typedef std::queue<KeyTimePair> KeyHeldQueue;

	class Hid
	{
		protected:
			BYTE *ks;
			BYTE *lastks;
			JOYINFOEX *joyinfo;
			JOYINFOEX *lastjoyinfo;
			int keyHoldDelayMs;
			KeyHeldQueue keyHeldQueue;
			int vMouseX, vMouseY;
			bool gamepadAvailable[GAMEPADS];
			unsigned int gamepadCount;
			unsigned int gamepadMaxIndex;

		public:
			int init();
			void poll();
			BYTE *getState();
			BYTE *getLastState();

			JOYINFOEX *getJoyState();
			JOYINFOEX *getLastJoyState();

			// Keyboard and mouse
			bool pressed(int);
			bool released(int);
			bool isDown(int);
			bool getToggleState(int);
			void setToggleState(int, bool);

			void press(int, bool = true);
			void hold(int);
			void release(int);

			void virtualPress(HWND, int, bool = true);
			void virtualHold(HWND, int);
			void virtualRelease(HWND, int);

			void getVirtualMousePos(int &, int &);
			void setVirtualMousePos(int, int);

			bool keyIsExtended(int);

			// Gamepads
			bool joyPressed(int, int);
			bool joyReleased(int, int);
			bool joyIsDown(int, int);

			DWORD joyPOV(int);
			bool joyPOVChanged(int);
			DWORD joyAxis(int, int);
			bool joyAxisChanged(int, int);

			unsigned int getGamepadCount();
			unsigned int getGamepadMaxIndex();
			bool gamepadIsAvailable(int);
			void repollGamepadMaxIndex();

			/* Lack of documentation on HW input simulation from Microsoft...
			void joyPress(int, int, bool = true);
			void joyHold(int, int);
			void joyRelease(int, int);

			void virtualJoyPress(HWND, int, int, bool = true);
			void virtualJoyHold(HWND, int, int);
			void virtualJoyRelease(HWND, int, int);*/

			void handleKeyHeldQueue();
	};

#endif
