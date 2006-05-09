#ifndef GRAEHL__SHARED__ACCUMULATE_HPP
#define GRAEHL__SHARED__ACCUMULATE_HPP

#include <boost/integer_traits.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <limits>

namespace graehl {

template <class A> inline
void set_multiply_identity(A &i)
{
    i=1;
}

template <class A> inline
void set_add_identity(A &i)
{
    i=0;
}

//FIXME: use boost::integer_traits, std::numeric_limits, boost::is_integral, is_floating_point
template <class A> inline
void set_min_identity(A &i,boost::enable_if<boost::is_integral<A> >* d=0)
{
    i=boost::integer_traits<A>::max;
}

template <class A> inline
void set_max_identity(A &i,boost::enable_if<boost::is_integral<A> >* d=0)
{
    i=boost::integer_traits<A>::min;
}

template <class A> inline
void set_min_identity(A &i,boost::enable_if<boost::is_float<A> >* d=0)
{
    i=std::numeric_limits<A>::max();
}

template <class A> inline
void set_max_identity(A &i,boost::enable_if<boost::is_float<A> >* d=0)
{
    i=-std::numeric_limits<A>::max();
}

struct accumulate_multiply 
{
    template <class A>
    void operator()(A &sum) const
    {
        set_multiply_identity(sum);
    }
    
    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        sum *= x;
    }
};

struct accumulate_sum
{
    template <class A>
    void operator()(A &sum) const
    {
        set_add_identity(sum);
    }

    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        sum += x;
    }
};

struct accumulate_max
{
    template <class A>
    void operator()(A &sum) const
    {
        set_max_identity(sum);
    }
    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        if (sum<x)
            sum=x;
    }
};

struct accumulate_min
{
    template <class A>
    void operator()(A &sum) const
    {
        set_min_identity(sum);
    }
    template <class A,class X>
    void operator()(A &sum,X const& x) const 
    {
        if (x < sum)
            sum=x;
    }
};

}//graehl

#endif
