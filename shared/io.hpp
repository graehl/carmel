#ifndef _IO_HPP
#define _IO_HPP

#include "genio.h"

// if you want custom actions/parsing while reading labels, make a functor with this signature and pass it as an argument to read_tree (or get_from):
template <class Label>
struct DefaultReader
{
  typedef Label value_type;
  template <class charT, class Traits>
        std::basic_istream<charT,Traits>&
         operator()(std::basic_istream<charT,Traits>& in,Label &l) const {
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

  EXPECTCH_SPACE_COMMENT_FIRST('(');
  for(;;) {
    EXPECTI_COMMENT(in>>c);
          if (c==')') {
                break;
          }
          in.unget();
#if 1
          typename std::iterator_traits<T>::value_type temp;
          if (deref(read)(in,temp).good())
                *out++=temp;
          else
                goto fail;
#else
          // doesn't work for back inserter for some reason
          if (!deref(read)(in,*&(*out++)).good()) {
                goto fail;
          }
#endif
          EXPECTI_COMMENT(in>>c);
          if (c != ',') in.unget();
  }
  return GENIOGOOD;
fail:
  return GENIOBAD;
}

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
    throw std::runtime_error("expected (a,b,c,d) or (a b c d) as range input");
}

#endif
