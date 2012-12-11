#ifndef GRAEHL_SHARED__NARY_TREE_HPP
#define GRAEHL_SHARED__NARY_TREE_HPP

// you may override this with a fully namespace qualified type - but be careful to do so consistently before every inclusion!
#ifndef CHILD_INDEX_TYPE
# define CHILD_INDEX_TYPE unsigned
#endif

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

typedef CHILD_INDEX_TYPE child_index;

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
  nary_tree(child_index n) : children(n) {}
};

//TODO: use pool_traits?

template <class T,class R=atomic_count,class U=alloc_new_delete>
struct shared_nary_tree
  : nary_tree<T, boost::intrusive_ptr<T> >
  , /* private */ intrusive_refcount<T,R,U> // if private need to friend intrusive_refcount
{
  friend struct intrusive_refcount<T,R,U>;
  typedef nary_tree<T, boost::intrusive_ptr<T> > TreeBase;
  typedef shared_nary_tree self_type;
  friend void intrusive_ptr_add_ref(self_type *p) { p->add_ref();  }
  friend void intrusive_ptr_release(self_type *p) { p->release(p); }

  shared_nary_tree() {}
  shared_nary_tree(child_index n) : TreeBase(n) {}
  shared_nary_tree(shared_nary_tree const& o) : TreeBase(o) {} // note: refcount doesn't get copied - so T needs to deep copy its data

};

}//ns

#endif
