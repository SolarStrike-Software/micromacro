#ifndef VERSION_H
#define VERSION_H

namespace AutoVersion{
	
	//Date Version Types
	static const char DATE[] = "21";
	static const char MONTH[] = "01";
	static const char YEAR[] = "2015";
	static const char UBUNTU_VERSION_STYLE[] =  "15.01";
	
	//Software Status
	static const char STATUS[] =  "Alpha";
	static const char STATUS_SHORT[] =  "a";
	
	//Standard Version Type
	static const long MAJOR  = 1;
	static const long MINOR  = 91;
	static const long BUILD  = 5;
	static const long REVISION  = 690;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT  = 699;
	#define RC_FILEVERSION 1,91,5,690
	#define RC_FILEVERSION_STRING "1, 91, 5, 690\0"
	static const char FULLVERSION_STRING [] = "1.91.5.690";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY  = 33;
	

}
#endif //VERSION_H
