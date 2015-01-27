#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "27";
	static const char MONTH[] = "01";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.01";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 91;
	static const long BUILD  = 17;
	static const long REVISION  = 976;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 806;
	#define RC_FILEVERSION 1,91,17,976
	#define RC_FILEVERSION_STRING "1, 91, 17, 976\0"
	static const char FULLVERSION_STRING [] = "1.91.17.976";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 90;
	

}
#endif //VERSION_H
