// Copyright 2014 Jonathan Graehl - http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef GRAEHL__SHARED__ANY_CALLABLE_HPP
#define GRAEHL__SHARED__ANY_CALLABLE_HPP

#include <iostream>
#include <boost/any.hpp>

namespace graehl {

//FIXME: CONCEPTUAL BUGS
//FIXME: const and nonconst anyarg versions

// want: any+type_info+name,fixedargs -> name((type)any,fixedargs)
template <class Name, class FixedArgs>
struct any_callable
{
  virtual void call(boost::any const& val, FixedArgs & args) const = 0;
};

// Derived should have call(Name *,AnyArg &a,FixedArgs &rest) - dispatch is done
// on AnyArg (and Name); FixedArgs are always part of static signature
template <class Derived, class Name, class AnyArg, class FixedArgs>
struct any_concrete_callable : public any_callable<Name, FixedArgs>
{
  virtual void call(boost::any const& val, FixedArgs &args) const //NOTE: const_cast so may actually not be :)
  {
    derived().call((Name *)0, retype_any(val), args);
  }
 private:
  AnyArg & retype_any(boost::any const& val) const
  { return const_cast<AnyArg &>(*boost::any_cast<AnyArg>(&val)); } // more efficient than any_cast<AnyArg>(val)

  Derived& derived() const
  { return *(Derived*)(this); }
};

struct name_print {};

template <class O = std::ostream>
struct any_printable
{
  typedef any_callable<name_print, O> type;
};


template <class Val, class O>
struct typed_printer :
      public any_concrete_callable<typed_printer<Val, O>,
                                   name_print, Val, O>
{
  void call(name_print *tag, Val &val, O &o) const
  {
    o << val;
  }
};

}


#endif
