#ifndef GRAEHL_SHARED__NARY_TREE_HPP
#define GRAEHL_SHARED__NARY_TREE_HPP

#include <graehl/shared/intrusive_refcount.hpp>
#include <boost/pool/pool.hpp>
#include <vector>

/*

  concept for binary child link traversal (class members).

  concept for traversal that takes predicate classing as real vs. internal. act like n-ary tree w/ closest real-children (skipping internal links) directly under real-parent in left->right order. copy to concrete n-ary tree?

  CRTP impl of it (concrete storage of child links)

  could be nary w/ optimization for small vectors. e.g. fixed storage for 2 children, auto-vivified vector for more

  strategy for nodes:

  intrusive (or not) refct ptr. lazy cow?
  regular ptr - A) i help you construct (pool object) B) you pass me ptr to constructed
  own/copy


  struct my : nary_tree<my,
*/

namespace graehl {

template <class T,class C>
struct nary_tree : public T
{
  typedef T crtp_type;
  typedef nary_tree self_type;
  typedef C child_type;
  typedef std::vector<C> children_type;
  typedef children_type Cs;
  Cs children;

  nary_tree() {}
  nary_tree(nary_tree const& o) : children(o.children) {}
};

#if 1
template <class T,class U=boost::default_user_allocator_new_delete,class R=boost::detail::atomic_count>
struct shared_nary_tree
  : public T
  , public nary_tree<shared_nary_tree<T,U,R>, boost::intrusive_ptr<shared_nary_tree<T,U,R> > >
  , private intrusive_refcount<shared_nary_tree<T,U,R>,U,R>
{
};
#else
//complete:
template <class T,class U=boost::default_user_allocator_new_delete,class R=boost::detail::atomic_count> // T=CRTP param
struct shared_nary_tree : public T,private intrusive_refcount<shared_nary_tree<T,U,R>,U,R>
{
  typedef nary_tree self_type;
  typedef intrusive_refcount<nary_tree,U,R> child_type;
  typedef T crtp_type;
  typedef std::vector<pointer_type> children_type;
  typedef children_type C;
  C children;

  nary_tree() {}
  nary_tree(nary_tree const& o) : children(o.children) {}
};
#endif
}//ns

#endif
