#ifndef MEMORY_ARCHIVE_HPP
#define MEMORY_ARCHIVE_HPP

#include "serialize_config.hpp"
#include <stddef>

template <class T>
inline void set_randomly(T *data,unsigned n)
{
    *data=T(n);
}

template <>
inline void set_randomly(std::string *data,unsigned n)
{
    data->assign(n,'s');
}



#ifdef TEST
# include "test.hpp"

template <class T>
inline void test_serialize_type(const T *dummy=0) 
{    
    T t1,t2;
    const unsigned n_buf=100000;
    char buf[n_buf];
    const unsigned N=5;
    for (unsigned i=0;i<N;++i) {
        set_randomly(&t1,i);
        from_buf_to_data(&t2,buf_,from_data_to_buf(t1,buf,n_buf));
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
