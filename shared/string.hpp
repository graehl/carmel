// string: as opposed to a tree.
#ifndef STRING_HPP
#define STRING_HPP

#include <graehl/shared/dynarray.h>

#include <graehl/tt/ttconfig.hpp>
#include <iostream>
#include <graehl/shared/myassert.h>
#include <graehl/shared/genio.h>
//#include <vector>
#include <graehl/shared/dynarray.h>
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <functional>

#include <graehl/shared/tree.hpp>

#ifdef TEST
#include <graehl/shared/test.hpp>
#endif

//using namespace boost::lambda;
namespace lambda=boost::lambda;
//using namespace std;

//template <class L, class Alloc=std::allocator<L> > typedef Array<L,Alloc> String<L,Alloc>;


template <class L, class Alloc=std::allocator<L> > struct String : public array<L,Alloc> {
  typedef L Label;
};









#endif
