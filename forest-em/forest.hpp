#ifndef GRAEHL_TT__FOREST_HPP
#define GRAEHL_TT__FOREST_HPP

#include <fstream>
#include <cstring>
#include <graehl/shared/gibbs.hpp>
#include <graehl/shared/random.hpp>
#include <graehl/shared/os.hpp>
#include <graehl/shared/intorpointer.hpp>
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/genio.h>
#include <graehl/shared/list.h>
#include <graehl/shared/weight.h>
#include <graehl/shared/threadlocal.hpp>
#undef THREADLOCAL
#define THREADLOCAL
//disable THREADLOCAL since we have non-pod (should group them all via a single pointer)
#include <graehl/shared/graphviz.hpp>
#include <graehl/shared/funcs.hpp>
#include <graehl/shared/stackalloc.hpp>
#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif
#define STATIC_HASHER
#define STATIC_HASH_EQUAL
#include <graehl/shared/2hash.h>


///we really want two types of indices; rule, and forest node.  but i decided to use absolute pointers for forest nodes, so LSB=0, and 2n+1 for rule indices.  more natural choice might be positive/negative or MSB=0/1.  due to pointer arithmetic, relative offsets to A* (difference = # of bytes rather than # of As, aka index) would be cumbersome, but doable.  division and multiplication by constants should be collapsed anyhow.

#define MAX_FOREST_DEPTH 100000
// forests can only be this deep.  could easily fix.

/// I/O format: http://twiki.isi.edu/NLP/DerivationTrees
/// note 0 is not a legit rule ID, sorry (actually used for OR)
/*(OR (1 #1(3 4) #2(5 6)) (2 (7 4) ...

  could be represented with less nodes:

  (OR (1 #1(3 #3(4)) #2(5 6)) (2 (7 #3) ...
*/

// WHY NOT: (OR ...) -> [...]?  so [(1 #1(3 4) ...) (...)]


#define OR_INT 0
#define IS_OR_INT(x) ((x)==OR_INT)
#define PRINT_OR_INT(o, i) if (IS_OR_INT(i)) o << "OR"; else o << (i);

namespace graehl {

struct ForestNode {
  typedef IntOrPointer<ForestNode> Label;
  ForestNode *next; // next sibling; terminated by external bound: when next=bound, no more children.
  // or in other words: all my descendants come in memory on (this,this->next)
  Label l;
  bool is_backref() const {
    return l.is_pointer();
  }
  unsigned label() const {
    Assert(!is_backref());
    return l.integer();
  }
  bool is_or() const {
    return IS_OR_INT(label());
  }
  bool is_leaf() const {
    return next==this+1;
  }
  GENIO_print {
    o << "(next=+" << next - this << ',' << l << ')';
    return GENIOGOOD;
  }
};

CREATE_INSERTER(ForestNode);


//#define COUNT_FLOAT_TYPE Float
//#define PROB_FLOAT_TYPE Float
//FIXME: presently must be same type for normalization to work
template <class Float = FLOAT_TYPE>
struct FForest {
  static THREADLOCAL gibbs_base *gibbs;
  static inline gibbs_base &g()
  {
    return *gibbs;
  }
  typedef FForest<Float> Forest;
  typedef logweight<Float> inside_t; // for inside/outside.
  typedef logweight<Float> count_t;
  typedef logweight<Float> prob_t;

  ForestNode *nodes;
  typedef ForestNode *iterator;
  iterator begin() const
  {
    return nodes;
  }
  iterator &end()
  {
    return nodes->next;
  }
  iterator end() const
  {
    return nodes->next;
  }
  // made static so we can open swapbatch in read-only mode (just as well could be member var otherwise)
  static THREADLOCAL inside_t *inside, *norm_outside; //changed "outside" to "norm_outside" denoting that the value is actually outside/inside[0] (so count += inside*norm_outside)
  static THREADLOCAL count_t *counts;
  static THREADLOCAL prob_t *rule_weights;

  static THREADLOCAL size_t max_ruleid; // static global return value
  static THREADLOCAL std::ostream *viterbi_out;
  static THREADLOCAL ForestNode **viterbi;


  /// you own this space:
  size_t size() const { return end()-nodes; }
  //    FRIEND_EXTRACTOR(Forest);
  //    FRIEND_INSERTER(Forest);
  template <class I>
  char *read(I &in, char *beginspace, char *endspace) {
    nodes = (ForestNode *)beginspace;
    end() = (ForestNode *)endspace;

    typedef char charT;
    typedef std::char_traits<charT> Traits;
    //GEN_EXTRACTOR(in,read(in));
    in >> *this;
    if (ran_out_of_space()) {
      in.clear_nodestroy();
      return NULL;
    } else
      return (char*)end();
  }

  void safe_destroy() {
    end() = nodes;
  }

  //template <class T>
  //std::ios_base::iostate read(T& in)

