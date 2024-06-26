#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "18";
	static const char MONTH[] = "04";
	static const char YEAR[] = "2024";
	static const char UBUNTU_VERSION_STYLE[] =  "24.04";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 99;
	static const long BUILD  = 3;
	static const long REVISION  = 1015;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 2207;
	#define RC_FILEVERSION 1,99,3,1015
	#define RC_FILEVERSION_STRING "1, 99, 3, 1015\0"
	static const char FULLVERSION_STRING [] = "1.99.3.1015";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 129;
	

}
#endif //VERSION_H
