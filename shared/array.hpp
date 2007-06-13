#ifndef GRAEHL_SHARED__ARRAY_HPP
#define GRAEHL_SHARED__ARRAY_HPP

#include <algorithm> //swap
namespace graehl {

///WARNING: only use for std::vector and similar
template <class Vec>
typename Vec::value_type * array_begin(Vec & v) 
{
    return &*v.begin();
}

template <class Vec>
typename Vec::value_type const* array_begin(Vec const& v) 
{
    return &*v.begin();
}

template <class Vec>
unsigned index_of(Vec const& v,typename Vec::value_type const* p) 
{
    return p-array_begin(v);
}

template <class C> inline
void resize_up_for_index(C &c,size_t i) 
{
    const size_t newsize=i+1;
    if (newsize > c.size())
        c.resize(newsize);
}

template <class Vec>
void remove_marked_swap(Vec & v,bool marked[]) {
    using std::swap;
    typedef typename Vec::value_type V;
    unsigned sz=v.size();
    if ( !sz ) return;
    unsigned to, i = 0;
    while ( i < sz && !marked[i] ) ++i;
    to = i; // find first marked (don't need to move anything below it)
    while (i<sz)
        if (!marked[i]) {
            swap(v[to++],v[i++]);
        } else {
            ++i;
        }
    v.resize(to);
}

template <class Vec,class P>
void remove_if_swap(Vec & v,P const& pred) {
    using std::swap;
    typedef typename Vec::value_type V;
    unsigned sz=v.size();
    if ( !sz ) return;
    unsigned to, i = 0;
    while ( i < sz && !pred(v[i]) ) ++i;
    to = i; // find first marked (don't need to move anything below it)
    while (i<sz)
        if (pred(v[i])) {
            swap(v[to++],v[i++]);
        } else {
            ++i;
        }
    v.resize(to);
}

template <class C>
void push_back(C &c)
{
    c.push_back(typename C::value_type());
}


}


#endif
