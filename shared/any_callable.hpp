#ifndef GRAEHL__SHARED__ANY_CALLABLE_HPP
#define GRAEHL__SHARED__ANY_CALLABLE_HPP

#include <iostream>
#include <boost/any.hpp>

namespace graehl {

//FIXME: CONCEPTUAL BUGS
//FIXME: const and nonconst anyarg versions

// want: any+type_info+name,fixedargs -> name((type)any,fixedargs)
template <class Name,class FixedArgs>
struct any_callable
{
    virtual void call(boost::any const& val,FixedArgs & args) const = 0;
};

// Derived should have call(Name *,AnyArg &a,FixedArgs &rest) - dispatch is done
// on AnyArg (and Name); FixedArgs are always part of static signature
template <class Derived,class Name,class AnyArg,class FixedArgs>
struct any_concrete_callable : public any_callable<Name,FixedArgs>
{
    virtual void call(boost::any const& val,FixedArgs &args) const //NOTE: const_cast so may actually not be :)
    {
        derived().call((Name *)0,retype_any(val),args);
    }
 private:
    AnyArg & retype_any(boost::any const& val) const
    { return const_cast<AnyArg &>(*boost::any_cast<AnyArg>(&val)); } // more efficient than any_cast<AnyArg>(val)
        
    Derived& derived() const
    { return *(Derived*)(this); }    
};

struct name_print {};

template <class O=std::ostream>
struct any_printable 
{
    typedef any_callable<name_print,O> type;
};
    

template <class Val,class O>
struct typed_printer :
public any_concrete_callable<typed_printer<Val,O>,
                             name_print,Val,O>
{
    void call(name_print *tag,Val &val,O &o) const
    {
        o << val;
    }
};
    
}


#endif
