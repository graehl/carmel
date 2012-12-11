#ifndef GRAEHL_SHARED__STRHASH_H
#define GRAEHL_SHARED__STRHASH_H
#include <graehl/shared/config.h>
#include <iostream>

#include <graehl/shared/myassert.h>
#include <graehl/shared/dynarray.h>
#include <graehl/shared/2hash.h>
# include <graehl/shared/array.hpp>
# include <graehl/shared/random.hpp>

#include <graehl/shared/stringkey.h>
#include <boost/config.hpp>

#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>
#endif

namespace graehl {

class StringPool {
    typedef HashTable<StringKey, unsigned> HT;

#ifdef STRINGPOOL
    static HT counts;
#endif
 public:
    BOOST_STATIC_CONSTANT(bool,is_noop=0);
    static StringKey borrow(StringKey s) {
        if (s.isDefault())            return s;
#ifdef STRINGPOOL
        hash_traits<HT>::insert_result_type
            i=counts.insert(HT::value_type(s,1));
        StringKey &canonical=const_cast<StringKey&>(i.first->first);
        if (i.second)
            canonical.clone();
        else
            ++ i.first->second;
        return canonical;
#else
        s.clone();
        return s;
#endif
    }
    static void giveBack(StringKey s) {
        if (s.isDefault()) return;
#ifdef STRINGPOOL
        Assert(has_key(counts,s) && counts[s]>0);
        if (--*find_second(counts,s) == 0) {
            counts.erase(s);
            s.kill();
        }
#else
        s.kill();
#endif
    }
    ~StringPool()
    {
#ifdef STRINGPOOL
        for ( HT::iterator i=counts.begin(); i!=counts.end() ; ++i )
            ((StringKey &)i->first).kill();
#endif
    }
};

template <class Sym>
struct NoStringPool {
    BOOST_STATIC_CONSTANT(bool,is_noop=1);
    static Sym borrow(const Sym &s) {
        return s;
    }
    static void giveBack(const Sym &s) {
    }
};


// Sym must be memcpy-moveable, char * initializable (for operator () only), define == and hash<Sym>, and default initialize to Sym.isDefault() (i.e. Sym())== Sym::empty
template <class Sym=StringKey,class StrPool=NoStringPool<Sym> >
class Alphabet {
 public:
    typedef dynamic_array<Sym> SymArray;
 private:
    dynamic_array<Sym> names;
    typedef HashTable<Sym, unsigned> SymIndex;
    SymIndex ht;
 public:
    const dynamic_array<Sym> &symbols() const { return names; }
    Alphabet() { }
    Alphabet(Sym c) {
        add(c,0);
    }
    Alphabet(Sym c,Sym d)
    {
        add(c,0);
        add(d,1);
    }

    Alphabet(const Alphabet &a) {
#ifdef NODELETE
        memcpy(this, &a, sizeof(Alphabet));
#else
        for ( unsigned i = 0 ; i < a.names.size(); ++i )
            add(a.names[i],i);
#endif
    }
    template <class S,class StrP>
    bool operator ==(const Alphabet<S,StrP> &r) const {
        return r.symbols() == symbols();
    }
    void compact() {
        names.compact();
    }
    void dump() const {   Config::debug() << ht; }
    bool verify() const {
#ifdef DEBUG
        for (unsigned i = 0 ; i < names.size(); ++i ) {
            static char buf[1000];
            if (!names[i].isDefault()) {
                Assert(*find(names[i])==i);
                strcpy(buf,names[i].c_str());
                Assert(find(buf));
            }
        }
#endif
        return true;
    }
    void swap(Alphabet &a)
    {
        const size_t s = sizeof(Alphabet);
        char swapTemp[s];
        memcpy(swapTemp, this, s);
        memcpy(this, &a, s);
        memcpy(&a, swapTemp, s);
    }

    unsigned add(Sym const& s)
    {
        unsigned ret=names.size();
        add(s,ret);
        return ret;
    }

    unsigned add_make_unique(Sym const& s)
    {
        if (!have(s))
            return add(s);

        std::stringstream b;
        b << s.c_str();
        while (have(b.str()))
//            b << "X";
            b.put(random_alphanum());
        return add(b.str());
    }

    bool have(Sym const& s)
    {
        return find_second(ht,s);
    }