  GENIO_read
  {
    DBP_INC_VERBOSE;
    self_destruct<Forest> suicide(this);
    List<ForestNode **> open_parens;
    //      open_parens.push(&end);
    ForestNode *stop = nodes; // points one past end of nodes
    char c;
    size_t backref_id;
    dynamic_array<ForestNode *> backrefs;

    bool follows_paren = false;
    //            bool opened=false;
    unsigned rule_id;
#ifdef XXXDEBUG
#define GOTOFAIL do { Assert(0); goto fail; } while (0)
#else
#define GOTOFAIL goto fail
#endif
    bool first = true;
    while (stop == nodes || open_parens.notEmpty()) {
      if (stop >= end()) { // fixed by ::read() (external fn) FIXED: dynamically expand (own your storage)
        set_out_of_space();
        DBPC3("out of space", stop, end());
        GOTOFAIL;
      }
      if (first) {
        EXPECTI_FIRST(in>>c);
        first = false;
      } else {
        EXPECTI(in >> c);
      }
      switch(c) {
        case '#':
          //                    Assert(!follows_paren);
          if (follows_paren) {
            GENIO_THROW("Bad # following paren in Forest");
            GOTOFAIL;
          }
          EXPECTI(in>>backref_id);
          EXPECTI(in.get(c));
          if (c == '(') {
            backrefs(backref_id) = stop;
          } else {
            //                        if (backref_id >= backrefs.size()) GOTOFAIL;
            stop->l = backrefs[backref_id];
#define STOPNEXT stop = stop->next = stop+1
            STOPNEXT;
            Assert(backrefs[backref_id] < end() && backrefs[backref_id] >= nodes);
          }
          in.unget();
          break;
        case '(':
          follows_paren = true;
          open_parens.push(&stop->next);
          break;
        case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
          in.unget();
          in >> rule_id;
          if (max_ruleid < rule_id)
            max_ruleid = rule_id;
          stop->l = rule_id;
          if (!follows_paren) {
            STOPNEXT;
          } else {
            follows_paren = false;
            ++stop;
          }
          break;
        case 'O':
          EXPECTCH('R');
          if (!follows_paren) {
            GENIO_THROW("OR not following paren in Forest");
            GOTOFAIL;
          }
          follows_paren = false;
          stop->l = OR_INT;
          ++stop;
          break;
        case ')':
          *(open_parens.top()) = stop;
          open_parens.pop();
          break;
        default:
          GENIO_THROW2("Forest: unexpected char ", boost::lexical_cast<std::string>(c));
          GOTOFAIL;
#undef GOTOFAIL
          break;
      }
    }
    end() = stop;
    DBPC2("Successfully read forest", *this);
    suicide.cancel();
    return GENIOGOOD;
 fail:
    return GENIOBAD;
#undef STOPNEXT
  }
  // note: pass by value (like Boost property_map); use ref(array) or array.begin() (pointer pass by val is ok)
  template <class Map>
  void assign_backref_ids(Map to) const
  {
    unsigned lastid = 0;
    for (ForestNode *p = nodes; p!=end(); ++p)
      if (p->is_backref()) {
        unsigned index = backref(p);;
        if (!to[index])
          to[index]=++lastid;
      }
  }

  const ForestNode &operator [](unsigned i) const {
    return nodes[i];
  }
  ForestNode &operator [](unsigned i) {
    return nodes[i];
  }

  bool ran_out_of_space() const {
    return nodes == 0;
  }
  void set_out_of_space() {
    nodes = 0;
  }
  ForestNode *next_unused_space() const {
    return end();
  }
  bool is_backref(unsigned i) const {
    return nodes[i].is_backref();
  }
  unsigned backref(unsigned i) const {
    return toi(nodes[i].l.pointer());
  }
  unsigned backref(ForestNode *p) const {
    return toi(p->l.pointer());
  }
  unsigned next(unsigned i) const {
    return toi(nodes[i].next);
  }

  bool is_leaf(unsigned i) const {
    return nodes[i].is_leaf();
  }
  GENIO_print {
    return print(o, nodes);
  }

  template <class charT, class Traits> \
  std::ios_base::iostate \
  print(std::basic_ostream<charT, Traits>& o, ForestNode *start_from) const

