/** \file

    named bits set to/from string:

    struct BitsList {
      template <class C>
      static void bits(C &c) {
       c("NAME0"); // starts at value 1
       c("NAME1", 1);
       c("NAME2", 1<<1);
       c("NAME3"); // gets last value (1<<1) * 2
       c("deprecated-synonym-for-NAME4", 4); // parses to value as NAME3 but will never be printed as name
      }
    };

    typedef named_bits<BitsList, unsigned> Bits;

    names that come earlier are preferred for output. values may have more than 1 set bit
*/

#ifndef GRAEHL_SHARED__CONFIGURE_BITS
#define GRAEHL_SHARED__CONFIGURE_BITS

#include <graehl/shared/hex_int.hpp>
#include <graehl/shared/string_to.hpp>
#include <graehl/shared/split.hpp>

namespace graehl {

template <class Int = unsigned>
struct bit_names {
  typedef std::pair<std::string, Int> NameValue;
  typedef std::vector<NameValue> NameValues;
  NameValues nv_;

  void operator()(std::string const& str, Int val) {
    nv_.push_back(NameValue(str, val));
    known_ |= val;
    next_ = val * 2;
  }

  void known_only(Int &val) const {
    val &= known_;
  }

  void operator()(std::string const& str) {
    (*this)(str, next_);
  }

  bit_names()
      : next_(1)
  {}

  std::string usage() const {
    string_builder b;
    usage(b);
    return b.str();
  }

  std::string name(Int val) const {
    string_builder b;
    append(val, b);
    return std::string(b.begin(), b.end());
  }

  void usage(string_builder &b) const {
    bool first = true;
    for(typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i) {
      b.space_except_first(first, '|');
      b(i->first);
    }
  }

  void append(Int val, string_builder &b) const {
    bool first = true;
    for(typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i) {
      Int const mask = i->second;
      if (val & mask == mask) {
        b.space_except_first(first, '|');
        b(i->first);
        val &= ~mask;
      }
    }
    if (val) {
      b.space_except_first(first, '|');
      b(to_string(hex_int<Int>(val)));
    }
  }

  std::string const& bitname(Int val, std::string const& fallback) const {
    for(typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i)
      if (i->second == val)
        return i->first;
    return to_string(hex_int<Int>(val));
  }

  Int operator[](std::string const& name) const {
    for(typename NameValues::const_iterator i = nv_.begin(), e = nv_.end(); i != e; ++i)
      if (i->first == name)
        return i->second;
    return hex_int<Int>(name);
  }

 private:
  Int next_;
  Int known_;
};

template <class NameList, class Int = unsigned>
struct cached_bit_names : bit_names<Int> {
  cached_bit_names() {
    NameList::bits(*this);
  }
};


template <class NameList, class Int>
struct parse_bit_names {
  static bit_names<Int> const& names() {
    static cached_bit_names<NameList, Int> gNames;
    return gNames;
  }

  bool operator()(std::string const& s) const {
    *val_ |= names()[s];
    return true;
  }

  parse_bit_names(Int &val)
      : val_(val)
  {}

  Int &val_;
};


template <class NameList, class Int = unsigned>
struct named_bits : hex_int<Int> {
  typedef hex_int<Int> Base;
  named_bits() {}

  named_bits(Int i) : Base(i)
  {}

  Base &base() {
    return static_cast<Base &>(*this);
  }

  typedef parse_bit_names<NameList, Int> Names;

  std::string type_string_impl(named_bits const&) { return Names::names().usage() + " or 0xfaceb00c hex or decimal"; }

  void known_only() {
    Names::names().known_only(base());
  }

  void append(string_builder &b) {
    Names::names().append(*this, b);
  }

  friend void string_to_impl(std::string const& s, named_bits &n) {
    n = 0;
    try {
      split_noquote(s, parse_bit_names<NameList, Int>(n), "|");
    } catch (string_to_exception &e) {
      string_to_impl(s, n.base);
    }
  }

  friend std::string to_string_impl(std::string const& s, named_bits const& n) {
    string_builder b;
    n.append(b);
    return std::string(b.begin(), b.end());
  }

  friend inline std::ostream& operator<<(std::ostream &out, named_bits const& self) {
    self.print(out);
    return out;
  }

  void print(std::ostream &out) const {
    string_builder b;
    Names::names().append(*this, b);
    out << b;
  }


};


}

#endif
