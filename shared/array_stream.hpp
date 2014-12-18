/** \file

    fixed size user-supplied memory read/write stream buffer (overflow = error
    on write, eof on read). the buffer type and size are the character type of
    the streambuf (array_streambuf is for char).

    NOTE: end() returns the current write position (i.e. forms the end of the
    range [begin(), end()) of chars written so far

    NOTE: unlike the usual streambuf used in stringstream, the end-of-readable
    marker isn't increased when you write; to switch from write to read, call
    reset_read(size())
*/

#ifndef GRAEHL_SHARED__ARRAY_STREAM_HPP
#define GRAEHL_SHARED__ARRAY_STREAM_HPP
#pragma once

#include <streambuf>
#include <ostream>
#include <string>
#include <utility>
#include <iostream>
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <cassert>

namespace graehl {

/**
   \return bytes written to sbuf. modifies tellp (put pointer).
*/
template <class cT, class cT_Traits>
inline std::size_t size_out(std::basic_streambuf<cT, cT_Traits>& sbuf) {
  return (std::size_t)sbuf.pubseekoff(0, std::ios::end, std::ios_base::out);
}

/**
   \return bytes readable from sbuf. modifies tellg (get pointer) - you can
   reset to beginning w/ rewind_in
*/
template <class cT, class cT_Traits>
inline std::size_t size_in(std::basic_streambuf<cT, cT_Traits>& sbuf) {
  return (std::size_t)sbuf.pubseekoff(0, std::ios::end, std::ios_base::in);
}

/**
   tellg(0) - reset to beginning.
*/
template <class cT, class cT_Traits>
inline void rewind_in(std::basic_streambuf<cT, cT_Traits>& sbuf) {
  sbuf.pubseekpos(0, std::ios_base::in);
}

/// for stringstream type buf where you haven't read yet (if you did, call rewind_in first)
template <class VecBytes, class cT, class cT_Traits>
inline void copy_written(VecBytes* vec, std::basic_streambuf<cT, cT_Traits>& buf) {
  std::streamsize const sz = buf.pubseekoff(0, std::ios::end, std::ios_base::out);
  vec->resize(sz);
  if (sz) {
    std::streamsize const got = buf.sgetn((char*)&*vec->begin(), sz);
    assert(got == sz);
  }
}

template <class cT, class cT_Traits = std::char_traits<cT> >
class basic_array_streambuf : public std::basic_streambuf<cT, cT_Traits> {
 public:
  typedef cT_Traits traits;
  typedef std::basic_string<cT, traits> string_type;
  typedef std::basic_streambuf<cT, traits> base;
  typedef basic_array_streambuf<cT, traits> self_type;
  typedef typename traits::char_type char_type;
  typedef typename base::pos_type pos_type;
  typedef typename base::off_type off_type;
  typedef typename base::int_type int_type;
  typedef std::streamsize size_type;

  typedef cT value_type;
  typedef value_type* iterator;  // NOTE: you can count on iterator being a pointer

  // shows what's been written
  template <class O>
  void print(O* o) const {
    o << "array_streambuf[" << capacity() << "]=[";
    for (typename self_type::iterator i = begin(), e = end(); i != e; ++i) o->put(*i);
    o << "]";
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o,
                                                basic_array_streambuf const& self) {
    self.print(o);
    return o;
  }

  /**
     initialize to an existing array, ready to read/write starting form beginning
  */
  explicit basic_array_streambuf(const char_type* p = 0, size_type sz = 0) : eof_() { set_array(p, sz); }

  void set_array(string_type const& str) { set_array((char_type const*)str.data(), str.size()); }

  void set_array(const char_type* p = 0, size_type sz = 0) {
    buf = const_cast<char_type*>(p);
    bufend = buf + sz;
    this->setg(buf, buf, bufend);
    this->setp(buf, bufend);
  }
  /**
     void * versions of above for convenience.
  */
  explicit basic_array_streambuf(const void* p, size_type sz = 0) : eof_() { set_array(p, sz); }
  inline void set_array(const void* p = 0, size_type sz = 0) { set_array((char const*)p, sz); }

