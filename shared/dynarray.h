#ifndef DYNARRAY_H 
#define DYNARRAY_H 1
#include "config.h"

#include <string>
#include <new>
#include "myassert.h"
#include <memory>
#include <stdexcept>

// caveat:  cannot hold arbitrary types T with self or mutual-pointer refs; only works when memcpy can move you
// FIXME: possible for this to not be valid for any object with a default constructor :-(
template <typename T,typename Alloc=std::allocator<T> > class DynamicArray : private Alloc {
  unsigned int space;
  unsigned int sz;
  T *vec;
  DynamicArray& operator = (const DynamicArray &a){std::cerr << "unauthorized assignment of a dynamic array\n";}; // Yaser
 public:
  DynamicArray(unsigned sp = 4) : space(sp), sz(0), vec(0) { vec=allocate(sp);}
  DynamicArray(const DynamicArray &a): space(a.space),sz(0),vec(0) { // added by Yaser 7-27-2000
    vec = allocate(a.space);
    for (int i = 0 ; i < a.sz ; i++)
      push_back(a.vec[i]) ;
    Assert(sz==a.sz);
  }
							 
  T & operator() (unsigned int index) {    
    if ( index >= sz ) {
      unsigned int newSpace = space;
      while ( index >= newSpace ) newSpace <<=1;
      resize(newSpace);
      for ( T *v = vec + sz ; v <= vec + index ; v++ )
	PLACEMENT_NEW(v) T();
      sz = ++index;
    }
    return vec[index];
  }
  T & at(unsigned int index) const { // run-time bounds-checked
	if (!(index < sz) )
	  throw std::out_of_range("dynarray");
	return vec[index];
  }
  T & operator[] (unsigned int index) const {
    Assert(index < sz);
    return vec[index];
  }
  unsigned int index_of(T *t) const {
	Assert(t>=begin() && t<end());
    return (unsigned int)(t-vec);
  }
  void push_back()
    {
      if ( sz >= space )
	resize(space << 1);
      PLACEMENT_NEW(vec+(sz++)) T();
    }    
  void push_back(const T& val)
    {
      PLACEMENT_NEW(push_backRaw()) T(val);
    }
  T *push_backRaw()
    {
      if ( sz >= space )
	resize(space << 1);
      return vec + (sz++);
    }
	T * begin() { return vec; }
	//T * end() {	  return vec+sz;	}
	const T* begin() const {
	  return vec;
	}
  const T* end() const {
	  return vec+sz;
  }
	
  void removeMarked(bool marked[]) {
    if ( !sz ) return;
    unsigned int f, i = 0;
    while ( i < sz && !marked[i] ) ++i;
    f = i;
    while ( i < sz )
      if ( !marked[i] )
	memcpy(&vec[f++], &vec[i++], sizeof(T));
      else 
	vec[i++].~T();
    sz = f;
  }
//  operator T *() { return vec; } // use at own risk (will not be valid after resize)
  // use begin() instead
  void resize(unsigned int newSpace) {
    T *oldVec  = vec;
    if ( newSpace < sz ) 
      newSpace = sz;
    vec = allocate(newSpace);
    // caveat:  cannot hold arbitrary types T with self or mutual-pointer refs
    memcpy(vec, oldVec, sz*sizeof(T));
    deallocate(oldVec,space);
    space = newSpace;
  }
  void reserve(unsigned int newSpace) {
	if (newSpace > space)
	  resize(newSpace);
  }
  unsigned int capacity() const { return space; }
  unsigned int size() const { return sz; }
  void set_size(unsigned int newSz) { sz=newSz; } // 
  void clear() {
      for ( unsigned int	i = 0 ; i < sz ; i++ )
		vec[i].~T();
    sz = 0;
  }
  ~DynamicArray() { 	
    clear();
	if(vec) // exception safety
     deallocate(vec,space);
	//vec = NULL;space=0; // don't really need but would be safer
  }
};

#endif
