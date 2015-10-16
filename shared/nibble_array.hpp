/** \file

 A nibble is conventionally a 4-bit value (half a byte - get it?). We provide a
 fixed size array of 2 or 4 bit elements.
*/

#ifndef GRAEHL__NIBBLE_ARRAY_HPP
#define GRAEHL__NIBBLE_ARRAY_HPP
#pragma once

#include <cstdlib>
#include <graehl/shared/log_intsize.hpp>

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif
namespace graehl {

template <unsigned nbits2or4 = 2, class unsigned_block_int = unsigned>
struct nibble_array {
  typedef unsigned_block_int Block;
  typedef Block value_type;
  typedef std::size_t size_type;
  Block* blocks_;
  size_type nbytes_;
  size_type sz_;
  explicit nibble_array(size_type sz)
      : blocks_((Block*)std::malloc(nbytes_ = nblocks(sz) * sizeof(Block))), sz_(sz) {
    assert(nbits2or4 == 2 || nbits2or4 == 4);
    zero();
  }
  ~nibble_array() { std::free(blocks_); }
  void clear() {
    std::free(blocks_);
    blocks_ = 0;
  }
  enum {
    nibblesz_ = nbits2or4,
    perbyte_ = CHAR_BIT / nbits2or4,
    perblock_ = sizeof(Block) * perbyte_,
    lognibblesz_ = nbits2or4 == 2 ? 1 : 2,
  };
  static const Block mask_nibble_ = (Block)(nbits2or4 == 2 ? 3 : 15);
  void zero() { std::memset(blocks_, 0, nbytes_); }
  void fill(value_type v) { std::memset(blocks_, repeated_byte(v), nbytes_); }

  value_type operator[](size_type i) const {
    assert(i < sz_);
    unsigned const shift = blockremainder(i) << lognibblesz_;
    Block const mask = mask_nibble_ << shift;
    Block const r = (blocks_[block(i)] & mask) >> shift;
    assert(!(r & ~(Block)mask_nibble_));
    return r;
  }
  /// return current (*this)[i], *then* setting new value to v
  value_type test_set(size_type i, value_type v) {
    assert(i < sz_);
    assert(!(v & ~(Block)mask_nibble_));
    Block& blockref = blocks_[block(i)];
    Block block = blockref;
    unsigned const shift = blockremainder(i) << lognibblesz_;
    Block const mask = (Block)mask_nibble_ << shift;
    Block const r = (block & mask) >> shift;
    block &= ~mask;
    block |= v << shift;
    blockref = block;
    assert(!(r & ~(Block)mask_nibble_));
    return r;
  }
  /// return x[i] setting new x[i] to vfalse if !x[i]
  value_type test_set_if_false(size_type i, value_type vfalse) {
    assert(i < sz_);
    assert(!(vfalse & ~(Block)mask_nibble_));
    Block& blockref = blocks_[block(i)];
    Block block = blockref;
    unsigned const shift = blockremainder(i) << lognibblesz_;
    Block const mask = (Block)mask_nibble_ << shift;
    Block const r = (block & mask) >> shift;
    if (!r) {
      block &= ~mask;
      block |= (vfalse << shift);
      blockref = block;
      return r;
    } else {
      assert(!(r & ~(Block)mask_nibble_));
      return r;
    }
  }
  void set(size_type i, value_type v) {
    assert(i < sz_);
    assert(!(v & ~(Block)mask_nibble_));
    Block& blockref = blocks_[block(i)];
    Block block = blockref;
    unsigned const shift = blockremainder(i) << lognibblesz_;
    Block const mask = (Block)mask_nibble_ << shift;
    block &= ~mask;
    block |= v << shift;
    blockref = block;
  }

 private:
  static inline size_type nblocks(size_type sz) { return (sz + perblock_ - 1) / perblock_; }
  static inline char repeated_byte(value_type v) {
    return nibblesz_ == 2 ? (v | (v << 2) | (v << 4) | (v << 6)) : (v | (v << 4));
  }
  static inline size_type block(size_type i) { return i / perblock_; }  // should optimize to bitshift
  static inline size_type blockremainder(size_type i) {
    return i % perblock_;
  }  // should optimize to and mask
};
}

#ifdef GRAEHL_TEST
BOOST_AUTO_TEST_CASE(nibble_array_test_case) {
  using namespace graehl;
  for (unsigned N = 6; N < 70; N += 3) {
    {
      nibble_array<2, std::size_t> A(N);
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], 0);
      }
      A.fill(2);
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], 2);
        BOOST_REQUIRE_EQUAL(A.test_set(i, 1), 2);
      }
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], 1);
        BOOST_REQUIRE_EQUAL(A.test_set(i, i % 4), 1);
      }
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], i % 4);
      }
    }
    {
      nibble_array<4, unsigned char> A(N);
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], 0);
      }
      A.fill(13);
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], 13);
        BOOST_REQUIRE_EQUAL(A.test_set(i, 1), 13);
      }
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], 1);
        BOOST_REQUIRE_EQUAL(A.test_set(i, i % 16), 1);
      }
      for (unsigned i = 0; i < N; ++i) {
        BOOST_REQUIRE_EQUAL(A[i], i % 16);
        unsigned v = (i + 1) % 16;
        A.set(i, v);
        BOOST_REQUIRE_EQUAL(A[i], v);
      }
    }
  }
}
#endif

#endif