  explicit basic_array_streambuf(string_type const& str) : eof_() { set_array(str); }

  /**
     seek to beginning, resetting maximum readable size to sz.
  */
  inline void reset_read(size_type sz) { setg(buf, buf, buf + sz); }

  /**
     seek read to beginning, set maximum readable to the length of what was written.

     so you can write some stuff, reset_read, then read it.
  */
  inline void reset_read() { base::setg(buf, buf, end()); }

  /**
     set no readable area but leave buffer alone
  */
  void reset() {
    base::setg(buf, buf, buf);
    set_ppos(0);
  }

  /**
     doesn't shrink the "written" region - use if you want to repeatedly write stuff and then get it with
     begin(), end()
  */
  void reset_write() { set_ppos(0); }

  /**
     seek read position, leaving end of readable area as is
  */
  void set_gpos(pos_type pos) { base::setg(buf, buf + pos, egptr()); }
  /**
     end() is normally what's been written so far; this modifies end()
  */
  void set_end(iterator end_) { base::setp(end_, bufend); }
  /**
     write seek.
  */
  void set_ppos(pos_type pos) { base::setp(buf + pos, bufend); }
  /**
     without seeking, change readable extent. if you put sz less than current
     read position, that's your bad (nothing is done to prevent/detect)
  */
  inline void set_read_size(size_type sz) { base::setg(buf, gptr(), buf + sz); }
  inline void set_write_size(size_type sz) { set_ppos(sz); }

  /**
     resize for both read and write
  */
  inline void set_size(size_type sz) {
    set_read_size(sz);
    set_write_size(sz);
  }

  inline std::ptrdiff_t capacity() const { return bufend - buf; }

  /**
     streambuf method - remaining bytes that can be read. default impl is correct.
  */
  using base::in_avail;

  /**
     streambuf method - remaining bytes that can be written. we use the default streambuf in_avail()
  */
  inline std::ptrdiff_t out_avail() { return bufend - pptr(); }

  bool read_complete() const {
    assert((gptr() == bufend) == !const_cast<basic_array_streambuf*>(this)->in_avail());
    return gptr() == bufend;
  }

  /**
     this buffer can't be (automatically) grown. so once you write too much, you're dead.
  */
  int_type overflow(int_type c) {
    if (out_avail() == 0 && !traits::eq_int_type(c, traits::eof())) {
      eof_ = true;
      return traits::eof();  // FAIL
    }
    return traits::not_eof(c);
  }

  /*
  //NOTE: maybe the default sputn would have been just as fast given we've set a buffer ... not sure
  // below defined for speedup (no repeated virtual get/put one char at a time)
  virtual std::streamsize xsputn(const char_type * from, std::streamsize sz)
  {
  if (out_avail()<sz)
  return 0;
  traits::copy(pptr(), from, sz);
  pbump(sz);
  return sz;
  }
  //ACTUALLY: will never get called if you just use single-character get()
  virtual std::streamsize xsgetn(char_type * to, std::streamsize sz)
  {
  const std::streamsize extracted = std::min(in_avail(), sz);
  traits::copy(to, gptr(), extracted);
  gbump(extracted);
  return extracted;
  }
  */

  /**
     supports stream seekp seekg operations. (and streambuf::pubseekoff)
  */
  virtual pos_type seekoff(off_type offset, std::ios_base::seekdir dir,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) {
    pos_type pos = offset;
    if (dir == std::ios_base::cur)
      pos += ((which & std::ios_base::out) ? pptr() : gptr()) - buf;
    else if (dir == std::ios_base::end)
      pos += ((which & std::ios_base::out) ? epptr() : egptr()) - buf;
    // else ios_base::beg
    return seekpos(pos, which);
  }
  virtual pos_type seekpos(pos_type pos,
                           std::ios_base::openmode which = std::ios_base::in | std::ios_base::out) {
    if (which & std::ios_base::out) set_ppos(pos);
    if (which & std::ios_base::in) set_gpos(pos);
    return pos;
  }

