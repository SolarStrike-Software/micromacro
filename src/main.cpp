/******************************************************************************
	Project: 	MicroMacro
	Author: 	SolarStrike Software
	URL:		www.solarstrike.net
	License:	Modified BSD (see license.txt)
******************************************************************************/

#include "wininclude.h"
#include "app.h"
#include "macro.h"
#include "logger.h"
#include "resource.h"
#include "encstring.h"

static BOOL WINAPI consoleControlCallback(DWORD);
void initNotifyIconData(HWND hwnd);
void addNotifyIcon();
void removeNotifyIcon();

NOTIFYICONDATA notifyIconData;

INT WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int nShow)
{
	// Create our App
	MicroMacro::App app(hinstance, cmdLine);

	// Any additional setup
	SetConsoleCtrlHandler(consoleControlCallback, true);
	initNotifyIconData(app.getHwnd());

	// Run it until we're done
	int success = app.run();

	removeNotifyIcon();
	return success;
}
// END OF MAIN

// Capture important events, such as force shutdown
static BOOL WINAPI consoleControlCallback(DWORD dwCtrlType)
{
	switch(dwCtrlType)
	{
		case CTRL_C_EVENT:
			Macro::instance()->getEngine()->setCloseState(true);
			return true;
		break;

		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			/* We pretty much have to call exit(0) here.
				Would be nice to be able to just set mainRunning = false
				and let it exit graciously, but, that's WIN32 for you.
			*/

			// Close. Down. EVERYTHING.
			/*	NOTE: We don't want to do this while a script could be running...
				That may cause a SEGFAULT
				TODO: Gracefully terminate the Lua thread, somehow.
			*/

			removeNotifyIcon();
			Logger::instance()->add("Process forcefully terminated (Win32 callback)");
			//exit(EXIT_SUCCESS);	// As it turns out, this is a terrible idea! Let's not do that again.
			return true;
		break;
	}
	return false;
}

void initNotifyIconData(HWND hwnd)
{
    memset( &notifyIconData, 0, sizeof( NOTIFYICONDATA ) ) ;

    notifyIconData.cbSize = sizeof(NOTIFYICONDATA);
    notifyIconData.hWnd = hwnd;
    notifyIconData.uID = ID_TRAY_APP_ICON;
    notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    notifyIconData.uCallbackMessage = WM_SYSICON;
    notifyIconData.hIcon = (HICON)LoadIcon( GetModuleHandle(NULL), MAKEINTRESOURCE(MAINICON) ) ;

	size_t buffSize = 64;
	TCHAR decrypted[buffSize+1];
	EncString::reveal(decrypted, buffSize, EncString::taskIconTitle);

    strncpy(notifyIconData.szTip, decrypted, sizeof(decrypted));

	securezero(decrypted, buffSize);
	addNotifyIcon();
}

void addNotifyIcon()
{
	Shell_NotifyIcon(NIM_ADD, &notifyIconData);
}

void removeNotifyIcon()
{
	Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
}
