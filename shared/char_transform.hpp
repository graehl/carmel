#ifndef GRAEHL_SHARED__CHAR_TRANSFORM_HPP
#define GRAEHL_SHARED__CHAR_TRANSFORM_HPP

#include <iterator>
#include <locale>
#include <string>
#include <cstring>
#include <boost/config.hpp>
#include <graehl/insert_to.hpp>

namespace graehl {


// transforms some chars. 0 value = not transformed (i.e. no escape needed)
struct char_transform
{
  typedef unsigned char chari;
  BOOST_STATIC_CONSTANT(unsigned, size = std::ctype<char>::table_size);
  typedef char * iterator;
  typedef iterator const_iterator;
  //    typedef bool const* const_iterator;
  iterator begin() const
  { return const_cast<char*>(table); }
  iterator end() const
  { return begin()+size; }
  void clear() {
    std::memset(table, 0, sizeof(table));
    std::memset(inverse, 0, sizeof(inverse));
  }
  char_transform(char escape='\\') : escapec(escape) {
    clear();
    map(escapec, escapec);
  }
  void set_escape(char escape='\\') {
  }
  void map(unsigned c, unsigned to) { table[c] = to; inverse[to] = c; }
  void identity(unsigned c) { map(c, c); }
  template <class I, class I2>
  std::size_t count(I i, I2 end) const {
    std::size_t s = 0;
    for (; i!=end; ++i)
      if (table[(unsigned char)*i])
        ++s;
    return s;
  }
  template <class S>
  std::size_t count(S const& s) const {
    return count(s.begin(), s.end());
  }
  std::string escape(std::string const& s) const {
    std::string r(s.size()+count(s),'\0');
    escape(s.begin(), s.end(), r.begin());
    return r;
  }
  template <class I, class O>
  O escape(I i, I end, O o) const {
    for (; i!=end; ++i) {
      chari c=*i;
      char d = table[c];
      if (d) {
        *o = escapec; ++o;
        *o = d; ++o;
      } else {
        *o = c; ++o;
      }
    }
    return o;
  }
  std::string unescape(std::string const& s) const {
    std::ostringstream o;
    unescape(s.begin(), s.end(), graehl::insert_to(o));
    return o.str();
  }
  template <class I>
  std::string unescape(I i, I end) const {
    std::ostringstream o;
    unescape(i, end, std::ostream_iterator<char>(o));
    return o.str();
  }
  template <class I, class O>
  O unescape(I i, I end, O o) const {
    for (; i!=end; ++i) {
      char c=*i;
      if (c==escapec) {
        ++i;
        if (i==end) throw std::runtime_error("end of input after escape char "+std::string(1, escapec));
        char d = inverse[(chari)c];
        if (!d) throw std::runtime_error("unknown escaped char "+std::string(1, c));
        *o = d; ++o;
      } else {
        *o = c; ++o;
      }
    }
    return o;
  }

 private:
  char escapec;
  char table[size];
  char inverse[size];
};

}//ns

#endif
