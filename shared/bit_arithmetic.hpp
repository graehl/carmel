#ifndef GRAEHL__SHARED__BIT_ARITHMETIC_HPP
#define GRAEHL__SHARED__BIT_ARITHMETIC_HPP

#include <cassert>
#include <limits>
#include <boost/cstdint.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <limits.h>
#ifndef CHAR_BIT
# define CHAR_BIT 8
#endif

#if defined(_MSC_VER)

#define GRAEHL_FORCE_INLINE __forceinline

#include <stdlib.h>

#define GRAEHL_ROTL32(x, y)  _rotl(x, y)
#define GRAEHL_ROTL64(x, y)  _rotl64(x, y)

#define GRAEHL_BIG_CONSTANT(x) (x)

#else

#define GRAEHL_FORCE_INLINE inline __attribute__((always_inline))

#define GRAEHL_ROTL32(x, y)  graehl::bit_rotate_left(x, y)
#define GRAEHL_ROTL64(x, y)  graehl::bit_rotate_left(x, y)

#define GRAEHL_BIG_CONSTANT(x) (x##LLU)

#endif


namespace graehl {

using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;
using boost::uint64_t;

inline void mixbits(uint64_t &h) {
  h ^= h >> 23;
  h *= GRAEHL_BIG_CONSTANT(0x2127599bf4325c37);
  h ^= h >> 47;
}

template <class Int>
inline bool is_power_of_2(Int i) {
  return (i & (i-1)) == 0;
}

/// return power of 2 >= x
inline uint32_t next_power_of_2(uint32_t x) {
  assert(x <= (1 << 30));
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  ++x;
  assert(is_power_of_2(x));
  return x;
}

/// return power of 2 >= x
inline uint64_t next_power_of_2(uint64_t x) {
  assert(x <= (1ULL << 60));
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  ++x;
  assert(is_power_of_2(x));
  return x;
}

inline
unsigned count_set_bits(uint32_t i)
{
  i = i - ((i >> 1) & 0x55555555);
  i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
  return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
}

inline
unsigned count_set_bits(int32_t i)
{
  return count_set_bits((uint32_t)i);
}

inline
unsigned count_set_bits(uint64_t x)
{
  return count_set_bits((uint32_t)x)+count_set_bits((uint32_t)(x>>32));
}

/// while this is covered by the complicated generic thing below, I want to be sure the hash fn. uses the right code
inline
uint32_t bit_rotate_left(uint32_t x, int8_t k)
{
#ifdef _MSC_VER
  return _rotl(x, k);
#else
  return (x << k) | (x>>(32-k));
#endif
}

inline
uint32_t bit_rotate_right(uint32_t x, int8_t k)
{
  return (x>>k) | (x << (32-k));
}

GRAEHL_FORCE_INLINE
uint64_t bit_rotate_left_64(uint64_t x, int8_t k)
{
#ifdef _MSC_VER
  return _rotl64(x, k);
#else
  return (x << k) | (x>>(64-k));
#endif
}

GRAEHL_FORCE_INLINE
uint64_t bit_rotate_right_64(uint64_t x, int8_t k)
{
  return (x>>k) | (x << (64-k));
}

// bit i=0 = lsb
template <class I, class J>
inline void set(typename boost::enable_if<boost::is_integral<I> >::type &bits, J i) {
  assert(i<(CHAR_BIT*sizeof(I)));
  I mask = (1 << i);
  bits |= mask;
}

template <class I>
inline void set_mask(I &bits, I mask) {
  bits |= mask;
}

template <class I, class J>
inline void reset(typename boost::enable_if<boost::is_integral<I> >::type &bits, J i) {
  assert(i<(CHAR_BIT*sizeof(I)));
  I mask = (1 << i);
  bits &= ~mask;
}

template <class I>
inline void reset_mask(I &bits, I mask) {
  bits &= ~mask;
}

template <class I, class J>
inline void set(typename boost::enable_if<boost::is_integral<I> >::type &bits, J i, bool to) {
  assert(i<(CHAR_BIT*sizeof(I)));
  I mask = (1 << i);
  if (to) set_mask(bits, mask); else reset_mask(bits, mask);
}

template <class I>
inline void set_mask(I &bits, I mask, bool to) {
  if (to) set_mask(bits, mask); else reset_mask(bits, mask);
}

template <class I, class J>
inline bool test(typename boost::enable_if<boost::is_integral<I> >::type bits, J i) {
  assert(i<(CHAR_BIT*sizeof(I)));
  I mask = (1 << i);
  return mask&bits;
}

// if any of mask
template <class I>
inline bool test_mask(typename boost::enable_if<boost::is_integral<I> >::type bits, I mask) {
  return mask&bits;
}

// return true if was already set, then set.
template <class I, class J>
inline bool latch(typename boost::enable_if<boost::is_integral<I> >::type &bits, J i) {
  assert(i<(CHAR_BIT*sizeof(I)));
  I mask = (1 << i);
  bool r = mask&bits;
  bits |= mask;
  return r;
}

template <class I>
inline bool latch_mask(typename boost::enable_if<boost::is_integral<I> >::type &bits, I mask) {
  bool r = mask&bits;
  bits |= mask;
  return r;
}

template <class I>
inline bool test_mask_all(typename boost::enable_if<boost::is_integral<I> >::type bits, I mask) {
  return (mask&bits)==mask;
}

////////////////////////////////////////////////////////////////////////////////
//
// the reason for the remove_cv stuff is that the compiler wants to turn
// the call
//    size_t x;
//    bit_rotate(x,3)
// into an instantiation of
//    template<class size_t&, int const>
//    size_t& bit_rotate_left(size_t&,int const);
//
////////////////////////////////////////////////////////////////////////////////
template <class I, class J>
inline typename boost::enable_if< typename boost::is_integral<I>
                                  , typename boost::remove_cv<I>::type
                                  >::type
bit_rotate_left(I x, J k)
{
  typedef typename boost::remove_cv<I>::type IT;
  assert(k < std::numeric_limits<IT>::digits);
  assert(std::numeric_limits<IT>::digits == CHAR_BIT * sizeof(IT));
  return ((x << k) | (x>>(std::numeric_limits<IT>::digits-k)));
}

template <class I, class J>
inline typename boost::enable_if< typename boost::is_integral<I>
                                  , typename boost::remove_cv<I>::type
                                  >::type
bit_rotate_right(I x, J k)
{
  typedef typename boost::remove_cv<I>::type IT;
  assert(k < std::numeric_limits<IT>::digits);
  assert(std::numeric_limits<IT>::digits == CHAR_BIT * sizeof(IT));
  return ((x << (std::numeric_limits<IT>::digits-k)) | (x>>k));
}

/// interpret the two bytes at d as a uint16 in little endian order
inline uint16_t unpack_uint16_little(void const*d)
{
  //FIXME: test if the #ifdef optimization is even needed (compiler may optimize portable to same?)
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__)    \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
  return *((const uint16_t *) (d));
#else
  return ((((uint32_t)(((const uint8_t *)(d))[1])) << CHAR_BIT)\
          +(uint32_t)(((const uint8_t *)(d))[0]) );
#endif
}

}//graehl

#endif
