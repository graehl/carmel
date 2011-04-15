#ifndef GRAEHL_SHARED__NULL_OUTPUT_ITERATOR_HPP
#define GRAEHL_SHARED__NULL_OUTPUT_ITERATOR_HPP

#include <iterator>

namespace graehl {

struct null_output_iterator {
  typedef std::output_iterator_tag iteratory_category;
  typedef void value_type;
  typedef void difference_type;
  typedef void pointer;
  typedef void reference;
  template <class V>
  void operator=(V const& v) const {}
  null_output_iterator const& operator*() const { return *this; }
  void operator++() const {}
  void operator++(int) const {}
};

}


#endif
