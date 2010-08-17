// treat a function object as an output iterator
#ifndef FUNCTION_OUTPUT_ITERATOR_HPP
#define FUNCTION_OUTPUT_ITERATOR_HPP

#include <graehl/shared/byref.hpp>
#include <iterator>

namespace graehl {


// treat an allocated array/vector as an output iterator

template <class I>
struct fill_reverse {
  I o; // set to end()-1 of vector (&back()) initially
  template <class V>
  void push_back(V const& v) const {
    *o--=v;
  }
};

template <class I>
struct fill_forward {
  I o; // set to begin() of vector (&front())
  template <class V>
  void push_back(V const& v) const {
    *o++=v;
  }
};

template <class UnaryFunction>
class function_output_iterator {
  typedef function_output_iterator self;
public:
  typedef std::output_iterator_tag iterator_category;
  typedef void                value_type;
  typedef void                difference_type;
  typedef void                pointer;
  typedef void                reference;

  explicit function_output_iterator() {}

  explicit function_output_iterator(const UnaryFunction& f)
    : m_f(f) {}

  struct output_proxy {
    output_proxy(UnaryFunction& f) : m_f(f) { }
    template <class T> output_proxy& operator=(const T& value) {
      deref(m_f)(value);
      return *this;
    }
    UnaryFunction& m_f;
  };
  output_proxy operator*() { return output_proxy(m_f); }
  self& operator++() { return *this; }
  self& operator++(int) { return *this; }
private:
  UnaryFunction m_f;
};

template <class UnaryFunction>
inline function_output_iterator<UnaryFunction>
make_function_output_iterator(const UnaryFunction& f = UnaryFunction()) {
  return function_output_iterator<UnaryFunction>(f);
}

} // namespace


#endif
