#ifndef MEMORY_ARCHIVE_HPP
#define MEMORY_ARCHIVE_HPP

#include "serialize_config.hpp"
#include "array_stream.hpp"
#include <stddef>

template <class T>
inline void set_randomly(T &v,unsigned n)
{
    v=T(n);
}

template <>
inline void set_randomly(std::string &v,unsigned n)
{
    v.assign(n,'s');
}

const char *example_random_strings[] = {
    "a",
    "",
    "jeff \n",
    "\t \t",
    "bob",
    "sue 37840",
    "xxxxxxxxxxxxxxxxxxxxyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz oooooooooooooooooooooooo mmmmmmmmmmmmmmmmmmmmmmmm"
    "q(repeats follow)"
};

    
inline void set_randomly(char *&v,unsigned n)
{
    const unsigned n_possible=sizeof(example_random_strings/sizeof(example_random_strings[0]));
    v=example_random_strings[n%n_possible];
}


template <class As>
inline void array_save(As &a,const Data &d)
{
    a.clear();
    default_oarchive o(a);
    o << d;
}

template <class As>
inline void array_load(const As &a,Data &d)
{
    default_iarchive i(a);
    i >> d;
}

#ifdef TEST
# include "test.hpp"

template <class T>
inline void test_serialize_type(const T *dummy=0) 
{    
    T t1,t2;
    const unsigned BUF_SIZE=100000;
    char buf[BUF_SIZE];
    array_stream as(buf,BUF_SIZE);
    const unsigned N=5;
    for (unsigned i=0;i<N;++i) {
        set_randomly(t1,i);
        array_save(as,t1);
        as.rewind();
        array_load(as,t2);
        BOOST_CHECK_EQUAL(t1,t2);
    }
}

BOOST_AUTO_UNIT_TEST( TEST_serialize_memory )
{
    test_serialize_type<int>();
    test_serialize_type<std::string>();
}

#endif

#endif
