#ifndef MEMORY_ARCHIVE_HPP
#define MEMORY_ARCHIVE_HPP

#include <graehl/shared/serialize_config.hpp>
#include <graehl/shared/array_stream.hpp>
//#include <cstddef>

namespace graehl {

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

static const char *example_random_strings[] = {
    "a",
    "",
    "jeff \n",
    "\t \t",
    "bob",
    "sue 37840",
    "xxxxxxxxxxxxxxxxxxxxyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz oooooooooooooooooooooooo mmmmmmmmmmmmmmmmmmmmmmmm"
    "q(repeats follow)"
};

template <class T>
inline void set_randomly(std::vector<T> &v,unsigned n)
{
    const unsigned max_random_size=999;    
    v.clear();
    unsigned sz = n % max_random_size;
    for (unsigned i=0;i<sz;++i) {
        v.push_back(T());
        set_randomly(v[i],n-i);
    }    
}

inline void set_randomly(const char *&v,unsigned n)
{
    const unsigned n_possible=sizeof(example_random_strings)/sizeof(example_random_strings[0]);
    v=example_random_strings[n%n_possible];
}


template <class As,class Data>
inline void array_save(As &a,const Data &d)
{
    a.reset();
    default_oarchive o(a,ARCHIVE_FLAGS_DEFAULT);
    o & d;
}

//NOTE: probably want to     a.reset_read(); first if you just wrote to As.
template <class As,class Data>
inline void array_load(As &a,Data &d)
{
//    DBP(a);
    default_iarchive i(a,ARCHIVE_FLAGS_DEFAULT);
    i & d;
}

#ifdef GRAEHL_TEST
# include "test.hpp"

template <class T>
inline void test_serialize_type(const T *dummy=0) 
{    
    T t1,t2;
    const unsigned N=27;
    const unsigned BUF_SIZE=N*N*1000;
    char buf[BUF_SIZE];
    array_stream as(buf,BUF_SIZE);
    for (unsigned i=0;i<N;++i) {
        set_randomly(t1,i);
        array_save(as,t1);
        as.reset_read();        
        array_load(as,t2);
        BOOST_CHECK(t1 == t2);
    }
}

BOOST_AUTO_TEST_CASE( TEST_serialize_memory )
{
    test_serialize_type<int>();
    test_serialize_type<std::string>();
    test_serialize_type<vector<int> >();
    test_serialize_type<vector<std::string > >();
}

#endif

}

#endif
