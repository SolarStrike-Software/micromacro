#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "28";
	static const char MONTH[] = "12";
	static const char YEAR[] = "2014";
	static const char UBUNTU_VERSION_STYLE[] =  "14.12";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 9;
	static const long BUILD  = 25;
	static const long REVISION  = 298;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 464;
	#define RC_FILEVERSION 1,9,25,298
	#define RC_FILEVERSION_STRING "1, 9, 25, 298\0"
	static const char FULLVERSION_STRING [] = "1.9.25.298";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 57;
	

}
#endif //VERSION_H
