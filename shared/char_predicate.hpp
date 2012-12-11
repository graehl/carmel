#ifndef GRAEHL__SHARED__CHAR_PREDICATE_HPP
#define GRAEHL__SHARED__CHAR_PREDICATE_HPP

/// COULD TEMPLATE FOR WIDE CHARS (lazy)

#include <limits.h>
#include <graehl/shared/predicate_compose.hpp>
#include <locale>
#include <string>
#include <boost/config.hpp>

namespace graehl {


// worth making into bitfield?
struct char_table
{
    BOOST_STATIC_CONSTANT(unsigned,size=UCHAR_MAX+1);   //std::ctype<char>::table_size
    bool table[size];
    typedef bool * iterator;
    typedef iterator const_iterator;
//    typedef bool const* const_iterator;
    iterator begin() const
    { return const_cast<bool*>(table); }
    iterator end() const
    { return begin()+size; }

    void operator |= (char_table const& o)
    {
        for (unsigned i=0;i<size;++i)
            if (o.table[i])
                table[i]=true;
    }
    void operator &= (char_table const& o)
    {
        for (unsigned i=0;i<size;++i)
            if (!o.table[i])
                table[i]=false;
    }
    void negate()
    {
        for (unsigned i=0;i<size;++i)
            table[i]=!table[i];
    }

    void yes(char c)
    { me()[c]=true; }
    void no(char c)
    { me()[c]=false; }

    bool & operator[](char c)
    { return table[(unsigned char)c]; }
    bool operator()(char c) const
    { return table[(unsigned char)c]; }

    void set_all(bool to=true)
    {
        for (unsigned i=0;i<size;++i)
            table[i]=to;
    }

    template <class It>
    void set_in(It i,It end,bool set_to=true)
    {
        for(;i!=end;++i)
            me()[*i]=set_to;
    }
    void set_in(std::string const&str,bool set_to=true)
    {
        set_in(str.begin(),str.end(),set_to);
    }

    template <class It>
    void set_in_excl(It i,It end,bool set_to=true)
    {
        set_all(!set_to);
        set_in(i,end,set_to);
    }
    void set_in_excl(std::string const&str,bool set_to=true)
    {
        set_in_excl(str.begin(),str.end(),set_to);
    }

 private:
    char_table & me()
    { return *this; }
};

struct char_predicate : public char_table,public std::unary_function<char,bool> {
    char_predicate() : char_table() {} // ensures!  init of table array to false
    char_predicate(char t1) : char_table()
    { yes(t1); }
    char_predicate(char t1,char t2) : char_table()
    { yes(t1);yes(t2); }
    template <class It>
    char_predicate(It begin,It end,bool set_to=true)
    {
        set_in(begin,end,true);
    }
    char_predicate(std::string const&str,bool set_to)
    {
        set_in_excl(str,set_to);
    }
    template <class F>
    char_predicate(F f) : char_table() {
        for (unsigned i=0;i<char_table::size;++i)
            table[i]=f((char)i);
    }
};

typedef predicate_ref<char_predicate> char_predicate_ref;

    //boost::integer_traits<char>::max+1;

// : public std::unary_function<char,bool>
template <char C>
struct true_for_char_c
{
    bool operator()(char c) const {
        return c == C;
    }
};

template <char C,char C2>
struct true_for_chars_c
{
    bool operator()(char c) const {
        return c == C || c == C2;
    }
};

struct false_for_all_chars
{
    bool operator()(char c) const {
        return false;
    }
};

template <class F>
struct or_true_for_char : public F {
    typedef or_true_for_char<F> self;
    char C;
    or_true_for_char(char thischar,const F &f=F()) : F(f), C(thischar) {}
    or_true_for_char(const self &s) : F(s),C(s.C) {}
    bool operator()(char c) const {
        return c == C || F::operator()(c);
    }
};

template <class F>
struct or_true_for_chars : public F {
    typedef or_true_for_chars<F> self;
    char C1,C2;
    or_true_for_chars(char c1,char c2,const F &f=F()) : F(f),C1(c1),C2(c2) {}
    or_true_for_chars(const self &s) : F(s),C1(s.C1),C2(s.C2) {}
    bool operator()(char c) const {
        return c == C1 || c == C2 || F::operator()(c);
    }
};

struct true_for_char {
    typedef bool result_type;
    char C1;
    true_for_char(char c1) : C1(c1) {}
    bool operator()(char c) const {
        return c == C1;
    }
};

struct true_for_chars {
    typedef bool result_type;
    char C1,C2;
    true_for_chars(char c1,char c2) : C1(c1),C2(c2) {}
    bool operator()(char c) const {
        return c == C1 || c == C2;
    }
};


} //graehl
#endif
