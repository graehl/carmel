#ifndef DYNARRAY_H 
#define DYNARRAY_H 1
// For MEMCPY-able-to-move types only!
#include "config.h"

#include <string>
#include <new>
#include "myassert.h"
#include <memory>
#include <stdexcept>

// doesn't manage its own space (use alloc(n) and dealloc() yourself).  0 length allowed.
template <typename T,typename Alloc=std::allocator<T> > class Array : protected Alloc {
protected:
  //unsigned int space;
  T *vec;
  T *endspace;
public:
  typedef T* iterator;
  typedef const T* const_iterator;
  bool empty() const {
	return vec==endspace;
  }
  void dealloc() {
	int cap=capacity();
	if (cap) {
	  Assert(vec);
	  deallocate(vec,cap);	  
	  Paranoid(vec=NULL;);
	  endspace=vec;
	}
  }
  // doesn't copy old elements like dynamicarray does
  void alloc_fresh(unsigned sp) {
	if(sp) 
	  vec=allocate(sp);
	endspace=vec+sp;
  }
  void alloc(unsigned sp) {
//	if (sp == space) return;
	  dealloc();
	  alloc_fresh(sp);
  }
  explicit Array(unsigned sp=4) { alloc_fresh(sp); }
  Array(unsigned sp, const Alloc &a): Alloc(a) { alloc_fresh(sp); }
  unsigned capacity() const { return (unsigned)(endspace-vec); }
  T * begin() { return vec; }
	T * end() {	  return endspace;	}
	const T* begin() const {
	  return vec;
	}
  const T* end() const {
	  return endspace;
  }
  T & at(unsigned int index) const { // run-time bounds-checked
	T *r=vec+index;
	if (!(r < end()) )
	  throw std::out_of_range("dynarray");
	return *r;
  }
  T & operator[] (unsigned int index) const {
    Assert(vec+index < end());
    return vec[index];
  }
  unsigned int index_of(T *t) const {
	Assert(t>=begin() && t<end());
    return (unsigned int)(t-vec);
  }
  ///XXX: must always properly set_capacity AFTER set_begin since this invalidates capacity
  void set_begin(T* buf) { 
	//unsigned cap=capacity(); 
	vec=buf; 
	//set_capacity(cap); 
  }
  void set_capacity(unsigned int newCap) { endspace=vec+newCap; }
};

// caveat:  cannot hold arbitrary types T with self or mutual-pointer refs; only works when memcpy can move you
// FIXME: possible for this to not be valid for any object with a default constructor :-(
template <typename T,typename Alloc=std::allocator<T> > class DynamicArray : public Array<T,Alloc> {
  //unsigned int sz;
  T *endvec;
  DynamicArray& operator = (const DynamicArray &a){std::cerr << "unauthorized assignment of a dynamic array\n";}; // Yaser
 public:
  DynamicArray(unsigned sp = 4) : Array<T,Alloc>(sp), endvec(vec) { }
#if 0
  DynamicArray(const DynamicArray &a): endvec(a.endvec){
	unsigned sz=size();
    alloc_fresh(sz);
	memcpy(vec,a.vec,sizeof(T)*sz);
  }
#endif

  const T* end() const { // Array code that uses vec+space for boundschecks is duplicated below
	  return endvec;
  }							 
    T* end()  { // Array code that uses vec+space for boundschecks is duplicated below
	  return endvec;
  }							 
  T & at(unsigned int index) const { // run-time bounds-checked
	T *r=vec+index;
	if (!(r < end()) )
	  throw std::out_of_range("dynarray");
	return *r;
  }
  T & operator[] (unsigned int index) const {
    Assert(vec+index < end());
    return vec[index];
  }
  unsigned int index_of(T *t) const {
	Assert(t>=begin() && t<end());
    return (unsigned int)(t-vec);
  }

  // NEW OPERATIONS:
  // like [], but bounds-safe: if past end, expands and default constructs elements between old max element and including new max index
  T & operator() (unsigned int index) {    
    if ( index >= size() ) {
      unsigned int newSpace = capacity();
      while ( index >= newSpace ) newSpace <<=1;
      resize(newSpace);
	  T *v = end();
	  endvec=vec+index+1;
      while( v < endvec )
		PLACEMENT_NEW(v++) T();
    }
    return vec[index];
  }
  // default-construct version (not in STL vector)
  void push_back()
    {
      PLACEMENT_NEW(push_backRaw()) T();
    }    
  void push_back(const T& val)
    {
      PLACEMENT_NEW(push_backRaw()) T(val);
    }
  // non-construct version (use PLACEMENT_NEW yourself) (not in STL vector either)
  T *push_backRaw()
    {
      if ( endvec >= endspace )
		resize(capacity() << 1);
      return endvec++;
    }
	
  void removeMarked(bool marked[]) {
	unsigned sz=size();
    if ( !sz ) return;
    unsigned int f, i = 0;
    while ( i < sz && !marked[i] ) ++i;
	  f = i; // find first marked (don't need to move anything below it)
#ifndef OLD_REMOVE_MARKED
	  if (i<sz) {
	    vec[i++].~T();
		for(;;) {
		  while(i<sz && marked[i]) 
		    vec[i++].~T();
		  if (i==sz)
			break;
		  unsigned i_base=i;
		  while (i<sz && !marked[i]) ++i;
		  if (i_base!=i) {
			unsigned run=(i-i_base);
//			DBP(f << i_base << run);
			memmove(&vec[f],&vec[i_base],sizeof(T)*run);
			f+=run;
		  }
		}
	  }
#else	  
    while ( i < sz )
      if ( !marked[i] )
		memcpy(&vec[f++], &vec[i++], sizeof(T));
      else 
		vec[i++].~T();
#endif
    set_size(f);
  }
//  operator T *() { return vec; } // use at own risk (will not be valid after resize)
  // use begin() instead
  void resize(unsigned int newSpace) {
	unsigned sz=size();
    if ( newSpace < sz ) 
      newSpace = sz;
    T *newVec = allocate(newSpace); // can throw but we've made no changes yet
    memcpy(newVec, vec, sz*sizeof(T));
	deallocate(vec,capacity()); // can't throw
	set_begin(newVec);set_capacity(newSpace);set_size(sz);
    // caveat:  cannot hold arbitrary types T with self or mutual-pointer refs
  }
  void reserve(unsigned int newSpace) {
	if (newSpace > capacity())
	  resize(newSpace);
  }

  unsigned int size() const { return (unsigned)(endvec-vec); }
  void set_size(unsigned int newSz) { endvec=vec+newSz; }
  void clear() {
      for ( T *i=begin();i<end();++i)
		i->~T();
    endvec=vec;
  }
  ~DynamicArray() { 	
    clear();
//	if(vec) // to allow for exception in constructor
	dealloc();
	//vec = NULL;space=0; // don't really need but would be safer
  }
};


