#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "18";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.02";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 91;
	static const long BUILD  = 39;
	static const long REVISION  = 998;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 945;
	#define RC_FILEVERSION 1,91,39,998
	#define RC_FILEVERSION_STRING "1, 91, 39, 998\0"
	static const char FULLVERSION_STRING [] = "1.91.39.998";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 112;
	

}
#endif //VERSION_H
