#ifndef GRAEHL__SHARED__HASH_FUNCTIONS_HPP
#define GRAEHL__SHARED__HASH_FUNCTIONS_HPP

#include <graehl/shared/function_macro.hpp>

#ifndef GOLDEN_MEAN_FRACTION
#define GOLDEN_MEAN_FRACTION 2654435769U
#endif

namespace graehl {

inline std::size_t cstr_hash (const char *p)
{
        std::size_t h=0;
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


#define HASH_FROM_FUNCTION(funcname) FUNCTION_OBJ_WRAP(funcname,std::size_t)

HASH_FROM_FUNCTION(cstr_hash);


inline std::size_t uint_hash(unsigned int key)
{
  /*
In Knuth's "The Art of Computer Programming", section 6.4, a multiplicative hashing scheme is introduced as a way to write hash function. The key is multiplied by the golden ratio of 2^32 (2654435761) to produce a hash result.

Since 2654435761 and 2^32 have no common factors in common, the multiplication produces a complete mapping of the key to hash result with no overlap. This method works pretty well if the keys have small values. Bad hash results are produced if the keys vary mostly in the upper bits. As is true in all multiplications, variations of upper digits do not influence the lower digits of the multiplication result.
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
  return (std::size_t)tmp;
# else

#ifndef TRIVIAL_INT_HASH
  key *= GOLDEN_MEAN_FRACTION; // mixes the lower bits into the upper bits, reversible
  key ^= (key >> 16); // gets some of the goodness back into the lower bits, reversible
  return (std::size_t)key;
#else
  return (std::size_t)key;
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

HASH_FROM_FUNCTION(uint_hash);

inline std::size_t mix_hash(std::size_t a, std::size_t b)
{
    a ^= (b<<1);
    if (sizeof(std::size_t) == 4)
        a ^= (b>>31 & 0x1); // you could just not do this and lose the MSB of b.
        // faster and probably not more collisions.  I
        // suppose with multiple mixhash it becomes an issue
    return a;

//     std::size_t tmp;
//     tmp = a;
//     tmp ^= (b<<1);
// #if sizeof(std::size_t)==4
//         tmp ^= (b>>31 & 0x1); // you could just not do this and lose the MSB of b.
//         // faster and probably not more collisions.  I
//         // suppose with multiple mixhash it becomes an issue
// # else
// #  if sizeof(std::size_t)!=8
// #   error please adjust bit rotation for std::size_t (which is not 4 or 8 bytes)
// #  endif
// //    else if (sizeof(std::size_t)==8)
//         tmp ^= (b>>63 & 0x1);
// #  endif
//     return tmp;
}

template <class I1,class I2,class Hval>
inline std::size_t hash_range(I1 i,I2 end,Hval h) 
{
    std::size_t hashed=0;
    for (;i!=end;++i) {
        hashed=mix_hash(hashed,h(*i));
    }
    return hashed;
}

}


#endif
