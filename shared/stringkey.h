#ifndef STRINGKEY_H
#define STRINGKEY_H

#include "config.h"

#include "string.h"
#include "2hash.h"

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
	char * operator=(char *c) { return str=c; }
	bool operator < ( const StringKey &a) const // for Dinkum / MS .NET 2003 hash table (buckets sorted by key, takes an extra comparison since a single valued < is used rather than a 3 value strcmp
	{
	  return strcmp(str,a.str)<0;
	}
	bool operator == ( const StringKey &a ) const // for cool STL / graehl unsorted-buckets hash table
	{
		return strcmp(str, a.str)==0;
	}
	bool isGlobalEmpty() const { return str == empty; }
	size_t hash() const
	{
		return cstr_hash(str);	
	}
};
HASHNS_B
template<> struct hash<StringKey>
{
  size_t operator()(StringKey s) const {
	return s.hash();
  }
};
HASHNS_E
inline size_t hash_value(StringKey s) { return s.hash(); }
//inline size_t hash(StringKey s) { return s.hash(); }

#endif
