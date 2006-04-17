// (unused) wrapper for the (non-STL-standard) vendor hash_map
#ifndef HASH_HPP
#define HASH_HPP

#ifdef __GNUC__
      #if __GNUC__ < 3
        #include <hash_map.h>
        namespace stdext { using ::hash_map; }; // inherit globals
      #else
        #include <ext/hash_map>
        #if __GNUC_MINOR__ == 0
          namespace stdext = std;               // GCC 3.0
        #else
          namespace stdext = ::__gnu_cxx;       // GCC 3.1 and later
        #endif
      #endif
#else      // ...  there are other compilers, right?
#include <hash_map>
#if !(defined(_MSC_VER))
//&& _MSC_VER >= 1300)
namespace stdext = std;
#endif

#endif // GNUC
//using stdext::hash_map;
//using stdext::hash_set;

#ifdef TEST
#include <graehl/shared/test.hpp>
#endif

#ifdef TWO_HASH_H
/*template <class K,class V> typename HashTable<K,V>::value_type::second_type *find_second(const HashTable<K,V>& ht,const K& k) {return ht.find_second(k);}
*/
#endif
/*
template <class H,class K>
typename H::value_type::second_type *find_second(const H& ht,const K& k)
{
  if (h::iterator_type i=ht.find(k) != ht.end())
    return &(h->second);
  else
    return NULL;
}
*/

#ifdef TEST
#include <graehl/shared/../carmel/src/stringkey.h>



BOOST_AUTO_UNIT_TEST( hash)
{
  stdext::hash_map<int,int> hm;
  hm[0]=1;
  BOOST_CHECK(hm.find(0)!=hm.end());
  BOOST_CHECK(hm.find(1)==hm.end());
  StringKey s("no");
  stdext::hash_map<StringKey,int> sm;
  sm["yes"]=0;
  sm[s]=1;
  sm["maybe"]=2;
  BOOST_CHECK(sm["no"] == 1);
  BOOST_CHECK(sm.find("maybe") != sm.end());
  BOOST_CHECK(sm.find("asdf") == sm.end());
}
#endif

#endif