  {
    fixed_array<unsigned> backref_ids(this->size());
    assign_backref_ids(backref_ids.begin());
    //      dynamic_array<ForestNode *> ends(DEFAULT_DEPTH);
    ForestNode * ends[MAX_FOREST_DEPTH]; //FIXME: duh, make it grow, or use List=stack+depth counter
    ends[0] = this->end();
    unsigned depth = 0;
    bool first = true;
    for (ForestNode *p = start_from; p!=end(); ++p) {
      //
      unsigned id = backref_ids[toi(p)];
      while (p==ends[depth]) {
        Assert(depth);
        o << ')';
        --depth;
      }
      if (first) {
        first = false;
      } else {
        o << ' ';
      }

      if (id) {
        Assert(p->l.is_integer());
        o << '#' << id;
      }
      if (p->is_backref()) {
        unsigned back_id = backref_ids[backref(p)];
        Assert(back_id);
        o << '#' << back_id;
      } else { // integer
        unsigned rule = p->l.integer();
        ForestNode *next = p->next;
        if (next == p+1) { // leaf
          if (id)
            o << '(';
          Assert(!IS_OR_INT(rule));
          o << rule;
          if (id)
            o << ')';
        } else {
          o << '(';
          Assert(depth < MAX_FOREST_DEPTH-1); //FIXME
          ends[++depth] = next; // there is one ')' output for every depth--, and depth starts and ends at 0.  one ++depth for every '(' output => balanced
          PRINT_OR_INT(o, rule);
        }
      }
    }
    while (depth--)
      o << ')';
    return GENIOGOOD;
  }
#undef PRINT_OR_INT
  void reset(ForestNode *_begin, ForestNode *_end) { nodes = _begin; end() = _end; }
  void reset(ForestNode *_begin) { nodes = _begin; }
  FForest() : nodes(0) {  }
  FForest(const Forest &f) : nodes(f.nodes) {}
  explicit FForest(ForestNode *_begin) :nodes(_begin) {  }
  unsigned toi(ForestNode *p) const {
    return p - nodes;
  }
  // property maps (like Weight *)
  inside_t compute_inside(prob_t *_rule_weights, inside_t *_inside) {
    SetLocal<prob_t *> guardp(rule_weights, _rule_weights);
    SetLocal<inside_t *> guard2(inside, _inside);
    return compute_inside();
  }
  inside_t compute_inside() {
    outside_order.clear_nodestroy();
    DBPC4("Prepared to compute inside", toi(nodes), toi(end()), *this);
    DBP_ADD_VERBOSE(20);
    inside_rec(nodes);
    return inside[0];
  }
  struct prepare_inside_outside {
    SetLocal<prob_t *> guardr;
    SetLocal<inside_t *> guardi, guardo;
    SetLocal<count_t *>    guardc;
    SetLocal<ForestNode **> guardv;
    prepare_inside_outside(count_t *_counts, prob_t *_rule_weights, inside_t *_inside, inside_t *_outside, ForestNode **_viterbi) : guardr(rule_weights, _rule_weights), guardi(inside, _inside), guardo(norm_outside, _outside), guardc(counts, _counts), guardv(viterbi, _viterbi)
    {}
  };
  typedef HashTable<unsigned, count_t> count_overflows;
  struct accumulate_counts {
    count_t total_overflow;
    unsigned n_overflows;
    unsigned n_rule_overflows;
    std::ostream &print(std::ostream &os) {
      if (n_overflows)
        os << " (Overflowed: "<<n_rule_overflows<<" rules, "<<n_overflows<<" times, "<<total_overflow<<" total counts"<<")";
      return os;
    }
    void reset_stats() {
      n_overflows = n_rule_overflows = 0;
      total_overflow.setZero();
    }
    count_t *counts;
    count_overflows *overflows;
    typedef typename count_overflows::iterator oit;
    inline void operator()(unsigned rule, inside_t inside, inside_t norm_outside) {
      DBP_VERBOSE(2);
      if (counts[rule].isNearAddOneLimit()) {
        (*overflows)[rule] += counts[rule]; // default 0 init!
        /*
          std::pair <oit,bool> iret=overflows->insert(make_pair(rule,counts[rule]));
          if (!iret.second)
          iret.first->second += counts[rule];
        */
        ++n_overflows;
        DBPC5("overflow #", n_overflows, rule, counts[rule], (*overflows)[rule]);

        counts[rule] = inside * norm_outside;
      } else {
        counts[rule] += inside * norm_outside;
      }
    }
    void visit(unsigned rule, count_t overflow_count) {
      DBP_VERBOSE(1);
      DBPC4("combine overflow", rule, counts[rule], overflow_count);
      counts[rule] += overflow_count;
      total_overflow += overflow_count;
      ++n_rule_overflows;
    }
    void finish_counts() {
      //TODO: make process/delete generic?
      overflows->visit_key_val(*this);
      overflows->clear();
    }
  };
  static accumulate_counts prepare_accumulate(count_t *c, count_overflows *o) {
    accumulate_counts a;
    a.counts = c;
    a.overflows = o;
    return a;
  }
  // caller should 0 (or by smoothing)-initialize counts then call this for each forest
  // don't call if inside[0] == 0
  // doesn't call compute_inside for you
  void collect_counts(accumulate_counts &a) {
    if (compute_norm_outside())
      visit_inside_norm_outside(boost::ref(a));
  }
  void compute()
  {
    compute_inside();
    compute_norm_outside();
  }
  // call compute first;
  template <class F>
  void visit_inside_norm_outside(F f) {
    for (unsigned i = 0, end = size(); i!=end; ++i) {
      const ForestNode &node = nodes[i];
      if (!node.is_backref()) { // node, not a ref to node
        unsigned rulei = node.l.integer();
        if (!IS_OR_INT(rulei)) { // and-node, not or
          DBPC6("adding counts for node # -> rule #", i, rulei, inside[i], norm_outside[i], inside[i]*norm_outside[i]);
          deref(f)(rulei, inside[i], norm_outside[i]);
        }
      }
    }
  }
  bool compute_norm_outside()
  {
    DBPC2("collect_counts: computing norm_outside", inside[0]); DBP_SCOPE;
    DBP_ADD_VERBOSE(10);
    DBP(*this);
    DBP(array<inside_t>(inside, inside+size()));
    //        DBP(outside_order); // because dependent type resolution doesn't work. (have_print etc)
    DBP(array<count_t>(counts, counts+max_ruleid+1));
    if (!(inside[0] > 0)) {
      Config::warn() << "\nCan't collect counts when inside[0] == 0!\n";
      DBPC("you screwed up: inside[0] == 0");
      return false;
    }
    inside_t *oi = norm_outside, *oe = norm_outside+size();
    *oi++=inside[0].inverse(); //...tricky: since we want to collect counts = inside*outside/inside[root], we compute norm_outside=(outside/inside[root]) directly instead of first computing outsides.
    for (; oi<oe; ++oi)
      *oi = 0;
    {
      DBP_INC_VERBOSE;
      for (Ancestry *i = outside_order.end(), *beg = outside_order.begin(); i>beg;) {
        --i;
        ForestNode *p = i->parent;
        ForestNode *child = i->child;
        //            unsigned p=i->parent.integer();
        //            unsigned c=i->child.integer();
        unsigned rulei = p->l.integer();
        if (IS_OR_INT(rulei)) {
          DBPC4("norm_outside+=(parent=OR)", toi(p), toi(child), norm_outside[toi(p)]);
          norm_outside[toi(child)] += norm_outside[toi(p)];
        } else {
          unsigned parenti = toi(p);
          if (child) {
            // contribute to child norm_outside:
            unsigned childi = toi(child);
            if (!inside[parenti].isZero()) { // otherwise you get 0/0 NaN
              inside_t childnorm_outside = norm_outside[parenti]*inside[parenti]/inside[childi]; // if you keep track of #children, monadic case: childnorm_outside=norm_outside[parenti]*rule_weights[parenti]
              DBPC4("norm_outside+=(parent=AND)", toi(p), childi, childnorm_outside);
              DBP4(norm_outside[parenti], inside[parenti], inside[childi], norm_outside[parenti]*inside[parenti]/inside[childi]);

              norm_outside[childi] += childnorm_outside;
            }
          }
        }
      }
    }
    Assert(nodes->l.is_integer()); // can't be pointer to another node; it's the root
    DBPC2("final normalized outside(/ inside[0])", array<inside_t>(norm_outside, oe));
    return true;
  }
  struct Ancestry {
    ForestNode *parent, *child;
    Ancestry(ForestNode *p, ForestNode *c) : parent(p), child(c) {}
    Ancestry() {}
    Ancestry(const Ancestry &o) :parent(o.parent), child(o.child) {}
  };
  typedef dynamic_array<Ancestry> Ancestries; //FIXME: could make this faster by preallocing maximum # needed (definitely max = max # nodes (instead of useless check: size < capacity)
  static THREADLOCAL Ancestries outside_order; // read backwards (reverse iterated), gives an order of adding outside scores from parent to child ... topological sort on ancestor relation (must know parent outside first)
  // also record leaves with c=NULL
  static void record_ancestry(ForestNode *p, ForestNode *c) {
    outside_order.push_back(Ancestry(p, c));
  }
  // saves into viterbi: pointer to best sub-Forest to best_or[i] whenever i is an OR-node
  void compute_viterbi(ForestNode **_viterbi, inside_t *_inside, prob_t *_rule_weights)
  {
    SetLocal<prob_t *> guard1(rule_weights, _rule_weights);
    SetLocal<inside_t *> guard2(inside, _inside);
    SetLocal<ForestNode **> guard3(viterbi, _viterbi);
    compute_viterbi();
  }
  void compute_viterbi()
  {
    viterbi_rec(nodes);
  }
  void viterbi_rec(ForestNode *b) {
    ForestNode *e = b->next;
    DBPC4W("computing viterbi", toi(b), toi(e), FForest(b), b); DBP_SCOPE;
    size_t i = toi(b);
    ForestNode::Label &l = b->l;
    if (l.is_pointer()) {
      ForestNode *shared = l.pointer();
      DBPC3("shared viterbi", toi(shared), inside[toi(shared)]);
      inside[i] = inside[toi(shared)];
    } else {
      unsigned rule_or = l.integer();
      //            ForestNode *parent=b;
      if (IS_OR_INT(rule_or)) {
        ++b;
        Assert(e!=b);
        ForestNode *n = b->next;
        viterbi_rec(b); // moved outside so instead of OR<-0, for children,
        // OR+=inside[child], do OR=inside[first-child],
        // OR+=inside[rest] - don't need to do this for AND
        //NOTE: OR operations only differecence between viterbi_rec and
        //inside_rec - genericize?  some performance hit if we do.  also
        //don't need to store ancestries twice, and don't need to print
        //best (viterbi) tree
        //OR INIT
        inside[i] = inside[i+1];
        viterbi[i] = b;

        DBPC4("  OR set best ", i, inside[i], inside[i+1]);
        for (b = n; b<e; b = n) { // 2nd and subsequent children
          n = b->next;
          viterbi_rec(b);
          size_t bi = toi(b);
          inside_t child_best = inside[bi];
          if ( inside[i] < child_best ) {
            //OR FOLD
            inside[i] = child_best;
            viterbi[i] = b;

            DBPC6("  OR improved best ", i, inside[i], toi(b), inside[toi(b)], viterbi[i]);
          } else {
            DBPC6("  OR didn't improve best ", i, inside[i], toi(b), inside[toi(b)], viterbi[i]);
          }
        }
      } else { // and-node
        //AND INIT
        inside[i] = rule_weights[rule_or];
        DBPC5("  AND=", i, inside[i], rule_or, rule_weights[rule_or]);
        ForestNode *n;
        ++b;
        for (; b<e; b = n) { // all children
          n = b->next;
          viterbi_rec(b);
          //AND FOLD
          inside[i] *= inside[toi(b)];
          DBPC5("  AND*=", i, inside[i], toi(b), inside[toi(b)]);
        }
      }
    }
    DBPC3("done computing viterbi ", i, inside[i]);
  }

