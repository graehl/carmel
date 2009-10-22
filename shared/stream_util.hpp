#ifndef GRAEHL__SHARED__STREAM_UTIL_HPP
#define GRAEHL__SHARED__STREAM_UTIL_HPP

#include <iomanip>
#include <iostream>
#include <cmath>
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

namespace graehl {

// faster c++ stream -> OS read/write
inline void unsync_stdio()
{
    std::ios_base::sync_with_stdio(false);
}

// also don't coordinate input/output (for stream processing vs. interactive prompt/response)
inline void unsync_cout()
{
    unsync_stdio();
    std::cin.tie(0);
}



template <class I,class O>
void copy_stream_to(O &o,I&i)
{
    o << i.rdbuf();
}

template <class I>
void rewind_read(I &i)
{
    i.seekg(0,std::ios::beg);
}


#if 0
std::ostream &trunc_(std::ostream &st, const char *s)
{
    if (st.opfx()) {
        int l = strlen(s), w = st.width(0);
        if (l > w)
            if (st.flags() & ios::right)
                st.write(s + l - w, w);
            else
                st.write(s, w);
        else
            st << s;
        st.osfx();
    }
    return st;
}


/* //omanip = old gcc extension?
omanip<const char *> trunc(const char *s)
{
    return omanip<const char *>(trunc_, s);
}
*/
#endif

}//graehl


#endif
