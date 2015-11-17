// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// (unused) wrapper for the (non-STL-standard) vendor hash_map
#ifndef GRAEHL_SHARED__HASH_HPP
#define GRAEHL_SHARED__HASH_HPP

#include <stdexcept>
#include <utility> // this is just to get stlport macros in

#if defined(_STLPORT_VERSION) || defined(__SGI_STL_PORT)
# include <hash_map>
# ifndef stdext_ns_alias_defined
#  define stdext_ns_alias_defined
namespace stdext = ::std;
namespace stdextp = ::std;
# endif
#else
# ifdef __GNUC__
#  if __GNUC__ < 3
#   include <hash_map.h>
namespace stdext { using ::hash_map; }; // inherit globals
#  else
#   ifdef __GXX_EXPERIMENTAL_CXX0X__
#    include <tr1/unordered_map>
#    define stdext::hash_map std::tr1::unordered_map
#   else
#    include <ext/hash_map>
#    if __GNUC__ == 3 && __GNUC_MINOR__ == 0
namespace stdext = ::std;               // GCC 3.0
#    else
namespace stdext = ::__gnu_cxx;       // GCC 3.1 and later
#    endif
namespace stdextp = ::std;
#   endif
#  endif
# else // ...  there are other compilers, right?
#  if defined(_MSC_VER)
#   if MSC_VER >= 1500)
#   include <unordered_map>
#   define stdext::hash_map ::std::unordered_map
#   else
#    if MSC_VER >= 1300)
#     include <hash_map>
namespace stdext = ::std;
#    endif
#   endif
#  endif
# endif
#endif

//using stdext::hash_map;
//using stdext::hash_set;

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

// like ht[k]=v, but you want to check your belief that ht[k] didn't exist before.  also may be faster
template <class K, class V, class H, class E, class A>
void add_new(stdext::hash_map<K, V, H, E, A> & ht, K const& key, V const &value = V())
{
  typedef stdext::hash_map<K, V, H, E, A> HT;
  stdextp::pair<typename HT::iterator, bool> r = ht.insert(typename HT::value_type(key, value));
  if (!r.second)
    throw std::runtime_error("Key already existed in add_new(key,val)");
}

// equivalent to ht[k]=v, may be faster (not likely)
template <class K, class V, class H, class E, class A>
void set_val(stdext::hash_map<K, V, H, E, A> & ht, K const& key, V const &value = V())
{
  typedef stdext::hash_map<K, V, H, E, A> HT;
  stdextp::pair<typename HT::iterator, bool> r = ht.insert(typename HT::value_type(key, value));
  if (!r.second)
    const_cast<V&>(r.first->second) = value;
}

// adds default val to table if key wasn't found, returns ref to val
template <class H, class K>
typename H::mapped_type & get_default(H &ht, K const& k, typename H::mapped_type const& v) {
  return const_cast<typename H::mapped_type &>(ht.insert(typename H::value_type(k, v)).first->second);
}

// the below could also return a ref to the mapped max/min.  they have the advantage of not falsely claiming an improvement when an equal value already existed.  otherwise you could just modify the get_default and if equal assume new.
template <class H, class K>
bool improve_mapped_max(H &ht, K const& k, typename H::mapped_type const& v) {
  std::pair<typename H::iterator, bool> inew = ht.insert(typename H::value_type(k, v));
  if (inew.second) return true;
  typedef typename H::mapped_type V;
  V &oldv = const_cast<V&>(inew.first->second);
  if (oldv<v) {
    oldv = v;
    return true;
  }
  return false;
}

template <class H, class K>
bool improve_mapped_min(H &ht, K const& k, typename H::mapped_type const& v) {
  std::pair<typename H::iterator, bool> inew = ht.insert(typename H::value_type(k, v));
  if (inew.second) return true;
  typedef typename H::mapped_type V;
  V &oldv = const_cast<V&>(inew.first->second);
  if (v<oldv) { // the only difference from above
    oldv = v;
    return true;
  }
  return false;
}

}

/*
  V * find_second(K const& key)
  {
  typename HT::iterator i=ht().find(key);
  return i==ht().end() ? 0 : &const_cast<V&>(i->second);
  }
  V const* find_second(K const& key) const
  {
  return const_cast<self_type *>(this)->find_second(key);
  }
*/

#ifdef GRAEHL_TEST
#define USE_GNU_HASH_MAP
#include <graehl/shared/stringkey.h>

BOOST_AUTO_TEST_CASE( hash)
{
  stdext::hash_map<int, int> hm;
  using namespace graehl;
  hm[0] = 1;
  BOOST_CHECK(hm.find(0)!=hm.end());
  BOOST_CHECK(hm.find(1)==hm.end());
  StringKey s("no");
  stdext::hash_map<StringKey, int> sm;
  sm["yes"] = 0;
  sm[s] = 1;
  sm["maybe"] = 2;
  add_new(sm, StringKey("feel"), 3);
  BOOST_CHECK(sm["feel"] == 3);
  graehl::set_val(sm, graehl::StringKey("feel"), 4);
  BOOST_CHECK(sm["feel"] == 4);
  set_val(sm, StringKey("good"), 5);
  BOOST_CHECK(sm["good"] == 5);
  BOOST_CHECK(sm["no"] == 1);
  BOOST_CHECK(sm.find("maybe") != sm.end());
  BOOST_CHECK(sm.find("asdf") == sm.end());
}
#endif

#endif
