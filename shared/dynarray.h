#ifndef DYNARRAY_H 
#define DYNARRAY_H 1
// For MEMCPY-able-to-move types only!
#include "config.h"

#include <string>
#include <new>
#include "myassert.h"
#include <memory>
#include <stdexcept>
#include <iostream>
#include "genio.h"

// if you want custom actions/parsing while reading labels, make a functor with this signature and pass it as an argument to read_tree (or get_from):
template <class Label>
struct DefaultReader
{
  typedef Label value_type;
  template <class charT, class Traits>
	std::basic_istream<charT,Traits>&
	 operator()(std::basic_istream<charT,Traits>& in,Label &l) const {
	  return in >> l;
	 }
};


  template <class charT, class Traits, class T>
std::ios_base::iostate range_print_on(std::basic_ostream<charT,Traits>& o,T begin, T end,bool multiline=false) 
  {	
	o << '(';
	if (multiline) {
#define LONGSEP "\n "
	 for (;begin!=end;++begin)
	  o << LONGSEP << *begin;
	 o << "\n)";

	 o << std::endl;
	} else {
	  bool first=true;
	  for (;begin!=end;++begin) {	  
		if (first)
		  first = false;
		else
			o << ' ';
  		o << *begin;
	  }
	 o << ')';
	}
  return std::ios_base::goodbit;
}

  // modifies out iterator.  if returns GENIOBAD then elements might be left partially extracted.  (clear them yourself if you want)
template <class charT, class Traits, class Reader, class T>
std::ios_base::iostate range_get_from(std::basic_istream<charT,Traits>& in,T &out,Reader read)

{  
  char c;
  
  EXPECTCH_SPACE('(');
  for(;;) {
    EXPECTI(in>>c);
	  if (c==')') {	
		break;
	  }
	  in.putback(c);
#if 1
	  typename Reader::value_type temp;
	  if (read(in,temp).good())
		*out++=temp;
	  else
		goto fail;
#else
	  // doesn't work for back inserter for some reason
	  if (!read(in,*&(*out++)).good()) {
		goto fail;
	  }
#endif
	  EXPECTI(in>>c);
	  if (c != ',') in.putback(c);
  }
  return GENIOGOOD;
fail:
  return GENIOBAD;
}


// doesn't manage its own space (use alloc(n) and dealloc() yourself).  0 length allowed.
// you must construct and deconstruct elements yourself.  raw dynamic uninitialized (classed) storage array
template <typename T,typename Alloc=std::allocator<T> > class Array : protected Alloc {
protected:
  //unsigned int space;
  T *vec;
  T *endspace;
public:
  typedef T value_type;
  typedef value_type *iterator;
  typedef const value_type *const_iterator;

  Array(const char *c) {
	std::istringstream(c) >> *this;
  }
	T& front() {
	  Assert(size());
	  return *begin();
	}
	T& back() {
	  Assert(size());
	  return *(end()-1);
	}

  template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& o,bool multiline=false) const
  {	
#if 0
	o << '(';
	if (multiline) {
#define LONGSEP "\n "
	 for (const_iterator i=begin(),e=end();i!=e;++i)
	  o << LONGSEP << *i;
	 o << "\n)";

	 o << std::endl;
	} else {
	  bool first=true;
	  for (const_iterator i=begin(),e=end();i!=e;++i) {
		if (first)
		  first = false;
		else
			o << ' ';
  		o << *i;
	  }
	 o << ')';
	}
  return std::ios_base::goodbit;
#else
	return range_print_on(o,begin(),end(),multiline);
#endif
}



// Reader passed by value, so can't be stateful (unless itself is a pointer to shared state)
template <class charT, class Traits, class Reader> friend
get_from_imp(Array<T,Alloc> *s,std::basic_istream<charT,Traits>& in,Reader read);

