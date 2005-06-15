// string: as opposed to a tree.
#ifndef STRING_HPP
#define STRING_HPP

#include "dynarray.h"

#include "ttconfig.hpp"
#include <iostream>
#include "myassert.h"
#include "genio.h"
//#include <vector>
#include "dynarray.h"
#include <algorithm>
#include <boost/lambda/lambda.hpp>
#include <functional>

#include "tree.hpp"

#ifdef TEST
#include "test.hpp"
#endif

//using namespace boost::lambda;
namespace lambda=boost::lambda;
//using namespace std;

//template <class L, class Alloc=std::allocator<L> > typedef Array<L,Alloc> String<L,Alloc>;


template <class L, class Alloc=std::allocator<L> > struct String : public Array<L,Alloc> {
  typedef L Label;
};









#endif
