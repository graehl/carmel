#ifndef GRAEHL_SHARED__STRINGKEY_H
#define GRAEHL_SHARED__STRINGKEY_H

#include <graehl/shared/config.h>
#include <string>
#include <graehl/shared/static_itoa.h>
#include <graehl/shared/hashtable_fwd.hpp>
#include <graehl/shared/stream_util.hpp>

namespace graehl {

struct StringKey {
  char *str;
  const char *c_str() const { return str; }
  static StringKey empty;
  StringKey() : str(empty.str) {}
  explicit StringKey(unsigned i) : str(static_utoa(i)) {} // big trouble if you try to kill() one of these
  StringKey(char const* s) : str(const_cast<char *>(s)) {}
  StringKey(StringKey const& o) : str(o.str) {}
  // warning: if s is temporary, it must last until you clone or stop using this:
  StringKey(std::string const& s) : str(const_cast<char *>(s.c_str())) {}

  // note: pass in string length (extra '\0' added for you)
  static inline
  char * alloc(unsigned len)
  {
    return (char *)::operator new(sizeof(char)*(len+1));
  }

  static inline
  void dealloc(char *p)
  {
    ::operator delete(p);
  }

  void clone()
  {
    //        char *old = str;
    str= *str ? std::strcpy(alloc(strlen(str)), str) : empty.str;
    //        return old;
  }
  void kill()
  {
    if (str != empty.str)
      dealloc(str);
    str = empty.str;
  }
  //operator char * () { return str; }
  //  char * operator =(char * c) { char *t = str; str = c; return t; } // returns old value: why?
  char * operator = (char *c) { return str = c; }
  StringKey const& operator = (StringKey const& o)
  {
    str = o.str;
    return *this;
  }

  bool operator < ( const StringKey &a) const // for Dinkum / MS .NET 2003 hash table (buckets sorted by key, takes an extra comparison since a single valued < is used rather than a 3 value strcmp
  {
    return strcmp(str, a.str)<0;
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
}

BEGIN_HASH_VAL(graehl::StringKey) {
  return x.hash();
} END_HASH
//inline size_t hash_value(StringKey s) { return s.hash(); }
//inline size_t hash(StringKey s) { return s.hash(); }


#endif
