#ifndef GRAEHL_CARMEL__FST_H
#define GRAEHL_CARMEL__FST_H

#include <graehl/carmel/src/config.hpp>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <sstream>
#include <iostream>
#include <graehl/shared/2hash.h>
#include <graehl/shared/list.h>
#include <graehl/shared/weight.h>
#include <graehl/shared/strhash.h>
#include <graehl/shared/graph.h>
#include <graehl/shared/threadlocal.hpp>
#include <graehl/carmel/src/train.h>
#include <graehl/shared/myassert.h>
#include <graehl/carmel/src/compose.h>
#include <iterator>
#include <algorithm>
#include <graehl/shared/kbest.h>
#include <boost/config.hpp>
#include <graehl/shared/config.h>
#include <graehl/shared/mean_field_scale.hpp>
#include <graehl/shared/size_mega.hpp>
#include <graehl/shared/debugprint.hpp>
#include <graehl/shared/gibbs_opts.hpp>

namespace graehl {

using namespace std;

class WFST;

//TODO: this is an abomination e.g. missing groupId; use pair<FSTArc,WFST *> instead?
struct PathArc {
    //const char *in;
    //const char *out;
    //const char *destState;
    int in,out,destState;
    int &symbol(int dir)
    {
        return dir ? out : in;
    }
    int symbol(int dir) const
    {
        return dir ? out : in;
    }
    const WFST *wfst;
    Weight weight;
};


std::ostream & operator << (std::ostream & o, const PathArc &p);

struct cascade_parameters; // in cascade.h, but we avoid circular dependency by knowing only about references in this header

class WFST {
 public:
    typedef Alphabet<StringKey,StringPool> alphabet_type;
    enum { DEFAULT_PER_LINE,STATE,ARC } /*Per_Line */;
    enum { DEFAULT_ARC_FORMAT,BRIEF,FULL } /*Arc_Format*/ ;
#define EPSILON_SYMBOL "*e*"
#define WILDCARD_SYMBOL "*w*"
    struct path_print
    {
        // options
        bool O,I,Q,AT,W,E;
        typedef dynamic_array<int> Output;
        // bound here for fun:
        std::ostream *pout;
        // state used/saved per arc:
        Output output;
        graehl::word_spacer sp;
        unsigned src;
        Weight w;
        path_print() : pout(&Config::out()),O(),I(),Q(),AT(),W(),E() {  }
        path_print(bool const* flags) : pout(&Config::out())
        {
            set_flags(flags);
        }
        void set_flags(bool const* flags)
        {
            O=flags['O'];
            I=flags['I'];
            Q=flags['Q'];
            AT=flags['@'];
            W=flags['W'];
            E=flags['E'];
        }
        std::ostream &out() const { return *pout; }
        void set_out(std::ostream &o) { pout=&o; }
        void start(WFST const&wfst)
        {
            src=0;
            sp.reset();
            output.clear();
            w.setOne();
        }
        bool needs_weight() const
        {
            return AT || (!W && (O || I));
        }

        void finish(WFST const&wfst)
        {
            std::ostream &out=*pout;
            using namespace std;
            if (AT ) {
                out << endl;
                sp.reset();
                for (Output::const_iterator i=output.begin(),e=output.end();i!=e;++i)
                    out << sp << wfst.outLetter(*i);
                out << endl;
            } else {
                if (!W)
                    out << sp << w;
                out << endl;
            }
        }
        static void out_maybe_quote(char const* str,ostream &out,bool quote)
        {
            if (quote)
                out << str;
            else
                outWithoutQuotes(str,out);
        }
        static void outWithoutQuotes(const char *str, ostream &out) {
            if ( *str != '\"') {
                out << str;
                return;
            }
            const char *psz = str;
            while ( *++psz ) ;
            if ( *--psz == '\"' ) {
                while ( ++str < psz )
                    out << *str;
            } else
                out << str;
        }
        void arc(PathArc const& p)
        {
            WFST const&wfst=*p.wfst;
            FSTArc a(p.in,p.out,p.destState,p.weight);
            arc(wfst,a);
        }
        void arc(WFST const& w,PathArc const& p) { arc(p); }
        void arc(WFST const& wfst,FSTArc const* a)
        {
            arc(wfst,*a);
        }
        void arc(WFST const& wfst,FSTArc const& arc)
        {
            std::ostream &out=*pout;
            w*=arc.weight;
            if (AT) {
                int inid=arc.in,outid=arc.out;
                if (outid!=WFST::epsilon_index)
                    output.push_back(outid);
                if (inid!=WFST::epsilon_index)
                    out << sp << wfst.inLetter(inid);
            } else {
                if ( O || I ) {
                    int id = O ? arc.out : arc.in;
                    if ( !(E && id==WFST::epsilon_index) ) {
                        out << sp;
                        out_maybe_quote(O ? wfst.outLetter(id) : wfst.inLetter(id),out,!Q);
                    }
                } else {
                    out << sp;
                    wfst.printArc(arc,src,out);
                }
            }
            src=(unsigned)arc.dest;
        }
        template <class I>
        void operator()(WFST const&w,I i,I end)
        {
            start(w);
            for(;i!=end;++i)
                arc(w,*i);
            finish(w);
        }
        template <class C>
        void operator()(WFST const&w,C const& path)
        {
            (*this)(w,path.begin(),path.end());
        }
    };

 private:
    static const int per_line_index; // handle to ostream iword for LogBase enum (initialized to 0)
    static const int arc_format_index; // handle for OutThresh


    void initAlphabet(int dir)
    {
        owner_alph[dir]=1;
        alph[dir]=NEW alphabet_type(EPSILON_SYMBOL,WILDCARD_SYMBOL);
    }

    void initAlphabet() {
        initAlphabet(0);
        initAlphabet(1);
    }

    void init()
    {
        initAlphabet();
        init_index();
    }

    void train_prune(); // delete states with zero counts

    void deleteAlphabet(int dir) {
        if (owner_alph[dir])
            delete alph[dir];
        owner_alph[dir]=0;
        alph[dir]=0;
    }
    void deleteAlphabet()
    {
        deleteAlphabet(0);
        deleteAlphabet(1);
    }
    /*
    BOOST_STATIC_CONSTANT(int,input=State::input);
    BOOST_STATIC_CONSTANT(int,output=State::output);
    */
    enum {input=0,output=1};

    inline static int opposite(int dir)
    {
        return dir ? 0 : 1;
    }


    void identity_alphabet_from(int dir)
    {
        int to = opposite(dir);
        deleteAlphabet(to);
        alph[to]=alph[dir];
    }

