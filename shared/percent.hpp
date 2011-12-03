#ifndef GRAEHL__SHARED__PERCENT_HPP
#define GRAEHL__SHARED__PERCENT_HPP

#include <graehl/shared/print_width.hpp>
#include <iosfwd>

namespace graehl {

template <int width=5>
struct percent
{
    double frac;
    percent(double f) : frac(f) {}
    percent(double num,double den) : frac(num/den) {}
    double get_percent() const
    {
        return frac*100;
    }
    template <class C, class T>
    void print(std::basic_ostream<C,T>& o) const
    {
        print_max_width_small(o,get_percent(),width-1);
        o << '%';
    }
    typedef percent<width> self_type;
};

template <class C, class T, int W>
std::basic_ostream<C,T>&
operator << (std::basic_ostream<C,T>& os, percent<W> const& p)
{
    p.print(os);
    return os;
}

template <int width=5>
struct portion
{
    double num,den;

    portion(double num,double den) : num(num),den(den) {}

    double get_fraction() const
    {
        return (double)num/den;
    }

    template <class C, class T>
    void print(std::basic_ostream<C,T>& o) const
    {
        o << percent<width>(num,den);
        o << " ("<<num<<"/"<<den<<")";
    }

};

template <class C, class T, int W>
std::basic_ostream<C,T>&
operator << (std::basic_ostream<C,T>& os, portion<W> const& p)
{
    p.print(os);
    return os;
}

}


#endif
