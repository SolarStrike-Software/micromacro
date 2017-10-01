#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "01";
	static const char MONTH[] = "10";
	static const char YEAR[] = "2017";
	static const char UBUNTU_VERSION_STYLE[] =  "17.10";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 95;
	static const long BUILD  = 1;
	static const long REVISION  = 1009;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1714;
	#define RC_FILEVERSION 1,95,1,1009
	#define RC_FILEVERSION_STRING "1, 95, 1, 1009\0"
	static const char FULLVERSION_STRING [] = "1.95.1.1009";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 123;
	

}
#endif //VERSION_H
