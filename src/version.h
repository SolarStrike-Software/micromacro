#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "29";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2021";
	static const char UBUNTU_VERSION_STYLE[] =  "21.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 98;
	static const long BUILD  = 2;
	static const long REVISION  = 1014;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 2070;
	#define RC_FILEVERSION 1,98,2,1014
	#define RC_FILEVERSION_STRING "1, 98, 2, 1014\0"
	static const char FULLVERSION_STRING [] = "1.98.2.1014";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 128;
	

}
#endif //VERSION_H
