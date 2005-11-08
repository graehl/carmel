// for sources defining hash functions but not using hash tables themselves
#ifndef HASHTABLE_FWD_HPP
#define HASHTABLE_FWD_HPP

#define GOLDEN_MEAN_FRACTION 2654435769U

inline size_t cstr_hash (const char *p)
{
        size_t h=0;
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

#ifndef GOLDEN_MEAN_FRACTION
#define GOLDEN_MEAN_FRACTION=2654435769U;
#endif
inline size_t uint_hash(unsigned int key)
{
  /*
In Knuth's "The Art of Computer Programming", section 6.4, a multiplicative hashing scheme is introduced as a way to write hash function. The key is multiplied by the golden ratio of 2^32 (2654435761) to produce a hash result.

Since 2654435761 and 2^32 has no common factors in common, the multiplication produces a complete mapping of the key to hash result with no overlap. This method works pretty well if the keys have small values. Bad hash results are produced if the keys vary in the upper bits. As is true in all multiplications, variations of upper digits do not influence the lower digits of the multiplication result.
*/
// (sqrt(5)-1)/2 = .6180339887.., * 2^32=~2654435769.4972302964775847707926

// HOWEVER (not in Knuth) ... the higher order bits after multiplication are determined by all the bits below it as well.  the "good" part of the hash is lost in the higher bits if you are using power-of-2 buckets (prime # buckets is fine), therefore, shift some of those good bits over and combine them with the lower (by using xor instead of addition, this should continue to make the function reversible)
//      return key * 2654435767U;
//      return key * 2654435761U;
        //return key*GOLDEN_MEAN_FRACTION;

// feel free to define TRIVIAL_INT_HASH if you're hashing into prime-numbers of buckets
// but first FIXME: we're using the same primes that are in boost::unordered_map bucketlist for multiplying out int-pair hashvals
    #ifdef EXPENSIVE_INT_HASH
  int tmp = key;

  if (key&0x1) {
    tmp^= 1798151309;
  }

  tmp += ~(tmp << 15);
  tmp ^=  ((unsigned)tmp >> 10);
  tmp +=  (tmp << 3);
  tmp ^=  ((unsigned)tmp >> 6);
  tmp += ~(tmp << 11);
  tmp ^=  ((unsigned)tmp >> 16);
  tmp +=  (tmp << 7);
  tmp ^= ((unsigned)tmp>> 22);
  return (size_t)tmp;
# else

#ifndef TRIVIAL_INT_HASH
  key *= GOLDEN_MEAN_FRACTION; // mixes the lower bits into the upper bits, reversible
  key ^= (key >> 16); // gets some of the goodness back into the lower bits, reversible
  return (size_t)key;
#else
  return (size_t)key;
#endif
#endif
  /*
    key -= (key << 15);
    key ^=  (key >> 10);
    key +=  (key << 3);
    key ^=  (key >> 6);
    key -= (key << 11);
    key ^=  (key >> 16);
    return key;
  */

}

inline size_t mix_hash(size_t a, size_t b)
{
    a ^= (b<<1);
    if (sizeof(size_t) ==4)
        a ^= (b>>31 & 0x1); // you could just not do this and lose the MSB of b.
        // faster and probably not more collisions.  I
        // suppose with multiple mixhash it becomes an issue
    return a;

//     size_t tmp;
//     tmp = a;
//     tmp ^= (b<<1);
// #if sizeof(size_t)==4
//         tmp ^= (b>>31 & 0x1); // you could just not do this and lose the MSB of b.
//         // faster and probably not more collisions.  I
//         // suppose with multiple mixhash it becomes an issue
// # else
// #  if sizeof(size_t)!=8
// #   error please adjust bit rotation for size_t (which is not 4 or 8 bytes)
// #  endif
// //    else if (sizeof(size_t)==8)
//         tmp ^= (b>>63 & 0x1);
// #  endif
//     return tmp;
}

template <class I1,class I2,class Hval>
inline size_t hash_range(I1 i,I2 end,Hval h) 
{
    size_t hashed=0;
    for (;i!=end;++i) {
        hashed=mix_hash(hashed,h(*i));
    }
    return hashed;
}


#ifdef GNU_HASH_MAP
# define USE_STD_HASH_MAP
# ifndef __NO_GNU_NAMESPACE__
//using namespace __gnu_cxx;
# endif
# include <ext/hash_map>
# define HashTable __gnu_cxx::hash_map
# define HASHNS  __gnu_cxx::          
# define HASHNS_B namespace __gnu_cxx {
# define HASHNS_E }
# include <string>
HASHNS_B
template <class Char,class T,class A>
struct hash<std::basic_string<Char,T,A> > 
{
    typedef std::basic_string<Char,T,A> arg_type;
    const size_t operator()( const  arg_type &s ) const 
    {
                size_t h=0;
//        return cstr_hash(s.c_str());
                for (typename arg_type::const_iterator i=s.begin(),e=s.end();i!=e;++i)
                h = 31 * h + *i; // should optimize to ( h << 5 ) - h if faster
        return h;
        
    }
};
HASHNS_E
#endif

#ifdef UNORDERED_MAP
# define USE_STD_HASH_MAP
//#error
#include <boost/unordered_map.hpp>
// no template typedef so ...
#define HashTable boost::unordered_map
#define HASHNS boost::          
#define HASHNS_B namespace boost {
#define HASHNS_E }

/*
template <class K,class V,class H,class P,class A>
inline typename HashTable<K,V,H,P,A>::iterator find_value(const HashTable<K,V,H,P,A>& ht,const K& first) {
  return ht.find(first);
}
*/


#endif


#ifdef USE_STD_HASH_MAP
template <class K,class V,class H,class P,class A>
inline V *add(HashTable<K,V,H,P,A>& ht,const K&k,const V& v=V())
{
  return &(ht[k]=v);
}
#else
// graehl hash map

template <class K,class V,class H,class P,class A> class HashTable ;

#define HASHNS
#define HASHNS_B
#define HASHNS_E

template <class T> struct hash;

  template <class C>
  struct hash { size_t operator()(const C &c) const { return c.hash(); } };
#endif

template<class T1,class T2,class T3,class T4,class T5,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const HashTable<T1,T2,T3,T4,T5>& t) {
  typename HashTable<T1,T2,T3,T4,T5>::const_iterator i=t.begin();
  out << "begin" << std::endl;
  for (;i!=t.end();++i) {
        out << *i << std::endl;
  }
  out << "end" << std::endl;
  return out;
}