  void write_viterbi(std::ostream &o, ForestNode **_viterbi)
  {
    SetLocal<ForestNode **> guard3(viterbi, _viterbi);
    SetLocal<std::ostream *> guard4(viterbi_out, &o);
    write_viterbi_rec(nodes);
  }
  void write_viterbi(std::ostream &o, inside_t sum)
  {
    SetLocal<std::ostream *> guard3(viterbi_out, &o);
    *viterbi_out << inside[0] << '/' << sum << '=' << 100*(inside[0]/sum).getReal() << "% ";
    write_viterbi_rec(nodes);
  }
  void write_viterbi()
  {
    *viterbi_out << inside[0] << ' ';
    write_viterbi_rec(nodes);
  }
  void write_viterbi_rec(ForestNode *b)
  {
    ForestNode *e = b->next;
    DBPC4W("writing viterbi", toi(b), toi(e), FForest(b), b); DBP_SCOPE;
#ifdef DEBUG
    size_t i = toi(b);
#endif
    ForestNode::Label &l = b->l;
    if (l.is_pointer()) {
      ForestNode *shared = l.pointer();
      DBPC3("writing shared", toi(shared), inside[toi(shared)]);
      write_viterbi_rec(shared);
    } else {
      unsigned rule_or = l.integer();
      //            ForestNode *parent=b;
      if (IS_OR_INT(rule_or)) {
        b = viterbi[toi(b)];
        DBPC3("OR: writing viterbi", toi(b), toi(viterbi[toi(b)]));
        write_viterbi_rec(b);
      } else { // and-node
        if (b+1 == b->next) {
          DBPC3(" writing viterbi leaf", toi(b), rule_or);
          *viterbi_out << rule_or;
        } else {
          *viterbi_out << '(';
          //AND INIT
          *viterbi_out << rule_or;
          DBPC3("  write AND root", i, rule_or);
          ForestNode *n;
          ++b;
          for (; b<e; b = n) { // all children
            n = b->next;
            *viterbi_out << ' ';
            write_viterbi_rec(b);
            //AND FOLD
            DBPC5(" wrote viterbi child", i, inside[i], toi(b), inside[toi(b)]);
          }

          *viterbi_out << ')';

        }
      }
    }
    DBPC2("done writing viterbi ", i);

  }

