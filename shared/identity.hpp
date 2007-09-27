#ifndef GRAEHL_SHARED__IDENTITY_HPP
#define GRAEHL_SHARED__IDENTITY_HPP

namespace graehl {

template <class V>
struct identity
{
    typedef V argument_type;
    typedef  V result_type;
    result_type operator()(argument_type a) const { return a; }
};

template <class V>
struct identity_ref
{
    typedef V argument_type;
    typedef V result_type;
    result_type const& operator()(argument_type const& a) const { return a; }
    result_type & operator()(argument_type & a) const { return a; }
};

// should be safe as identity<V const&>
template <class V>
struct identity_cref
{
    typedef V const& argument_type;
    typedef  V const& result_type;
    result_type operator()(argument_type a) const { return a; }
};

    
}



#endif
