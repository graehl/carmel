// Copyright 2014 Jonathan Graehl-http://graehl.org/
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/** \file

    tree template. n-ary meaning variable n as opposed to e.g. strictly
    binary. allows sharing via intrusive reference counting (useful for
    lazy_forest_kbest.hpp)
*/


#ifndef GRAEHL_SHARED__NARY_TREE_HPP
#define GRAEHL_SHARED__NARY_TREE_HPP
#pragma once

// you may override this with a fully namespace qualified type - but be careful
// to do so consistently before every inclusion!

#ifndef CHILD_INDEX_TYPE
#define CHILD_INDEX_TYPE unsigned
#endif

#include <graehl/shared/intrusive_refcount.hpp>
#include <graehl/shared/stable_vector.hpp>
#include <vector>

/*

  concept for child link traversal (class members).

  concept for traversal that takes predicate classing as real vs. internal. act like n-ary tree w/ closest
  real-children (skipping internal links) directly under real-parent in left->right order. copy to concrete
  n-ary tree?

  CRTP impl of it (concrete storage of child links)

  could be nary w/ optimization for small vectors. e.g. fixed storage for 2 children, auto-vivified vector for
  more

  strategy for nodes:

  intrusive (or not) refct ptr. lazy cow?
  regular ptr - A) i help you construct (pool object) B) you pass me ptr to constructed
  own/copy


  struct my : nary_tree<my, my *> { int label; }
*/

namespace graehl {

typedef CHILD_INDEX_TYPE child_index;
template <class T, class C>
// cppcheck-suppress mallocOnClassError
struct nary_tree {
  typedef T crtp_type;
  typedef nary_tree self_type;
  typedef C child_type;
  typedef std::vector<C> children_type;
  typedef children_type Cs;
  Cs children;
  nary_tree() {}
  nary_tree(nary_tree const& o)
      : children(o.children) {}
  nary_tree(child_index n)
      : children(n) {}
  nary_tree(child_index n, child_type const& child)
      : children(n, child) {}
};


/// call on vector of shared_ptr or intrusive_ptr of nary_tree nodes to avoid stack recursion
template <class TreePtrs>
void freeTreePtrs(TreePtrs& agenda) {
  while (!agenda.empty()) {
    typename TreePtrs::value_type parent{std::move(agenda.back())};
    agenda.pop_back();
    if (parent)
      for (auto& child : parent->children)
        agenda.push_back(std::move(child));
  }
}

template <class TreePtr>
void freeTreePtr(TreePtr& tree) {
  std::vector<TreePtr> agenda;
  agenda.push_back(std::move(tree));
  freeTreePtr(agenda);
}


template <class T, class R = atomic_count, class U = alloc_new_delete>
struct shared_nary_tree : nary_tree<T, boost::intrusive_ptr<T>>, intrusive_refcount<T, R, U> {
  typedef intrusive_refcount<T, R, U> RefCount;
  typedef boost::intrusive_ptr<T> Ptr;
  friend struct intrusive_refcount<T, R, U>;
  typedef nary_tree<T, Ptr> TreeBase;
  typedef shared_nary_tree self_type;

  shared_nary_tree() {}
  shared_nary_tree(child_index n)
      : TreeBase(n) {}
  shared_nary_tree(child_index n, typename TreeBase::child_type const& child)
      : TreeBase(n, child) {}
  shared_nary_tree(shared_nary_tree const& o)
      : TreeBase(o) {} // note: refcount doesn't get copied - so T needs to deep copy its data


  /// call this first to prevent destructor of the root of a 180,000 deep tree
  /// overflowing even a large linux stack
  void clearDeep() {
    std::vector<Ptr> agenda(std::move(this->children));
    freeTreePtrs(agenda);
  }

  /// alternative clearDeep impl; not sure which is faster
  void clearDeepChildren() {
    typedef typename TreeBase::children_type Children;
    stable_vector<Children> agenda; // or std::vector
    agenda.push_back(std::move(this->children));
    while (!agenda.empty()) {
      Children children{std::move(agenda.back())};
      agenda.pop_back(false);
      for (auto const& child : children)
        if (child)
          agenda.push_back(std::move(child->children));
    }
  }
};

} // namespace graehl

#endif
