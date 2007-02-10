#ifndef GRAEHL__SHARED__BIT_ARITHMETIC_HPP
#define GRAEHL__SHARED__BIT_ARITHMETIC_HPP

#include <boost/cstdint.hpp>

namespace graehl {

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

//FIXME: make sure this is as fast as macro version - supposed to compile (optimized) to native rotate instruction
inline uint32_t bit_rotate_left(uint32_t x,unsigned k) 
{
    return ((x<<k) | (x>>(32-k)));
}

//FIXME: untested (change of variable: old k -> new 32-k (and vice versa)
inline uint32_t bit_rotate_right(uint32_t x,unsigned k)
{
    return ((x<<(32-k)) | (x>>k));
}

//FIXME: untested (esp. on 32 bit)
inline uint64_t bit_rotate_left(uint64_t x,unsigned k) 
{
    return ((x<<k) | (x>>(64-k)));
}

//FIXME: untested (esp. on 32 bit)
inline uint64_t bit_rotate_right(uint64_t x,unsigned k) 
{
    return ((x<<(64-k)) | (x>>k));
}

/// interpret the two bytes at d as a uint16 in little endian order
inline uint16_t unpack_uint16_little(void const*d) 
{
//FIXME: test if the #ifdef optimization is even needed (compiler may optimize portable to same?)
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
return *((const uint16_t *) (d)));
#else
return ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
        +(uint32_t)(((const uint8_t *)(d))[0]) );
#endif
}

}//graehl

#endif
