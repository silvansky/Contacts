#ifndef _rbase_h_
#define _rbase_h_

#ifndef DISALLOW_EVIL_CONSTRUCTORS
// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
	TypeName(const TypeName&);                    \
	void operator=(const TypeName&)
#endif


#endif
