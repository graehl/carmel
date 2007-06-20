#ifndef GRAEHL_SHARED__ARRAY_HPP
#define GRAEHL_SHARED__ARRAY_HPP

// note: tests in dynarray.hpp

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

// o[i]=j where v[i] before remove_marked(v,marked) == v[j] after.  o[i]=-1 if marked[i]. returns size after removing (max o[i]+1)
template <class O>
unsigned indices_after_remove_marked(O o,bool marked[],unsigned N) 
{
    int f=0;
    for (unsigned i=0; i <  N;++i)
        if (marked[i]) {
            *o++ = -1;
        } else {
            *o++ = f++;
        }
    return f;
}


// outputs sequence to iterator out, of new indices for each element i, corresponding to deleting element i from an array when remove[i] is true (-1 if it was deleted, new index otherwise), returning one-past-end of out (the return value = # of elements left after deletion)
template <class AB,class ABe,class O>
unsigned new_indices(AB i, ABe end,O out) {
    int f=0;
    while (i!=end)
        *out++ = *i++ ? -1 : f++;
    return f;
};

template <class AB,class O>
unsigned new_indices(AB remove,O out) {
    return new_indices(remove.begin(),remove.end());
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