    // s must be new, and added at index n
    void add(Sym s,unsigned n) {
        Assert(find(s) == NULL);
#ifdef DEBUG_STRINGPOOL
        Config::debug() << "\nadding to alphabet: " <<s;
#endif

        if (!StrPool::is_noop)
            s = StrPool::borrow(s);
        //ht[s]=names.size();
        Assert(names.size()==n);
        graehl::add(ht,s,names.size());
#ifdef DEBUG_STRINGPOOL
        Config::debug() << " index="<<names.size();
#endif
        names.push_back(s);
#ifdef DEBUG_STRINGPOOL
        Config::debug() << " token="<<names.back() << " lookup_index(token)="<<*find(s)<<std::endl;
#endif
    }
    void reserve(unsigned n) {
        names.reserve(n);
    }
    unsigned const*find(Sym name) const {
        return find_second(ht,name);
    }
    bool is_index(unsigned pos) const {
        return pos < names.size();
    }
    unsigned index_of(Sym const& s) {
        return indexOf(s);
    }
    unsigned indexOf(Sym const& s) {
        //Assert(name);
        //Sym s = const_cast<char *>(name);

        typename hash_traits<SymIndex>::insert_result_type it;
        if ( (it = ht.insert(typename SymIndex::value_type(s,names.size()))).second ) {
            if (StrPool::is_noop)
                names.push_back(s);
            else
                names.push_back(*const_cast<Sym*>(&(it.first->first)) = StrPool::borrow(s)); // might seem naughty, (can't change hash table keys) but it's still equal.
        }
#ifdef DEBUG_STRINGPOOL
        Config::debug() << "got token="<<ht.find(s)->first << " lookup_index(token)="<<ht.find(s)->second<<std::endl;
#endif
        return it.first->second;
    }
    Sym operator[](unsigned pos) const {
        //Assert(pos < size() && pos >= 0);
        return names[pos];
    }
    // (explicit named_states flag in WFST) but is used in int WFST::getStateIndex(const char *buf)

    Sym operator()(unsigned pos) {
        unsigned iNum = pos;
        if (names.at_grow(iNum).isDefault()) {
            // decimal string for int
            Sym k = static_utoa(iNum);
            if (!StrPool::is_noop)
                k = StrPool::borrow(k);
            names[iNum] = k;
            ht[k]=iNum;
            Assert(names.size() > iNum);
            return k;
        } else
            return names[iNum];
    }

    // syncs hashtable in preparation for deleting entries from names array.  note: oldToNew must be derived from marked by array.hpp:indices_after_remove_marked already.  anything N or over is removed (marked is of size N).
    void removeMarked(bool marked[], int* oldToNew,unsigned N) {
        verify();
//            assert (N==names.size()); // just remove any excess over N
        for ( unsigned int i = 0 ; i < names.size() ; ++i ) {
            Sym const &s=names[i];
            if ( marked[i] || i >= N) {
                Assert(find_second(ht,s));
                ht.erase(s);
                Assert(!find_second(ht,s));
#ifdef DEBUG_STRINGPOOL
                Config::debug() << "removing from alphabet: " << s<<std::endl;
#endif
#ifndef NODELETE
                if (!StrPool::is_noop)
                    StrPool::giveBack(s);
#endif
            } else {
                unsigned & rI=ht.find(s)->second;
                rI=oldToNew[rI];
            }
        }
        remove_marked_swap(names,marked);
        verify();
    }
    void clear() {
        if (size() > 0) {
            giveBackAll();
            names.clear();
            ht.clear();
        }
    }
    void mapTo(const Alphabet &o, int *aMap) const
    // aMap will give which letter in Alphabet o the letters in a
    // correspond to, or -1 if the letter is not in Alphabet o.
    {
        unsigned const  *ip;
        for ( unsigned i = 0 ; i < size() ; ++i )
            aMap[i] = (( ip = o.find(names[i])) ? (*ip) : -1 );
    }
    unsigned size() const { return names.size(); }
    ~Alphabet()
    {
        giveBackAll();
    }
    template<class T,class P>  friend std::ostream & operator << (std::ostream &out, Alphabet<T,P> &alph);
 private:
    void giveBackAll() {
#ifndef NODELETE
        if (!StrPool::is_noop)
            for ( typename SymArray::iterator i=names.begin(),end=names.end();i!=end;++i)
//  if ( *i != Sym::empty )
                StrPool::giveBack(*i);
#endif
    }

};


    template<class T,class P>
    inline std::ostream & operator << (std::ostream &out, Alphabet<T,P> &alph)
    {
        for ( unsigned int i = 0 ; i < alph.names.size() ; ++i )
            out << alph.names[i] << '\n';
        return out;
    }

#ifdef GRAEHL_TEST
    BOOST_AUTO_TEST_CASE( TEST_static_utoa )
    {
        BOOST_CHECK(!strcmp(static_utoa(0),"0"));
        BOOST_CHECK(!strcmp(static_utoa(3),"3"));
        BOOST_CHECK(!strcmp(static_utoa(10),"10"));
        BOOST_CHECK(!strcmp(static_utoa(109),"109"));
        BOOST_CHECK(!strcmp(static_utoa(190),"190"));
        BOOST_CHECK(!strcmp(static_utoa(199),"199"));
        BOOST_CHECK(!strcmp(static_utoa(1534567890),"1534567890"));
    }

#endif

}

#ifdef GRAEHL__SINGLE_MAIN
#include <graehl/shared/strhash.cc>
#endif

#endif