template <class charT, class Traits, class Reader>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& in,Reader read) {
  return get_from_imp(this,in,read);
}

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
  void copyto(T *to,unsigned n) {
	memcpy(to,vec,sizeof(T)*n);
  }

  void copyto(T *to) {
	copyto(to,capacity());
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
  explicit Array(T* buf,unsigned sz) : vec(buf), endspace(buf+sz) {}
  explicit Array(unsigned sp=4) { alloc_fresh(sp); }
  Array(unsigned sp, const Alloc &a): Alloc(a) { alloc_fresh(sp); }
  unsigned capacity() const { return (unsigned)(endspace-vec); }
  unsigned size() const { return capacity(); }
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
  DynamicArray (const char *c) {
	std::istringstream(c) >> *this;
  }

  DynamicArray(unsigned sp = 4) : Array<T,Alloc>(sp), endvec(vec) { }

  DynamicArray(const DynamicArray &a) {
	unsigned sz=a.size();
    alloc_fresh(sz);
	memcpy(vec,a.vec,sizeof(T)*sz);
	endvec=endspace;
  }
  
  void copyto(T *to,unsigned n) {
	memcpy(to,vec,sizeof(T)*n);
  }
  void copyto(T *to) {
	copyto(to,size());
  }


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
      PLACEMENT_NEW(push_back_raw()) T();
    }    
  void push_back(const T& val)
    {
      PLACEMENT_NEW(push_back_raw()) T(val);
    }
  // non-construct version (use PLACEMENT_NEW yourself) (not in STL vector either)
  T *push_back_raw()
    {
      if ( endvec >= endspace )
		resize(capacity() << 1);
      return endvec++;
    }
	void undo_push_back_raw() {
	  --endvec;
	}
	T& front()  {
	  Assert(size());
	  return *begin();
	}
	T& back() {
	  Assert(size());
	  return *(end()-1);
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
  void compact() {
	//equivalent to resize(size());
	unsigned newSpace=size();
    T *newVec = allocate(newSpace); // can throw but we've made no changes yet
    memcpy(newVec, vec, newSpace*sizeof(T));
	deallocate(vec,capacity()); // can't throw
	set_begin(newVec);
	//set_capacity(newSpace);set_size(sz);
	endspace=endvec=vec+newSpace;
  }
  void compact(Array<T,Alloc> *into) {
	unsigned sz=size();
    into->alloc_fresh(sz);
	copyto(into->begin());	
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
template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& o,bool multiline=false) const
  {	
#if 0
	o << '(';
	bool first=true;
	for (const_iterator i=begin(),e=end();i!=e;++i) {
	  if (first)
		first = false;
	  else
		o << ' ';
	  o << *i;
	}
	o << ')';
  
  return std::ios_base::goodbit;
#else
   return range_print_on(o,begin(),end(),multiline);
#endif
}

  // Reader passed by value, so can't be stateful (unless itself is a pointer to shared state)
template <class charT, class Traits, class Reader>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& in,Reader read)

{  
#if 1
  // slight optimization from not needing temporary like general output iterator version.
  char c;
  
  EXPECTCH_SPACE('(');
  clear();
  for(;;) {
    EXPECTI(in>>c);
	  if (c==')') {	
		break;
	  }
	  in.putback(c);
	  push_back_raw();
	  if (!read(in,back()).good()) {
		undo_push_back_raw();
		goto fail;
	  }
	  EXPECTI(in>>c);
	  if (c != ',') in.putback(c);
  }
  return GENIOGOOD;
fail:
  clear();
  return GENIOBAD;
#else
  clear();
  std::ios_base::iostate ret=range_get_from(in,back_inserter(*this),read);
  if (ret == GENIOBAD)
	clear();
  return ret;
#endif
}

};



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

template <typename T,typename Alloc,class charT, class Traits, class Reader>
//std::ios_base::iostate Array<T,Alloc>::get_from(std::basic_istream<charT,Traits>& in,Reader read)
std::ios_base::iostate get_from_imp(Array<T,Alloc> *a,std::basic_istream<charT,Traits>& in,Reader read)
{  
  DynamicArray<T,Alloc> s;
  std::ios_base::iostate ret=s.get_from(in,read);
  s.compact(a);
  return ret;
}

