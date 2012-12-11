// top-down matching of a tree-trie of patterns(tree prefixes) against a subtree.  see jonathan@graehl.org for explanation.
#ifndef TREETRIE_HPP
#define TREETRIE_HPP

#include <graehl/shared/symbol.hpp>
#include <graehl/shared/tree.hpp>
#include <graehl/shared/indexgraph.hpp>
#include <graehl/shared/container.hpp>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif


// we're going to fix the label type as Symbol for now, and use only index_graph<Symbol,HashS>, not the BSearchS or MapS
// we use Symbol::phony_int() to handle where and rank nodes.  what is a tree trie?  see NOTES.

namespace graehl {

template <class C>
bool is_leaf_any_rank(const C& c) {
  return c.any_rank(); //TODO: for transducer
}

template <class C>
rank_type get_leaf_rank(const C& c) {
  return c.rank(); //TODO
}

template <class C>
Symbol get_symbol(const C& c) {
  return is_any_symbol(c) ? Symbol::ZERO : c.symbol();
}


/*
template <class C>
bool is_any_symbol(const C& c) {
    return c.any_symbol(); //TODO - transducer
}

bool is_any_symbol(const Symbol &s)
{
    return false;
}


template <class C>
unsigned &get_index(const C& c) {
  return c.phony_int();
}
*/

/// pass by ref(array_mapper) to functors
template <class V>
struct array_mapper : public dynamic_array<V> {
  typedef V mapped_type;
  typedef unsigned key_type;
  void operator()(key_type key,const mapped_type &val) {
    (*this)(key)=val;
  }
};


/// WHAT THIS CAN DO FOR YOU: interpret and index any types of objects (through above global fns) interpreted as base trees with possibly variable leaves - can then take a concrete Tree<Symbol> and find all the matching bases.  you'll get a contiguous integer vertex_descriptor (type V) which you can use to map any associated data (e.g. the pointer to the originally inserted object).
template <class MatchF> // MatchF is the functor for visiting trie vertices during matching
struct treetrie {
  enum make_not_anon_25 {TYPICAL_TREE_NODES=64};
  typedef Symbol key_type; // some explicit refs to Symbol below, needs work to genericize (get_symbol<key_type>?)
  typedef index_graph<key_type> Index;
  typedef Index::vertex_descriptor V;
  Index index;
  //  dynamic_array<List<void *> matches;
  typedef shared_tree<key_type> MTree;
  dynamic_array<MTree *> where_subtrees; // where (see NOTES) -> subtree with that where
  treetrie() {}

  unsigned nextwhere;

  /// creates a path through trie for pattern t, returning final state index
  /// note get_symbol returns Symbol::ZERO if the symbol is a wildcard
  /// (currently only supported for leaves in lhs, but works here generally
  /// F: (where,Tree *) - should be like F[where] = Tree *
  /// boost::reference<array_mapper<Tree<L> *> > works fine as an F
  //// 1/27/2006: can't remember WHY anyone inserting would want to play with an F callback - not used for anything internally, just exposes impl. detail - commented out with /* */
  template <class L/*,class F*/>
  V insert(shared_tree<L> *t/*, F f*/) {
    nextwhere=0; // root=0, children start at 1
    // visit: root label
    V node=index.force(index.begin(),get_symbol(t->label));
    return insert_expand(t,node/*,f*/);
  }
  ///append the expansion of _t_ (rank,children,recursive expansions ..>) to the
  ///trie ending at _node_. if _rightmost_, done (end of trie-string) when hit a
  ///leaf.  if _terminal_leaves_, any leaf in the rule that isn't a variable
  ///node must have rank 0
  template <class L/*,class F*/>
  V insert_expand(shared_tree<L> *t,V node/*,F f*/) {
    unsigned where=nextwhere++;
/*    deref(f)(where,t); */
    unsigned rank=t->rank;
    L &lab=t->label;
    typedef typename shared_tree<L>::iterator child_it;
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
          subtree_end_state=insert_expand(*i,node/*,f*/);
      }
      return subtree_end_state;
    } else { // leaf:
      if (!is_leaf_any_rank(lab)) {
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
    /// 1/27/2006 - made f a reference ALWAYS (sorry stateless/temporary functors)
  void match(MTree *t, MatchF f) {
    V *np=index.find(index.begin(),t->label);
    if (np) {
      where_subtrees.clear_nodestroy(); // pointers don't need destruction
      where_subtrees.push_back(t);
      match_functor=f;
      match_expand(0,*np);
    }
  }

      //// 1/27/2006: although I do know why on matching you might want the matched subtrees immediately, you can just tell your visitor about where_subtrees (consider it part of the public interface), simplifying the interface
  void visit_match(V node) {
      deref(match_functor)(node/*,where_subtrees.begin()*/);
  }
    template <class F>
    void match_expand(unsigned where,V node,F match_functor) {
    visit_match(node);
    V *np;
    MTree *t=where_subtrees[where];
    unsigned rank=t->rank;
    //MTree::Label &lab=t->label; // FIXME: why unused?
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
    DONE:
      assert(0); //TODO
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

}

#ifdef GRAEHL_TEST
#endif

#endif
