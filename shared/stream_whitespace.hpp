#ifndef GRAEHL__SHARED__STREAM_WHITESPACE_HPP
#define GRAEHL__SHARED__STREAM_WHITESPACE_HPP

#include <locale>
#include <streambuf>
#include <iostream>
#include <algorithm>

namespace graehl {

class ctype_mod_ws: public std::ctype<char>
{
 public:
    enum make_not_anon_14 {
        ADD,REPLACE,REMOVE
    };
    template <class CharPred>
    ctype_mod_ws(CharPred pred,int mode=REMOVE): std::ctype<char>(get_table(pred,mode)) {}
 private:
    static inline void clear_space(std::ctype_base::mask& c) { c &= ~space; }
    static inline void set_space(std::ctype_base::mask& c) { c &= ~space; }
    template <class CharPred>
    static std::ctype_base::mask* get_table(CharPred pred,int mode) {
        static std::ctype_base::mask rc[table_size];
        std::copy(classic_table(), classic_table() + table_size, rc);
        if (mode == REPLACE)
            std::for_each(rc, rc + table_size, clear_space);
        for(unsigned i=0;i<table_size;++i) {
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
        return rc;
    }

};

//note: you must free the new in locale's ctype! (or minor memory leak)
template <class CharPred,class Ch, class Tr>
inline void change_ws(std::basic_istream<Ch,Tr> &in, CharPred pred, int mode=ctype_mod_ws::ADD)
{
    std::locale l;
    ctype_mod_ws *new_traits=new ctype_mod_ws(pred,mode);
    std::locale new_l(l, new_traits);
    in.imbue(new_l);
}
template <char C,class Ch, class Tr>
inline void add_ws(std::basic_istream<Ch,Tr> &in, int mode=ctype_mod_ws::ADD)
{
    change_ws(in,true_for_char<C>(),mode);
}
template <char C,class Ch, class Tr>
inline void replace_ws(std::basic_istream<Ch,Tr> &in, int mode=ctype_mod_ws::REPLACE)
{
    change_ws(in,true_for_char<C>(),mode);
}
template <char C,class Ch, class Tr>
inline void remove_ws(std::basic_istream<Ch,Tr> &in, int mode=ctype_mod_ws::REMOVE)
{
    change_ws(in,true_for_char<C>(),mode);
}

} //graehl

#endif