    int getStateIndex(const char *buf); // creates the state according to named_states, returns -1 on failure
 public:
    // openfst MutableFst<LogArc> or <StdArc>, eg StdVectorFst
    template <class Fst>
    void to_openfst(Fst &fst) const
    {
        typedef typename Fst::Arc A;
        typedef typename A::Weight W;
        typedef typename A::StateId I;
        fst.DeleteStates();
        for (unsigned i=0,e=numStates();i!=e;++i) {
            I s=fst.AddState();
            assert(s==i);
            states[i].to_openfst(fst,i);
        }
        fst.SetStart(0);
        fst.SetFinal(final,W::One());
    }
    template <class FstWeight>
    Weight openfst_to_weight(FstWeight const& w)
    {
        return Weight(w.Value(),negln_weight());
    }

    // scoped object: on creation, turns the arcs into identity-FSA with symbol
    // = pair of old input/output.  on destruction, restores the pairs into
    // original input/output alphabet.  during the duration, any symbol<->string
    // lookup (e.g. i/o) is prohibited
    struct as_pairs_fsa
    {
        WFST &wfst;
        bool keep_epsilon; // *e*:*e* -> *e* if true, non-epsilon pair-symbol if false
//        typedef std::pair<int,int> newsym;
        typedef IOPair newsym;
        typedef std::vector<newsym> newsyms;
        newsyms syms;
        bool to_pairs;

        // does nothing if to_pairs=false
        as_pairs_fsa(WFST &wfst,bool keep_epsilon=true,bool to_pairs=true)
            :wfst(wfst)
            ,keep_epsilon(keep_epsilon)
            ,to_pairs(to_pairs)
        {
            if (!to_pairs) return;

            typedef HashTable<newsym,unsigned> tosyms;

            tosyms to;

            syms.push_back(newsym(0,0));
            if (keep_epsilon)
                to.add(syms[0],0);

            for (StateVector::iterator i=wfst.states.begin(),e=wfst.states.end();i!=e;++i) {
                State::Arcs &arcs=i->arcs;
                for ( State::Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l ) {
                    newsym s(l->in,l->out);
                    tosyms::insert_return_type ins=to.insert(s,syms.size());
                    if (ins.second)
                        syms.push_back(s);
                    l->in=l->out=ins.first->second;
                }
            }
        }
        ~as_pairs_fsa()
        {
            if (!to_pairs) return;

            for (StateVector::iterator i=wfst.states.begin(),e=wfst.states.end();i!=e;++i) {
                State::Arcs &arcs=i->arcs;
                for ( State::Arcs::val_iterator l=arcs.val_begin(),end=arcs.val_end() ; l != end ; ++l ) {
                    newsym &s=syms[l->in];
                    l->in=s.in;
                    l->out=s.out;
                }
            }
        }

    };


#ifdef USE_OPENFST
    // here we depend on ArcIterator directly
    // openfst ExpandedFst<LogArc>
    template <class Fst>
    void from_openfst(Fst &fst)
    {
        typedef typename Fst::Arc A;
        typedef typename A::Weight W;
        typedef typename A::StateId I;
        typedef fst::ArcIterator<Fst> AI;

        const W zero=W::Zero();
        const W one=W::One();

        unsigned n_final=0;
        unsigned max_final=0;
        unsigned o_n=fst.NumStates();
        for (unsigned i=0;i!=o_n;++i) {
            if (zero != fst.Final(i)) {
                ++n_final;
                DBP2(i,fst.Final(i));
                max_final=i;
            }
        }
        if (n_final==0) {
            clear();
            return;
        }
        bool need_extra_final=(n_final>1 || fst.Final(max_final)!=one); // todo: if final is in no cycles, make all arcs entering final bear the final cost if not==1

        unsigned ns=o_n+(need_extra_final?1:0);
        unNameStates();
        states.clear();
        states.resize(ns);
//        DBP4(states.size(),ns,o_n,need_extra_final);
        State::arc_adder arc_add(states);

        unsigned o2s[o_n]; //FIXME: does an expandedfst always have ids [0...numstates)?
        unsigned s=0,i=0;
        unsigned o_start=fst.Start();

        //... precomputing may save time but we're really just doing i==start?0:(i<start?i+1:i)
        for (unsigned o=0;o<o_n;++o) {
            o2s[o]=
                (o < o_start) ? o+1 : o;
        }
        o2s[o_start]=0;

        // actually iterate over the arcs for each state, translating ids by our constraint that start index must be 0

        for (unsigned o=0;o<o_n;++o) {
            for (AI ai(fst,o);!ai.Done();ai.Next()) {
                A const&arc=ai.Value();
                const FSTArc a(arc.ilabel,arc.olabel,o2s[arc.nextstate],openfst_to_weight(arc.weight));
//                DBP5(o,o2s[o],arc.nextstate,o2s[arc.nextstate],a);
                arc_add(o2s[o],a);
            }
        }

        // set final status, adding a state and arcs if necessary (carmel has no final state weights)
        if (!need_extra_final) {
            final=max_final;
            Assert(final<numStates());
        } else {
            final=numStates()-1;
            for (unsigned i=0;i<=max_final;++i) {
                W fw=fst.Final(i);
                if (zero != fw) {
                    states[i].addArc(FSTArc(epsilon_index,epsilon_index,final,openfst_to_weight(fw)));
                }
            }
        }


    }

    template <class Fst>
    void roundtrip_openfst() // for debugging, to_openfst then from_openfst, will lose arc indices
    {
        Fst f;
//        DBP(numStates());
        to_openfst(f);
        from_openfst(f);
//        DBP(numStates());
    }

    // if determinize is false, input must already be deterministic.  connect=false means don't prune unconnected states.  determinize=true -> may not terminate (if lacking twins property, see openfst.org)
    template <class Fst>
    bool minimize_openfst(bool determinize=true,bool rmepsilon=false,bool minimize=true,bool connect=true,bool inverted=false,bool as_pairs=false,bool pairs_keep_epsilon=true) // for debugging, to_openfst then from_openfst, will lose arc indices
    {
        ensure_final_sink();
        if (inverted)
            invert();
        Fst f,f2;
        {
            as_pairs_fsa scoped_pairs(*this,pairs_keep_epsilon,as_pairs);
//        DBP(numStates());
            if (determinize) {
                to_openfst(f2);
                Determinize(f2,&f);
            } else {
                to_openfst(f);
                if (!f.Properties(fst::kIDeterministic,true)) {
//                Config::log() << " (FST not input deterministic, skipping openfst minimize) ";
                    return false;
                }
            }
            if (rmepsilon) {
                RmEpsilon(&f);
            }
            if (minimize) {
                Minimize(&f);
            }
            if (connect)
                Connect(&f);
            /*
              } catch(std::exception &e) {
              Config::log() << "\nERROR: "<<e.what()<<std::endl;
              return false;
              }
            */
            from_openfst(f);
        }
        if (inverted)
            invert();
        return true;
//        DBP(numStates());
    }

#endif
    enum { epsilon_index=FSTArc::epsilon,wildcard_index=FSTArc::wildcard,start_normal_index };