template<class C1,class C2,class A,class B>
inline
std::basic_ostream<A,B>&
         operator<< (std::basic_ostream<A,B> &out, const std::pair< C1, C2 > &p)
{
  return out << '(' << p.first << ',' << p.second << ')';
}

#define BEGIN_HASH_VAL(C) \
HASHNS_B \
template<> struct hash<C> \
{ \
  size_t operator()(const C x) const



#define BEGIN_HASH(C) \
HASHNS_B \
template<> struct hash<C> \
{ \
  size_t operator()(const C& x) const

#define END_HASH        \
};\
HASHNS_E

HASHNS_B
  template<class P> struct hash<P *>
  {
    const size_t operator()( const P *x ) const
    {
//      const unsigned GOLDEN_MEAN_FRACTION=2654435769U;
      return ((size_t)x / sizeof(P))*GOLDEN_MEAN_FRACTION;
    }
  };
HASHNS_E

template <class C,class H=
          HASHNS hash<typename C::value_type>
>
struct hash_container
{
    const size_t operator()(const C& c) const 
    {
        return hash_range(c.begin(),c.end(),H());
    }
};


template <class K,class V,class H,class P,class A>
inline V *find_second(const HashTable<K,V,H,P,A>& ht,const K& first)
{
  return ht.find_second(first);
}

template <class K,class V,class H,class P,class A>
inline
  typename HashTable<K,V,H,P,A>::insert_return_type insert(HashTable<K,V,H,P,A>& ht,const K& first,const V &v=V())
{
  return ht.insert(first,v);
}

#endif
