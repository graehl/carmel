#ifndef STRINGKEY_H
#define STRINGKEY_H

#include <graehl/shared/config.h>
#include <string>
#include <graehl/shared/static_itoa.h>
#include <graehl/shared/2hash.h>
#include <graehl/shared/stream_util.hpp>

struct StringKey {
    char *str;
    const char *c_str() const { return str; }
    static StringKey empty;
    StringKey() : str(empty.str) {}
    explicit StringKey(unsigned i) : str(static_itoa(i)) {}
    StringKey(const char *c) : str(const_cast<char *>(c)) {}
    const char *clone()
    {
        char *old = str;
        str = std::strcpy(NEW char[strlen(str)+1], str);
        return old;
    }
    void kill()
    {
        if (str != empty.str)
            delete str;
        str = empty.str;
    }
    //operator char * () { return str; }
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
    int cmp( const StringKey &a ) const 
    {
        return strcmp(str, a.str);
    }

    bool isDefault() const { return str == empty.str; }
    size_t hash() const
    {
        return cstr_hash(str);	
    }
    template <class O> void print(O&o) const
    {
        o<<c_str();
    }
    typedef StringKey self_type;
    TO_OSTREAM_PRINT
};
BEGIN_HASH_VAL(StringKey) {
	return x.hash();
} END_HASH
//inline size_t hash_value(StringKey s) { return s.hash(); }
//inline size_t hash(StringKey s) { return s.hash(); }

#endif
