#ifndef GRAEHL__SHARED__IS_NULL_HPP
#define GRAEHL__SHARED__IS_NULL_HPP

template <class C> inline
bool is_null(C const &c) 
{ return !c; }

template <class C> inline
void set_null(C &c)
{ c=C(); }


#endif