  void* data() const { return (void*)buf; }

  iterator begin() const { return buf; }
  /**
     same as tellp (put position). so [begin(), end()) are bytes written so far
  */
  iterator end() const { return pptr(); }
  bool full() const { return end() == epptr(); }
  bool empty() const { return end() == begin(); }

  /**
     \return  number of bytes written.
  */
  size_type size() const { return end() - begin(); }
  /**
     show current position without a wasteful seekoff(0, ios::cur)
  */
  size_type tellg() const { return gptr() - begin(); }
  /**
     if not empty(), last written character (mutable).
  */
  value_type& back() const { return *(end() - 1); }
  /**
     like stringstream::str().
  */
  std::basic_string<cT, traits> str() const { return std::basic_string<cT, traits>(begin(), end()); }
  /**
     \return str().c_str(), but if you intend to use this, you must reserve one
     extra byte in your array for null termination
  */
  cT* c_str() {
    if (*end() != traits::eos()) {
      if (full()) throw std::runtime_error("fixed size array_stream full - couldn't NUL-terminate c_str()");
      *end() = traits::eos();
    }
    return begin();
  }
  bool eof_;

 private:
  using base::pptr;
  using base::pbump;
  using base::epptr;
  using base::gptr;
  using base::egptr;
  using base::gbump;
  basic_array_streambuf(const basic_array_streambuf&);
  basic_array_streambuf& operator=(const basic_array_streambuf&);

 protected:
  virtual self_type* setbuf(char_type* p, size_type n) {
    set_array(p, n);
    return this;
  }
  char_type* buf, *bufend;
};

typedef basic_array_streambuf<char> array_streambuf;
typedef basic_array_streambuf<wchar_t> warray_streambuf;

template <class cT, class traits = std::char_traits<cT> >
class basic_array_stream : public std::basic_iostream<cT, traits> {
  typedef std::basic_iostream<cT, traits> base;
  typedef basic_array_streambuf<cT, traits> Sbuf;

 public:
  typedef std::streamsize size_type;
  typedef cT value_type;
  typedef value_type* iterator;  // NOTE: you can count on iterator being a pointer
  typedef basic_array_stream<cT, traits> self_type;

  template <class O>
  void print(O& o) const {
    o << sbuf_;
  }
  template <class Ch, class Tr>
  friend std::basic_ostream<Ch, Tr>& operator<<(std::basic_ostream<Ch, Tr>& o, basic_array_stream const& self) {
    self.print(o);
    return o;
  }

  basic_array_stream() : base(&sbuf_) {}
  explicit basic_array_stream(const value_type* p, size_type size) : base(&sbuf_) {
    set_array(p, size);
    base::rdbuf(&sbuf_);
  }
  template <class C>
  explicit basic_array_stream(const C& c)
      : base(&sbuf_) {
    set_array(c);
    base::rdbuf(&sbuf_);
  }

  /**
     all of the below are forwards of the extended array_streambuf interface (not part of regular iostream)
  */

  inline void set_array(const value_type* p) { sbuf_.set_array(p, traits::length(p)); }
  inline void set_array(const value_type* p, size_type size) { sbuf_.set_array(p, size); }
  inline void set_array(const void* p = 0, size_type size = 0) { sbuf_.set_array(p, size); }
  inline void set_array(const std::basic_string<value_type>& s) { sbuf_.set_array(s.c_str(), s.size()); }
  size_type tellg() const { return sbuf_.tellg(); }
  Sbuf& buf() { return sbuf_; }
  void reset() {
    sbuf_.reset();
    base::clear();
  }
  // allows what was written to be read.
  void reset_read() { sbuf_.reset_read(); }
  // restart reading at the beginning, and hit eof after N read chars
  void reset_read(size_type N) { sbuf_.reset_read(N); }

  iterator begin() const { return sbuf_.begin(); }
  iterator end() const { return sbuf_.end(); }
  size_type size() const { return sbuf_.size(); }
  size_type capacity() const { return sbuf_.capacity(); }
  bool empty() const { return sbuf_.empty(); }
  bool full() const { return sbuf_.full(); }