    // some algorithms (e.g. determinizing) are confused when the final state has arcs leaving it.  this will add a new final state with no exit, if necessary (and a p=1 epsilon transition from old final state to new one)
    void ensure_final_sink()
    {
        if (!states[final].size)
            return;
        unsigned old_final=final;
        final=add_state("FINAL_SINK");
        states[old_final].addArc(FSTArc(epsilon_index,epsilon_index,final,1));
    }


    static THREADLOCAL int default_per_line;
    static THREADLOCAL int default_arc_format;

    static inline void set_arc_default_per(int per)
    {
        default_per_line=per;
    }
    static inline void set_arc_default_format(int ver)
    {
        default_arc_format=ver;
    }

    static inline int get_arc_format(std::ostream &os)
    {
        int r=os.iword(arc_format_index);
        return r==DEFAULT_ARC_FORMAT ? default_arc_format : r;
    }

    static inline int get_per_line(std::ostream &os)
    {
        int r=os.iword(per_line_index);
        return r==DEFAULT_PER_LINE ? default_per_line : r;
    }

    static inline void set_arc_format(std::ostream &os,int i)
    {
        os.iword(arc_format_index)=i;
    }

    static inline void set_per_line(std::ostream &os,int i)
    {
        os.iword(per_line_index)=i;
    }


    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_state_per_line(std::basic_ostream<A,B>& os) { os.iword(per_line_index) = STATE; return os; }
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_per_line(std::basic_ostream<A,B>& os) { os.iword(per_line_index) = ARC; return os; }
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_default_per(std::basic_ostream<A,B>& os) { os.iword(per_line_index) = DEFAULT_PER_LINE; return os; }

    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_brief(std::basic_ostream<A,B>& os) { os.iword(arc_format_index) = BRIEF; return os; }
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_full(std::basic_ostream<A,B>& os) { os.iword(arc_format_index) = FULL; return os; }
    template<class A,class B> static inline std::basic_ostream<A,B>&
    out_arc_default_format(std::basic_ostream<A,B>& os) { os.iword(arc_format_index) = DEFAULT_ARC_FORMAT; return os; }


    void read_training_corpus(std::istream &in,training_corpus &c);

    static inline double randomFloat()       // in range [0, 1)
    {
        return rand() * (1.f / (RAND_MAX+1.f));
    }

    bool owner_alph[2];
    alphabet_type *alph[2];
    bool named_states;
    alphabet_type &in_alph() const
    {
        return *alph[input];
    }
    alphabet_type &out_alph() const
    {
        return *alph[output];
    }

    alphabet_type stateNames;
    unsigned final;	// final state number - initial state always number 0
    typedef dynamic_array<State> StateVector;
    // note: std::vector<State> doesn't work with State::state_adder because copies by value are made during readLegible

    StateVector states;

    //  HashTable<IntKey, int> tieGroup; // IntKey is FSTArc *; value in group number (0 means fixed weight)
    //  WFST(WFST &) {}		// disallow copy constructor - Yaser commented this ow to allow copy constructors
    //  WFST & operator = (WFST &){return *this;} Yaser
    //WFST & operator = (WFST &){std::cerr <<"Unauthorized use of assignemnt operator\n";;return *this;}
    int abort();			// called on a bad read
    int readLegible(istream &,bool alwaysNamed=false);	// returns 0 on failure (bad input)
    int readLegible(const string& str, bool alwaysNamed=false);
    void writeArc(ostream &os, const FSTArc &a,bool GREEK_EPSILON=false); // for graphviz
    void writeLegible(ostream &,bool include_zero=false);
    void writeLegibleFilename(std::string const& name,bool include_zero=false);
    void writeGraphViz(ostream &); // see http://www.research.att.com/sw/tools/graphviz/
    int numStates() const { return states.size(); }
    bool isFinal(int s) { return s==final; }
    void setPathArc(PathArc *pArc,const FSTArc &a) const {
        pArc->in = a.in;
        pArc->out = a.out;
        pArc->destState = a.dest;
        pArc->weight = a.weight;
        pArc->wfst=this;
    }
    std::ostream & printArc(const FSTArc &a,std::ostream &o) const {
        PathArc p;
        setPathArc(&p,a);
        return o << p;
    }
    std::ostream & printArc(const FSTArc &a,int source,std::ostream &o,bool weight=true) const {
        o << '(' << stateName(source) << " -> " << stateName(a.dest) << ' ' << inLetter(a.in) << " : " << outLetter(a.out);
        if (weight) o<< " / " << a.weight;
        o << ")";
    }

    /*void insertPathArc(GraphArc *gArc, List<PathArc>*);
      void insertShortPath(int source, int dest, List<PathArc> *);*/
    template <class T>
    void insertPathArc(GraphArc *gArc,T &l)
    {
        PathArc pArc;
        setPathArc(&pArc,*gArc->data_as<FSTArc*>());
        *(l++) = pArc;
    }
    template <class T>
    void insertShortPath(GraphState *shortPathTree,int source, int dest, T &l)
    {
        GraphArc *taken;
        for ( int iState = source ; iState != dest; iState = taken->dest ) {
            taken = &shortPathTree[iState].arcs.top();
            insertPathArc(taken,l);
        }
    }
    /*template <>
      void insertPathArc(GraphArc *gArc, List<PathArc>*l) {
      insertPathArc(gArc,insert_iterator<List<PathArc> >(*l,l->erase_begin()));
      }
      template <>
      void insertShortPath(GraphState *shortPathTree,int source, int dest, List<PathArc> *l) {
      insertShortPath(shortPathTree,source,dest,insert_iterator<List<PathArc> >(*l,l->erase_begin()));
      }*/

    static int indexThreshold;
    enum norm_group_by { CONDITIONAL, // all arcs from a state with the same input will add to one
                           JOINT, // all arcs from a state will add to one (thus sum of all paths from start to finish = 1 assuming no dead ends
                           NONE //
    } ;
    enum prior_group_by {
        FIXED, // don't rescale at all
        SINGLE, // all prior counts scale in same direction (for a transducer)
        LOCAL // scale each normgroup in a diff direction
    } ;

    static char const* norm_group_name (norm_group_by n)
    {
        return (n==CONDITIONAL) ? "Conditional"
            : (n==JOINT) ? "Joint"
            : "None";
    }

    static char const* priorgroup_name (prior_group_by p)
    {
        return (p==FIXED) ? "0 (fixed priors)" : (p==SINGLE) ? "1 (single prior scale)" : "2 (per-normgroup prior scale)";
    }

