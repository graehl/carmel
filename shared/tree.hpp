#ifndef _TREE_HPP
#define _TREE_HPP

#include <iostream>
#include "../carmel/src/genio.h"
#include <vector>
#include <algorithm>
#include <boost/lambda/lambda.hpp>

#ifdef TEST
#include "test.hpp"
#include <string>
#endif

//using namespace boost::lambda;
namespace lambda=boost::lambda;
using namespace std;

// Tree owns its own child pointers list but nothing else - routines for creating trees through new and recurisvely deleting are provided outside the class.  Reading from a stream does create children using new.
template <class Label, class Alloc=std::allocator<void *> > struct Tree {
  typedef Tree<Label, Alloc> Self;
  typedef Label label_type;
  Alloc alloc;
  Label label;
  size_t n_child;
  Self **children;
  Tree() : n_child(0) {}
  Tree (const Label &l) : n_child(0),label(l) {  }
  Tree (const Label &l,size_t n) : label(l) { allocate(n); }
  void allocate(size_t _n_child) {
	n_child=_n_child;
	if (n_child)
	  children = (Self **)alloc.allocate(n_child);
  }
  explicit Tree(size_t _n_child,Alloc _alloc=Alloc()) : alloc(_alloc) {
	allocate(_n_child);
  }
  void free() {
	if (n_child)
	  alloc.deallocate((void **)children,n_child);
	n_child=0;
  }
  void free_recursive(); 
  
  ~Tree() {
	free();
  }
  // STL container stuff
  typedef Self *value_type;
  typedef value_type *iterator;

  typedef const Self *const_value_type;
  typedef const const_value_type *const_iterator;

  value_type & operator [](size_t i) { return children[i]; }

  iterator begin() {
	return children;
  }
  iterator end() {
	return children+n_child;
  }
  const_iterator begin() const {
	return children;
  }
  const_iterator end() const {
	return children+n_child;
  }
  template <class T>
friend size_t tree_count(const T *t);

	size_t count_nodes() const
	{
	  return tree_count(this);
	}
template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& o) const
  {	
  o << label;
  if (n_child) {
	o << '(';
	bool first=true;
	for (const_iterator i=begin(),e=end();i!=e;++i) {
	  if (first)
		first = false;
	  else
		o << ' ';
	  o << **i;
	}
	o << ')';
  }
  return std::ios_base::goodbit;
}

template <class charT, class Traits, class T>
  friend T *read_tree(std::basic_istream<charT,Traits>&);
template <class T>
friend void delete_tree(T *);

template <class charT, class Traits>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& in)
// doesn't free old children if any
{  
  char c;
  n_child=0;
  in >> label;
  if (!in.good()) return std::ios_base::badbit;
  
  vector<Self *> in_children;
  if ((c = in.get()) == '(') {
	while ((c = in.get()) != ')') {
	  in.putback(c);
	  Self *in_child = read_tree<Self>(in);
	  if (in_child)
		in_children.push_back(in_child);
	  else {
		for (vector<Self *>::iterator i=in_children.begin(),end=in_children.end();i!=end;++i)
		  delete_tree(*i);
		return std::ios_base::badbit;
	  }
	  if ((c = in.get()) != ',') in.putback(c);
	}
	free();
	allocate(in_children.size());
	copy(in_children.begin(),in_children.end(),begin());	
  } else {	
	in.putback(c);
  }
  return std::ios_base::goodbit;
}
};


template <class charT, class Traits,class L,class A>
std::basic_istream<charT,Traits>&
operator >>
 (std::basic_istream<charT,Traits>& is, Tree<L,A> &arg)
{
	return gen_extractor(is,arg);
}

template <class charT, class Traits,class L,class A>
std::basic_ostream<charT,Traits>&
operator <<
 (std::basic_ostream<charT,Traits>& os, const Tree<L,A> &arg)
{
	return gen_inserter(os,arg);
}


template <class C>
void delete_arg(C &c) {
  delete c;
#ifdef DEBUG
  c=NULL;
#endif
}

