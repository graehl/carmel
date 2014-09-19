/** \file

    Little-Endian base-128 encoding of unsigned integer types

    in LEB128 you have a sequence of bytes starting least significant, except
    the bytes only hold 7 bits of info. the high byte (128) is 0 if no more
    bytes follow.
*/

#ifndef GRAEHL_SHARED__LEB128_HPP
#define GRAEHL_SHARED__LEB128_HPP

#include <stdexcept>
#include <cstddef>
#include <cassert>
#include <cstring>

namespace graehl {

struct leb128error : std::exception {
  const char* what() const throw() { return "binary encoding didn't fit in buffer"; }
  ~leb128error() throw() {}
};

typedef unsigned char byte;
typedef byte const* const_byteptr;
typedef byte *byteptr;

template <class Uint>
const_byteptr decode_leb128(Uint &result, const_byteptr p) {
  Uint x = 0;
  for (;;) {
    byte const c = *p;
    byte const sig = c & 0x7f;
    x <<= 7;
    x |= sig;
    ++p;
    if (c == sig) {
      result = x;
      return p;
    }
  }
}

template <class Uint>
const_byteptr decode_leb128(Uint &result, const_byteptr p, const_byteptr end) {
  Uint x = 0;
  for (;;) {
    byte const c = *p;
    byte const sig = c & 0x7f;
    x <<= 7;
    x |= sig;
    ++p;
    if (c == sig) {
      result = x;
      return p;
    }
    if (p == end)
      throw leb128error();
  }
}

template <class Uint>
unsigned leb128_max_bytes(Uint) {
  return (sizeof(Uint) * 8 + 7) / 7;
}


template <class Uint>
byteptr encode_leb128(byteptr p, Uint x) {
  for (;;) {
    byte c = x;
    x >>= 7;
    if (x)
      *p++ = c | 0x80;
    else {
      *p++ = c & 0x7f;
      return p;
    }
  }
}

template <class Uint>
byteptr encode_leb128(byteptr p, byteptr e, Uint x) {
  if (leb128_max_bytes(x) + p > e)
    throw leb128error();
  return encode_leb128(p, x);
}

template <class Uint>
struct leb128_codec {
  typedef Uint value_type;
  enum { k_max_bytes = (sizeof(Uint) * 8 + 7) / 7 };
  static const_byteptr decode(Uint &x, const_byteptr p) {
    return decode_leb128(x, p);
  }
  static const_byteptr decode(Uint &x, const_byteptr p, const_byteptr end) {
    return decode_leb128(x, p, end);
  }
  static byteptr encode(byteptr p, Uint x) {
    return encode_leb128(x, p);
  }
  static byteptr encode(byteptr p, const_byteptr end, Uint x) {
    return encode_leb128(x, p, end);
  }
};

template <class Uint>
struct identity_codec {
  typedef Uint value_type;
  enum { k_max_bytes = sizeof(Uint) };
  static const_byteptr decode(Uint &x, const_byteptr p) {
    x = *(Uint*)p;
    return p + sizeof(Uint);
  }
  static const_byteptr decode(Uint &x, const_byteptr p, const_byteptr end) {
    if (p + sizeof(Uint) > end)
      throw leb128error();
    x = *(Uint*)p;
    return p + sizeof(Uint);
  }
  static byteptr encode(byteptr p, Uint x) {
    *(Uint*)p = x;
    return p + sizeof(Uint);
  }
  static byteptr encode(byteptr p, const_byteptr end, Uint x) {
    if (p + sizeof(Uint) > end)
      throw leb128error();
    *(Uint*)p = x;
    return p + sizeof(Uint);
  }
};

template <class Uint>
inline unsigned char need_fixed_bytes(Uint x) {
  if (sizeof(x) == 8)
    return
        (x & 0xffffffff00000000ull) ?
        (x & 0xffff000000000000ull ? (x & 0xff00000000000000ull ? 8 : 7) : (x & 0xff0000000000ull ? 6 : 5)) :
        (x & 0xffff0000u ? (x & 0xff000000u ? 4 : 3) : (x & 0xff00u ? 2 : 1));
  else if (sizeof(x) == 4)
    return x & 0xffff0000u ? (x & 0xff000000u ? 4 : 3) : (x & 0xff00u ? 2 : 1);
  else if (sizeof(x) == 2)
    return x & 0xff00u ? 2 : 1;
  else {
    unsigned n = 0;
    do {
      x >>= 8;
      n++;
    } while (x);
    return n;
  }
}


template <class Uint>
struct fixed_codec {
  typedef Uint value_type;
  enum { k_max_bytes = sizeof(Uint) };
  unsigned char fixed_bytes;
  fixed_codec(unsigned char bytes = sizeof(Uint))
  {
    set_bytes(bytes);
    assert(fixed_bytes <= sizeof(Uint));
  }
  void set_fixed_max(Uint max) {
    fixed_bytes = need_fixed_bytes(max);
  }
  void set_bytes(unsigned char bytes) {
    fixed_bytes = bytes;
  }

