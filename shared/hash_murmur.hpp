/** \file

    usage (MurmurHash3):

    uint32_t hash = MurmurHash(val, sizeof(val), seed) [optional uint32_t seed defaults to 0]

    uint64_t widehash = WideMurmurHash(val, sizeof(val), seed) (equally fast as 32 for a 16 byte key on x64 but probably a bit slower / higher latency for odd sizes) - value differs across 32 and 64 bit
*/

#ifndef GRAEHL__SHARED__HASH_MURMUR_HPP
#define GRAEHL__SHARED__HASH_MURMUR_HPP

#include <graehl/shared/have_64_bits.hpp>
#include <graehl/shared/bit_arithmetic.hpp>

namespace graehl {

typedef uint32_t MurmurHashResult;
typedef uint32_t MurmurSeed;

static const MurmurSeed GRAEHL_MURMUR_DEFAULT_SEED = 0U;

// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio


//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

GRAEHL_FORCE_INLINE uint32_t getblock32 ( const uint32_t * p, int i )
{
  return p[i];
}

GRAEHL_FORCE_INLINE uint64_t getblock64 ( const uint64_t * p, int i )
{
  return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

GRAEHL_FORCE_INLINE uint32_t fmix32 ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

//----------

GRAEHL_FORCE_INLINE uint64_t fmix64 ( uint64_t k )
{
  k ^= k >> 33;
  k *= GRAEHL_BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= GRAEHL_BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

inline void MurmurHash3_x86_128 ( const void * key, const int len,
                           uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;

  uint32_t h1 = seed;
  uint32_t h2 = seed;
  uint32_t h3 = seed;
  uint32_t h4 = seed;

  const uint32_t c1 = 0x239b961b;
  const uint32_t c2 = 0xab0e9789;
  const uint32_t c3 = 0x38b34ae5;
  const uint32_t c4 = 0xa1e38b93;

  //----------
  // body

  const uint32_t * blocks = (const uint32_t *)(data + nblocks*16);

  for(int i = -nblocks; i; i++)
  {
    uint32_t k1 = getblock32(blocks, i*4+0);
    uint32_t k2 = getblock32(blocks, i*4+1);
    uint32_t k3 = getblock32(blocks, i*4+2);
    uint32_t k4 = getblock32(blocks, i*4+3);

    k1 *= c1; k1  = GRAEHL_ROTL32(k1,15); k1 *= c2; h1 ^= k1;

    h1 = GRAEHL_ROTL32(h1,19); h1 += h2; h1 = h1*5+0x561ccd1b;

    k2 *= c2; k2  = GRAEHL_ROTL32(k2,16); k2 *= c3; h2 ^= k2;

    h2 = GRAEHL_ROTL32(h2,17); h2 += h3; h2 = h2*5+0x0bcaa747;

    k3 *= c3; k3  = GRAEHL_ROTL32(k3,17); k3 *= c4; h3 ^= k3;

    h3 = GRAEHL_ROTL32(h3,15); h3 += h4; h3 = h3*5+0x96cd1c35;

    k4 *= c4; k4  = GRAEHL_ROTL32(k4,18); k4 *= c1; h4 ^= k4;

    h4 = GRAEHL_ROTL32(h4,13); h4 += h1; h4 = h4*5+0x32ac3b17;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint32_t k1 = 0;
  uint32_t k2 = 0;
  uint32_t k3 = 0;
  uint32_t k4 = 0;

  switch(len & 15)
  {
  case 15: k4 ^= tail[14] << 16;
  case 14: k4 ^= tail[13] << 8;
  case 13: k4 ^= tail[12] << 0;
           k4 *= c4; k4  = GRAEHL_ROTL32(k4,18); k4 *= c1; h4 ^= k4;

  case 12: k3 ^= tail[11] << 24;
  case 11: k3 ^= tail[10] << 16;
  case 10: k3 ^= tail[ 9] << 8;
  case  9: k3 ^= tail[ 8] << 0;
           k3 *= c3; k3  = GRAEHL_ROTL32(k3,17); k3 *= c4; h3 ^= k3;

  case  8: k2 ^= tail[ 7] << 24;
  case  7: k2 ^= tail[ 6] << 16;
  case  6: k2 ^= tail[ 5] << 8;
  case  5: k2 ^= tail[ 4] << 0;
           k2 *= c2; k2  = GRAEHL_ROTL32(k2,16); k2 *= c3; h2 ^= k2;

  case  4: k1 ^= tail[ 3] << 24;
  case  3: k1 ^= tail[ 2] << 16;
  case  2: k1 ^= tail[ 1] << 8;
  case  1: k1 ^= tail[ 0] << 0;
           k1 *= c1; k1  = GRAEHL_ROTL32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len; h3 ^= len; h4 ^= len;

  h1 += h2; h1 += h3; h1 += h4;
  h2 += h1; h3 += h1; h4 += h1;

  h1 = fmix32(h1);
  h2 = fmix32(h2);
  h3 = fmix32(h3);
  h4 = fmix32(h4);

  h1 += h2; h1 += h3; h1 += h4;
  h2 += h1; h3 += h1; h4 += h1;

  ((uint32_t*)out)[0] = h1;
  ((uint32_t*)out)[1] = h2;
  ((uint32_t*)out)[2] = h3;
  ((uint32_t*)out)[3] = h4;
}

//-----------------------------------------------------------------------------

inline void MurmurHash3_x64_128 ( const void * key, const int len,
                           const uint32_t seed, void * out )
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = GRAEHL_BIG_CONSTANT(0x87c37b91114253d5);
  const uint64_t c2 = GRAEHL_BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(int i = 0; i < nblocks; i++)
  {
    uint64_t k1 = getblock64(blocks, i*2+0);
    uint64_t k2 = getblock64(blocks, i*2+1);

    k1 *= c1; k1  = GRAEHL_ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = GRAEHL_ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = GRAEHL_ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = GRAEHL_ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= ((uint64_t)tail[14]) << 48;
  case 14: k2 ^= ((uint64_t)tail[13]) << 40;
  case 13: k2 ^= ((uint64_t)tail[12]) << 32;
  case 12: k2 ^= ((uint64_t)tail[11]) << 24;
  case 11: k2 ^= ((uint64_t)tail[10]) << 16;
  case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
  case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
           k2 *= c2; k2  = GRAEHL_ROTL64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
  case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
  case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
  case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
  case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
  case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
  case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
  case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
           k1 *= c1; k1  = GRAEHL_ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  ((uint64_t*)out)[0] = h1;
  ((uint64_t*)out)[1] = h2;
}

//-----------------------------------------------------------------------------


/// from MurmurHash3_x64_32
inline MurmurHashResult MurmurHash(void const *key, int len, MurmurSeed seed = GRAEHL_MURMUR_DEFAULT_SEED)
{
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 4;

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  //----------
  // body

  const uint32_t * blocks = (const uint32_t *)(data + nblocks*4);

  for(int i = -nblocks; i; i++)
  {
    uint32_t k1 = getblock32(blocks, i);

    k1 *= c1;
    k1 = GRAEHL_ROTL32(k1,15);
    k1 *= c2;

    h1 ^= k1;
    h1 = GRAEHL_ROTL32(h1,13);
    h1 = h1*5+0xe6546b64;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*4);

  uint32_t k1 = 0;

  switch(len & 3)
  {
  case 3: k1 ^= tail[2] << 16;
  case 2: k1 ^= tail[1] << 8;
  case 1: k1 ^= tail[0];
          k1 *= c1; k1 = GRAEHL_ROTL32(k1,15); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len;

  return fmix32(h1);
}

/// for large hashed containers (for signatures prefer 128 bit MurmurHash3_x64_128)
inline uint64_t WideMurmurHash(void const *key, int len, MurmurSeed seed = GRAEHL_MURMUR_DEFAULT_SEED)
{
#if HAVE_64_BITS
  const uint8_t * data = (const uint8_t*)key;
  const int nblocks = len / 16;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = GRAEHL_BIG_CONSTANT(0x87c37b91114253d5);
  const uint64_t c2 = GRAEHL_BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint64_t * blocks = (const uint64_t *)(data);

  for(int i = 0; i < nblocks; i++)
  {
    uint64_t k1 = getblock64(blocks, i*2+0);
    uint64_t k2 = getblock64(blocks, i*2+1);

    k1 *= c1; k1  = GRAEHL_ROTL64(k1,31); k1 *= c2; h1 ^= k1;

    h1 = GRAEHL_ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;

    k2 *= c2; k2  = GRAEHL_ROTL64(k2,33); k2 *= c1; h2 ^= k2;

    h2 = GRAEHL_ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
  }

  //----------
  // tail

  const uint8_t * tail = (const uint8_t*)(data + nblocks*16);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch(len & 15)
  {
  case 15: k2 ^= ((uint64_t)tail[14]) << 48;
  case 14: k2 ^= ((uint64_t)tail[13]) << 40;
  case 13: k2 ^= ((uint64_t)tail[12]) << 32;
  case 12: k2 ^= ((uint64_t)tail[11]) << 24;
  case 11: k2 ^= ((uint64_t)tail[10]) << 16;
  case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
  case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
           k2 *= c2; k2  = GRAEHL_ROTL64(k2,33); k2 *= c1; h2 ^= k2;

  case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
  case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
  case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
  case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
  case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
  case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
  case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
  case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
           k1 *= c1; k1  = GRAEHL_ROTL64(k1,31); k1 *= c2; h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= len; h2 ^= len;

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;

  return h1;
#else
  return MurmurHash(key, len, seed);
#endif
}

inline void MurmurHash3_x86_32 ( const void * key, int len,
                          uint32_t seed, void * out )
{
  *(uint32_t*)out = MurmurHash(key, len, seed);
}


// MurmurHash2, by Austin Appleby

#if HAVE_64_BITS

inline uint64_t MurmurHash64( const void * key, int len, MurmurSeed seed = GRAEHL_MURMUR_DEFAULT_SEED)
{
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;

  uint64_t h = seed ^ (len * m);

#if defined(__arm) || defined(__arm__)
  const size_t ksize = sizeof(uint64_t);
  const unsigned char * data = (const unsigned char *)key;
  const unsigned char * end = data + (std::size_t)(len/8) * ksize;
#else
  const uint64_t * data = (const uint64_t *)key;
  const uint64_t * end = data + (len/8);
#endif

  while (data != end)
  {
#if defined(__arm) || defined(__arm__)
    uint64_t k;
    memcpy(&k, data, ksize);
    data += ksize;
#else
    uint64_t k = *data++;
#endif

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
    case 7: h ^= uint64_t(data2[6]) << 48;
    case 6: h ^= uint64_t(data2[5]) << 40;
    case 5: h ^= uint64_t(data2[4]) << 32;
    case 4: h ^= uint64_t(data2[3]) << 24;
    case 3: h ^= uint64_t(data2[2]) << 16;
    case 2: h ^= uint64_t(data2[1]) << 8;
    case 1: h ^= uint64_t(data2[0]);
      h *= m;
  };

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

inline uint32_t MurmurHash32(void const *key, int len, MurmurSeed seed = GRAEHL_MURMUR_DEFAULT_SEED)
{
  return (uint32_t) MurmurHash64(key, len, seed);
}


#else
// 32-bit

// Note - This code makes a few assumptions about how your machine behaves -
// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4
inline uint32_t MurmurHash32 ( const void * key, int len, MurmurSeed seed = GRAEHL_MURMUR_DEFAULT_SEED)
{
  // 'm' and 'r' are mixing constants generated offline.
  // They're not really 'magic', they just happen to work well.

  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  // Initialize the hash to a 'random' value

  uint32_t h = seed ^ len;

  // Mix 4 bytes at a time into the hash

  const unsigned char * data = (const unsigned char *)key;

  while (len >= 4)
  {
    uint32_t k = *(uint32_t *)data;

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  // Handle the last few bytes of the input array

  switch(len)
  {
    case 3: h ^= data[2] << 16;
    case 2: h ^= data[1] << 8;
    case 1: h ^= data[0];
      h *= m;
  };

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

inline MurmurHashResult MurmurHash ( const void * key, int len, MurmurSeed seed = GRAEHL_MURMUR_DEFAULT_SEED) {
  return MurmurHash32(key, len, seed);
}

// 64-bit hash for 32-bit platforms

inline uint64_t MurmurHash64 ( const void * key, int len, MurmurSeed seed = GRAEHL_MURMUR_DEFAULT_SEED)
{
  const unsigned m = 0x5bd1e995;
  const int r = 24;

  unsigned h1 = seed ^ len;
  unsigned h2 = 0;

#if defined(__arm) || defined(__arm__)
  size_t ksize = sizeof(unsigned);
  const unsigned char * data = (const unsigned char *)key;
#else
  const unsigned * data = (const unsigned *)key;
#endif

  unsigned k1, k2;
  while (len >= 8)
  {
#if defined(__arm) || defined(__arm__)
    memcpy(&k1, data, ksize);
    data += ksize;
    memcpy(&k2, data, ksize);
    data += ksize;
#else
    k1 = *data++;
    k2 = *data++;
#endif

    k1 *= m; k1 ^= k1 >> r; k1 *= m;
    h1 *= m; h1 ^= k1;
    len -= 4;

    k2 *= m; k2 ^= k2 >> r; k2 *= m;
    h2 *= m; h2 ^= k2;
    len -= 4;
  }

  if (len >= 4)
  {
#if defined(__arm) || defined(__arm__)
    memcpy(&k1, data, ksize);
    data += ksize;
#else
    k1 = *data++;
#endif
    k1 *= m; k1 ^= k1 >> r; k1 *= m;
    h1 *= m; h1 ^= k1;
    len -= 4;
  }

  switch(len)
  {
    case 3: h2 ^= ((unsigned char*)data)[2] << 16;
    case 2: h2 ^= ((unsigned char*)data)[1] << 8;
    case 1: h2 ^= ((unsigned char*)data)[0];
      h2 *= m;
  };

  h1 ^= h2 >> 18; h1 *= m;
  h2 ^= h1 >> 22; h2 *= m;
  h1 ^= h2 >> 17; h1 *= m;
  h2 ^= h1 >> 19; h2 *= m;

  uint64_t h = h1;

  h = (h << 32) | h2;

  return h;
}

#endif
//32bit


}

#endif