template <class T,class F>
void postorder(T *tree,F func)
{
  for (T::iterator i=tree->begin(), end=tree->end(); i!=end; ++i)
	postorder(*i,func);
  func(tree);
}

template <class T,class F>
void postorder(const T *tree,F &func)
{
  for (T::const_iterator i=tree->begin(), end=tree->end(); i!=end; ++i)
	postorder(*i,func);
  func(tree);
}


template <class T>
void delete_tree(T *tree)
{
  postorder(tree,delete_arg<T *>);
}

template <class L,class A> 
void Tree<L,A>::free_recursive() {
	for(iterator i=begin(),e=end();i!=e;++i)
	  delete_tree(*i);
	//foreach(begin(),end(),delete_tree<Self>);
	free();
  }

template <class T>
bool tree_equal(const T& a,const T& b)
{
  if (a.n_child != b.n_child || a.label != b.label) 
	return false;
  for (T::const_iterator a_i=a.begin(), a_end=a.end(), b_i=b.begin(); a_i!=a_end; ++a_i,++b_i)
	if (!(tree_equal(**a_i,**b_i)))
	  return false;
  return true;
}

template <class L,class A> 
inline bool operator !=(const Tree<L,A> &a,const Tree<L,A> &b)
{
  return !(a==b);
}

template <class L,class A> 
inline bool operator ==(const Tree<L,A> &a,const Tree<L,A> &b)
{
  return tree_equal(a,b);
}



template <class T>
struct TreeCount {
  size_t count;
  void operator()(const T * tree) { 
	++count;
  }
  TreeCount() : count(0) {}
};

template <class T>
size_t tree_count(const T *t)
{
  TreeCount<T> n;
  postorder(t,n);
  return n.count;
}

template <class T,class O>
struct Emitter {
  O out;
  Emitter(O o) : out(o) {}
  void operator()(T * tree) {
	o << tree;
  }
};



template <class T,class O>
void emit_postorder(const T *t,O out)
{
  /*Emitter e(out);
  postorder(t,e);*/
  postorder(t,o << lambda::_1);
}


template <class L>
Tree<L> *new_tree(const L &l) {
  return new Tree<L> (l);
}

template <class L>
Tree<L> *new_tree(const L &l, Tree<L> *c1) {
  Tree<L> *ret = new Tree<L>(l,1);
  (*ret)[0]=c1;
  return ret;
}

template <class L>
Tree<L> *new_tree(const L &l, Tree<L> *c1, Tree<L> *c2) {
  Tree<L> *ret = new Tree<L>(l,2);
  (*ret)[0]=c1;
  (*ret)[1]=c2;
  return ret;
}

template <class L>
Tree<L> *new_tree(const L &l, Tree<L> *c1, Tree<L> *c2, Tree<L> *c3) {
  Tree<L> *ret = new Tree<L>(l,3);
  (*ret)[0]=c1;
  (*ret)[1]=c2;
  (*ret)[2]=c3;
  return ret;
}

template <class T,class charT, class Traits>
T *read_tree(std::basic_istream<charT,Traits>& in)
{
  T *ret = new T;
  in >> *ret;
  if (!in.good()) {
	delete_tree(ret);
	return NULL;
  }
  return ret;
}

#ifdef TEST


BOOST_AUTO_UNIT_TEST( tree )
{
  Tree<int> a,b,*c;
  string sa="1(2,3(4,5,6))";
  string sb="1(2 3(4 5 6))";
  stringstream o;
  istringstream(sa) >> a;
  o << a;
  BOOST_CHECK(o.str() == sb);
  o >> b;
  c=new_tree(1,
		new_tree(2),
		new_tree(3,
		  new_tree(4),new_tree(5),new_tree(6)));
  BOOST_CHECK(a == a);
  BOOST_CHECK(a == b);
  BOOST_CHECK(a == *c);
  BOOST_CHECK(a.count_nodes()==6);
  a.free_recursive();
  b.free_recursive();
  delete_tree(c);
}


#endif

#endif