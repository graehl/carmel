/*******************************************************************************
* This software ("Carmel") is licensed for research use only, as described in  *
* the LICENSE file in this distribution.  The software can be downloaded from  *
* http://www.isi.edu/natural-language/licenses/carmel-license.html.  Please    *
* contact Yaser Al-Onaizan (yaser@isi.edu) or Kevin Knight (knight@isi.edu)    *
* with questions about the software or commercial licensing.  All software is  *
* copyrighted C 2000 by the University of Southern California.                 *
*******************************************************************************/
#ifndef DYNARRAY_H 
#define DYNARRAY_H 1

#include <string.h>
#include <new.h>
#include "assert.h"


template <typename T> class DynamicArray {
  int space;
  int sz;
  T *vec;
  DynamicArray& operator = (const DynamicArray &a){cerr << "unauthorized assignment of a dynamic array\n";}; // Yaser
public:
  DynamicArray(unsigned sp = 4) : space(sp), sz(0), vec((T*)::operator new ((size_t)sizeof(T)*sp)) {}
  DynamicArray(const DynamicArray &a): space(a.space),sz(0){ // added by Yaser 7-27-2000
     vec = (T*)::operator new((size_t)a.space*sizeof(T));
     for (int i = 0 ; i < a.sz ; i++)
        pushBack(a.vec[i]) ;
     Assert(sz==a.sz);
  }
							 
  T & operator() (int index) {
    Assert(index >= 0);
    if ( index >= sz ) {
      int newSpace = space;
      while ( index >= newSpace ) newSpace <<=1;
      resize(newSpace);
      for ( T *v = vec + sz ; rv <= vec + index ; v++ )
	new(v) T();
      sz = ++index;
    }
    return vec[index];
  }
  T & operator[] (int index) const {
    Assert(index < sz);
    return vec[index];
  }
  void pushBack()
  {
    if ( sz >= space )
      resize(space << 1);
    new(vec+(sz++)) T();
  }    
  void pushBack(const T& val)
  {
    if ( sz >= space )
      resize(space << 1);
    new(vec+(sz++)) T(val);
  }
  void removeMarked(bool marked[]) {
    if ( !sz ) return;
    int f, i = 0;
    while ( i < sz && !marked[i] ) ++i;
    f = i;
    while ( i < sz )
      if ( !marked[i] )
	memcpy(&vec[f++], &vec[i++], sizeof(T));
      else 
	vec[i++].~T();
    sz = f;
  }
  operator T *() { return vec; } // use at own risk (will not be valid after resize)
  void resize(int newSz) {
    T *oldVec  = vec;
    if ( newSz < sz ) 
      newSz = sz;
    vec = (T*)::operator new((size_t)newSz*sizeof(T));
    // caveat:  cannot hold arbitrary types T with self or mutual-pointer refs
    // also, gcc complains about delete (void *)
    memcpy(vec, oldVec, sz*sizeof(T));
    delete (void *)oldVec;
    space = newSz;
  }
  int size() const { return space; }
  int count() const { return sz; }
  ~DynamicArray() {
    if (vec) {
      for ( int	i = 0 ; i < sz ; i++ )
	vec[i].~T();
      delete (void *)vec;
    }
    vec = NULL;
    sz = 0;
  }
};

#endif
