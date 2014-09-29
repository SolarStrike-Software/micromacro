#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "29";
	static const char MONTH[] = "09";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.09";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 9;
	static const long BUILD  = 20;
	static const long REVISION  = 275;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 344;
	#define RC_FILEVERSION 1,9,20,275
	#define RC_FILEVERSION_STRING "1, 9, 20, 275\0"
	static const char FULLVERSION_STRING [] = "1.9.20.275";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 53;
	

}
#endif //VERSION_H
