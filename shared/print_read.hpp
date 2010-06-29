#ifndef GRAEHL__SHARED__PRINT_READ_HPP
#define GRAEHL__SHARED__PRINT_READ_HPP

#include <iostream>

/* usage:

struct T {
typedef T self_type;
    template <class charT, class Traits>
    void read(std::basic_istream<charT,Traits>& in)
    {
    }
    template <class charT, class Traits>
    void print(std::basic_ostream<charT,Traits>& o) const
   {
   }

   /// or, even shorter:

    template <class I>
    void read(I& in)
    {}

    template <class O>
    void print(O& o) const
    {}
};
*/

#define TO_OSTREAM_PRINT                                                                     \
    template <class Char,class Traits> \
    inline friend std::basic_ostream<Char,Traits> & operator <<(std::basic_ostream<Char,Traits> &o, self_type const& me)     \
    { me.print(o);return o; } \
    typedef self_type has_print;

#define FROM_ISTREAM_READ                                                 \
    template <class Char,class Traits> \
    inline friend std::basic_istream<Char,Traits>& operator >>(std::basic_istream<Char,Traits> &i,self_type & me)     \
    { me.read(i);return i; }

#define TO_OSTREAM_PRINT_FREE(self_type) \
    template <class Char,class Traits> inline \
    std::basic_ostream<Char,Traits> & operator <<(std::basic_ostream<Char,Traits> &o, self_type const& me)      \
    { me.print(o);return o; } \

#define FROM_ISTREAM_READ_FREE(self_type)                                                    \
    template <class Char,class Traits> inline                                                              \
    std::basic_istream<Char,Traits>& operator >>(std::basic_istream<Char,Traits> &i,self_type & me)     \
    { me.read(i);return i; }
#endif
