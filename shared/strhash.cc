#define BOOST_AUTO_TEST_MAIN

#include "strhash.h"

std::ostream & operator << (std::ostream & out, const StringKey &s)
{
  out << (char *)(StringKey)s;
  return out;
}
void Alphabet::dump()
 {
	Config::debug() << ht;
  }

void dump_ht(HashTable<StringKey,int> &ht)
{
  Config::debug() << ht;
}

#ifdef STRINGPOOL
HashTable<StringKey, int> StringPool::counts;
#endif

#include "stringkey.cc"

#ifdef TEST
#include "../../tt/test.hpp"
BOOST_AUTO_UNIT_TEST( strhash )
{
  Alphabet a;
  Alphabet b;
  const char *s[]={"u","ul","mu","pi"};
  const int n=sizeof(s)/sizeof(s[0]);
  a.add("a");
  for (int i=0;i<n;++i)
	BOOST_CHECK(a.indexOf(s[i]) == b.indexOf(s[i])+1);
  a.verify();
  b.verify();  
}
#endif