  //TODO: this is same as above except w/ ancestry recording.  make that a type of W visitor?
  void inside_rec(ForestNode *b) {
    ForestNode *e = b->next;
    DBPC4W("computing inside", toi(b), toi(e), FForest(b), b); DBP_SCOPE;
    size_t i = toi(b);
    ForestNode::Label &l = b->l;
    if (l.is_pointer()) {
      ForestNode *shared = l.pointer();
      DBPC3("shared inside", toi(shared), inside[toi(shared)]);
      inside[i] = inside[toi(shared)];
    } else {
      unsigned rule_or = l.integer();
      ForestNode *parent = b;
      if (IS_OR_INT(rule_or)) {
        ++b;
        Assert(e!=b);
        ForestNode *n = b->next;
        inside_rec(b); // moved outside so instead of OR<-0, for children,
        // OR+=inside[child], do OR=inside[first-child],
        // OR+=inside[rest] - don't need to do this for AND
        //OR INIT
        inside[i] = inside[i+1];
        DBPC4("  OR=", i, inside[i], inside[i+1]);
        for (b = n; b<e; b = n) { // 2nd and subsequent children
          n = b->next;
          inside_rec(b);
          //OR FOLD
          inside[i] += inside[toi(b)];
          DBPC5("  OR+=", i, inside[i], toi(b), inside[toi(b)]);

        }
      } else { // and-node
        //AND INIT
        inside[i] = rule_weights[rule_or];
        DBPC5("  AND=", i, inside[i], rule_or, rule_weights[rule_or]);
        ForestNode *n;
        ++b;
        for (; b<e; b = n) { // all children
          n = b->next;
          inside_rec(b);
          //AND FOLD
          inside[i] *= inside[toi(b)];
          DBPC5("  AND*=", i, inside[i], toi(b), inside[toi(b)]);
        }
      }
      // now queue up parent->child links
      b = parent+1;
      if (b==e) { // leaf:
        //record_ancestry(parent); // no longer needed (we visit all nodes without using ancestry)
      } else {
        do { // for children
          Assert(b<e);
          ForestNode::Label &l = b->l;
          if (l.is_pointer())
            record_ancestry(parent, l.pointer());
          else
            record_ancestry(parent, b);
          b = b->next;
        } while (b!=e);
      }
    }
    DBPC2("done computing inside", inside[i]);
  }

