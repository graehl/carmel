#ifndef IO_HPP
#define IO_HPP

#include "genio.h"

// if you want custom actions/parsing while reading labels, make a functor with this signature and pass it as an argument to read_tree (or get_from):
template <class Label>
struct DefaultReader
{
  typedef Label value_type;
  template <class charT, class Traits>
        std::basic_istream<charT,Traits>&
         operator()(std::basic_istream<charT,Traits>& in,value_type &l) const {
          return in >> l;
         }
};

struct DefaultWriter
{
  template <class charT, class Traits,class Label>
        std::basic_ostream<charT,Traits>&
         operator()(std::basic_ostream<charT,Traits>& o,const Label &l) const {
          return o << l;
         }
};

struct LineWriter
{
  template <class charT, class Traits,class Label>
        std::basic_ostream<charT,Traits>&
         operator()(std::basic_ostream<charT,Traits>& o,const Label &l) const {
      return o << l << std::endl;
         }
};

template <class W,class O=std::ostream>
struct BindWriter : public W
{
    O &o;
    BindWriter(O &o_,const W &w_=W()) : o(o_),W(w_) {}
    BindWriter(const BindWriter<O,W> &r) :o(r.o),W(r) {}
    template <class L>
    O &
    operator()(const L &l) const
    {
        return ((W*)this)->operator()(o,l);
    }
};


template <class Label>
    struct max_reader {
        typedef Label value_type;
        value_type max; // default init = 0
        template <class charT, class Traits>
        std::basic_istream<charT,Traits>&
        operator()(std::basic_istream<charT,Traits>& in,value_type &v) const {
            in >> v;
            if (max < v)
                max = v;
        }
    };


// can only be passed to class that itself reads things with a Reader get_from method.
template <class R>
struct IndirectReader
{
    IndirectReader() {}
    R reader;
    IndirectReader(const R& r) : reader(r) {}
    typedef void value_type; // not really

    template <class Target,class charT, class Traits>
        std::basic_istream<charT,Traits>&
         operator()(std::basic_istream<charT,Traits>& in,Target &l) const {
        return gen_extractor(in,l,reader);
    }
};

  template <class charT, class Traits, class T,class Writer>
        std::ios_base::iostate range_print_on(std::basic_ostream<charT,Traits>& o,T begin, T end,Writer writer,bool multiline=false)
  {
        o << '(';
        if (multiline) {
#define LONGSEP "\n "
          for (;begin!=end;++begin) {
           o << LONGSEP;
           deref(writer)(o,*begin);
          }
         o << "\n)";

         o << std::endl;
        } else {
          bool first=true;
          for (;begin!=end;++begin) {
                if (first)
                  first = false;
                else
                        o << ' ';
                deref(writer)(o,*begin);
          }
         o << ')';
        }
  return GENIOGOOD;
}

  // modifies out iterator.  if returns GENIOBAD then elements might be left partially extracted.  (clear them yourself if you want)
template <class charT, class Traits, class Reader, class T>
std::ios_base::iostate range_get_from(std::basic_istream<charT,Traits>& in,T &out,Reader read)

{

    char c;
    EXPECTI_COMMENT_FIRST(in>>c);
    if (c=='(') {
        for(;;) {
            EXPECTI_COMMENT(in>>c);
            if (c==')') {
                break;
            }
            in.unget();
#if 1
            //typename std::iterator_traits<T>::value_type
# define IFBADREAD                              \
            typename Reader::value_type   temp; \
            if (deref(read)(in,temp).good())    \
                *out++=temp;                    \
            else
#else
            // doesn't work for back inserter for some reason
# define IFBADREAD                                  \
            if (!deref(read)(in,*&(*out++)).good())
#endif
            IFBADREAD goto fail;
            EXPECTI_COMMENT(in>>c);
            if (c != ',') in.unget();
        }
        return GENIOGOOD;
    } else {
        in.unget();
        for(;;) {
            IFBADREAD {
                if (in.eof())
                    return GENIOGOOD;
                else
                    goto fail;
            }
        }
    }
fail:
  return GENIOBAD;
}
#undef IFBADREAD

// note: may attempt to read MORE than [begin,end) - looks for closing paren, fails if not found after no more than end-begin elements (throwing an exception on failure)
template <class charT, class Traits,class T>
T read_range(std::basic_istream<charT,Traits>& in,T begin,T end) {
#if 1
    bounded_iterator<T> o(begin,end);
    if (range_get_from(in,o,DefaultReader<typename std::iterator_traits<T>::value_type >()) != GENIOGOOD)
        goto fail;
    return o.base();
#else
    char c;

    EXPECTCH_SPACE_COMMENT_FIRST('(');
    for(;;) {
        EXPECTI_COMMENT(in>>c);
        if (c==')') {
            break;
        }
        in.unget();
        if (begin == end)
            throw std::out_of_range("read_range exceeded provided storage");
        in >> *begin;
        ++begin;
        EXPECTI_COMMENT(in>>c);
        if (c != ',') in.unget();
    }
    return begin;
#endif
  fail:
    throw std::runtime_error("expected e.g. ( a,b,c,d ) or (a b c d) as range input");
}


// hardcoded to look for input id=N so we don't need full boyer-moore algorithm
// not chartraits sensitive - assumes 0...9 coded in order.
template <class A,class I,class O>
void insert_byid(const A& vals,I &in,O &out)
{
    char c;
    unsigned N;
    enum {waiting_i,seen_i,seen_id,scan_number} state=waiting_i;
    while (in.get(c)) {
        switch(state) {
        case waiting_i:
            if (c=='i')
                state=seen_i;
            break;
        case seen_i:
            if (c=='d')
                state=seen_id;
            else
                state=waiting_i;
            break;
        case seen_id:
            if (c=='=') {
                N=0;
                state=scan_number;
            } else
                state=waiting_i;
            break;
        case scan_number:
            switch (c) {
            case '1':case '2':case '3':case '4':case '5':
            case '6':case '7':case '8':case '9':case '0':
                N*=10;
                N+=(c-'0');

                break;
            default:
                state=waiting_i;
#define OUTN do { deref(vals)(out,N); } while(0)
                OUTN;
            }
        }
        out << c;
    }
    if (state == scan_number) // file ends without newline.
                OUTN;
#undef OUTN
}


#endif
