// Deterministic-FSA-like graph.  given a state and a key, what's the next state (if any?) - used by treetrie.hpp.  implemeneted a space-efficient bsearch/sorted-array map but didn't test it yet.
#ifndef INDEXGRAPH_HPP
#define INDEXGRAPH_HPP

#include <graehl/shared/container.hpp>

#include <algorithm>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

// returns NULL if k is not in [l,u), otherwise returns iterator pointing to k
// (assumes [l,u) sorted increasing on <:lt so that lt(l,u))
template <class R,class K,class L>
inline R binary_search(R l, R u, const K &k,L lt) {
  R ret=lower_bound(l,u,k,lt);
  if (ret!=u && !(k < *ret))
    return ret;
  return NULL;
}
template <class R,class K,class L>
inline R binary_search(R l, R u, const K &k) {
  R ret=lower_bound(l,u,k);
  if (ret!=u && !(k < *ret))
    return ret;
  return NULL;
}


// adjacency graph with map-keyed edges on vertices, which are indexed by
// [0,num_vertices())

// - no provision to attach data to or identify edges.  can add later (would
// return edge_descriptor instead from find and force).  data could be attached
// by defining key type appropriately.  would then get at the data by
// verts[v].find(key).first

// more general concept would be a nested map - in this case we hard coded the
// outside/first map to be a DynamicArray, keys [0,...,) ... plus graph
// semantics (value of 2nd nested map = key of 1st)

// didn't add lessthan predicate (using operator<) ... easy to generalize later
template <class K,class MapSelector=HashS>
struct index_graph {
  //  struct vertex;
  typedef unsigned vertex_descriptor; // can't use pointer because we dynamically grow verts and would then have to re-base
  typedef K key_type;
  typedef vertex_descriptor data_type;
  //  typedef VertexData vertex_data_type;
  typedef typename MapSelector::template map<key_type,vertex_descriptor>::type map_type;
  typedef typename map_type::iterator map_it;
  //  typedef typename map_traits<map_type>::find_type find_type;
  typedef typename map_type::value_type value_type;
  //  typedef std::pair<vertex_descriptor,key> bkey;
  typedef dynamic_array<map_type> Vertices;

  bool invariant() {
    return verts.size() > 0; // always have start node=0.
  }
  Vertices verts;
  unsigned num_vertices() const {
    return verts.size();
  }

  vertex_descriptor begin() const {
    return 0;
  }
  vertex_descriptor end() const {
    return num_vertices();
  }

  //  typedef vertices::iterator iterator;
  unsigned num_edges() const {
    unsigned total=0;
    for (typename Vertices::iterator i=verts.begin(),e=verts.end();i!=e;++i)
      total+=i->size();
    return total;
  }
  unsigned num_edges(vertex_descriptor v) const {
    return verts[v].size();
  }
  vertex_descriptor *find(vertex_descriptor from,const key_type & key) const {
    return find_second(verts[from],key);
  }
  vertex_descriptor force(vertex_descriptor from, const key_type &key) { // creates if isn't already there
    vertex_descriptor new_desc=verts.size();
    typename map_traits<map_type>::insert_result_type ret=insert(verts[from],key,new_desc);
    if (ret.second) { // actually inserted
      verts.push_back();
      return new_desc;
    } else
      return ret.first->second;
  }
  template <class F>
  struct binderfrom {
    F f;
    vertex_descriptor from;
    binderfrom(F f_,vertex_descriptor from_) : f(f_),from(from_) {}
    template <class A> // expect pair<const key,val>
    void operator()(const A &v) {
      deref(f)(from,v.first,v.second);
    }
  };
  template <class F>
  void enumerate_edges(vertex_descriptor from,F f) {
    //    for (vertex_descriptor i=begin(),e=end();i!=e;++i)
    graehl::enumerate(verts[from],binderfrom<F>(f,from));
  }
  template <class F>
  void enumerate(vertex_descriptor from,F f) {
    enumerate(verts[from],f);
  }
  template <class F>
  void enumerate_edges(F f) {
    for (vertex_descriptor i=begin(),e=end();i!=e;++i)
      enumerate_edges(i,f);
  }
  index_graph() {verts.push_back();}
};


#ifdef UNTESTED
// const map: nothing may be inserted/deleted (although values may be changed)
template <class K, class V,class Comp=std::less<K> >
struct bsearch_map {
  typedef std::pair<K,V> value_type;
  typedef value_type * iterator;
  typedef const Pair * const_iterator;
  typedef value_type & reference;
  typedef K key_type;
  typedef V data_type;
  typedef Comp key_compare;

