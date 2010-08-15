#ifndef GRAEHL_SHARED__INDICES_AFTER_HPP
#define GRAEHL_SHARED__INDICES_AFTER_HPP


//TODO: instead of making REMOVED a constant, make it class state (defaults to -1 as before, of course).

#include <assert.h>
#include <boost/config.hpp> // STATIC_CONSTANT
#include <algorithm> //swap
#include <iterator>

namespace graehl {

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

// iterator wrapper.  inverts boolean value.
template <class AB>
struct iter_not
{
    typedef iter_not<AB> self_type;
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
template <class KEEP,class KEEPe,class O>
unsigned new_indices_keep(KEEP i, KEEPe end,O out,bool means_keep=true) {
    unsigned f=0;
    while (i!=end)
      *out++ = (*i++ == means_keep) ? f++ : (unsigned)-1;
    return f;
};

template <class It,class KeepIf,class O>
unsigned new_indices_keep_if_n(unsigned n,It i,KeepIf const& r,O out)
{
    unsigned f=0;
    while(n--)
      *out++ = r(*i++) ? f++ : (unsigned)-1;
    return f;
}

template <class KEEP,class O>
unsigned new_indices_keep(KEEP keep,O out,bool means_keep=true) {
  return new_indices_keep(keep.begin(),keep.end(),out,means_keep);
}

template <class V,class Out,class Permi>
void copy_perm_to(Out o,V const& from,Permi i,Permi e) {
  for (;i!=e;++i)
    *o++=from[*i];
}

//to cannot be same as from, for most permutations.  for to==from, use indices_after::init_inverse_order instead.
template <class Vto,class Vfrom,class Perm>
void remap_perm_to(Vto &to,Vfrom const& from,Perm const& p) {
  to.resize(p.size());
  copy_perm_to(to.begin(),from,p.begin(),p.end());
}

// given a vector and a parallel sequence of bools where true means keep, keep only the marked elements while maintaining order.
// this is done with a parallel sequence to the input, marked with positions the kept items would map into in a destination array, with removed items marked with the index -1.  the reverse would be more compact (parallel to destination array, index of input item that goes into it) but would require the input sequence be random access.
struct indices_after
{
  BOOST_STATIC_CONSTANT(bool,KEEP=true);
  typedef indices_after self_type;
  BOOST_STATIC_CONSTANT(unsigned,REMOVED=(unsigned)-1);
  unsigned *map; // map[i] == REMOVED if i is deleted
  unsigned n_kept; // important to init this.
  unsigned n_mapped;

  indices_after() : n_mapped(0) {}
  ~indices_after()
  {
    free();
  }
  template <class AB,class ABe>
  indices_after(AB i, ABe end,bool mk=KEEP) {
    init(i,end,KEEP);
  }
  template <class A>
  indices_after(A const& a,bool mk=KEEP)
  {
    init(a.begin(),a.end(),mk);
  }

  template <class AB,class ABe>
  void init(AB i, ABe end,bool mk=KEEP) {
    n_mapped=end-i;
    if (n_mapped>0) {
      map=(unsigned *)::operator new(sizeof(unsigned)*n_mapped);
      n_kept=new_indices_keep(i,end,map,mk);
    } else {
      n_kept=0;
      map=NULL;
    }
  }
  template <class A>
  void init(A const& a,bool mk=KEEP)
  {
    init(a.begin(),a.end(),mk);
  }

  template <class Order>
  void init_inverse_order(unsigned from_sz,Order const& order) {
    init_inverse_order(from_sz,order.begin(),order.end());
  }
  template <class OrderI>
  void init_inverse_order(unsigned from_sz,OrderI i,OrderI end) {
    init_alloc(from_sz);
    unsigned d=0;
    n_kept=0;
    for(;i!=end;++i) {
      assert(d<from_sz);
      map[d++]=*i;
      ++n_kept;
    }
    for(;d<from_sz;++d)
      map[d]=REMOVED;
  }

  template <class Vec,class R>
  void init_keep_if(Vec v,R const& r)
  {
    n_mapped=v.size();
    if ( !n_mapped ) return;
    map=(unsigned *)::operator new(sizeof(unsigned)*n_mapped);
    n_kept=new_indices_keep_if_n(n_mapped,r,map);
  }
  // contents uninit.
  void init_alloc(unsigned n) {
    free();
    n_mapped=n;
    map=n_mapped>0 ?
      (unsigned *)::operator new(sizeof(unsigned)*n_mapped)
      : 0;
  }
  void init_const(unsigned n,unsigned map_all_to) {
    init_alloc(n);
    for (unsigned i=0;i<n;++i)
      map[i]=map_all_to;
    n_kept=(map_all_to==REMOVED)?0:n;
  }

  void init_keep_none(unsigned n) {
    init_const(n,REMOVED);
    n_kept=0;
  }

  void free() {
    if (n_mapped)
      ::operator delete((void*)map);
    n_mapped=0;
  }

  bool removing(unsigned i) const
  {
    return map[i] == REMOVED;
  }
  bool keeping(unsigned i) const
  {
    return map[i] != REMOVED;
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
    for (;i<n_mapped&&keeping(i);++i) ;
    for(;i<n_mapped;++i)
      if (keeping(i))
        v[map[i]]=v[i];
    v.resize(n_kept);
  }

  template <class Vec>
  void do_moves_swap(Vec &v) const
  {
    using std::swap;
    assert(v.size()==n_mapped);
    unsigned i=0;
    for (;i<n_mapped&&keeping(i);++i) ;
    for(;i<n_mapped;++i)
      if (keeping(i))
        std::swap(v[map[i]],v[i]);
    v.resize(n_kept);
  }

  template <class Vecto,class Vecfrom>
  void copy_to(Vecto &to,Vecfrom const& v) const {
    to.resize(n_kept);
    for (unsigned i=0;i<n_mapped;++i)
      if (keeping(i))
        to[map[i]]=v[i];
  }

  //transform collection of indices into what we're remapping.  (input/output iterators)
  template <class IndexI,class IndexO>
  void reindex(IndexI i,IndexI const end,IndexO o) const {
    for(;i<end;++i) {
      unsigned m=map[*i];
      if (m!=REMOVED)
        *o++=m;
    }
  }

  template <class VecI,class VecO>
  void reindex_push_back(VecI const& i,VecO &o) const {
    reindex(i.begin(),i.end(),std::back_inserter(o));
  }

private:
  indices_after(self_type const&)
  {
    map=NULL;
  }
};

struct indices_after_removing : public indices_after {
  BOOST_STATIC_CONSTANT(bool,KEEP=false);
  indices_after_removing() {  }
  template <class AB,class ABe>
  indices_after_removing(AB i, ABe end,bool mk=KEEP) {
    indices_after::init(i,end,mk);
  }
  template <class A>
  indices_after_removing(A const& a,bool mk=KEEP)
  {
    indices_after::init(a.begin(),a.end(),mk);
  }
  template <class AB,class ABe>
  void init(AB i, ABe end,bool mk=KEEP) {
    indices_after::init(i,end,mk);
  }
  template <class A>
  void init(A const& a,bool mk=KEEP)
  {
    indices_after::init(a.begin(),a.end(),mk);
  }
};

}//ns


#endif