    struct NormalizeMethod
    {
        prior_group_by priorgroup;
        norm_group_by group;
        mean_field_scale scale;
        Weight add_count;
        NormalizeMethod() { set_default(); }
        void set_default()
        {
            group=CONDITIONAL;
            scale.set_default();
            add_count=0;
            priorgroup=SINGLE;
        }
        void parse_group(char c)
        {
            if (c=='j'||c=='J') group=JOINT;
            else if (c=='c'||c=='C') group=CONDITIONAL;
            else group=NONE;
        }
        void parse_priorgroup(char c)
        {
            if (c=='0') priorgroup = FIXED;
            else if (c=='1') priorgroup=SINGLE;
            else if (c=='2') priorgroup=LOCAL;
            else throw std::runtime_error("prior-groupby characters must be 0 (no scaling), 1 (same scaling for whole xdcr), or 2 (separate scaling for each normgroup)");
        }

        typedef std::string const& str;
        struct f_group
        {
            static void set(NormalizeMethod &m,char c)
            {
                m.parse_group(c);
            }
            typedef char arg_type;
            static char const* get(NormalizeMethod &m) { return norm_group_name(m.group); }
        };
        struct f_scale
        {
            static void set(NormalizeMethod &m,double s)
            {
                m.scale.set_alpha(s);
            }
            typedef double arg_type;
            static arg_type const& get(NormalizeMethod &m) { return m.scale.alpha; }
        };
        struct f_prior
        {
            typedef Weight arg_type;
            static void set(NormalizeMethod &m,Weight const& w)
            {
                m.add_count=w;
            }
            static arg_type const& get(NormalizeMethod &m) { return m.add_count; }
        };
        struct f_priorgroup
        {
            typedef char arg_type;
            static void set(NormalizeMethod &m,char c)
            {
                m.parse_priorgroup(c);
            }
            static char const* get(NormalizeMethod &m) { return priorgroup_name(m.priorgroup); }
        };
    };

    WFST(const WFST &a) { throw std::runtime_error("No copying of WFSTs allowed!"); }
 public:


    int indexed_by;

    void index(int dir) {
        if (indexed_by!=dir) {
            indexed_by=dir;
            for ( int s = 0 ; s < numStates() ; ++s ) {
                states[s].flush();
                states[s].indexBy(dir);
            }
        }
    }
    void indexInput() {
        index(State::input);
    }
    void indexOutput() {
        index(State::output);
    }

    void project(int dir=State::input,bool identity_fsa=false) {
        if (identity_fsa)
            identity_alphabet_from(dir);
        for ( int s = 0 ; s < numStates() ; ++s )
            states[s].project(dir,identity_fsa);
    }

    void init_index()
    {
        indexed_by=State::none;
    }

    void indexFlush() { // index on input symbol or output symbol depending on composition direction
        init_index();
        for ( int s = 0 ; s < numStates() ; ++s ) {
            states[s].flush();
        }
    }
    WFST() { init(); named_states=0; final=0;states.push_back(); }
    //  WFST(const WFST &a):
    //ownerInOut(1), in(((a.in == 0)? 0:(NEW Alphabet(*a.in)))), out(((a.out == 0)? 0:(NEW Alphabet(*a.out)))),
    //stateNames(a.stateNames), final(a.final), states(a.states),

    WFST(istream & istr,bool alwaysNamed=false) {
        init();
        if (!this->readLegible(istr,alwaysNamed))
            final = invalid_state;
    }

    WFST(const string &str, bool alwaysNamed){
        init();
        if (!this->readLegible(str,alwaysNamed))
            final = invalid_state;
    }

    WFST(const char *buf); // make a simple transducer representing an input sequence
    WFST(const char *buf, int& length,bool permuteNumbers); // make a simple transducer representing an input sequence lattice - Yaser
    WFST(WFST &a, WFST &b, bool namedStates = false, bool preserveGroups = false);	// a composed with b
    WFST(cascade_parameters &cascade,WFST &a, WFST &b, bool namedStates = false,bool preserveGroups = false);	// a composed with b, but remembering in cascade the identities.  preserveGroups is meaningless since cascade keeps refs to original arcs anyway
    void set_compose(cascade_parameters &cascade,WFST &a, WFST &b, bool namedStates = false, bool preserveGroups = false);
    // resulting WFST has only reference to input/output alphabets - use ownAlphabet()
    // if the original source of the alphabets must be deleted

    /* cascade usage (compose then train original transducers):

    cascade_parameters cascade;
    cascade.prepare_compose(false,false);
    WFST x(cascade,a,b);
    cascade.prepare_compose(true,false);
    WFST y(cascade,x,c);
    cascade.prepare_compose(false,true);
    WFST z(cascade,d,y); // now you've done d*((a*b)*c)
    cascade.done_composing(z);
    z.train(cascade,...); // trains d,a,b,c parameters via paths in z explaining corpus

    */

    alphabet_type &alphabet(int dir)
    {
        return *alph[dir];
    }

    alphabet_type const&alphabet(int dir) const
    {
        return *alph[dir];
    }

    void listAlphabet(ostream &out, int dir = input);
    friend ostream & operator << (ostream &,  WFST &); // Yaser 7-20-2000
    // I=PathArc output iterator; returns length of path or -1 on error
    int randomPath(List<PathArc> *l,int max_len=-1) {
        return randomPath(l->back_inserter(), max_len);
    }
    // makes locally random choices; doesn't pick a path relative to sum-of-all-paths
    template <class I> int randomPath(I i,int max_len=-1)
    {
        PathArc p;
        int s=0;
        unsigned int len=0;
        unsigned int max=*(unsigned int *)&max_len;
        for (;;) {
            if (s == final)
                return len;
            if  ( len > max || states[s].arcs.isEmpty() )
                return -1;
            // choose random arc:
            Weight arcsum;
            typedef List<FSTArc> LA;
            typedef LA::const_iterator LAit;
            const LA& arcs=states[s].arcs;
            LAit start=arcs.const_begin(),end=arcs.const_end();
            for (LAit li = start; li!=end; ++li) {
                arcsum+=li->weight;
            }
            Weight which_arc = arcsum * randomFloat();
#ifdef DEBUG_RANDOM_GENERATE
            Config::debug() << " chose "<<which_arc<<"/"<<arcsum;
#endif
            arcsum.setZero();
            for (LAit li = start; li!=end; ++li) {
                if ( (arcsum += li->weight) >= which_arc) {
                    // add arc
                    setPathArc(&p,*li);
                    *i++ = p;
#ifdef DEBUG_RANDOM_GENERATE
                    Config::debug() << " arc="<<*li;
#endif
                    s=li->dest;
                    ++len;
                    //				if (!(i))
                    //					return -1;
                    goto next_arc;
                }
            }
            throw std::runtime_error("Failed to choose a random arc when generating random paths.  Since we remove useless states, this should never happen");
        next_arc:
            ++len;
        }
    }

    List<List<PathArc> > * randomPaths(int k, int max_len=-1); // gives a list of (up to) k paths
    // random paths to final
    // labels are pointers to names in WFST so do not
    // use the path after the WFST is deleted
    // list is dynamically allocated - delete it
    // yourself when you are done with it