  iterator _begin,_end;
  key_compare lt;

  // invariant: [b,e) are pairs of key,data sorted increasing on key by key_compare
  bool invariant() {
    /*
    for (iterator i=begin(),e=end()-1;i<e;++i) {
      if (lt(*(i+1),*i))
        return false;
    }
    return true;
    */
    return is_sorted(begin(),end(),lt);
  }
  bsearch_map(Pair *b,Pair *e) : _begin(b),_end(e) { Assert(invariant()); }
  bsearch_map(Pair *b,Pair *e,Comp c) : _begin(b),_end(e), lt(c_) { Assert(invariant()); }
  static iterator between(iterator a,iterator b) {
    return a+(b-a)/2;
  }
  iterator begin() const {
    return _begin;
  }
  iterator end() const {
    return _end;
  }
  unsigned size() const {
    return end()-begin();
  }
  iterator find(const key_type &k) {
    /*
    unsigned l=0;
    unsigned u=size();
    while ( l < u ) { // invariant: k is either in [l,u) or won't be found
      int mid=(l+u)/2; // l<=mid<u
      iterator m=begin()+i;
      if (lt(k,*m)) l=++mid; // k < mid => k in [mid+1,u)
      else if (lt(*m,k)) u=mid; // k > mid => k in [l,mid)
      else return m;  // k == mid
    }
    return end();

    */
    //    return equal_range(begin(),end(),k,lt).first;
        iterator ret=lower_bound(begin(),end(),k,lt);
    if (ret!=end() && !(k < *ret))
      return ret;
    return end();

    //    return binary_search(begin(),end(),k,lt); // returns NULL, bad
  }
  data_type & operator [](const key_type &k) const {
    iterator *ret=find(k);
    Assert(ret!=end());
    return ret;
  }
};
#endif

#ifdef UNTESTED
struct BSearchS {};

template <class K>
struct index_graph<K,BSearchS> {
  typedef unsigned vertex_descriptor;
  typedef K key_type;
  typedef vertex_descriptor data_type;
  typedef std::pair<key_type,data_type> value_type;
  typedef   auto_array<value_type> Table;
  typedef Table::iterator EntryP;
  typedef   auto_array<EntryP> Bounds;

  Table sorted;
  Bounds bounds; // for source i, [*bounds[i],...,*bounds[i+1]) are sorted on .first (key)

private:
  Table::iterator back; // used in init only
  void operator()(const value_type &v) { // add edge
    Assert(back<sorted.end());
    new(back++)(v);
  }
public:
  unsigned num_edges() {
    return sorted.size();
  }
  unsigned num_vertices() {
    return bounds.size()-1;
  }
  bool invariant() {
    if (bounds.back() != sorted.end() || bounds.front() != sorted.begin())
      return false;
    for (Bounds::iterator i=bounds.begin(),e=bounds.end()-1;i!=e,++i) {
      if (!is_sorted(*i,*(i+1)))
        return false;
    }
    return true;
  }
  template <class M>
  index_graph(const index_graph<K,M> &g) : sorted(g.num_edges()),bounds(g.num_vertices()+1) {
    back=sorted.begin();
    Table::iterator back=sorted.begin();
    for (vertex_descriptor i=g.begin(),e=g.end(); ;++i) {
      bounds[i]==back;
      if (i==e) break;
      g.verts[i].enumerate(ref(*this));
      sort(bounds[i],back);
    }
    Assert(back==sorted.end());
  }
  vertex_descriptor *find(vertex_descriptor from,const key_type & key) const {
    return binary_search(bounds[from],bounds[from+1],key);
  }
};
#endif


#ifdef GRAEHL_TEST

BOOST_AUTO_TEST_CASE( TEST_indexgraph )
{
  index_graph<char> ig;
  BOOST_CHECK(ig.num_vertices()==1);
  BOOST_CHECK(ig.find(0,'0')==NULL);
  BOOST_CHECK(ig.force(0,'1')==1);
  BOOST_CHECK(ig.find(0,'1')!=NULL);
  BOOST_CHECK(*ig.find(0,'1')==1);
  BOOST_CHECK(ig.force(1,'2')==2);
  BOOST_CHECK(ig.force(0,'0')==3);
  BOOST_CHECK(ig.num_vertices()==4);
  {  test_counter c;
  ig.enumerate_edges(c);
  //  DBP(c.n);
  BOOST_CHECK(c.n==3);
  }
  test_counter c;
  ig.enumerate_edges(0,c);
  //  DBP(c.n);
  BOOST_CHECK(c.n==2);

}
#endif
}
#endif
