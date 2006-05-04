// determine whether a class has specified a print or print with writer method
#ifndef PRINT_ON_HPP
#define PRINT_ON_HPP

#include <boost/utility/enable_if.hpp>
#include <boost/ref.hpp>
//namespace boost { template <class C> class reference_wrapper; }

// usage: template <class A> void func(A &a,typename has_print_plain<A>::type* dummy = 0)

// how this works: has_print<C>::type is either defined (iff C::has_print was typedefed to void), or undefined.  this allows us to exploit SFINAE (google) for function overloads depending on whether C::has_print was typedefed void ;)

// ok - actually, as long as C::has_print is *any* type ...

namespace graehl {

template <class C,class V=void>
struct has_print;

template <class C,class V>
struct has_print {
};

template <class C>
struct has_print<C,typename C::has_print> {
  typedef void type;
};



template <class C,class V=void>
struct has_print_writer;

template <class C,class V>
struct has_print_writer {
};

template <class C>
struct has_print_writer<C,typename C::has_print_writer> {
  typedef void type;
};




template <class C,class V=void>
struct not_has_print_writer;

template <class C,class V>
struct not_has_print_writer {
  typedef void type;
};

template <class C>
struct not_has_print_writer<C,typename C::has_print_writer> {
};



template <class C,class V=void,class V2=void>
struct has_print_plain;

template <class C,class V,class V2>
struct has_print_plain {
};

template <class C>
struct has_print_plain<C,typename not_has_print_writer<C>::type, typename has_print<C>::type> {
  typedef void type;
};



// type is defined iff you're not a pointer type and you have no print(o) or print(o,writer)
template <class C,class V=void>
struct not_has_print;

template <class C,class V>
struct not_has_print {
  typedef void type;
};

template <class C>
struct not_has_print<C,typename has_print<C>::type> {
};

template <class C>
struct not_has_print<C,typename has_print_writer<C>::type> {
};

template <class C>
struct isa_pointer;


template <class C>
struct isa_pointer {
};

template <class C>
struct isa_pointer<C *> {
    typedef void type;
};
template <class C>
struct isa_pointer<C * const> {
    typedef void type;
};
template <class C>
struct isa_pointer<C * volatile> {
    typedef void type;
};
template <class C>
struct isa_pointer<C * const volatile> {
    typedef void type;
};

template <class C>
struct true_value 
{
    static const bool value=true;
};

template <class C>
struct not_has_print<C,typename isa_pointer<C>::type> {
};


template <class C,class V=void>
struct has_const_iterator 
{
    static const bool value=false;
};

template <class C>
struct has_const_iterator<C,typename boost::enable_if<true_value<typename C::const_iterator> > > {
    typedef void type;
    static const bool value=true;
};


template <class C,class V=void>
struct not_has_const_iterator 
{    
    typedef void type;
    static const bool value=true;
};

template <class C>
struct not_has_const_iterator<C,typename boost::enable_if<true_value<typename C::const_iterator> > > {
    static const bool value=false;
};


#  ifndef DEFAULT_PRINT_ON_NO_REFERENCE_WRAPPER

template <class C>
struct has_print<boost::reference_wrapper<C> > : public has_print<C> {};
template <class C>
struct not_has_print<boost::reference_wrapper<C> > : public not_has_print<C> {};
template <class C>
struct has_print_writer<boost::reference_wrapper<C> > : public has_print_writer<C> {};
template <class C>
struct has_print_plain<boost::reference_wrapper<C> > : public has_print_plain<C> {};
template <class C>
struct not_has_print_writer<boost::reference_wrapper<C> > : public not_has_print_writer<C> {};

template <class C>
struct has_const_iterator<boost::reference_wrapper<C> > : public has_const_iterator<C> {};
template <class C>
struct not_has_const_iterator<boost::reference_wrapper<C> > : public not_has_const_iterator<C> {};

#  endif 

}

#endif