    // Visitor needs to accept GraphArc (from makeGraph ... (FSTArc *)->data gives WFST FSTArc - see kbest.h for visitor description.  deprecated for visit_kbest
    template <class Visitor> void bestPaths(unsigned k,Visitor &v,bool throw_on_cycle=true) {
        Graph graph = makeGraph();
        graehl::bestPaths(graph,0,final,k,v,throw_on_cycle);
        freeGraph(graph);
    }

    // if you prefer to use a visitor that deals with FST arcs rather than graph arcs
    template <class V>
    struct arc_visitor
    {
        V *pv;
        bool SIDETRACKS_ONLY;

        arc_visitor(V &v) : pv(&v) {
            SIDETRACKS_ONLY=pv->SIDETRACKS_ONLY;
        }

        void start_path(unsigned k,double cost)
        {
            pv->start_path(k,Weight(cost,cost_weight()));
        }
        void end_path()
        {
            pv->end_path();
        }
        void visit_best_arc(const GraphArc &a)
        {
            pv->visit_best_arc(*(FSTArc*)a.data);
        }
        void visit_sidetrack_arc(const GraphArc &a)
        {
            pv->visit_best_arc(*(FSTArc*)a.data);
        }
    };

    template <class Visitor>
    void visit_kbest(unsigned k,Visitor &v,bool throw_on_cycle=true) {
        arc_visitor<Visitor> wrap_visitor(v);
        bestPaths(k,wrap_visitor,throw_on_cycle);
    }

    template <class Visitor>
    void visit_kbest(unsigned k,Visitor const &v,bool throw_on_cycle=true) {
        Visitor v2(v);
        visit_kbest(k,v,throw_on_cycle);
    }

    typedef dynamic_array<int> string_type;
    typedef dynamic_array<FSTArc *> path_type;


    static inline
    void path_yield_into(string_type &s,path_type const& path,int dir=WFST::input,bool skip_epsilon=true)
    {
        for (path_type::const_iterator i=path.begin(),e=path.end();i!=e;++i) {
            FSTArc const& a=**i;
            if (!(skip_epsilon && a.is_epsilon(dir)))
                s.push_back(a.symbol(dir));
        }
    }

    /*
    struct yield_from_path : public string_type
    {
        yield_from_path(path_type const& path,int dir=WFST::input,bool skip_epsilon=true)
        {
            WFST::path_yield_into(*this,path,dir,skip_epsilon);
        }
    };

    // path_yield_visitor<MyVisitor> yield(MyVisitor(),WFST::input,true); wfst.visit_kbest(10,yield);
    template <class V>
    struct path_yield_visitor : public string_type
    {
        enum { SIDETRACKS_ONLY=0 };
        int dir; // input or output
        bool skip_epsilon;
        V &v;
        path_yield_visitor(V &v,int dir=WFST::input,bool skip_epsilon=true) : v(v),dir(dir), skip_epsilon(skip_epsilon) {}

        void start_path(unsigned k,Weight w) { this->clear(); }
        void end_path() {
            v(*this);
        }
        void visit_best_arc(const FSTArc &a) { visit_arc(a); }
        void visit_sidetrack_arc(const FSTArc &a) { visit_arc(a); }
        void visit_arc(const FSTArc &a)
        {
            if (!(skip_epsilon && a.is_epsilon(dir)))
                this->push_back(a.symbol(dir));
        }
    };
    */
    /* the following is for dir=input.  reverse if dir=output.
       given
       taking all the input strings in transducer E=edit_distance_to
       produce a subset of all edits
    WFST(std::vector<int> const& string,WFST &edit_distance_to,Weight ins,Weight del,Weight subst,bool epsilon_outer=false,int dir=input)
    */



    struct annotated_path
    {
        path_type p;
        unsigned k;
        Weight w; // used as sort key, holds mbr score, provided to path_visitor, etc.
        Weight orig_w;

        annotated_path(unsigned k,Weight w) : k(k),w(w) {
            orig_w=w;
        }

        typedef annotated_path self_type;

        void swap_impl(self_type &a) throw() {
            swap(p,a.p);
            swap(w,a.w);
            swap(orig_w,a.orig_w);
            using std::swap;
            swap(k,a.k);
        }
        friend void swap(self_type &a,self_type &b) throw()
        {
            a.swap_impl(b);
        }

        // for sort best-first, so reverse.
        inline bool operator <(self_type const&o) const
        {
            return w > o.w;
        }

        inline int cmp(self_type const& o) const
        {
            return w.cmp(o.w);
        }

        // note: i didn't care to annotate w/ sidetrack vs. not (for -% path output) so everything is reported as a best_arc.  visitor must accept FSTArc &, not GraphArc &
        template <class V>
        void replay_to(V &v) const
        {
            v.start_path(k,w);
            for (path_type::const_iterator i=p.begin(),e=p.end();i!=e;++i)
                v.visit_best_arc(**i);
            v.end_path();
        }

    };

    typedef dynamic_array<annotated_path> annotated_paths_type;

    struct annotated_paths : public annotated_paths_type
    {
        annotated_paths(WFST &w,unsigned k,bool cc=true)
        {
            w.visit_kbest(k,*this,cc);
        }
        void start_path(unsigned k,Weight w)
        {
            this->push_back(annotated_path(k,w));
        }
        void end_path() {}
        enum { SIDETRACKS_ONLY=0 };

        void visit_sidetrack_arc(FSTArc &a)
        {
            visit_best_arc(a);
        }

        void visit_best_arc(FSTArc &a)
        {
            this->back().p.push_back(&a);
        }

        template <class V>
        void replay_to(V &v,unsigned i) const
        {
            Assert(i>0);
            (*this)[i-1].replay_to(v);
        }

        // show only the top k
        template <class V>
        void replay_first_k_to(V &v,unsigned max_k=(unsigned)-1) const
        {
            for (unsigned i=1,e=std::min(max_k,this->size());i<=e;++i)
                replay_to(v,i);
        }

        // for sort, descending order by weight (i.e. best first)
        static int compare(void const*a,void const*b)
        {
            annotated_path const& pa=*(annotated_path*)a;
            annotated_path const& pb=*(annotated_path*)b;
            return pb.cmp(pa);
        }


        void resort()
        {
//            std::sort(this->begin(),this->end()); // would need operator = for dynamic_array.
            std::qsort(this->begin(),this->size(),sizeof(annotated_path),compare);
        }

    };


    /* take the current WFSA (project on chosen direction) as a weighted distribution (the accepting paths are normalized).  alpha sharpens(>1)/softens(<1)/neutral(=1) (e^alph*a)/sum(e^(alph*a_i)).  then choose the highest weight path out of the top search_k under MBR edit distance, and emit the best rescored visit_k

    visitor V is called with:
    v.visit(WFST &w,unsigned k,unsigned pre_mbr_k,Weight mbr_score,path_type const& p,yield_type const& y)

    of course w,k,and y are redundant.  but i like you so they're given.

    */




