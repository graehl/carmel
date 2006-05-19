#ifndef GRAEHL__SHARED__SIZE_MEGA_HPP
#define GRAEHL__SHARED__SIZE_MEGA_HPP

#include <iomanip>
#include <boost/lexical_cast.hpp>
#include <stdexcept>
#include <graehl/shared/stream_util.hpp>
#include <graehl/shared/program_options.hpp>

namespace graehl {

template <class size_type,class outputstream>
inline outputstream & print_size(outputstream &o,size_type size,bool decimal_thousand=true) {
    typedef double size_compute_type;
    size_compute_type thousand=decimal_thousand ? 1000 : 1024;
    if (size < thousand)
        return o << size;
    size_compute_type base=thousand;
    const char *suffixes=decimal_thousand ? "kmgt" : "KMGT";
    const char *suff=suffixes;
    for(;;) {
        size_compute_type nextbase=base*thousand;
        if (size < nextbase || suff[1]==0)
            return o << size/(double)base << *suff;
        base = nextbase;
        ++suff;
    }
    return o;
}

template <bool decimal_thousand=true,class size_type=double>
struct size_mega
{
    size_type size;
    operator size_type &() 
    {
        return size;
    }
    operator size_type  () const
    {
        return size;
    }
    explicit size_mega(size_type size_) : size(size_) {}
    template <class Ostream>
    friend Ostream & operator <<(Ostream &o,const size_mega &me) 
    {
        local_stream_flags<Ostream> save(o);
//        o << std::setprecision(2);
        o << std::setw(4);
        return print_size(o,me.size,decimal_thousand);
    }
};

typedef size_mega<true,double> size_megabytes;

template <class size_type,class inputstream>
inline size_type parse_size(inputstream &i) {
    double number;

    if (!(i >> number))
        goto fail;
    char c;
    if (i.get(c)) {
        switch(c) {
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
            goto fail;
        }
    }
    if (number - (size_type)number > 1)
        throw std::runtime_error(std::string("Overflow - size too big to fit: ").append(boost::lexical_cast<std::string>(number)));
    return (size_type)number;
fail:    throw std::runtime_error(std::string("Expected nonnegative number followed by optional k,m, or g (2^10,2^20,2^30) suffix."));
}

} //graehl

namespace boost {    namespace program_options {

inline void validate(boost::any& v,
                     const std::vector<std::string>& values,
                     size_t* target_type, int)
{
    typedef size_t value_type;
    using namespace graehl;

    std::istringstream i(boost::program_options::validators::get_single_string(values));
    v=boost::any(graehl::parse_size<value_type>(i));
    must_complete_read(i,"Read a size_mega, but didn't parse whole string ");
}

}}


#endif
