# micromacro
MicroMacro programmable macro &amp; automation tool for Windows



### What is it used for?
MicroMacro is a full Lua interpreter with extended functionality to quickly and easily automate interactions between windows and processes. This includes simulating keypresses, manipulating memory, creating or modifying processes, or anything else you can think of.

Your creativity is the only limiting factor. MicroMacro has been used to "bot" various computer games, to parse and extract useful data, scrape webpages, patch or manipulate running executables, and to remap keyboard/mouse or gamepad input.

For a full list of supported functionality, see the wiki here: http://www.solarstrike.net/wiki/index.php?title=Manual



### What platforms are supported?
As of now, Windows NT (XP+) is required. Although there has been some luck getting it to run via Wine on Linux, not everything has been functional in that environment.

If anyone is interested in porting this, let me know.



### Required & optional libraries to build
**Required:** (Standard Win32 libraries italicized)
* Lua 5.3
* Libncurses
* Sqlite 3
* *Netapi32*
* *Winmm*
* *Psapi*
* *GDI32*
* *Shlwapi*
* *Comdlg32*

**Optional:**
* OpenAL  *(Use AUDIO_ENABLED #define)*
* Alut    *(required w/ OpenAL)*
* Ws2_32 *(ie Winsock 2; Use NETWORKING_ENABLED #define)*