  /* for gibbs: */

  // v.record(rule_id)
  template <class V>
  void choose_random(inside_t *ins, V &v, Float power = 1) // use compute_inside to set inside[i] first
  {
    SetLocal<inside_t *> guard2(inside, ins);
    choose_random(nodes, v, power);
  }


  //possible random-choice speedup: destructively normalize inside into doubles, at most once (bool flag array needed for shared subforests).  flags init is linear time, just like inside computation.  random choice should be faster than inside because only part of forest is visited.
  static THREADLOCAL inside_t choose_norm; // made static so we can open swapbatch in read-only mode (just as well could be member var otherwise)

  struct or_iterator
  {
    ForestNode *p;
    or_iterator(ForestNode *p = NULL) : p(p) {  }
    ForestNode *operator*()
    {
      return p;
    }
    void operator++()
    {
      p = p->next;
    }
    bool operator==(or_iterator const& o) const
    {
      return p==o.p;
    }
    bool operator!=(or_iterator const& o) const
    {
      return p!=o.p;
    }
  };

  template <class V>
  void choose_random(ForestNode *b, V &v, Float power = 1)
      /* use compute_inside(b,v) to set inside[i] first via v(ruleid)=prob.
       * choose_random calls v.record(ruleid). */
  {
    ForestNode *e = b->next;
    ForestNode::Label &l = b->l;
    if (l.is_pointer())
      choose_random(l.pointer(), v);
    else {
      unsigned rule_or = l.integer();
      if (IS_OR_INT(rule_or)) {
        choose_norm.setZero();
        ++b;
        // choose one child:
        for (ForestNode *i = b; i!=e; i = i->next)
          choose_norm += inside[toi(i)].pow(power);
        ForestNode *i = b, *n;
        double choice = random01();
        for (;;) {
          choice -= (inside[toi(i)].pow(power)/choose_norm).getReal();
          if (choice<0) break;
          n = i->next;
          if (n==e) break;
          i = n;
        }
        choose_random(i, v, power); // i is chosen or-branch
      } else { // AND
        v.record(rule_or);
        ++b;
        for (; b<e; b = b->next)  // all children
          choose_random(b, v, power);
      }
    }
  }

  // ins is ptr to array big enough for forest
  template <class W>
  void compute_inside(inside_t *ins, W const& w)
  {
    SetLocal<inside_t *> guard2(inside, ins);
    compute_inside(nodes, w);
  }

  // w(ruleid)=Weight
  template <class W>
  void compute_inside(ForestNode *b, W const& w)
  {
    ForestNode *e = b->next;
    DBP_ADD_VERBOSE(10);
    //        DBPC4W("computing inside",toi(b),toi(e),FForest(b),b); //FIXME: segfault on OR node child?
    DBPC3("computing inside", toi(b), toi(e));
    DBP_SCOPE;
    size_t i = toi(b);
    ForestNode::Label &l = b->l;
    if (l.is_pointer()) {
      ForestNode *shared = l.pointer();
      DBPC3("shared inside", toi(shared), inside[toi(shared)]);
      inside[i] = inside[toi(shared)];
    } else {
      unsigned rule_or = l.integer();
      if (IS_OR_INT(rule_or)) {
        ++b;
        Assert(e!=b);
        ForestNode *n = b->next;
        compute_inside(b, w); // moved outside so instead of OR<-0, for children,
        // OR+=inside[child], do OR=inside[first-child],
        // OR+=inside[rest] - don't need to do this for AND
        //OR INIT
        inside[i] = inside[i+1];
        DBPC4("  OR=", i, inside[i], inside[i+1]);
        for (b = n; b<e; b = n) { // 2nd and subsequent children
          n = b->next;
          compute_inside(b, w);
          //OR FOLD
          inside[i] += inside[toi(b)];
          DBPC5("  OR+=", i, inside[i], toi(b), inside[toi(b)]);

        }
      } else { // and-node
        //AND INIT
        inside[i] = w(rule_or); // proposal prob (used nowhere else)
        DBPC5("  AND=", i, inside[i], rule_or, w(rule_or));
        ForestNode *n;
        ++b;
        for (; b<e; b = n) { // all children
          n = b->next;
          compute_inside(b, w);
          //AND FOLD
          inside[i] *= inside[toi(b)];
          DBPC5("  AND*=", i, inside[i], toi(b), inside[toi(b)]);
        }
      }
    }
    DBPC2("done computing inside", inside[i]);
  }


};

CREATE_EXTRACTOR_T1(FForest);
CREATE_INSERTER_T1(FForest);


template <class Float>
inline void dbgout (std::ostream &o, const FForest<Float> &f) {
#ifdef VERBOSE_DEBUG
  array<ForestNode> a(f.nodes, f.end());
  o << dbgstr(a);
#else
  o << f;
#endif
}

template <class Float>
inline void dbgout (std::ostream &o, const FForest<Float> &f, ForestNode *start) {
  gen_inserter(o, f, start);
}


struct ForestNodeLabeler {
  void print(std::ostream &o, int l) {
    if (IS_OR_INT(l))
      o << "label=OR,shape=box";
    else
      o << "label=" << l;
  }
};

template <class Labeler = ForestNodeLabeler , class Float = FLOAT_TYPE>
struct ForestVizPrinter : public GraphvizPrinter {
  typedef ForestNode N;
  typedef FForest<Float> Forest;
  Forest f;

