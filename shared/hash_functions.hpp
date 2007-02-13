#ifndef GRAEHL__SHARED__HASH_FUNCTIONS_HPP
#define GRAEHL__SHARED__HASH_FUNCTIONS_HPP

#include <boost/cstdint.hpp>
#include <graehl/shared/function_macro.hpp>
#include <graehl/shared/hash_jenkins.hpp>

namespace graehl {

///golden ratio: 1.6180339887498948482045868343656381177203
//static const double golden_ratio=1.6180339887498948482045868343656381177203;
static const uint32_t golden_ratio_fraction_32=2654435769U; // (floor of 2^32/golden_ratio)
static const uint64_t golden_ratio_fraction_64=0x9E3779B97F4A7C15ULL; // (floor of 2^64/golden_ratio)

/// seed (should not be 0) can be used to chain (combine) several hashes together
inline uint64_t hash_quads_64(
    const uint32_t *k,                   /* the key, an array of uint32_t values */
    size_t          length,               /* the length of the key, in uint32_ts */
    uint64_t seed=golden_ratio_fraction_64) // note: if seed is 0 then the returned hash has same upper/lower 32 bits
{
    uint32_t *pc=reinterpret_cast<uint32_t*>(&seed);
    hashword2(k,length,pc,pc+1); //FIXME: warning about breaking strict aliasing - how does one annotate code such that strict aliasing optimizations are used?
    return seed;
}

inline uint64_t hash_bytes_64(
    const void *k,                   /* the key, an array of bytes */
    size_t          length,               /* the length of the key, in uint32_ts */
    uint64_t seed=golden_ratio_fraction_64)
{
    uint32_t *pc=reinterpret_cast<uint32_t*>(&seed);
    hashlittle2(k,length,pc,pc+1);
    return seed;
}


/// sucks for arbitrary bytestrings (yay ascii/en)
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


/* // hashes one char at a time
   uint32_t one_at_a_time(char *key, uint32_t len)
   {
   uint32_t   hash, i;
   for (hash=0, i=0; i<len; ++i)
   {
   hash += key[i];
   hash += (hash << 10);
   hash ^= (hash >> 6);
   }
   hash += (hash << 3);
   hash ^= (hash >> 11);
   hash += (hash << 15);
   return (hash & mask);
   }
*/


/// Bob Jenkins' "One at a time" hash: http://burtleburtle.net/bob/hash/doobs.html
struct incremental_hasher 
{
    uint32_t a;
      
    incremental_hasher(uint32_t seed=golden_ratio_fraction_64) : a(seed) {}
      
    template <class C>
    void append(C c) 
    {
        a+=c;
        a+= a<<10;
        a^=a>>6;
    }
      
