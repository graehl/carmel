#include <iostream>
#include "../carmel/src/genio.h"
#include <vector>

// Tree owns its own child pointers list but nothing else - routines for creating trees through new and recurisvely deleting are provided outside the class.  Reading from a stream does create children using new.
template <class Label, class Alloc=std::allocator<void *> > struct Tree {
  typedef Tree<Label, Alloc> Self;
  typedef Label label_type;
  Alloc alloc;
  Label label;
  unsigned int n_child;
  Self **children;
  Tree() : n_child(0) {}
  Tree (Label &l) : n_child(0) { label=l; }
  void set_size(unsigned int _n_child) {
	n_child=_n_child;
	if (n_child)
	  children = alloc.allocate(n_child);
  }
  explicit Tree(unsigned int _n_child,Alloc _alloc=Alloc()) : alloc(_alloc) {
	set_size(_n_child);
  }
  ~Tree() {
	if (n_child)
	  alloc.deallocate(children,n_child);
  }

  // STL container stuff
  typedef Self *value_type;
  typedef value_type *iterator;

  typedef const Self *const_value_type;
  typedef const const_value_type *const_iterator;
  iterator begin() {
	return children;
  }
  iterator end() {
	return children+n_child;
  }
  const_iterator const_begin() const {
	return children;
  }
  const_iterator const_end() const {
	return children+n_child;
  }

template <class charT, class Traits>
std::ios_base::iostate print_on(std::basic_ostream<charT,Traits>& os) const
  {
  o << label;
  if (n_child) {
	o << '(';
	bool first=true;
	for (vector<T *>::const_iterator i=in_children.const_begin(),end=in_children.const_end();i!=end;++i) {
	  if (first)
		first = false;
	  else
		o << ' ';
	  o << *i;
	}
	o << ')';
  }
  return std::ios_base::goodbit;
}


template <class charT, class Traits>
std::ios_base::iostate get_from(std::basic_istream<charT,Traits>& in)
// doesn't free old children if any
{
  char c;
  n_child=0;
  L l;
  in >> l;
  if (!in.good()) return std::ios_base::badbit;
  typedef Tree<L,A> T;
  vector<T *> in_children;
  if ((c = in.get()) == '(') {
	while ((c = in.get()) != ')') {
	  in.putback(c);
	  T *in_child = read_tree(in);
	  if (in_child)
		in_children.push_back(in_child);
	  else {
		for (vector<T *>::iterator i=in_children.begin(),end=in_children.end();i!=end;++i)
		  delete_tree(*i);
		return std::ios_base::badbit;
	  }
	  if ((c = in.get()) != ',') in.putback(c);
	}
	set_size(in_children.size());
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
  c=NULL;
}

template <class T>
bool tree_equal(const T &a,const T& b) {
  if (a.n_child != b.n_child || a.label != b.label) return false;
  for (T::const_iterator a_i=a.const_begin(), a_end=a.const_end(), b_i=b.const_begin(); a_i!=a_end; ++a_i,++b_i)
	if (!tree_equal(*a_i,*b_i)) return false;
}

template <class L,class A> 
inline bool operator ==(const Tree<L,A> &a,const Tree<L,A> &b)
{
  return tree_equal(a,b);
}

template <class T,class F>
void postorder(T *tree,F func)
{
  for (T::iterator i=tree.begin(), end=tree.end(); i!=end; ++i)
	postorder(*i,func);
  func(tree);
}

template <class T>
void delete_tree(T *tree)
{
  postorder(tree,delete_arg<T *>);
}

template <class L>
Tree<L> *new_tree(L &l) {
  return new Tree<L> (l);
}

template <class L>
Tree<L> *new_tree(L &l, Tree<L> *c1) {
  Tree<L> *ret = new T(1);
  ret[0]=c1;
  return ret;
}

template <class L>
Tree<L> *new_tree(L &l, Tree<L> *c1, Tree<L> *c2) {
  Tree<L> *ret = new T(2);
  ret[0]=c1;
  ret[1]=c2;
  return ret;
}

template <class L>
Tree<L> *new_tree(L &l, Tree<L> *c1, Tree<L> *c2, Tree<L> *c3) {
  Tree<L> *ret = new T(3);
  ret[0]=c1;
  ret[1]=c2;
  ret[2]=c3;
  return ret;
}

template <class charT, class Traits,class T>
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
