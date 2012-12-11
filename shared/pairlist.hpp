#ifndef GRAEHL__SHARED__PAIRLIST_HPP
#define GRAEHL__SHARED__PAIRLIST_HPP

#ifdef GRAEHL_TEST
#  include <graehl/shared/test.hpp>
#  include <graehl/shared/debugprint.hpp>
#  include <boost/lexical_cast.hpp>
#  define PAIRLIST_DEBUG(x) x
#else
#  define PAIRLIST_DEBUG(x)
#endif

#include <sstream>
#include <list>
#include <utility>
#include <stdexcept>
#include <iostream>
#include <sstream>
#include <graehl/shared/word_spacer.hpp>
#include <graehl/shared/stream_whitespace.hpp>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/program_options.hpp>
#include <cctype>

namespace graehl {


template <class O,class It,class Def> inline
void print_pairlist(O &o,It b,It end,Def const& default_val,bool always_print_default=false,char pair_sep=',',char key_val_sep=':')
{
    word_spacer sp(pair_sep);
    for (;b!=end;++b) {
        o << sp << b->first;
        if (always_print_default || b->second != default_val)
            o << key_val_sep << b->second;
    }
}

// writer for first only
template <class O,class It,class Def,class W1> inline
void print_pairlist_w1(O &o,W1 const& w1,It b,It end,Def const& default_val,bool always_print_default=false,char pair_sep=',',char key_val_sep=':')
{
    word_spacer sp(pair_sep);
    for (;b!=end;++b) {
        o << sp;
        w1(o,b->first);
        if (always_print_default || b->second != default_val)
            o << key_val_sep << b->second;
    }
}

// see also string_match.hpp: tokenize_key_val_pairs for an approach that splits substrings without left->right input extraction; parse_pairlist, to the contrary, allows extraction to contain consumed separators.   on the other hand, those separators are considered whitespace, and normal whitespace isn't, for the duration.

// calls F(key,val) for each of ",k1,k2:v2,k3,k2:v3 ...".  the , and : are optional, but the : must be there if v2 follows, so we know v2 is a value and not a key.  if no value follows, then default is used
// to enable parsing of strings, only pair_sep and key_val_sep count as 'whitespace' for the duration.  THIS MAY BE CONFUSING :)
// stop char may be gobbled up by vN parsing; if so, then add a final separator before it.
template <class I,class F> inline
void parse_pairlist(I &in,F const& f,char pair_sep=',',char key_val_sep=':',char stop='\n')
{
    using namespace std;
    local_whitespace<I> lw(in,true_for_chars(pair_sep,key_val_sep));
    local_stream_flags<I> sf(in);
    in.unsetf(ios_base::skipws);
    typedef typename F::first_argument_type Key;
    typedef typename F::second_argument_type Val;
    char c;
    Key k;
    Val v;
    bool first=true;
    while(in) {
        if (!(in.get(c))) return;
//        PAIRLIST_DEBUG(DBP(c));
        if (c!=pair_sep) {
            if (c==stop)
                return;
            if (first) {
                in.unget();
                first=false;
            } else {
                throw runtime_error("missing separator before next key in read_pairlist");
            }
        }
        if (in.get(c)) { // allow final "sep stop"
            if (c==stop) return;
            in.unget();
        }
        if (!(in >> k)) return;
        if (in.get(c)) {
//            PAIRLIST_DEBUG(DBP(c));
            if (c==key_val_sep) {
                if ((in >> v)) {
                    f(k,v);
                    continue;
                } else if (!in.eof())
                    throw runtime_error("couldn't read value in read_pairlist");
            } else {
                in.unget();
            }
        }
        f(k);
    }
}


template <class Cont>
struct insert_value
{
    template <class C,class V>
    static inline void insert(C &c,V const& v)
    {
        c.push_back(v);
    }
};

# define PAIRLIST_USE_INSERT_4(C) \
    template <class c1,class c2,class c3,class c4>      \
    struct insert_value<C<c1,c2,c3,c4> >                  \
    {                                                   \
        template <class Cont,class V>                   \
        static inline void insert(Cont &c,V const& v)   \
        { c.insert(v); }                                \
    };

PAIRLIST_USE_INSERT_4(std::map)
PAIRLIST_USE_INSERT_4(std::multimap)


//e.g. Pairlist = vector<pair<K,V> >
template <class Pairlist>
struct read_pairlist_callback
{
    Pairlist *p;
    typedef typename Pairlist::value_type value_type;
    typedef typename value_type::first_type first_argument_type;
    typedef typename value_type::second_type second_argument_type;
    second_argument_type default_val;
    read_pairlist_callback(Pairlist &pairlist, second_argument_type const& default_val, bool append=false)
        : p(&pairlist), default_val(default_val)
    { if (!append) p->clear(); }

    void operator()(first_argument_type const& key) const
    {
        (*this)(key,default_val);
    }
    void operator()(first_argument_type const& key,second_argument_type const& val) const
    {
//        p->push_back(std::make_pair(key,val));
        insert_value<Pairlist>::insert(*p,std::make_pair(key,val));
    }
};


template <class Pairlist,class V>
read_pairlist_callback<Pairlist> make_read_pairlist_callback(Pairlist &pairlist,V const& d,bool append=false)
{
    return read_pairlist_callback<Pairlist>(pairlist,d,append);
}

template <class I,class Pairlist,class Def> inline
void read_pairlist(I &in,Pairlist &p,Def const& default_val,char pair_sep=',',char key_val_sep=':',bool append=false,char stop='\n')
{
    parse_pairlist(in,make_read_pairlist_callback(p,default_val,append),pair_sep,key_val_sep,stop);
}

template <class List>
struct read_list_callback
{
    List *p;
    typedef typename List::value_type value_type;
    typedef typename value_type::first_type first_argument_type;
    typedef bool second_argument_type; // we won't use this
    read_list_callback(List &list,bool append=false)
        : p(&list)
    { if (!append) p->clear(); }

