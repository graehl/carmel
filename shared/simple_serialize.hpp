#ifndef GRAEHL_SHARED__SIMPLE_SERIALIZE_HPP
#define GRAEHL_SHARED__SIMPLE_SERIALIZE_HPP

/* nonportable binary serialization.  no header. no portability across diff. arch./byte order

"serialize" name would conflict w/ boost::serialization, but the version number argument is omitted, so perhaps defining w/ a version arg allows you to support both.  if not, rename to "sserialize" later.

no (provided) handling of pointers

no version numbers

classes other than basic ints/strings must implement a member:

template <class Archive> void serialize(Archive &a) { a & x; a & y; }

or free:

template <class A> void serialize(Archive &a,MyClass &c)  { a & c.x & c.y; }

inspired by boost serialization - but i think it may be easier to get what i
want without their complicated design.  as well, lack of header means you can
catenate and iterate over archived items.  note that it may be possible to
create some custom archive in boost serialization that has the same advantages

to use: the expression a&x either saves x to archive a, if a is a saving
archive (equiv: a>>x), or loads x from a (equiv:a<<x), if a is a loading
archive.

*/

#include <iostream>
#include <iterator>
#include <stdexcept>
#include <boost/config.hpp>

namespace graehl {

struct simple_archive_error : public std::runtime_error
{
    simple_archive_error() : std::runtime_error("simple_archive serialization error") {}
};


template <class S>
struct simple_oarchive
{
    BOOST_STATIC_CONSTANT(bool,is_saving=true);
    BOOST_STATIC_CONSTANT(bool,is_loading=false);
    S &s;

    simple_oarchive(S &s) : s(s) {}
//    simple_oarchive(simple_oarchive const& o) : s(o.s) {}

    void load_binary(void const*d,std::streamsize n) { throw simple_archive_error(); }
    void save_binary(void const*d,std::streamsize n) {
        if (!s.write((char const*)d,n)) throw simple_archive_error(); }
    void binary(void const* d,std::streamsize n) { save_binary(d,n); }

};

template <class S>
struct simple_iarchive
{
    BOOST_STATIC_CONSTANT(bool,is_saving=false);
    BOOST_STATIC_CONSTANT(bool,is_loading=true);
    S &s;
    simple_iarchive(S &s) : s(s) {}
//    simple_iarchive(simple_iarchive const& o) : s(o.s) {}

    void load_binary(void *d,std::streamsize n) {
        if (!s.read((char *)d,n)) throw simple_archive_error();
// no need to check s.gcount() because s will be bad if full read didn't happen
    }
    void save_binary(void const*d,std::streamsize n) { throw simple_archive_error(); }
    void binary(void const* d,std::streamsize n) { load_binary(d,n); }

};

typedef simple_iarchive<std::istream> istream_archive;
typedef simple_oarchive<std::ostream> ostream_archive;

// serialize(a,d), which can be found by type-dependent lookup, is the central dispatch; we define it for built-in integral types, and also provide a default that forwards to the serialize member:

#define GRAEHL_SPLIT_SAVE_LOAD_MEMBER() \
    template <class A> void serialize(A &a)     \
    { graehl::split_save_load_member(a,*this);         \
    }                                           \

#define GRAEHL_SPLIT_SAVE_LOAD_FREE(D) \
    template <class A> void serialize(A &a,D &d)           \
    { graehl::split_save_load_free(a,d);         \
    }                                           \

#define GRAEHL_PRIMITIVE_SERIALIZE(T) \
    template <class S> void serialize(graehl::simple_iarchive<S> &a,T &t) { a.load_binary((void*)&t,sizeof(t)); }       \
    template <class S> void serialize(graehl::simple_oarchive<S> &a,T const&t) { a.save_binary((void const*)&t,sizeof(t)); } \
    template <class S> void serialize(graehl::simple_oarchive<S> &a,T &t) { a.save_binary((void const*)&t,sizeof(t)); }


GRAEHL_PRIMITIVE_SERIALIZE(bool)
GRAEHL_PRIMITIVE_SERIALIZE(char)
GRAEHL_PRIMITIVE_SERIALIZE(short)
GRAEHL_PRIMITIVE_SERIALIZE(int)
GRAEHL_PRIMITIVE_SERIALIZE(long long)
GRAEHL_PRIMITIVE_SERIALIZE(unsigned char)
GRAEHL_PRIMITIVE_SERIALIZE(unsigned short)
GRAEHL_PRIMITIVE_SERIALIZE(unsigned int)
GRAEHL_PRIMITIVE_SERIALIZE(unsigned long)
GRAEHL_PRIMITIVE_SERIALIZE(unsigned long long)
GRAEHL_PRIMITIVE_SERIALIZE(float)
GRAEHL_PRIMITIVE_SERIALIZE(double)
GRAEHL_PRIMITIVE_SERIALIZE(void *)
//FIXME: these types may not all be distinct: http://en.wikipedia.org/wiki/64-bit#64-bit_data_models
//TODO: support string and wstring, STD containers?

template <class A,class D>
void serialize(A &a,D &d)
{
    d.serialize(a);
}

template <class S,class D>
simple_iarchive<S> & operator &(simple_iarchive<S> &a,D &d)
{
    serialize(a,d);
    return a;
}

template <class S,class D>
simple_oarchive<S> & operator &(simple_oarchive<S> &a,D &d)
{
    serialize(a,d);
    return a;
}

template <class S,class D>
simple_oarchive<S> & operator &(simple_oarchive<S> &a,D const&d)
{
    serialize(a,d);
    return a;
}

// << and >> are synonyms for &
template <class S,class D>
simple_oarchive<S> & operator <<(simple_oarchive<S> &s,D &d)
{
    return s & d;
}

template <class S,class D>
simple_oarchive<S> & operator <<(simple_oarchive<S> &s,D const&d)
{
    return s & d;
}

template <class S,class D>
simple_iarchive<S> & operator >>(simple_iarchive<S> &s,D &d)
{
    return s & d;
}



/*
template <class A,class D>
void serialize(A &a,D const& d)
{
    d.serialize(a);
}
*/

template <class A,class D>
void split_save_load_member(A &a,D &d)
{
    if (a.is_loading) {
        d.load(a);
    } else {
//        assert(a.is_saving);
        d.save(a);
    }
}

template <class A,class D>
void split_save_load_free(A &a,D &d)
{
    if (a.is_loading) {
        load(a,d);
    } else {
//        assert(a.is_saving);
        save(a,d);
    }
}

}

template <class A,class C>
void save_container(A &a,C &c)
{
    std::size_t sz=c.size();
    a & sz;
    for (typename C::iterator i=c.begin(),e=c.end();i!=e;++i)
        a & *i;
}


template <class A,class C>
void load_container(A &a,C &c)
{
    std::size_t sz;
    a & sz;
//    using namespace std;
    c.clear();
    std::back_insert_iterator<C> b(c);
    while (sz-- > 0) {
        typedef typename C::value_type V;
        V v;
        a & v;
        *b++=v;
    }
}

// uses c.size() c.clear() and back_insert_iterator(c)
template <class A,class C>
void serialize_container(A &a,C &c)
{
    if (A::is_saving)
        save_container(a,c);
    else
        load_container(a,c);
}


#endif
