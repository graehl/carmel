#ifndef GRAEHL_SHARED__MAX_WIDTH_HPP
#define GRAEHL_SHARED__MAX_WIDTH_HPP

#include <cmath>
#include <iomanip>
#include <iostream>
#include <graehl/shared/print_read.hpp>

namespace graehl {

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

// similar to print_width but only works (well) for 10000>d>1 i.e. for size_mega
template <class C, class T>
std::basic_ostream<C,T>&
print_width_small(std::basic_ostream<C,T>& o, double d, int width=4)
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


// for a positive exponent!
inline unsigned sig_for_exp(unsigned width,unsigned exp)
{
    unsigned dexp=(exp<100?2:3);
    int r=width-dexp-3; // . and e and +- (+ is mandatory w/ scientific, as is padding exp to 2 digits)
    return r>0?r:0;
}

/*
On the default floating-point notation, the precision field specifies the
maximum number of meaningful digits to display both before and after the decimal
point, while in both the fixed and scientific notations, the precision filed
specifies exactly how many digits to display after the decimal point, even if
they are trailing decimal zeros.

bug (w/ g++ lib): scientific forces minimum precision=2
*/
// minprec sig digits within at most width chars
template <class C, class T>
std::basic_ostream<C,T>&
print_width(std::basic_ostream<C,T>& o, double d, int width=6, int minprec=0)
{
    const double epsilon=1e-8;
//    return o << std::setprecision(width) << d;
    if (width>=20 || d==0. || width <=0) {
        o<<d;
        return o;
    }
    if (minprec<0)
        minprec=width/3;
    typedef std::basic_ostream<C,T> stream_t;
    local_stream_format<stream_t> save(o);
    double p=d;
    if (d<0) {
        p=-d;
        --width;
    }
    double wholes=std::log10(p*(1+epsilon)); //1: log=0, digits=1
    if (wholes<=width && d==(double)(int)d)
        return o<<d;
    if (p<1) {
        int a=(int)-wholes;
        const int dot0=2;
        int need=dot0+minprec+a;
        if (need >= width)
            return o << std::scientific << std::setprecision(sig_for_exp(width,a)-1) << d;
        return o << std::setprecision(width-dot0-a) << d;
    } else {
        int a=(int)wholes;
        int need=1+a;
        if (need > width)
            return o << std::scientific << std::setprecision(sig_for_exp(width,a)-1) << d;
        o << std::fixed;
        int need_dot=need+1;
        return o << std::setprecision(need_dot<width?width-need_dot:0) << d;
    }
}

struct width
{
  int chars;
  width(int chars=6) : chars(chars) {}
  width(width const& o) : chars(o.chars) {}
};

template <class C, class T>
std::basic_ostream<C,T>&
print(std::basic_ostream<C,T>& o, double d, int width)
{
  print_width(o,d,width);
  return o;
}

template <class O>
void print(O& o, double d, width const& w)
{
  print_width(o,d,w.chars);
}

template <class C, class T>
std::basic_ostream<C,T>&
print_max_width(std::basic_ostream<C,T>& o, double d, int width=6)
{
#if 1
    return print_width(o,d,width);
#else
    typedef std::basic_ostream<C,T> stream_t;
    local_stream_format<stream_t> save(o);
    if (width > 0) {
        double p=std::fabs(d);
        if (d<0)
            --width;
        int wholes=(int)std::log10(p); //1: log=0, digits=1
        int need=1 + (wholes<0?-wholes:wholes);
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
#endif
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

}

#ifdef SAMPLE
# undef SAMPLE
# include <fstream>
# include <iostream>
using namespace std;
using namespace graehl;
void d(double d,unsigned w=5)
{
    print_width(cout,d,w);cout << " .\n";
}

int main()
{
//    cout<<fixed<<setprecision(0)<<1.234;cout<<" .\n";
//    cout<<scientific<<setprecision(0)<<1.234e4;cout << " .\n";
    d(.0008123,5);

    double b=10;
    for (unsigned w=4;w<8;++w) {
        double x=5.4321;
        cout << "\n\nwidth="<<w<<":\n";
        for (unsigned i=0;i<8;++i,x*=b) {
            print_width(cout,x,w);
            cout << '\t';
            print_width(cout,1e99*x,w);
            cout << '\t';
            print_width(cout,1/x,w);
            cout << '\t';
            print_width(cout,-x,w);
            cout << '\t';
            print_width(cout,-1/x,w);
            cout << "\t.\n";
        }
    }

    return 0;

}
#endif

#endif
