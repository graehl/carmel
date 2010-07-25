#ifndef GRAEHL_SHARED__ARRAY_HPP
#define GRAEHL_SHARED__ARRAY_HPP

// note: tests in dynarray.hpp

#include <boost/config.hpp> // STATIC_CONSTANT
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

// iterator wrapper.  inverts boolean value.
template <class AB>
struct remove_not_marked
{
    typedef remove_not_marked<AB> self_type;
    AB i;

    bool operator *() const
    {
        return !*i;
    }
    void operator++()
    {
        ++i;
    }
    bool operator ==(self_type const& o)
    {
        return i==o.i;
    }
};



// outputs sequence to out iterator, of new indices for each element i, corresponding to deleting element i from an array when remove[i] is true (-1 if it was deleted, new index otherwise), returning one-past-end of out (the return value = # of elements left after deletion)
template <class REMOVE,class REMOVEe,class O>
unsigned new_indices(REMOVE i, REMOVEe end,O out,bool remove=true) {
    unsigned f=0;
    while (i!=end)
        *out++ = *i++ ? (unsigned)-1 : f++;
    return f;
};

template <class It,class RemoveIf,class O>
unsigned new_indices_remove_if_n(unsigned n,It i,RemoveIf const& r,O out)
{
    unsigned f=0;
    while(n--)
        *out++ = r(*i++) ? (unsigned)-1 : f++;
    return f;
}

template <class REMOVE,class O>
unsigned new_indices(REMOVE remove,O out) {
  return new_indices(remove.begin(),remove.end(),out);
}

// given a vector and a parallel sequence of bools where true means remove, remove the marked elements while maintaining order
struct indices_after_removing
{
    BOOST_STATIC_CONSTANT(unsigned,REMOVED=(unsigned)-1);
    unsigned *map; // map[i] == REMOVED if i is deleted
    unsigned n_kept;
    unsigned n_mapped;
    template <class AB,class ABe>
    indices_after_removing(AB i, ABe end) {
        init(i,end);
    }

    template <class Vec,class R>
    void init_remove_if(Vec v,R const& r)
    {
        n_mapped=v.size();
        if ( !n_mapped ) return;
        map=(unsigned *)::operator new(sizeof(unsigned)*n_mapped);
        n_kept=new_indices_remove_if_n(n_mapped,r,map);
    }


    template <class AB,class ABe>
    void init(AB i, ABe end) {
        n_mapped=end-i;
        if (n_mapped>0) {
            map=(unsigned *)::operator new(sizeof(unsigned)*n_mapped);
            n_kept=new_indices(i,end,map);
        } else
            map=NULL;
    }
    template <class A>
    void init(A const& a)
    {
        init(a.begin(),a.end());
    }

    template <class A>
    indices_after_removing(A const& a)
    {
        init(a.begin(),a.end());
    }
    indices_after_removing() : n_mapped(0) {}
    ~indices_after_removing()
    {
        if (n_mapped)
            ::operator delete((void*)map);
    }
    bool removing(unsigned i) const
    {
        return map[i] == REMOVED;
    }

    unsigned operator[](unsigned i) const
    {
        return map[i];
    }

    template <class Vec>
    void do_moves(Vec &v) const
    {
        assert(v.size()==n_mapped);
        unsigned i=0;
        for (;i<n_mapped&&!removing(i);++i) ;
        for(;i<n_mapped;++i)
            if (!removing(i))
                v[map[i]]=v[i];
        v.resize(n_kept);
    }

    template <class Vec>
    void do_moves_swap(Vec &v) const
    {
        using std::swap;
        assert(v.size()==n_mapped);
        unsigned r=n_mapped;
        unsigned i=0;
        for (;i<n_mapped&&!removing(i);++i) ;
        for(;i<n_mapped;++i)
            if (!removing(i))
                std::swap(v[map[i]],v[i]);
        v.resize(n_kept);
    }

 private:
    indices_after_removing(indices_after_removing const& o)
    {
        map=NULL;
    }
};


// return index of one past last rewritten element.  moves v[i] to v[ttable[i]], and calls Rewrite(v[ttable[i]],t) where ttable[i] is the new index in v.  if ttable[i] == -1, then swaps v[i] to the end, calling Rewrite(v[ttable[i]])
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
