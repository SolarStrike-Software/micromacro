#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "29";
	static const char MONTH[] = "06";
	static const char YEAR[] = "2016";
	static const char UBUNTU_VERSION_STYLE[] =  "16.06";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 93;
	static const long BUILD  = 2;
	static const long REVISION  = 1008;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 1564;
	#define RC_FILEVERSION 1,93,2,1008
	#define RC_FILEVERSION_STRING "1, 93, 2, 1008\0"
	static const char FULLVERSION_STRING [] = "1.93.2.1008";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 122;
	

}
#endif //VERSION_H
