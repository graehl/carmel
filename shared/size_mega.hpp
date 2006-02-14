#ifndef SIZE_MEGA_HPP
#define SIZE_MEGA_HPP

#include <iomanip>

template <class size_type,class outputstream>
inline outputstream & print_size(outputstream &o,size_type size,bool decimal_thousand=true) {
    size_type thousand=decimal_thousand ? 1000 : 1024;
    if (size < thousand)
        return o << size;
    size_type base=thousand;
    const char *suffixes=decimal_thousand ? "kmgt" : "KMGT";
    const char *suff=suffixes;
    for(;;) {
        size_type nextbase=base*thousand;
        if (size < nextbase || suff[1]==0)
            return o << size/(double)base << *suff;
        base = nextbase;
        ++suff;
    }
    return o;
}

template <class Stream>
struct restore_stream 
{
    typedef Stream stream_type;
    stream_type *pstream;
    std::ios::fmtflags saved_flags;
    restore_stream(stream_type &stream) : pstream(&stream), saved_flags(stream.flags()) {}
    ~restore_stream() 
    {
        pstream->flags(saved_flags);
    }
};

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
        restore_stream<Ostream > save(o);
        o << std::setw(5);
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
            number *= (1000*1000*1000);
            break;
        case 'G':
            number *= (1024*1024*1024);
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


#endif