    template <class V>
    void edit_distance_mbr(unsigned search_k,unsigned visit_k,V &v,double alpha=1.,int dir=WFST::input)
    {
        annotated_paths paths(*this,search_k,true);
        raisePower(alpha);

        //STUB:
        paths.replay_first_k_to(v,visit_k);

        raisePower(1./alpha);
    }


    void set_string(alphabet_type &a,string_type const& str,bool clone_alph=true)
    {
        assert(0);
        //TODO
    }



    // best paths to final
    // labels are pointers to names in WFST so do not
    // use the path after the WFST is deleted
    // list is dynamically allocated - delete it
    // yourself when you are done with it
    struct symbol_ids : public List<int>
    {
        symbol_ids(WFST & wfst,char const* buf,int output=0,int line=-1)
        {
            wfst.symbolList(this,buf,output,line);
        }
    };

    static inline bool is_special(int letter)
    {
        return letter>=start_normal_index;
    }

    static void print_yield(ostream &o,List<PathArc> const &path,bool output=false,bool show_special=false)
    {
        if (path.empty())
            return;
        graehl::word_spacer sp(' ');
        for (List<PathArc>::const_iterator li=path.const_begin(),end=path.const_end(); li != end; ++li ) {
            WFST const& w=*li->wfst;
            int id=li->symbol(output);
            if (show_special || !is_special(id))
                o << sp << w.letter(id,output);
        }
    }

    static void print_training_pair(ostream &o,List<PathArc> const &path,bool show_special=false)
    {
        print_yield(cout,path,false,show_special);
        o << endl;
        print_yield(cout,path,true,show_special);
        o << endl;
    }


    void symbolList(List<int> *ret,const char *buf, int output=0,int line=-1);
    // takes space-separated symbols and returns a list of symbol numbers in the
    // input or output alphabet
    char const* letter_or_eps(unsigned i,int dir,char const* eps="&#949",bool use_eps=true)
    {
        return (use_eps && i==epsilon_index) ? eps : letter(i,dir);
    }

    const char *inLetter(unsigned i) const {
        return letter(i,0);
    }
    const char *outLetter(unsigned i) const {
        return letter(i,1);
    }
    char const* letter(unsigned i,int dir) const
    {
        Assert ( i < alphabet(dir).size() );
        return alphabet(dir)[i].c_str();
    }

    //NB: uses static (must use or copy before next call) return string buffer if !named_states
    const char *stateName(int i) const {
        Assert ( i >= 0 );
        Assert ( i < numStates() );
        if (named_states)
            return stateNames[i].c_str();
        else
            return static_utoa(i);
    }
    Weight sumOfAllPaths(List<int> &inSeq, List<int> &outSeq);
    // gives sum of weights of all paths from initial->final with the input/output sequence (empties are elided)
    void randomScale() {  // randomly scale weights (of unlocked arcs) before training by (0..1]
        changeEachParameter(scaleRandom());
    }
    void randomSet() { // randomly set weights (of unlocked arcs) on (0..1]
        changeEachParameter(setRandom());
    }
    template <class V>
    void visit_arcs_sourceless(V &v) const
    {
        for (StateVector::const_iterator i=states.begin(),e=states.end();i!=e;++i) {
            i->visit_arcs(v);
        }
    }

    void set_constant_weights(Weight w=Weight::ONE())
    {
        changeEachParameter(set_constant_weight(w));
    }

    void zero_arcs()
    {
        set_constant_weights(Weight::ZERO());
    }

    // bool uniform_zero_normgroups=true -> if a group's arcs' weights are all 0, set them uniform instead of leaving them 0
    void normalize(NormalizeMethod const& method,bool uniform_zero_normgroups=false);

    // if weight_is_prior_count, weights before training are prior counts.  smoothFloor counts are also added to all arcs
    // NEW weight = normalize(induced forward/backward counts + weight_is_prior_count*old_weight + smoothFloor).
    // corpus may have examples w/ no derivations removed from it!

    struct random_restart_acceptor
    {
        /* when you ask for N random restarts, some of them may be rejected if
         * they're especially bad, according to a simple criteria: the corpus
         * prob. after 1 iteration must be at least as high as the prob. at the
         * same iteration in the start that holds the current best converged
         * point.  you can supply a tolerance (as a likelihood ratio with the
         * same meaning as converge_perplexity_ratio in train for slightly worse
         * restarts (i.e. tolerance .999 means start must be better than before,
         * 1.001 means it can be slightly worse), and an exponentiation rate r
         * for this tolerance, so that tolerance^(1/(i*r*N)) is the likelihood
         * ratio used at restart i from 1...N, i.e. tolerance is moved towards
         * 1 over time
         */
        Weight best_start; // perplexity, lower is better
        Weight tolerance; // >1 means allow worse, <1 means improvement must be significant
        Weight final_tolerance; // greater r moves tolerance toward 1 in later restarts
        double N; // number of restarts over which to vary
        Weight likelihood_ratio(unsigned i=0) const
        {
            if (i>=N)
                return final_tolerance;
            if (tolerance.isInfinity())
                return tolerance;
            return tolerance*(final_tolerance/tolerance).pow((i-1)/(N-1));
        }
        random_restart_acceptor() : tolerance(inf_weight()),final_tolerance(inf_weight()),N(0) {}
        random_restart_acceptor(double final_at_N,Weight tol,Weight final_tol=Weight::ZERO()) : tolerance(tol),final_tolerance(final_tol),N(final_at_N)
        {
            if (tolerance.isZero())
                tolerance.setInfinity();
            if (final_tolerance.isZero())
                final_tolerance=tolerance;
        }

        bool accept(Weight this_start,Weight converged_best,unsigned restart_i,std::ostream *o=0)
        {
            Weight lr=likelihood_ratio(restart_i);
            if (restart_i==0) {
                best_start=this_start;
                if (o)
                    *o << "Initial best start point ppx="<<this_start.as_base(2)<<std::endl;
                return true;
            }
            *o << "For restart "<<restart_i<<", ";
            Weight ppr=this_start.relative_perplexity_ratio(best_start);
            bool r=(lr>ppr);
            if (o)
                *o << (r?"accepting":"rejecting") <<" worse random start of "<<this_start.as_base(2)<<" compared to " <<best_start.as_base(2)<<" with relative ppx ratio="<<ppr<<" compared to target of "<<lr<<std::endl;
            return r;
        }
    };

    static void output_format(bool *flags,std::ostream *fstout=NULL); // flags as described in carmel -h

    enum { cache_nothing=0,cache_forward=1,cache_forward_backward=2,cache_disk=3,matrix_fb=4
    }; // for train_opts.  cache disk only caches forward since disk should be slower than recomputing backward from forward
  // matrix fb is deprecated - explicit intersection with derivations.h is MUCH better in sparse cases.
    struct deriv_cache_opts
    {
        std::string out_derivfile;
        bool do_prune;
        bool prune() const
        {
            return do_prune;
        }

