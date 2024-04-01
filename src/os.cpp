/******************************************************************************
    Project:    MicroMacro
    Author:     SolarStrike Software
    URL:        www.solarstrike.net
    License:    Modified BSD (see license.txt)
******************************************************************************/

#include "os.h"
#include "strl.h"

// Helper to get the user's privilege level
DWORD OS::getUserPriv()
{
    LPUSER_INFO_1 ui1 = NULL;
    wchar_t username[UNLEN + 1];
    DWORD usernameSize = UNLEN + 1;
    NET_API_STATUS status;

    GetUserNameW((WCHAR *)&username, &usernameSize);
    status = NetUserGetInfo(NULL, (WCHAR *)&username, (DWORD)1, (LPBYTE *)&ui1);

    if( ui1 == NULL || status != NERR_Success )
    {   /* If some error occurs, assume guest */
        return USER_PRIV_GUEST;
    }

    return ui1->usri1_priv;
}

// Helper to get a group name of a privilege level
std::string OS::getPrivName(DWORD priv)
{
    switch(priv)
    {
        case USER_PRIV_GUEST:
            return "Guest";
            break;
        case USER_PRIV_USER:
            return "User";
            break;
        case USER_PRIV_ADMIN:
            return "Administrator";
            break;
        default:
            return "Unknown";
            break;
    }
}

// Returns the name of the OS as a string
std::string OS::getOsName()
{
    //std::string servicepacksz = "";
    OSVERSIONINFOEX osvi;

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx( (OSVERSIONINFO*)&osvi);

    // version info from:
    // http://msdn2.microsoft.com/en-us/library/ms724834.aspx
    if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) // 4, 0
        return std::string("Windows 95");
    if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) // 4, 10
        return std::string("Windows 98");
    if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) // 4, 90
        return std::string("Windows ME");
    if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) // 5, 0
        return std::string("Windows 2000");
    if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 ) // 5, 1
        return std::string("Windows XP");
    if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 ) // 5, 2
    {
        if( (osvi.wProductType == VER_NT_WORKSTATION) )
            return std::string("Windows XP 64-bit");

        if( osvi.wSuiteMask == 0x00008000 )
            return std::string("Windows Home Server");

        if( GetSystemMetrics(SM_SERVERR2) == 0 )
            return std::string("Windows Server 2003");
        else
            return std::string("Windows Server 2003 R2");
    }
    if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0 ) // 6, 0
    {
        if( osvi.wProductType == VER_NT_WORKSTATION )
            return std::string("Windows Vista");
        else
            return std::string("Windows Server 2008");
    }
    if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1 ) // 6, 1
    {
        if( osvi.wProductType == VER_NT_WORKSTATION )
            return std::string("Windows 7");
        else
            return std::string("Windows Server 2008 R2");
    }
    if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 2 ) // 6, 2
    {
        if( osvi.wProductType == VER_NT_WORKSTATION )
            return std::string("Windows 8");
        else
            return std::string("Windows Server 2012");
    }
    if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 3 ) // 6, 3
    {
        if( osvi.wProductType == VER_NT_WORKSTATION )
            return std::string("Windows 8.1");
        else
            return std::string("Windows Server 2012 R2");
    }
    if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 4 ) // 6, 4
    {
        return std::string("Windows 10");
    }
    if ( osvi.dwMajorVersion == 10 && osvi.dwMinorVersion == 0 ) // 10, 0
    {
        return std::string("Windows 11");
    }

    // unknown OS
    char buffer[256];
    slprintf((char*)&buffer, sizeof(buffer) - 1, "Unknown Windows OS v%i.%i",
             (unsigned int)osvi.dwMajorVersion, (unsigned int)osvi.dwMinorVersion);
    return std::string(buffer);
}

/*  Obviously, it changes our permission level for a process.
    This is called to grant SeDebugPrivilege.
*/
int OS::modifyPermission(HANDLE hProcess, const char *PrivName, bool allow)
{
    HANDLE hToken;
    LUID sedebugnameValue;
    TOKEN_PRIVILEGES tkp;

    if ( !OpenProcessToken(hProcess, TOKEN_ADJUST_PRIVILEGES
                           | TOKEN_QUERY, &hToken ) )
    {
        CloseHandle(hToken);
        return false;
    }

    //Don't bother looking up and adjusting privilege if removing rights
    if(allow)
    {
        if ( !LookupPrivilegeValue( NULL, PrivName, &sedebugnameValue ) )
        {
            CloseHandle( hToken );
            return false;
        }

        tkp.PrivilegeCount = 1;
        tkp.Privileges[0].Luid = sedebugnameValue;
        tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    }

    if (AdjustTokenPrivileges( hToken, false, &tkp, sizeof tkp,
                               NULL, NULL ) == 0)
    {
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

