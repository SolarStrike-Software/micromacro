#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "08";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 9;
	static const long BUILD  = 19;
	static const long REVISION  = 269;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 344;
	#define RC_FILEVERSION 1,9,19,269
	#define RC_FILEVERSION_STRING "1, 9, 19, 269\0"
	static const char FULLVERSION_STRING [] = "1.9.19.269";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 52;
	

}
#endif //VERSION_H