 private:
  Sbuf sbuf_;
  /*


   */
};

typedef basic_array_stream<char> array_stream;
typedef basic_array_stream<wchar_t> warray_stream;

}  // ns

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#include <cstring>

namespace graehl {
namespace unit_test {

template <class C>
void TEST_check_array_stream(C& i1) {
  using namespace std;
  std::string const TESTARRAYSTR1("eruojdcv53341");
  std::string const TESTARRAYSTR2("0asd");
  std::string const TESTARRAYSTR(" " + TESTARRAYSTR1 + "\n " + TESTARRAYSTR2 + "\t");
  const char* tarrs = TESTARRAYSTR.c_str();
  i1.set_array(tarrs, TESTARRAYSTR.size());
  string tstr = tarrs;
  unsigned slen = (unsigned)tstr.length();
  string s;
  BOOST_CHECK(i1 >> s);
  BOOST_CHECK(s == TESTARRAYSTR1);
  BOOST_CHECK(i1 >> s);
  BOOST_CHECK(s == TESTARRAYSTR2);

  streambuf::off_type i;
  for (i = 0; i < slen; ++i) {
    int t;
    i1.seekg(i, ios_base::beg);
    t = (int)i1.tellg();
    BOOST_CHECK(t == (int)i);
    i1.seekg(0);
    BOOST_CHECK(i1.tellg() == 0);
    i1.seekg(i);
    t = (int)i1.tellg();
    BOOST_CHECK(t == (int)i);
    int c = i1.get();
    BOOST_CHECK(c == tstr[i]);
    int e = (int)(-i - 1);
    BOOST_CHECK(i1.seekg(e, ios_base::end));
    t = (int)i1.tellg();
    int p = slen + e;
    BOOST_CHECK(t == p);
    BOOST_CHECK(i1.get() == tstr[p]);
  }
  i1.seekp(0);
  i1.seekg(0);
  for (i = 0; i < slen; ++i) {
    BOOST_CHECK((int)i1.tellg() == (int)i);
    BOOST_CHECK((int)i1.tellp() == (int)i);
    BOOST_CHECK(i1.put((char)i));
    BOOST_CHECK((char)i == i1.get());
    BOOST_CHECK(i1.begin()[i] == (char)i);
  }
}

template <class C>
inline void TEST_check_memory_stream1(C& o, char* buf, unsigned n) {
  const unsigned M = 13;
  for (unsigned i = 0; i < n; ++i) {
    char c = i % M + 'a';
    buf[i] = c;
  }
  o.write(buf, n);
  BOOST_REQUIRE(o);
  //    BOOST_CHECK_EQUAL_COLLECTIONS(buf, buf+n, o.begin(), o.end());
  for (unsigned i = 0; i < n; ++i) buf[i] = 0;
  /*
    o.read(buf, n);
    BOOST_REQUIRE(o);
    BOOST_CHECK(o.gcount()==n);
    BOOST_CHECK_EQUAL_COLLECTIONS(buf, buf+n, o.begin(), o.end());
  */
}

template <class C>
inline void TEST_check_memory_stream2(C& o, char* buf, unsigned n) {
  const unsigned M = 13;
  for (unsigned i = 0; i < n; ++i) {
    BOOST_CHECK(o.put(i % M));
    BOOST_CHECK_EQUAL(o.begin()[i], i % M);
    BOOST_CHECK_EQUAL(o.size(), i + 1);
  }
}

template <class C>
inline void TEST_check_memory_stream(C& o, char* buf, unsigned n) {
  TEST_check_memory_stream1(o, buf, n);
  o.reset();
  TEST_check_memory_stream2(o, buf, n);
}

BOOST_AUTO_TEST_CASE(TEST_array_stream) {
  array_stream i1;
  TEST_check_array_stream(i1);

  const unsigned N = 20;
  char buf[N], buf2[N];

  array_stream o2(buf, N);
  TEST_check_memory_stream(o2, buf2, N);
}


}

}  // ns
#endif
// TEST

#endif
