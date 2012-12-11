#ifndef GRAEHL__SHARED__SIZE_MEGA_HPP
#define GRAEHL__SHARED__SIZE_MEGA_HPP

#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/print_width.hpp>
#include <graehl/shared/print_read.hpp>
#include <graehl/shared/program_options.hpp>
#include <sstream>
#include <cstddef>
#include <string>

namespace graehl {

/// maxWidth, if positive, limits total number of characters. decimalThousand selects the 10^(3k) SI suffixes (k m g t) instead of 2^(10k) (K G M T)
template <class size_type,class outputstream>
inline outputstream & print_size(outputstream &o,size_type size,bool decimal_thousand=true,int max_width=-1) {
    typedef double size_compute_type;
    size_compute_type thousand=decimal_thousand ? 1000 : 1024;
    if (size < thousand)
        return o << size;
    size_compute_type base=thousand;
    const char *suffixes=decimal_thousand ? "kmgt" : "KMGT";
    const char *suff=suffixes;
    for(;;) {
        size_compute_type nextbase=base*thousand;
        if (size < nextbase || suff[1]==0) {
            double d=size/(double)base;
            print_max_width_small(o,d,max_width);
            return o << *suff;
        }

        base = nextbase;
        ++suff;
    }
    return o; // unreachable
}

template <class size_type>
size_type scale_mega(char suffix,size_type number=1) {
        switch(suffix) {
        case 't':
            number *= (1000.*1000.*1000.*1000.);
            break;
        case 'T':
            number *= (1024.*1024.*1024.*1024.);
            break;
        case 'g':
            number *= (1000.*1000*1000);
            break;
        case 'G':
            number *= (1024.*1024*1024);
            break;
        case 'm':
            number *= (1000*1000);
            break;
        case 'M':
            number *= (1024*1024);
            break;
        case 'k':
            number *=1000;
            break;
        case 'K':
            number *= 1024;
            break;
        default:
          throw std::runtime_error("unknown suffix - expected tTgGmMkK (tera giga mega kilo): "+std::string(1,suffix));
        }
        return number;
}

template <class size_type,class inputstream>
inline size_type parse_size(inputstream &i) {
    double number;
    if (!(i >> number))
        goto fail;
    char c;
    if (i.get(c))
      return (size_type)scale_mega(c,number);
    if (number - (size_type)number > 1)
        throw std::runtime_error(std::string("Overflow - size too big to fit: ").append(boost::lexical_cast<std::string>(number)));
    return (size_type)number;
fail:    throw std::runtime_error(std::string("Expected nonnegative number followed by optional k, m, g, or t (10^3,10^6,10^9,10^12) suffix, or K, M, G, or T (2^10,2^20,2^30,2^40), e.g. 1.5G"));
}

template <class size_type>
inline size_type size_from_str(std::string const &str) {
    std::istringstream i(str);
    size_type ret=parse_size<size_type>(i);
    must_complete_read(i,"Read a size_mega, but didn't parse whole string ");
    return ret;
}

template <class size_type>
inline void size_from_str(std::string const &str,size_type &sz) {
    sz=size_from_str<size_type>(str);
}


template <bool decimal_thousand=true,class size_type=double>
struct size_mega
{
    typedef size_mega<decimal_thousand,size_type> self_type;
    size_type size;
    operator size_type &()
    {
        return size;
    }
    operator size_type  () const
    {
        return size;
    }
    size_mega() : size() {}
    size_mega(self_type const& o) : size(o.size) {}
    size_mega(size_type size_) : size(size_) {}
    size_mega(std::string const& str,bool unused)
    {
        init(str);
    }
    void init(std::string const &str)
    {
        size=(size_type)size_from_str<size_type>(str);
    }

    self_type & operator =(std::string const&s)
    {
        init(s);
    }

    template <class Ostream>
    void print(Ostream &o) const
    {
//        local_precision<Ostream> prec(o,2);
//        o << std::setw(4);
        print_size(o,size,decimal_thousand,5);
    }
    TO_OSTREAM_PRINT
    FROM_ISTREAM_READ
    template <class I>
    void read(I &i)
    {
        /*
        std::string s;
        if (i>>s)
            init(s);
        else
            size=0;
        */
        size=parse_size<size_type>(i);
    }

};

typedef size_mega<false,double> size_bytes;
typedef size_mega<false,unsigned long long> size_bytes_integral;
typedef size_mega<false,std::size_t> size_t_bytes;
typedef size_mega<true,std::size_t> size_t_metric;
typedef size_mega<true,double> size_metric;


} //graehl

namespace boost {    namespace program_options {

inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     size_t* target_type, int)
{
    typedef size_t value_type;
    using namespace graehl;

    v=boost::any(graehl::size_from_str<value_type>(get_single_arg(v,values)));
}

}}


#endif