  template <class Config>
  void configure(Config &config) {
    config("fixed-bytes", &fixed_bytes).self_init()
        ("(for non-leb128) use this many bytes (must be <= actual size of int)");
  }
  const_byteptr decode(Uint &x, const_byteptr p) const {
    x = 0;
    std::memcpy((byteptr)&x, p, fixed_bytes);
    return p + fixed_bytes;
  }
  const_byteptr decode(Uint &x, const_byteptr p, const_byteptr end) const {
    if (p + fixed_bytes > end)
      throw leb128error();
    return decode(x, p);
  }
  byteptr encode(byteptr p, Uint x) const {
    std::memcpy(p, (const_byteptr)&x, fixed_bytes);
    return p + fixed_bytes;
  }
  byteptr encode(byteptr p, const_byteptr end, Uint x) const {
    if (p + fixed_bytes > end)
      throw leb128error();
    return encode(p, x);
  }
};

template <class Uint>
struct maybe_leb128_codec : fixed_codec<Uint> {
  typedef Uint value_type;
  bool leb128; // else identity
  maybe_leb128_codec(bool leb128 = false)
      : leb128(leb128)
  {}
  template <class Config>
  void configure(Config &config) {
    config("leb128", &leb128).self_init()
        ("enable little endian base 128 encoding (false means use fixed width integers (base 256 bytes as usual))");
  }

  typedef leb128_codec<Uint> codec;
  typedef identity_codec<Uint> identity;

  enum { k_max_bytes = codec::k_max_bytes };

  const_byteptr decode(Uint &x, const_byteptr p) const {
    if (leb128)
      return codec::decode(x, p);
    else
      return identity::decode(x, p);
  }
  const_byteptr decode(Uint &x, const_byteptr p, const_byteptr end) const {
    if (leb128)
      return codec::decode(x, p, end);
    else
      return identity::decode(x, p, end);
  }
  byteptr encode(byteptr p, Uint x) const {
    if (leb128)
      return codec::encode(p, x);
    else
      return identity::encode(p, x);
  }
  byteptr encode(byteptr p, const_byteptr end, Uint x) const {
    if (leb128)
      return codec::encode(p, end, x);
    else
      return identity::encode(p, end, x);
  }
};

template <class Codec, class Uint>
const_byteptr decodeArray(Uint *xs, unsigned n, const_byteptr p, const_byteptr end) {
  if (Codec::k_max_bytes * n + p > end)
    for (unsigned i = 0; i < n; ++i)
      p = Codec::decode(xs[i], p, end);
  else
    for (unsigned i = 0; i < n; ++i)
      p = Codec::decode(xs[i], p);
  return p;
}

template <class Codec, class Uint>
byteptr encodeArray(byteptr p, byteptr end, Uint const* xs, unsigned n) {
  if (Codec::k_max_bytes * n + p > end)
    for (unsigned i = 0; i < n; ++i)
      p = Codec::encode(p, end, xs[i]);
  else
    for (unsigned i = 0; i < n; ++i)
      p = Codec::encode(p, xs[i]);
  return p;
}

template <class Codec, class Uint>
byteptr encodeArray(Codec const& codec, byteptr p, byteptr end, Uint const* xs, unsigned n) {
  if (codec.k_max_bytes * n + p > end)
    for (unsigned i = 0; i < n; ++i)
      p = codec.encode(p, end, xs[i]);
  else
    for (unsigned i = 0; i < n; ++i)
      p = codec.encode(p, xs[i]);
  return p;
}

template <class Codec, class Uint>
const_byteptr decodeArray(Codec const& codec, Uint *xs, unsigned n, const_byteptr p, const_byteptr end) {
  if (codec.k_max_bytes * n + p > end)
    for (unsigned i = 0; i < n; ++i)
      p = codec.decode(xs[i], p, end);
  else
    for (unsigned i = 0; i < n; ++i)
      p = codec.decode(xs[i], p);
  return p;
}


template <class Value>
struct codec {
  typedef Value value_type;
  codec(unsigned max_bytes)
      : max_bytes(max_bytes)
  {}