    void operator()(first_argument_type const& key) const
    {
        p->push_back(key);
    }
    void operator()(first_argument_type const& key,second_argument_type const& val) const
    {
        p->push_back(key);
    }
};

template <class List>
read_list_callback<List> make_read_list_callback(List &list,bool append=false)
{
    return read_list_callback<List>(list,append);
}


// warning: nul chars will trigger bool parsing.  c strings can't have nul chars anyway, and i'm too lazy to write a similar parse_list from parse_pairlist
template <class I,class List> inline
void read_list(I &in,List &p,char sep=',',bool append=false,char stop='\n')
{
    parse_pairlist(in,make_read_list_callback(p,append),sep,'\0',stop);
}

template <class K,class V>
class pairlist
{
    typedef pairlist<K,V> self_type;
 public:
    typedef std::pair<K,V> pair_type;
    typedef void is_pairlist;
    typedef std::vector<pair_type > list_type;
    list_type l;
    list_type & list()
    { return l; }
    list_type const& list() const
    { return l; }
    typedef typename list_type::iterator iterator;
    typedef typename list_type::value_type value_type;
    iterator begin()
    { return list().begin(); }
    iterator end()
    { return list().end(); }
    V default_val;
    char pair_sep,key_val_sep;
    pairlist(V const& default_val,char pair_sep,char key_val_sep)
        : default_val(default_val),pair_sep(pair_sep),key_val_sep(key_val_sep) {}

    template <class I>
    void read(I &in)
    {
        read_pairlist(in,list(),default_val,pair_sep,key_val_sep);
    }
    template <class O>
    void print(O &o,bool always_print_default=false) const
    {
        print_pairlist(o,list().begin(),list().end(),default_val,always_print_default,pair_sep,key_val_sep);
    }
    std::string usage() const
    {
        std::ostringstream o;
        o << "list of key"<<key_val_sep<<"val pairs, separated by '"<<pair_sep<<"', with missing "<<key_val_sep<<"val given the default value: "<<default_val<<" - e.g. k1"<<pair_sep<<"k2"<<key_val_sep<<"v2";
        return o.str();
    }

    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
};

template <class K,class V,char PairSep,char KVSep>
class pairlist_c : public pairlist<K,V>
{
    typedef pairlist<K,V> parent_type;
 public:
    pairlist_c() : parent_type(V(),PairSep,KVSep) {}
};

/*
template<class pairlist> inline
pairlist
lexical_cast(std::string const& s,typename pairlist::is_pairlist *tag=0)
{
    pairlist ret;
    std::istringstream i(s);
    ret.read(i);
    return ret;
}
*/
/*
template <class C,class T,class K,class V,char PairSep,char KVSep>
std::basic_istream<C,T>& operator >>(std::basic_istream<C,T> &in,pairlist_c<K,V,PairSep,KVSep> & me)
{
    me.read(in);
    return in;
}
*/

}//graehl


namespace boost {    namespace program_options {

template <class pairlist>
inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     pairlist* target_type, int,typename pairlist::is_pairlist *tag=0)
{
    typedef size_t value_type;
    using namespace graehl;

    std::istringstream i(boost::program_options::validators::get_single_string(values));

    pairlist p;
    p.read(i);
    v=boost::any(p);
    must_complete_read(i);
}

}}

# ifdef TEST
BOOST_AUTO_TEST_CASE(TEST_pairlist) {
    using namespace std;
    using namespace boost;
    using namespace graehl;

    typedef pairlist_c<string,string,',',':'> unk_tags_t;
    unk_tags_t unk_tags;
    unk_tags_t::list_type &l=unk_tags.list();
    {
        unk_tags=boost::lexical_cast<unk_tags_t>(string("NNP")) ;
        BOOST_CHECK(l.size()==1);
        BOOST_CHECK(l.front().first=="NNP");
    }
    {
    std::istringstream is("NNP");
    unk_tags.read(is);
    BOOST_CHECK(l.size()==1);
    BOOST_CHECK(!is.bad());
    BOOST_CHECK(l.front().first=="NNP");
    }

    {
    std::istringstream is("NNP:c");
    unk_tags.read(is);
    BOOST_CHECK(l.size()==1);
    BOOST_CHECK(!is.bad());
    BOOST_CHECK_EQUAL(l.front().first,"NNP");
    PAIRLIST_DEBUG(DBP(l.front().first));
    }
    {
    std::istringstream is(",NNP:a,");
    unk_tags.read(is);
    BOOST_CHECK(l.size()==1);
    BOOST_CHECK(!is.bad());
    BOOST_CHECK_EQUAL(l.front().first,"NNP");
    BOOST_CHECK_EQUAL(l.front().second,"a");

    }

    {
        std::istringstream is("NNP:a,NN:b");
    unk_tags.read(is);
    BOOST_CHECK(l.size()==2);
    BOOST_CHECK(!is.bad());
    unk_tags_t::iterator i=unk_tags.begin();
    pair<string,string> &f=*i,&s=*++i;
    BOOST_CHECK_EQUAL(f.first,"NNP");
    BOOST_CHECK_EQUAL(f.second,"a");
    BOOST_CHECK_EQUAL(s.first,"NN");
    BOOST_CHECK_EQUAL(s.second,"b");

    }

}
#endif


#endif
