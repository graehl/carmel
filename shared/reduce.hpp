#ifndef GRAEHL_SHARED__REDUCE_HPP
#define GRAEHL_SHARED__REDUCE_HPP

#include <boost/range.hpp>
#include <boost/iterator/reverse_iterator.hpp>

/* foldl - see http://en.wikipedia.org/wiki/Fold_(higher-order_function)

explicit zero version (foldl) allows any left (result) type

to save typing and allow default zero construction, reduce is foldl but assumes
result type = sequence type

*/

namespace graehl {

// (1,2) -> (0+1)+2
template <class I,class Op,class Zero>
Zero
foldl(I i,I const& end,Op const& op,Zero z) 
{
    for (;i!=end;++i)
        z=op(z,*i);
    return z;
}

template <class R,class Op,class Zero>
Zero
foldl(R const& range,Op const& op,Zero z) 
{
    return foldl(boost::begin(range),boost:end(range),op,z);
}

// unlike foldr, changes associativity of op, without changing order of iteration
// (1,2) -> 2+(1+0)
template <class I,class Op,class Zero>
Zero
foldr_reversed(I i,I const& end,Op const& op,Zero z) 
{
    for (;i!=end;++i)
        z=op(*i,z);
    return z;
}

// (1,2) -> 1+(2+0)
template <class I,class Op,class Zero>
Zero
foldr(I i,I const& end,Op const& op,Zero z) 
{
    boost::reverse_iterator<I> ri(end),re(i);
    return foldr_reversed(ri,re,op,z);
}

template <class R,class Op,class Zero>
Zero
foldr(R const& range,Op const& op,Zero z) 
{
    return foldr_reversed(boost::rbegin(range),boost:rend(range),op,z);
}

// all of what follows is for default zero w/ same type as sequence:

template <Op2,class V>
struct op2_result 
{
    typedef typename boost:result_of<Op2(V,V)>::type type;
};

template <Op2,class I>
struct op2_iterator_result
{
    typedef typename op2_result<Op2,boost::iterator_value<I>::type>
};

template <Op2,class I>
struct op2_range_result
{
    typedef typename op2_result<Op2,boost::range_value<I>::type>
};

template <class I,class Op>
typename op2_iterator_result<Op,I>::type
reduce(I i,I const& end,Op const& op
       ,typename op2_iterator_result<Op,I>::type z=
       typename op2_iterator_result<Op,I>::type()
    )
{
    /*
      for (;i!=end;++i)
      z=op(z,*i);
      return z;
    */
    return foldl(i,end,op,z);
}

template <class R,class Op>
typename op2_range_result<Op,R>::type
reduce(R const& range,Op const& op
       ,typename op2_range_result<Op,R>::type z=
       typename op2_range_result<Op,R>::type()
    )
{
//    return reduce(boost::begin(range),boost:end(range),op,z);
    return foldl(range,op,z);
}


}


#endif
