#ifndef GRAEHL_SHARED__NARY_TREE_HPP
#define GRAEHL_SHARED__NARY_TREE_HPP

#include <graehl/shared/intrusive_refcount.hpp>
#include <boost/pool/pool.hpp>
#include <vector>

/*

  concept for child link traversal (class members).

  concept for traversal that takes predicate classing as real vs. internal. act like n-ary tree w/ closest real-children (skipping internal links) directly under real-parent in left->right order. copy to concrete n-ary tree?

  CRTP impl of it (concrete storage of child links)

  could be nary w/ optimization for small vectors. e.g. fixed storage for 2 children, auto-vivified vector for more

  strategy for nodes:

  intrusive (or not) refct ptr. lazy cow?
  regular ptr - A) i help you construct (pool object) B) you pass me ptr to constructed
  own/copy


  struct my : nary_tree<my,my *> { int label; }
*/

namespace graehl {

template <class T,class C>
struct nary_tree
{
  typedef T crtp_type;
  typedef nary_tree self_type;
  typedef C child_type;
  typedef std::vector<C> children_type;
  typedef children_type Cs;
  Cs children;

  nary_tree() {}
  nary_tree(nary_tree const& o) : children(o.children) {}
  nary_tree(unsigned n) : children(n) {}
};

//TODO: use pool_traits?

template <class T,class U=boost::default_user_allocator_new_delete,class R=boost::detail::atomic_count>
struct shared_nary_tree
  : public nary_tree<T, boost::intrusive_ptr<T> >
  , private intrusive_refcount<T,U,R>
{
  typedef nary_tree<T, boost::intrusive_ptr<T> > TreeBase;
  shared_nary_tree() {}
  shared_nary_tree(unsigned n) : TreeBase(n) {}
  shared_nary_tree(shared_nary_tree const& o) : TreeBase(o) {} // note: refcount doesn't get copied - so T needs to deep copy its data

};

}//ns

#endif
