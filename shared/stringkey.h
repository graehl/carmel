#ifndef STRINGKEY_H
#define STRINGKEY_H

#include "config.h"

inline unsigned int cstr_hash (const char *p)
{
	unsigned int h=0;
#ifdef OLD_HASH
	unsigned int g;
	while (*p != 0) {
		h = (h << 4) + *p++;
		if ((g = h & 0xf0000000) != 0)
			h = (h ^ (g >> 24)) ^ g;
	}
	return (h >> 4);
#else
	// google for g_str_hash X31_HASH to see why this is better (less collisions, good performance for short strings, faster)
	while (*p != '\0')
		h = 31 * h + *p++; // should optimize to ( h << 5 ) - h if faster
	return h;
#endif
}

#include "string.h"

class StringKey {
public:
	char *str;
	static char *empty;
	StringKey() : str(empty) {}
	StringKey(char *c) : str(c) {}
	const char *clone()
	{
		char *old = str;
		str = strcpy(NEW char[strlen(str)+1], str);
		return old;
	}
	void kill()
	{
		if (str != empty)
			delete str;
		str = empty;
	}
	operator char * () { return str; }
	//	char * operator =(char * c) { char *t = str; str = c; return t; } // returns old value: why?
	void operator=(char *c) { str=c; }
	bool operator < ( const StringKey &a) const
	{
	  return strcmp(str,a.str)<0;
	}
	bool operator == ( const StringKey &a ) const
	{
		return strcmp(str, a.str)==0;
	}
	bool isGlobalEmpty() const { return str == empty; }
	unsigned int hash() const
	{
		return cstr_hash(str);	
	}
};

inline unsigned int hash_value(StringKey s) { return s.hash(); }
inline unsigned int hash(StringKey s) { return s.hash(); }

#endif
