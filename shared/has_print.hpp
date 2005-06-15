// determine whether a class has specified a print_on or print_on with writer method
#ifndef PRINT_ON_HPP
#define PRINT_ON_HPP

// how this works: has_print_on<C>::type is either defined (iff C::has_print_on was typedefed to void), or undefined.  this allows us to exploit SFINAE (google) for function overloads depending on whether C::has_print_on was typedefed void ;)


template <class C,class V=void>
struct has_print_on;

template <class C,class V>
struct has_print_on {
};

template <class C>
struct has_print_on<C,typename C::has_print_on> {
  typedef void type;
};



template <class C,class V=void>
struct has_print_on_writer;

template <class C,class V>
struct has_print_on_writer {
};

template <class C>
struct has_print_on_writer<C,typename C::has_print_on_writer> {
  typedef void type;
};




template <class C,class V=void>
struct not_has_print_on_writer;

template <class C,class V>
struct not_has_print_on_writer {
  typedef void type;
};

template <class C>
struct not_has_print_on_writer<C,typename C::has_print_on_writer> {
};



template <class C,class V=void,class V2=void>
struct has_print_on_plain;

template <class C,class V,class V2>
struct has_print_on_plain {
};

template <class C>
struct has_print_on_plain<C,typename not_has_print_on_writer<C>::type, typename has_print_on<C>::type> {
  typedef void type;
};



// type is defined iff you're not a pointer type and you have no print_on(o) or print_on(o,writer)
template <class C,class V=void>
struct not_has_print_on;

template <class C,class V>
struct not_has_print_on {
  typedef void type;
};

template <class C>
struct not_has_print_on<C,typename has_print_on<C>::type> {
};

template <class C>
struct not_has_print_on<C,typename has_print_on_writer<C>::type> {
};

namespace boost {
template <class C> struct reference_wrapper;
}

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
struct not_has_print_on<C,typename isa_pointer<C>::type> {
};




#  ifndef DEFAULT_PRINT_ON_NO_REFERENCE_WRAPPER


template <class C>
struct has_print_on<boost::reference_wrapper<C> > : public has_print_on<C> {};
template <class C>
struct has_print_on_writer<boost::reference_wrapper<C> > : public has_print_on_writer<C> {};
template <class C>
struct not_has_print_on<boost::reference_wrapper<C> > : public not_has_print_on<C> {};
template <class C>
struct has_print_on_plain<boost::reference_wrapper<C> > : public has_print_on_plain<C> {};
template <class C>
struct not_has_print_on_writer<boost::reference_wrapper<C> > : public not_has_print_on_writer<C> {};

#  endif 

#endif
