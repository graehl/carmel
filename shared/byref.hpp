#ifndef BYREF_HPP
#define BYREF_HPP

//boost::ref and boost::cref that return instances of boost::reference_wrapper<T>
// boost::unwrap_reference<T>::type = T
// boost::unwrap_reference<boost::reference_wrapper<T> > = T

#include <boost/ref.hpp>

#include "dummy.hpp"

template <class C>
struct dummy<boost::reference_wrapper<C> > {
  //  static C dummy_imp;
  static boost::reference_wrapper<C> var();
};

template <class C>
boost::reference_wrapper<C> dummy<boost::reference_wrapper<C> >::var() {
  static boost::reference_wrapper<C> var(*(C*)NULL);
  return var;
}

/*
template <class C>
struct ByRef {
  typedef C type;
  operator C&() const { return *c; }
  explicit ByRef(C &c_) : c(&c_) {}
  ByRef(ByRef<C> &b) : c(b.c) {}
  private:
    C *c;
};
*/

//#define ByRef boost::reference_wrapper
/*
template <class C>
struct ByRef : public boost::reference_wrapper<C> {
  explicit ByRef(C &c_) : boost::reference_wrapper<C>(c_) {}
};
*/

template <class T> inline
typename boost::unwrap_reference<T>::type &
deref(T& t) {
  return t;
}

template <class T> inline
const typename boost::unwrap_reference<T>::type &
deref(const T& t) {
  return t;
}
  //return *const_cast<boost::unwrap_reference<T>::type *>&(t);

#include <iterator>

namespace my {

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


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST_MAIN
template<class C>
void f(C c) {
  deref(c)=1;
}

void g(int &p) {
  p=2;
}

template<class C>
void h(C c) {
  g(c);
}

BOOST_AUTO_UNIT_TEST( TEST_byref )
{
  int t=0;
  f(t);
  BOOST_CHECK(t==0);
  f(boost::ref(t));
  BOOST_CHECK(t==1);
  h(t);
  BOOST_CHECK(t==1);
  h(boost::ref(t));
  BOOST_CHECK(t==2);
}
#endif

#endif
