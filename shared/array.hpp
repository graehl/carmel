#ifndef GRAEHL_SHARED__ARRAY_HPP
#define GRAEHL_SHARED__ARRAY_HPP

// note: tests in dynarray.hpp

#include <boost/config.hpp> // STATIC_CONSTANT
#include <algorithm> //swap
#include <graehl/shared/indices_after.hpp>

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
void resize_up_for_index(C &c,std::size_t i)
{
    const std::size_t newsize=i+1;
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

// return index of one past last rewritten element.  moves (safely) all v[i] to v[ttable[i]], and calls Rewrite(v[ttable[i]],t) where ttable[i] is the new index in v.  if ttable[i] == -1, then swaps v[i] to the end, calling Rewrite(v[ttable[i]])
// note: does the movements in a safe order (assigning to a dest. only after it's already been used as a source).
template <class T,class Rewrite>
unsigned shuffle_removing(T *v,indices_after_removing const& ttable,Rewrite &r)
{
    using std::swap;
    unsigned to, i = 0, sz= ttable.n_mapped;
//    if (sz==0) return 0;
    for ( ; i < sz && !ttable.removing(i) ;++i )
        r(v[i],ttable);

    to = i; // find first marked (don't need to move anything below it)
    while (i<sz) {
        if (ttable.removing(i)) {
            r(v[i]); // rewrite for deletion
            ++i;
        } else {
            r(v[i],ttable);
            swap(v[to++],v[i++]);
        }
    }
    return to;
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
