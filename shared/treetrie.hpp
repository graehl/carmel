#ifndef TREETRIE_HPP
#define TREETRIE_HPP

#include "symbol.hpp"
#include "tree.hpp"
#include "indexgraph.hpp"
#include "container.hpp"

// we're going to fix the label type as Symbol for now, and use only index_graph<Symbol,HashS>, not the BSearchS or MapS
// we use Symbol::phony_int() to handle where and rank nodes.  what is a tree trie?  see NOTES.

template <class C>
bool is_any_rank(const C& c) {
  return c.any_rank();
}

template <class C>
bool is_any_symbol(const C& c) {
  return c.any_symbol();
}

template <class C>
rank_type get_leaf_rank(const C& c) {
  return c.rank();
}

template <class C>
Symbol get_symbol(const C& c) {
  return is_any_symbol(c) ? Symbol::ZERO : c.symbol();
}

template <class C>
unsigned &get_index(const C& c) {
  return c.phony_int();
}

/// pass by ref(array_mapper) to functors
template <class V>
struct array_mapper : public DynamicArray<V> {
  typedef V mapped_type;
  typedef unsigned key_type;
  void operator()(key_type key,const mapped_type &val) {
    (*this)(key)=val;
  }
};

template <class MatchF> // MatchF is the functor for visiting trie vertices during matching
struct tree_trie {
  enum {TYPICAL_TREE_NODES=64};
  typedef Symbol key_type; // some explicit refs to Symbol below, needs work to genericize (get_symbol<key_type>?)
  typedef index_graph<key_type> Index;
  typedef Index::vertex_descriptor V;
  Index index;
  //  DynamicArray<List<void *> matches;
  typedef Tree<key_type> MTree;
  DynamicArray<MTree *> where_subtrees; // where (see NOTES) -> subtree with that where
  tree_trie() {}

  unsigned nextwhere;

  /// creates a path through trie for pattern t, returning final state index
  /// note get_symbol returns Symbol::ZERO if the symbol is a wildcard
  /// (currently only supported for leaves in lhs, but works here generally
  /// F: (where,Tree *) - should be like F[where] = Tree *
  /// boost::reference<array_mapper<Tree<L> *> > works fine as an F
  template <class L,class F>
  V insert(Tree<L> *t, F f) {
    nextwhere=0; // root=0, children start at 1
    // visit: root label
    V node=index.force(index.begin(),get_symbol(t->label));
    return insert_expand(t,node,f);
  }
  ///append the expansion of _t_ (rank,children,recursive expansions ..>) to the
  ///trie ending at _node_. if _rightmost_, done (end of trie-string) when hit a
  ///leaf.  if _terminal_leaves_, any leaf in the rule that isn't a variable
  ///node must have rank 0
  template <class L,class F>
  V insert_expand(Tree<L> *t,V node, F f) {
    unsigned where=nextwhere++;
    deref(f)(where,t);
    unsigned rank=t->rank;
    L &lab=t->label;
    typedef typename Tree<L>::iterator child_it;
    if (rank) { // rule internal node
      // add (where,rank) to expand t
      node=index.force(node,Symbol(where,Symbol::PHONYINT));
      node=index.force(node,Symbol(rank,Symbol::PHONYINT));
      nextwhere+=rank; // number children before recurisvely expanding
      // list off children:
      for(child_it i=t->begin(),e=t->end();i!=e;++i) {
        node=index.force(node,get_symbol(lab));
      }
      // recursively expand children:
      V subtree_end_state;
      for(child_it i=t->begin(),e=t->end();i!=e;++i) {
          subtree_end_state=insert_expand(*i,node,f);
      }
      return subtree_end_state;
    } else { // leaf:
      if (!is_any_rank(lab)) {
        node=index.force(node,Symbol(where,Symbol::PHONYINT));
        node=index.force(node,Symbol(get_leaf_rank(lab),Symbol::PHONYINT));
      }
      return node;
    }
  }

  MatchF match_functor;

  ///FIXME: haven't implemented any_symbol lookups

  // visits match_functor(node,where_subtrees.begin()) at every trie node.  f
  // should keep track of lists of matching rules at each node
  void match(MTree *t, MatchF f) {
    V *np=index.find(index.begin(),t->label);
    if (np) {
      where_subtrees.clear_nodestroy(); // pointers don't need destruction
      where_subtrees.push_back(t);
      match_functor=f;
      match_expand(0,*np);
    }
  }

  void visit_match(V node) {
    deref(match_functor)(node,where_subtrees.begin());
  }
  void match_expand(unsigned where,V node) {
    visit_match(node);
    V *np;
    MTree *t=where_subtrees[where];
    unsigned rank=t->rank();
    MTree::Label &lab=t->label;
    typedef MTree::iterator child_it;
    child_it beg=t->begin(),e=t->end();
    // expand @where
    if ( np=index.find(node,Symbol(rank,Symbol::PHONYINT)) ) {
      visit_match(*np); // can omit this if no ^rank leaves specified
      //FIXME: any_symbol rules not checked

      // scan children:
      for(child_it i=beg;i!=e;++i) {
        MTree *c=*i;
        if (np=index.find(*np,c->label)) // could also visit_match here if there
                                         // were AB* type children (don't care
                                         // about children to right)
          where_subtrees.push_back(c);
        else
          goto DONE;
      }

      // recursively choose to expand something to the right of us, based on what's in the trie:
      // don't do this: // np=index.find(*np,Symbol(where,Symbol::PHONYINT))
      index.enumerate(*np,ref(*this));

    }
  }
  void operator()(const std::pair<Symbol,V> &where_to) {
    unsigned where=where_to.first.phony_int();
    //    MTree *t=where_subtrees[where];
    unsigned old_size=where_subtrees.size(); // keeps the where_subtrees in sync with the where when rule was leftmost inserted
    match_expand(where,where_to.second);
    where_subtrees.reduce_size_nodestroy(old_size); // ... by restoring the size of array in sync with stack vars
  }
};


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST
#endif

#endif