        bool use_matrix() const
        {
            return cache_level == matrix_fb;
        }
        unsigned cache_level;
        std::string disk_cache_filename;
        size_t_bytes disk_cache_bufsize;
        bool use_disk() const
        {
            return cache_level == cache_disk;
        }
        bool cache() const
        {
            return cache_level != cache_nothing && cache_level != matrix_fb;
        }
        bool cache_backward() const
        {
            return cache_level == cache_forward_backward;
        }
        deriv_cache_opts() { set_defaults(); }
        void set_defaults()
        {
            do_prune=true;
            cache_level=cache_nothing;
            disk_cache_filename="/tmp/carmel.derivations.XXXXXX";
            disk_cache_bufsize=256*1024*1024;
        }
    };


    // TODO: move more of the train params into here
    struct train_opts
    {
        deriv_cache_opts cache;
        int max_iter;
        double learning_rate_growth_factor;
        int ran_restarts;
        random_restart_acceptor ra;

        train_opts() { set_defaults(); }
        void set_defaults()
        {
            max_iter=500;
            cache.set_defaults();
            learning_rate_growth_factor=1.;
            ran_restarts=0;
            ra=random_restart_acceptor();
        }
    };

    typedef dynamic_array<NormalizeMethod> NormalizeMethods;

    void train_gibbs(cascade_parameters &cascade, training_corpus &corpus,NormalizeMethods & methods,train_opts const& topt, gibbs_opts const& gopt,path_print const&printer=path_print(),double min_prior=1e-2);

    Weight train(training_corpus & corpus,NormalizeMethods const& methods,bool weight_is_prior_count, Weight smoothFloor,Weight converge_arc_delta, Weight converge_perplexity_ratio, train_opts const& opts);
    Weight train(cascade_parameters &cascade,training_corpus & corpus,NormalizeMethods const& methods,bool weight_is_prior_count, Weight smoothFloor,Weight converge_arc_delta, Weight converge_perplexity_ratio, train_opts const& opts,bool restore_old_weights=false);
    // set weights in original composed transducers (the transducers that were composed w/ the given cascade object and must still be valid for updating arcs/normalizing)
    // returns per-example perplexity achieved


//    Weight trainFinish();
    // stop if greatest change in arc weight, or per-example perplexity is less than criteria, or after set number of iterations.

    void invert();		// switch input letters for output letters
    void reduce();		// eliminate all states not along a path from
                                // initial state to final state
    void consolidateArcs(bool sum=true,bool clamp=true);	// combine identical arcs, with combined weight = sum, or just max if sum=false.  sum clamped to max of 1 if clamp=true
    void pruneArcs(Weight thresh);	// remove all arcs with weight < thresh
    enum {UNLIMITED=-1};
    void prunePaths(int max_states=UNLIMITED,Weight keep_paths_within_ratio=Weight::INF());
    // throw out rank states by the weight of the best path through them, keeping only max_states of them (or all of them, if max_states<0), after removing states and arcs that do not lie on any path of weight less than (best_path/keep_paths_within_ratio)


    void assignWeights(const WFST &weightSource); // for arcs in this transducer with the same group number as an arc in weightSource, assign the weight of the arc in weightSource.  if no arc having same group number in weightSource is found, remove the arc from *this
    unsigned numberArcsFrom(unsigned labelStart=1); // sequentially number each arc (placing it into that group) starting at labelStart - labelStart must be >= 1.  returns next available label
    void lockArcs();		// put all arcs in group 0 (weights are locked)
    //  void unTieGroups() { tieGroup.~HashTable(); PLACEMENT_NEW (&tieGroup) HashTable<IntKey, int>; }
    void unTieGroups();


    int generate(int *inSeq, int *outSeq, int minArcs, int maxArcs);
    BOOST_STATIC_CONSTANT(unsigned,invalid_state=(unsigned)-1);
    int valid() const { return ( final != invalid_state ); }
    unsigned int size() const { if ( !valid() ) return 0; else return numStates(); }
    unsigned n_edges() const
    {
        return numArcs();
    }
    unsigned numArcs() const {
        unsigned a = 0;
        for (int i = 0 ; i < numStates() ; ++i )
            a += states[i].size;
        return a;
    }

    //FIXME: you could run out of memory translating the FST into a graph just to report a summary.  topo sort and arc propagation from graph.h aren't that complicated to repeat (or abstract)

    // *p_n_back_edges = 0 iff no cycles (optional output arg)
    Weight numNoCyclePaths(unsigned *p_n_back_edges=0) {
        if ( !valid() ) return Weight();
        fixed_array<Weight> nPaths(numStates());
        Graph g = makeGraph();
        countNoCyclePaths(g, nPaths.begin(), 0,p_n_back_edges);
        delete[] g.states;
        Weight ret = nPaths[final];
        return ret;
    }

    bool isEmpty()
    {
        return states.empty();
    }

    struct weight_for_cost
    {
        typedef Weight result_type;
        Weight operator()(GraphArc const& a) const
        {
            return Weight(a.weight,cost_weight());
        }
    };

    Weight sum_acyclic_paths()
    {
        fixed_array<Weight> w(numStates());
        w[0]=1;
        Graph g = makeGraph();
        weight_for_cost f;
        propagate_paths(g,f,w,0);
        delete[] g.states;
        return w[final];
    }

    static void setIndexThreshold(int t) {
        if ( t < 0 )
            WFST::indexThreshold = 0;
        else
            WFST::indexThreshold = t;
    }

    //FIXME: these aren't technically const because they leave a mutable pointer to orig. arc, but can we make a const version for truly const uses?
    Graph makeGraph(); // weights = -log, so path length is sum and best path
    // is the shortest; GraphArc::data is a pointer
    // to the FSTArc it corresponds to in the WFST
    Graph makeEGraph(); // same as makeGraph, but restricted to *e* / *e* arcs

    //untested
    void ownAlphabet() {
        ownAlphabet(0);
        ownAlphabet(1);
    }

    //untested
    void stealAlphabet(WFST &from,int dir)
    {
        if (from.owner_alph[dir] && alph[dir]==from.alph[dir] ) { // && !owner_alph[dir] // unnecessary
            from.owner_alph[dir]=0;
            owner_alph[dir]=1;
        } else
            ownAlphabet(dir);
    }

    // untested
    void ownAlphabet(int dir)
    {
        if (!owner_alph[dir] && alph[dir]) {
            alph[dir]=NEW alphabet_type(*alph[dir]);
            owner_alph[dir]=1;
        }
    }

    void unNameStates() {
        if (named_states) {
            stateNames.~Alphabet();
            named_states=false;
            PLACEMENT_NEW (&stateNames) alphabet_type();
        }
    }

    void raisePower(double exponent=1.0)
    {
        if (exponent==1.0) return;
        for ( int s = 0 ; s < numStates() ; ++s )
            states[s].raisePower(exponent);
    }