  unsigned max_bytes;

  virtual byteptr encode(byteptr p, Value const& x) const = 0;
  virtual const_byteptr decode(Value &x, const_byteptr p) const = 0;

  virtual const_byteptr decodeArray(Value *xs, unsigned n, const_byteptr p, const_byteptr end) const {
    for (unsigned i = 0; i < n; ++i)
      p = decode(xs[i], p);
    return p;
  }
  virtual byteptr encodeArray(byteptr p, byteptr end, Value const* xs, unsigned n) const {
    for (unsigned i = 0; i < n; ++i)
      p = encode(p, xs[i]);
    return p;
  }
  virtual const_byteptr decode(Value &x, const_byteptr p, const_byteptr end) const {
    return decode(x, p);
  }
  virtual byteptr encode(byteptr p, const_byteptr end, Value const& x) const {
    if (max_bytes + p > end) throw leb128error(); ///override this if you want to be more precise
    return encode(p, x);
  }
};

template <class impl>
struct codec_dynamic : codec<typename impl::value_type> {
  typedef typename impl::value_type Uint;
  codec_dynamic()
      : codec<Uint>(impl::k_max_bytes)
  {}
  virtual const_byteptr decodeArray(Uint *xs, unsigned n, const_byteptr p, const_byteptr end) const {
    return graehl::decodeArray<impl>(xs, n, p, end);
  }
  virtual byteptr encodeArray(byteptr p, byteptr end, Uint const* xs, unsigned n) const {
    return graehl::decodeArray<impl>(p, end, xs, n);
  }
  virtual byteptr encode(byteptr p, Uint x) const {
    return impl::encode(p, x);
  }
  virtual const_byteptr decode(Uint &x, const_byteptr p) const {
    return impl::decode(x, p);
  }
  virtual const_byteptr decode(Uint &x, const_byteptr p, const_byteptr end) const {
    return impl::decode(x, p, end);
  }
  virtual byteptr encode(byteptr p, const_byteptr end, Uint x) const {
    return impl:: encode(p, end, x);
  }
};

typedef maybe_leb128_codec<unsigned> maybe_leb128_unsigned;
typedef maybe_leb128_codec<std::size_t> maybe_leb128_size_t;
typedef leb128_codec<unsigned> leb128_unsigned;
typedef leb128_codec<std::size_t> leb128_size_t;
typedef identity_codec<unsigned> identity_unsigned;
typedef identity_codec<std::size_t> identity_size_t;

typedef codec_dynamic<maybe_leb128_unsigned> maybe_leb128_unsigned_dynamic;
typedef codec_dynamic<maybe_leb128_size_t> maybe_leb128_size_t_dynamic;
typedef codec_dynamic<leb128_unsigned> leb128_unsigned_dynamic;
typedef codec_dynamic<leb128_size_t> leb128_size_t_dynamic;
typedef codec_dynamic<identity_unsigned> identity_unsigned_dynamic;
typedef codec_dynamic<identity_size_t> identity_size_t_dynamic;


}

#endif