  dynamic_array<unsigned> backref_ids;
  Labeler labeler;

  ForestVizPrinter(std::ostream &out, const std::string &prelude="", bool indirect_nodes_ = true, bool label_edges_ = false, const Labeler &labeler_ = Labeler(), const char *graphname="forest") : GraphvizPrinter(out,"node [shape=ellipse];\n"+prelude, graphname), labeler(labeler_), label_edges(label_edges_), indirect_nodes(indirect_nodes_) {
  }
  void coda() {
    o << "}\n";
  }
  bool label_edges;
  bool indirect_nodes;
  bool same_rank;
  unsigned base_nodeid;

  void print(const Forest &forest, bool same_rank_ = false, bool number_edges = false, bool pointer_nodes = false) {
    same_rank = same_rank_;
    indirect_nodes = pointer_nodes;
    label_edges = number_edges;
    f.reset(forest.nodes);
    base_nodeid = next_node;
    unsigned sz = f.size();
    next_node += sz; // pessimistic.
    backref_ids.reinit_nodestroy(sz, 0);
    f.assign_backref_ids(backref_ids.begin());
    print(0, sz);
    o << std::endl;
  }
  unsigned tonode(unsigned forest_index) const {
    return base_nodeid+forest_index;
  }
  void print(unsigned b, unsigned e) {
    unsigned mynode = tonode(b);
    const ForestNode &n = f[b];
    Assert(!n.is_backref());
    o << " " << mynode << " [";
    labeler.print(o, n.label());
    o << "]\n";
    int lastnode = -1;
    unsigned edge_no = 0;
    for (unsigned cb = b+1, cnext; cb!=e; cb = cnext) {
      cnext = f.next(cb);
      unsigned parentnode = mynode;
      unsigned child = cb;
      if (f.is_backref(cb)) {
        child = f.backref(cb);
      } else {
        print(cb, cnext);
      }
      if (indirect_nodes && backref_ids[child]) {
        o << next_node << " -> " << child << "\n";
        o << ' ' << next_node << " [shape=point,width=.03,height=.03,label=\"\"]\n";
        child = next_node;
        ++next_node;
      }
      unsigned childnode = tonode(child);
      o << " " << parentnode << " -> " << childnode;
      if (label_edges)
        o << " [taillabel=" << ++edge_no << ']';
      o << '\n';
      if (same_rank) {
        if (lastnode >= 0) {
          o << " {rank=same ";
          o << lastnode << " -> " << childnode << " [style=invis]}\n";
        }
        lastnode = childnode;
      }
    }
  }
};

template <class Float>
void read(std::istream &in, FForest<Float> &f, StackAlloc &a) // throw(genio_exception,StackAlloc::Overflow)
{
  typedef FForest<Float> Forest;

  a.align<ForestNode>();

  DBP_INC_VERBOSE;
  self_destruct<Forest> suicide(&f);
  List<ForestNode *> open_parens;
  ForestNode *stop; // points one past end of nodes
  char c;
  size_t backref_id;
  dynamic_array<ForestNode *> backrefs;

  bool follows_paren = false;
  unsigned rule_id;
  //    bool first_char=true;

  f.nodes = a.next<ForestNode>();

#define ALLOCSTOP stop = a.alloc<ForestNode>()
  // // stop is the next to be allocated (end/top) ForestNode * - this is the only way space is reserved!

  EXPECTI_FIRST(in>>c);

  goto firstchar;
  for (;;) {
    EXPECTI(in >> c);
 firstchar:
    switch(c) {
      case '#':
        if (follows_paren) {
          GENIO_THROW("Unexpected '#' following paren in Forest");
        }
        EXPECTI(in>>backref_id);
        EXPECTI(in.get(c));
#define ANEXT a.next<ForestNode>()
        if (c == '(') {
          backrefs(backref_id) = ANEXT;
          goto saw_open_paren;
        } else {
          Assert2(backrefs[backref_id], < ANEXT);
          Assert2(backrefs[backref_id], >= f.nodes);
          ALLOCSTOP;
          stop->l = backrefs[backref_id];
          stop->next = ANEXT;
          in.unget();
        }
        break;
      case '(':
      saw_open_paren:
        follows_paren = true;
        open_parens.push(ANEXT);
        break;
      case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':
        in.unget();
        in >> rule_id;
        if (Forest::max_ruleid < rule_id)
          Forest::max_ruleid = rule_id;
        ALLOCSTOP;
        stop->l = rule_id;
        if (!follows_paren) {
          // child or leaf-root
          stop->next = ANEXT;
          if (open_parens.empty())
            goto done;
        } else {
          // root
          follows_paren = false;
        }
        break;
      case 'O':
        EXPECTCH('R');
        if (!follows_paren) {
          GENIO_THROW("OR not following paren in Forest");
        }
        follows_paren = false;
        ALLOCSTOP;
        stop->l = OR_INT;
        break;
      case ')':
        if (open_parens.top()!=ANEXT) // why?  because we haven't allocated ANEXT yet ... would cause rare bug if it's not safe to write one off the end of StackAlloc
          open_parens.top()->next = ANEXT;
        open_parens.pop();
        if (open_parens.empty())
          goto done;
        break;
      default:
        GENIO_THROW2("Forest: unexpected char ", boost::lexical_cast<std::string>(c));
#undef ALLOCSTOP
        break;
    }
  }
done:
  f.end() = ANEXT; //FIXME: is this now redundant?
#undef ANEXT
  DBPC2("Successfully read forest", f);
  suicide.cancel();

  return;
fail:
  GENIO_FAIL(in);
  // can fail because of eof only
}


#ifdef GRAEHL_TEST
#define MAX_TEST_FOREST_NODES 100000
ForestNode forest_space[MAX_TEST_FOREST_NODES];
ForestNode *forest_space_end = forest_space+MAX_TEST_FOREST_NODES;
const char *test_forests[] = {
  "1"
  , "#1(1 #1 (2 #1 (1 1)))"
  ,     "(1 4)"
  , "(OR (1 4) (1 3))"
  ,     "(OR (1 4 4) (2 3 4) (2 4 3) (1 5))"
  ,       "(OR (1 #1(4) #1) (2 #2(3) #1) (2 #1 #2) (1 5))"
};


BOOST_AUTO_TEST_CASE( TEST_FOREST )
{
  using namespace std;
  typedef FForest<> Forest;

  Forest f, f2;

  ForestNode *forest_porch = forest_space+MAX_TEST_FOREST_NODES/2;
  for (int i = 0; i<sizeof(test_forests)/sizeof(test_forests[0]); ++i) {
#define RESET f.reset(forest_space, forest_porch)
    RESET;
    BOOST_CHECK(test_extract(test_forests[i], f));
    RESET;
    BOOST_CHECK(test_extract_insert(test_forests[i], f));
#undef RESET
    unsigned fsize = f.size()*sizeof(ForestNode);
    memcpy(forest_porch, forest_space, fsize);
    BOOST_CHECK(!memcmp(forest_space, forest_porch, fsize));
    StackAlloc a;
    a.init(forest_space, forest_porch);
    tmp_fstream i1(test_forests[i]);
    read((istream &)i1.file, f2, a);
    BOOST_CHECK_EQUAL(f.end(), f2.end());
    BOOST_CHECK(!memcmp(forest_space, forest_porch, fsize));

  }
}
#endif

template <class Float>
inline std::ostream & operator <<(std::ostream &o, const typename FForest<Float>::Ancestry &a) {
  o << "(parent=" << *a.parent;
  if (a.child) {
    o << " child=";
    o << *a.child;
  }
  o << ')';
  return o;
}

#ifdef GRAEHL__SINGLE_MAIN
template <class Float>
THREADLOCAL gibbs_base *FForest<Float>::gibbs;
template <class Float>
THREADLOCAL dynamic_array<typename FForest<Float>::Ancestry> FForest<Float>::outside_order;
template <class Float>
THREADLOCAL size_t FForest<Float>::max_ruleid; // static global return value;
template <class Float>
THREADLOCAL ForestNode **FForest<Float>::viterbi; // records which OR-node subforest is taken (values don't matter otherwise)
template <class Float>
THREADLOCAL std::ostream *FForest<Float>::viterbi_out;
template <class Float> THREADLOCAL typename FForest<Float>::inside_t FForest<Float>::choose_norm;
template <class Float>
THREADLOCAL typename FForest<Float>::inside_t *FForest<Float>::inside;
template <class Float>
THREADLOCAL typename FForest<Float>::inside_t *FForest<Float>::norm_outside;
template <class Float>
THREADLOCAL typename FForest<Float>::prob_t *FForest<Float>::rule_weights;
template <class Float>
THREADLOCAL typename FForest<Float>::count_t *FForest<Float>::counts;

#endif

typedef FForest<FLOAT_TYPE> Forest;

} //ns

#endif
