#ifndef GRAEHL__SHARED__STREAM_UTIL_HPP
#define GRAEHL__SHARED__STREAM_UTIL_HPP

#include <iomanip>
#include <iostream>
#include <cmath>
#include <graehl/shared/abs_int.hpp>

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

template <class Stream>
struct local_stream_flags
{
    typedef Stream stream_type;
    stream_type *pstream;
    std::ios::fmtflags saved_flags;
    local_stream_flags(stream_type &stream) : pstream(&stream), saved_flags(stream.flags()) {}
    ~local_stream_flags() 
    {
        pstream->flags(saved_flags);
    }
};

template <class Stream>
struct local_precision
{
    typedef Stream stream_type;
    stream_type *pstream;
    std::streamsize saved_prec;
    local_precision(stream_type &stream,unsigned prec) : pstream(&stream), saved_prec(stream.precision(prec)) {}
    local_precision(stream_type &stream) : pstream(&stream), saved_prec(stream.precision()) {}
    ~local_precision() 
    {
        pstream->precision(saved_prec);
    }
};

template <class O>
struct local_stream_format
{
    local_stream_flags<O> f;
    local_precision<O> p;
    local_stream_format(O &o) : f(o),p(o) {}
};

    

//FIXME: TEST
template <class C, class T>
std::basic_ostream<C,T>& 
print_max_width(std::basic_ostream<C,T>& o, double d, int width=6) 
{
    typedef std::basic_ostream<C,T> stream_t;
    local_stream_format<stream_t> save(o);
    if (width > 0) {
        double p=std::fabs(d);
        if (d<0)
            --width;
        int wholes=(int)std::log10(p); //1: log=0, digits=1
        int need=1 + abs(wholes);
        if (need > width) {
            int unit_e_exp=1+1+2;
            if (width >= unit_e_exp)
                o << std::scientific << std::setprecision(width-unit_e_exp);
        } else {
            o << std::fixed;
            int need_dot=need+1;
            if (need_dot < width)
                o << std::setprecision(width-need_dot);
            else
                o << std::setprecision(0);
        }
    }
    return o << d;
}

template <class C, class T>
std::basic_ostream<C,T>& 
print_max_width_small(std::basic_ostream<C,T>& o, double d, int width=4)
{
    typedef std::basic_ostream<C,T> stream_t;
    local_stream_format<stream_t> save(o);
    int p=0;
    if (width>0) {    
        if (d>=0 && d< 10000) {
            if (d < 10) {
                p=width-2;
            } else if (d<100) {
                p=width-3;
            } else if (d<1000) {
                p=width-4;
            }
        }
    }
    if (p>0)
        o << std::fixed << std::setprecision(p);
    return o << d;
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