    uint32_t val() const
    {
        uint32_t r=a;
        r+=r<<3;
        r^=r>>11;
        r+=r<<15;
        return r;
    }
};


#define HASH_FROM_FUNCTION(funcname) FUNCTION_OBJ_WRAP(funcname,std::size_t)
#define HASH_FROM_FUNCTION_RETURNING(funcname,returns) FUNCTION_OBJ_WRAP(funcname,returns)

//cstr_hash_f
HASH_FROM_FUNCTION(cstr_hash);


inline boost::uint32_t uint32_hash(boost::uint32_t a)
{
    /*
      In Knuth's "The Art of Computer Programming", section 6.4, a multiplicative hashing scheme is introduced as a way to write hash function. The key is multiplied by the golden ratio of 2^32 (2654435761) to produce a hash result.

      Since 2654435761 and 2^32 have no common factors in common, the multiplication produces a complete mapping of the key to hash result with no overlap. This method works pretty well if the keys have small values. Bad hash results are produced if the keys vary mostly in the upper bits. As is true in all multiplications, variations of upper digits do not influence the lower digits of the multiplication result.
    */
// (sqrt(5)-1)/2 = .6180339887.., * 2^32=~2654435769.4972302964775847707926

// HOWEVER (not in Knuth) ... the higher order bits after multiplication are determined by all the bits below it as well.  the "good" part of the hash is lost in the higher bits if you are using power-of-2 buckets (prime # buckets is fine), therefore, shift some of those good bits over and combine them with the lower (by using xor instead of addition, this should continue to make the function reversible)
//      return key * 2654435767U;
//      return key * 2654435761U;
    //return key*golden_ratio_fraction_32;

// feel free to define TRIVIAL_INT_HASH if you're hashing into prime-numbers of buckets
// but first FIXME: we're using the same primes that are in boost::unordered_map bucketlist for multiplying out int-pair hashvals
    using boost::uint32_t;
#ifdef EXPENSIVE_INT_HASH
/*
    if (a&0x1) {
        a^= 1798151309;
    }

    a += ~(a << 15);
    a ^=  ((uint32_t)a >> 10);
    a +=  (a << 3);
    a ^=  ((uint32_t)a >> 6);
    a += ~(a << 11);
    a ^=  ((uint32_t)a >> 16);
    a +=  (a << 7);
    a ^= ((uint32_t)a>> 22);
*/
    // (provided you use the low bits, hash & (SIZE-1), rather than the high bits if you can't use the whole value):
#      if 1
    a += ~(a<<15);
    a ^=  (a>>10);
    a +=  (a<<3);
    a ^=  (a>>6);
    a += ~(a<<11);
    a ^=  (a>>16);
    /*
      a -= (a << 15);
      a ^=  (a >> 10);
      a +=  (a << 3);
      a ^=  (a >> 6);
      a -= (a << 11);
      a ^=  (a >> 16);
    */
#      else
    // bob jenkins (do large constants have a performance impact?  larger code/cache impact at least
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
#     endif 
# else // not expensive
#ifndef TRIVIAL_INT_HASH
    a *= golden_ratio_fraction_32; // mixes the lower bits into the upper bits, reversible
    a ^= (a >> 16); // gets some of the goodness back into the lower bits, reversible
#endif
#endif
    return a;
}

// uint32_hash_f
HASH_FROM_FUNCTION_RETURNING(uint32_hash,boost::uint32_t);

inline std::size_t mix_hash(std::size_t a, std::size_t b)
{
    a ^= bit_rotate_left(b,1);
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
inline std::size_t hash_range(I1 i,I2 end,Hval h,std::size_t seed=0) 
{
    for (;i!=end;++i) {
        seed=mix_hash(seed,h(*i));
    }
    return seed;
}

template <class I1,class I2>
inline uint64_t hash_range_pod(I1 i,I2 end,uint64_t seed=golden_ratio_fraction_64) 
{
    for (;i!=end;++i)
        seed=hash_bytes_64(&*i,sizeof(typename I1::value_type),seed);
    return seed;
}


/// NOTE: only std::vector and similar (actual contiguous array layout) containers will work; others will segfault
template <class Vec>
inline uint64_t hash_pod_vector(Vec const& v,uint64_t seed=golden_ratio_fraction_64) 
{
    typedef typename Vec::value_type Val;
    //FIXME: detect weird array alignment requirements e.g. to 8 bytes
    if (sizeof(Val)%4 == 0) {
        return hash_quads_64(reinterpret_cast<uint32_t const*>(&*v.begin()),(sizeof(Val)/sizeof(uint32_t))*v.size(),seed);
    } else if (sizeof(Val)==2 || sizeof(Val)==1) {
        return hash_bytes_64(&*v.begin(),sizeof(Val)*v.size(),seed);
    } else {
        // there's alignment to 4 bytes padding; so we can't trust it to be constant
//        return hash_range(v.begin(),v.end(),boost::hash<Val>()); //,uint32_hash_f()
        return hash_range_pod(v.begin(),v.end(),seed);
    }   
}

/* Paul Hsieh: http://www.azillionmonkeys.com/qed/hash.html - faster than Jenkins (and has same good properties) but only 32 bit hash value, no seed chaining (although chaining would be a minor tweak)
 */
inline uint32_t hash_bytes_32 (void const* k, int len) {
    char const* data=(char const* )k;
    uint32_t hash = len, tmp;
    int rem;
    
    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += unpack_uint16_little (data);
        tmp    = (unpack_uint16_little (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
    case 3: hash += unpack_uint16_little (data);
        hash ^= hash << 16;
        hash ^= data[sizeof (uint16_t)] << 18;
        hash += hash >> 11;
        break;
    case 2: hash += unpack_uint16_little (data);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += *data;
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* Force "avalanching" of final 127 bits */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

}


#endif