#ifdef TEST
#include "../../tt/test.hpp"
bool rm1[] = { 0,1,1,0,0,1,1 };
bool rm2[] = { 1,1,0,0,1,0,0 };
int a[] = { 1,2,3,4,5,6,7 };
int a1[] = { 1, 4, 5 };
int a2[] = {3,4,6,7};
#include <algorithm>
#include <iterator>
BOOST_AUTO_UNIT_TEST( dynarray )
{
  const int sz=7;

  using namespace std;
  Array<int> aa(sz);
  BOOST_CHECK(aa.capacity() == sz);
  DynamicArray<int> da;
  DynamicArray<int> db(sz);
  BOOST_CHECK(db.capacity() == sz);
  copy(a,a+sz,aa.begin());
  copy(a,a+sz,back_inserter(da));
  copy(a,a+sz,back_inserter(db));
  BOOST_CHECK(db.capacity() == sz); // shouldn't have grown
  BOOST_CHECK(search(a,a+sz,aa.begin(),aa.end())==a); // really just tests begin,end as proper iterators
  BOOST_CHECK(da.size() == sz);
  BOOST_CHECK(da.capacity() >= sz);  
  BOOST_CHECK(search(a,a+sz,da.begin(),da.end())==a); // tests push_back
  BOOST_CHECK(search(a,a+sz,db.begin(),db.end())==a); // tests push_back
  BOOST_CHECK(search(da.begin(),da.end(),aa.begin(),aa.end())==da.begin());
  for (int i=0;i<sz;++i) {
    BOOST_CHECK(a[i]==aa.at(i));
	BOOST_CHECK(a[i]==da.at(i));
	BOOST_CHECK(a[i]==db(i));
  }

  const int sz1=3,sz2=4;;
  da.removeMarked(rm1); // removeMarked
  BOOST_REQUIRE(da.size()==sz1);
  for (int i=0;i<sz1;++i)
    BOOST_CHECK(a1[i]==da[i]);
  db.removeMarked(rm2);
  BOOST_REQUIRE(db.size()==sz2);
  for (int i=0;i<sz2;++i)
	BOOST_CHECK(a2[i]==db[i]);
  db(10)=1;
  BOOST_CHECK(db.size()==11);
  BOOST_CHECK(db.capacity()>=11);
  BOOST_CHECK(db[10]==1);
  aa.dealloc();
}
#endif

#endif
