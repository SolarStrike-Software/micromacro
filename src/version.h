#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "01";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2024";
	static const char UBUNTU_VERSION_STYLE[] =  "24.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 99;
	static const long BUILD  = 1;
	static const long REVISION  = 1014;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 2144;
	#define RC_FILEVERSION 1,99,1,1014
	#define RC_FILEVERSION_STRING "1, 99, 1, 1014\0"
	static const char FULLVERSION_STRING [] = "1.99.1.1014";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 128;
	

}
#endif //VERSION_H
