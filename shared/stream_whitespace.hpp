#ifndef GRAEHL__SHARED__STREAM_WHITESPACE_HPP
#define GRAEHL__SHARED__STREAM_WHITESPACE_HPP

#include <locale>
#include <streambuf>
#include <iostream>
#include <algorithm>
#include <graehl/shared/char_predicate.hpp>
#include <boost/config.hpp>

namespace graehl {

struct ctype_table
{
    typedef std::ctype<char> CT;
    static inline void clear_space(std::ctype_base::mask& c) { c = c & (~CT::space); }
    static inline void set_space(std::ctype_base::mask& c) { c = c | CT::space; }
    typedef std::ctype_base::mask mask_type;

    mask_type rc[CT::table_size];
    operator mask_type *()
    { return rc; }

    BOOST_STATIC_CONSTANT(int,ADD=0);
    BOOST_STATIC_CONSTANT(int,REPLACE=1);
    BOOST_STATIC_CONSTANT(int,REMOVE=2);
    template <class CharPred,class WTF>
    ctype_table(CharPred pred,int mode,WTF const& wtf) {
        std::copy(wtf, wtf + CT::table_size, rc);
        if (mode == REPLACE)
            std::for_each(rc, rc + CT::table_size, clear_space);
        for(unsigned i=0;i<CT::table_size;++i) {
            std::ctype_base::mask &m=rc[i];
            if (pred(i)) {
                if (mode==REMOVE)
                    clear_space(m);
                else
                    set_space(m);
            } else {
                if (mode==REPLACE)
                    clear_space(m);
            }
        }
    }
};


// so we can init ctype_table FIRST.
class ctype_mod_ws: private ctype_table,public std::ctype<char>
{
 public:
    BOOST_STATIC_CONSTANT(int,ADD=ctype_table::ADD);
    BOOST_STATIC_CONSTANT(int,REPLACE=ctype_table::REPLACE);
    BOOST_STATIC_CONSTANT(int,REMOVE=ctype_table::REMOVE);

    template <class CharPred>
    ctype_mod_ws(CharPred pred,int mode=REMOVE): ctype_table(pred,mode,classic_table()),std::ctype<char>(rc) {}
};

//returns old locale.  when you restore_ws, the new facet will be freed, otherwise, leaks memory
template <class Tr,class CP>
inline std::locale change_ws(std::basic_istream<char,Tr> &in,CP pred,int mode=ctype_table::REPLACE)
{
    return in.imbue(std::locale(std::locale(), new ctype_mod_ws(pred,mode)));
}

template <class Ch, class Tr>
inline void restore_ws(std::basic_istream<Ch,Tr> &in,std::locale const& old_locale)
{
    in.imbue(old_locale);
}

template <class I>
struct change_whitespace
{
    I &i;
    std::locale old;
    enum {
        ADD=ctype_mod_ws::ADD,
        REPLACE=ctype_mod_ws::REPLACE,
        REMOVE=ctype_mod_ws::REMOVE
    };

    //    ctype_mod_ws ws;

    template <class CharPred>
    change_whitespace(I &i,CharPred pred,int mode=REPLACE) :
        i(i),
        //ws(pred,mode),
        old(change_ws(i,pred,mode)) {}

    void restore()
    { restore_ws(i,old); }

};

template <class I>
struct local_whitespace : public change_whitespace<I>
{
    ~local_whitespace()
    { this->restore(); }
    typedef change_whitespace<I> parent_type;

    template <class CharPred>
    local_whitespace(I &i,CharPred pred,int mode=parent_type::REPLACE) : parent_type(i,pred,mode) {}
};

} //graehl

#endif
