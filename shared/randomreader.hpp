// select random subsequences of input (without storing the whole thing)
#ifndef RANDOMREADER_HPP
#define RANDOMREADER_HPP

#include <graehl/shared/random.hpp>

namespace graehl {

template <class Label>
struct RandomReader
{
    typedef Label value_type;
    double prob_keep;
    RandomReader(double prob_keep_) : prob_keep(prob_keep_) {}
    RandomReader(const RandomReader<value_type> &r) : prob_keep(r.prob_keep) {}
    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &l) const {
            in >> l;
            while (in && random01() >= prob_keep) {
                in >> l;
            }
            return in;
    }
};

//FIXME: serious bug: last item always oversampled on delimeter (because we don't set eof/fail)
//FIXME: need for this (really parsing outside context) indicates that readers
//should return bool if they want a potential retry instead
template <class Label
        ,char terminator=')'
          >
struct RandomReaderTerm
{
    typedef Label value_type;
    double prob_keep;
    RandomReaderTerm(double prob_keep_) : prob_keep(prob_keep_) {}
    RandomReaderTerm(const RandomReaderTerm<value_type,terminator> &r) : prob_keep(r.prob_keep) {}
    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &l) const {
        in >> l;
        while (in && random01() >= prob_keep) {
#define END_IF_TERMINATOR(terminator)  \
                char c; \
                if (!(in >> c)) \
                    break; \
                in.unget(); \
                if (c==terminator) { \
                    break; \
                }
            END_IF_TERMINATOR(terminator);
//                    in.setstate(std::ios::failbit );  

            in >> l;
        }
        return in;
    }
};


template <class Reader
        ,char terminator=')'
          >
struct RandomReadWrapperTerm
{
    typedef typename Reader::value_type value_type;
    Reader reader;
    double prob_keep;
    RandomReadWrapperTerm(const Reader &reader_,double prob_keep_) : prob_keep(prob_keep_),reader(reader_) {}
    RandomReadWrapperTerm(const RandomReadWrapperTerm<value_type,terminator> &r) : prob_keep(r.prob_keep),reader(r.reader) {}
    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &l) const {
            in >> l;
            while (in && random01() >= prob_keep) {
                END_IF_TERMINATOR(terminator);
                deref(reader)(in,l);                // only difference
//                in >> l;
            }
            return in;
    }
};

template <class Label, class F
        ,char terminator=')'
          >
struct RandomReaderTermCallback
{
    typedef Label value_type;
    double prob_keep;
    F f;
    RandomReaderTermCallback(const F &f_,double prob_keep_) : prob_keep(prob_keep_),f(f_) {}
    RandomReaderTermCallback(const RandomReaderTermCallback<value_type,F,terminator> &r) : prob_keep(r.prob_keep),f(r.f) {}
    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &l)  {
            in >> l;
            while (in && random01() >= prob_keep) {
                END_IF_TERMINATOR(terminator);
#undef END_IF_TERMINATOR
                in >> l;
                deref(f)(l);                // only difference
            }
            return in;
    }
};

}

#endif