    // returns id of newly added empty state
    unsigned add_state(char const* name="NEWSTATE")
    {
        unsigned r=states.size();
        states.push_back();
        if (named_states) {
            unsigned equals_r=stateNames.add_make_unique(name);
            Assert(equals_r==r);
        }
        return r;
    }

    void clear() {
        final = invalid_state;
        unNameStates();
        states.clear();
        destroy();
    }
    ~WFST() {
        destroy();
    }
    struct setRandom {
        void operator()(Weight *w) const {
            w->setRandomFraction();
        }
    };

    struct scaleRandom {
        void operator()(Weight *w) const {
            Weight random;
            random.setRandomFraction();
            *w *= random;
        }
    };

    struct set_constant_weight
    {
        Weight c;
        set_constant_weight(Weight c) : c(c) {}
        set_constant_weight(set_constant_weight const& o) : c(o.c) {}
        void operator()(Weight *w) const
        {
            *w=c;
        }
    };

    template <class F>
    F changeEachParameter(F f) {
        State::modify_parameter_once<F> m=f;
        for ( int s = 0 ; s < numStates() ; ++s )
            states[s].visit_arcs(s,m);
        return m;
    }

    typedef std::vector<Weight> saved_weights_t;

    struct param_saver
    {
        saved_weights_t &save;
        param_saver(saved_weights_t &save) : save(save) {}
        void operator()(unsigned source,FSTArc &a)
        {
            save.push_back(a.weight);
        }
    };


    typedef saved_weights_t::const_iterator saved_weight_p;

    struct param_restorer
    {
        saved_weight_p i;         //FIXME: end for boundscheck
        param_restorer(saved_weight_p i) : i(i) {}
        void operator()(unsigned source,FSTArc &a)
        {
            a.weight=*i++;
        }
    };


    // appends, so you can chain several store_weights on different WFST, just so long as you restore them in the same order
    void save_weights(saved_weights_t &save) const
    {
        //FIXME: instead of visit_arcs, changeEachParameter? (if large tie groups, you would benefit w/ less memory required to store)? ... remember, restore_weights must use the same method.  for now, I say not worth the overhead.
        param_saver s=save;
        const_cast<WFST&>(*this).visit_arcs(s);
    }

    // returns position of next unused weight, so you can use the same array for several WFST in a particular order
    saved_weight_p restore_weights(saved_weight_p i)
    {
        param_restorer r=i;
        return visit_arcs(r).i;
    }

    void restore_weights(saved_weights_t const& s)
    {
        restore_weights(s.begin());
    }

    void removeMarkedStates(bool marked[]);  // remove states and all arcs to
    // states marked true
    BOOST_STATIC_CONSTANT(int,no_group=FSTArc::no_group);
    BOOST_STATIC_CONSTANT(int,locked_group=FSTArc::locked_group);
    static inline bool isNormal(int groupId) {
        return FSTArc::normal(groupId);
    }
    static inline bool isLocked(int groupId) {
        return FSTArc::locked(groupId);
    }
    static inline bool isTied(int groupId) {
        return FSTArc::tied(groupId);
    }

    // v(unsigned source_state,FSTArc &arc)
    template <class V>
    V & visit_arcs(V & v)
    {
//        unsigned arcno=0;
        for (unsigned s=0,e=numStates();s<e;++s)
            states[s].visit_arcs(s,v);
        return v;
    }
    //FIXME: same as unTieGroups - keep just 1
    void clear_groups()
    {
        FSTArc::clear_group_f f;
        visit_arcs(f);
    }



 private:


    //	lastChange = train_maximize(method);
    // counts must have been filled in (happens in trainFinish) so not useful to public
    Weight train_maximize(NormalizeMethod const& method,double delta_scale=1); // normalize then exaggerate (then normalize again), returning maximum change

    void destroy()  // just in case we're sloppy, this is idempotent.  note: the actual destructor may not be - std::vector, etc.
    {
        deleteAlphabet();
    }

    void invalidate() {		// make into empty/invalid transducer
        clear();
    }
};

class NormGroupIter {
    WFST &wfst;
    State *begin;
    State *state;
    State *end;
    typedef HashTable<IntKey, List<HalfArc> >::iterator Cit;
    typedef List<HalfArc>::const_iterator Cit2;
    typedef List<FSTArc>::val_iterator Jit;
    Cit Ci;
    Cit2 Ci2,Cend;
    Jit Ji,Jend;
    const WFST::norm_group_by method;
    bool empty_state() { return state->size == 0; }
    void beginState() {
        if(method==WFST::CONDITIONAL)
            if (!empty_state())
                Ci = state->index->begin();
    }
 public:
    unsigned source()
    {
        return state-begin;
    }
    NormGroupIter(WFST::norm_group_by meth,WFST &wfst_) : wfst(wfst_),method(meth) {
        state=begin=&*wfst.states.begin();
        end=begin+wfst.numStates();
        beginState();
    }
    bool moreGroups() { return state != end; }
    template <class charT, class Traits>
    std::ios_base::iostate print(std::basic_ostream<charT,Traits>& os) const {
        if(method==WFST::CONDITIONAL) {
            os << "(conditional normalization group for input=" << wfst.inLetter(Ci->first) << " in ";
        } else if (method==WFST::JOINT) {
            os << "(joint normalizaton group for ";
        } else {
            os << "(no normalization ";
        }
        os << "state=" << wfst.stateName(index_of(wfst.states,state)) << ")";
        return std::ios_base::goodbit;
    }
    void beginArcs() {
        if(method==WFST::CONDITIONAL) {
            if (empty_state())
                return;
            Ci2 = Ci->second.const_begin(); // segfault w/ *e* selfloop as only arc
            Cend = Ci->second.const_end();
        } else {
            Ji = state->arcs.val_begin();
            Jend = state->arcs.val_end();
        }
    }
    bool moreArcs() {
        if(method==WFST::CONDITIONAL) {
            if (empty_state())
                return false;
            return Ci2 != Cend;
        } else {
            return Ji != Jend;
        }
    }
    FSTArc * operator *() {
        if(method==WFST::CONDITIONAL) {
            return *Ci2;
        } else {
            return &*Ji;
        }
    }
    void nextArc() {
        if(method==WFST::CONDITIONAL) {
            ++Ci2;
        } else {
            ++Ji;
        }
    }
    void nextGroup() {
        if(method==WFST::CONDITIONAL) {
            if ( !empty_state() )
                ++Ci;
            while (empty_state() || Ci == state->index->end()) {
                ++state;
                if (moreGroups())
                    beginState();
                else
                    break;
            }
        } else {
            ++state;
        }
    }
};

void warn_no_derivations(WFST const& x,IOSymSeq const& s,unsigned n);

ostream & operator << (ostream &o, WFST &w);

ostream & operator << (std::ostream &o, List<PathArc> &l);



}

#endif