template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
 (std::basic_istream<charT,Traits>& is, Array<L,A> &arg)
{
	return gen_extractor(is,arg,DefaultReader<L>());
}

template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
 (std::basic_istream<charT,Traits>& is, DynamicArray<L,A> &arg)
{
	return gen_extractor(is,arg,DefaultReader<L>());
}

template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
 (std::basic_ostream<charT,Traits>& os, const Array<L,A> &arg)
{
	return gen_inserter(os,arg);
}


template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
 (std::basic_ostream<charT,Traits>& os, const DynamicArray<L,A> &arg)
{
	return gen_inserter(os,arg);
}

#define ARRAYEQIMP \
  if (l.size() != r.size()) return false; \
  typename L::const_iterator il=l.begin(),iend=l.end(); \
  typename R::const_iterator ir=r.begin(); \
  while (il!=iend) \
	if (*il++ != *ir++) return false; \
  return true;


template<class L,class A,class L2,class A2>
bool operator ==(const DynamicArray<L,A> &l, const DynamicArray<L2,A2> &r)
{
  typedef DynamicArray<L,A> L;
  typedef DynamicArray<L2,A2> R;
ARRAYEQIMP
}


template<class L,class A,class L2,class A2>
bool operator ==(const DynamicArray<L,A> &l, const Array<L2,A2> &r)
{
  typedef DynamicArray<L,A> L;
  typedef Array<L2,A2> R;
ARRAYEQIMP
}

template<class L,class A,class L2,class A2>
bool operator ==(const Array<L,A> &l, const DynamicArray<L2,A2> &r)
{
  typedef Array<L,A> L;
  typedef DynamicArray<L2,A2> R;
ARRAYEQIMP
}


template<class L,class A,class L2,class A2>
bool operator ==(const Array<L,A> &l, const Array<L2,A2> &r)
{
  typedef Array<L,A> L;
  typedef Array<L2,A2> R;
  ARRAYEQIMP
}




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
  BOOST_CHECK(da==aa);
  BOOST_CHECK(db==aa);
  const int sz1=3,sz2=4;;
  da.removeMarked(rm1); // removeMarked
  BOOST_REQUIRE(da.size()==sz1);
  for (int i=0;i<sz1;++i)
    BOOST_CHECK(a1[i]==da[i]);
  db.removeMarked(rm2);
  BOOST_REQUIRE(db.size()==sz2);
  for (int i=0;i<sz2;++i)
	BOOST_CHECK(a2[i]==db[i]);
  Array<int> d1map(sz),d2map(sz);
  BOOST_CHECK(3==new_indices(rm1,rm1+sz,d1map.begin()));
  BOOST_CHECK(4==new_indices(rm2,rm2+sz,d2map.begin()));
  int c=0;
  for (unsigned i=0;i<d1map.size();++i)
	if (d1map[i]==-1)
	  ++c;
	else
	  BOOST_CHECK(da[d1map[i]]==aa[i]);
  BOOST_CHECK(c==4);
  db(10)=1;
  BOOST_CHECK(db.size()==11);
  BOOST_CHECK(db.capacity()>=11);
  BOOST_CHECK(db[10]==1);
  aa.dealloc();

  string sa="( 2 ,3 4\n \n\t 5,6)";
  string sb="(2 3 4 5 6)";

#define EQIOTEST(A,B)  { A<int> a;B<int> b;stringstream o;istringstream(sa) >> a;\
  o << a;BOOST_CHECK(o.str() == sb);o >> b;BOOST_CHECK(a==b);}

  EQIOTEST(Array,Array)
	EQIOTEST(Array,DynamicArray)
	EQIOTEST(DynamicArray,Array)
	EQIOTEST(DynamicArray,DynamicArray)
}
#endif

#endif
