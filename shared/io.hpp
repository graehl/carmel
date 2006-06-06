// helpful template functions related to input/output.  important concept: Reader and Writer-two argument functors read(in,val) and write(out,val)
#ifndef GRAEHL__SHARED__IO_HPP
#define GRAEHL__SHARED__IO_HPP

#include <graehl/shared/genio.h>
#include <graehl/shared/funcs.hpp>

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <graehl/shared/has_print.hpp>
#include <iostream>
#include <graehl/shared/shell_escape.hpp>
#include <graehl/shared/word_spacer.hpp>
#ifdef TEST
#include <graehl/shared/test.hpp>
#include <cstring>
#endif

namespace graehl {

template <class O>
struct bound_printer 
{
    O *po;
    template <class T>
    void operator()(T const& t) const 
    {
        *po << t;
    }
};

template <class O>
bound_printer<O>
make_bound_printer(O &o)
{
    bound_printer<O> ret;
    ret.po=&o;
    return ret;
}


template <class T>
void extract_from_filename(const char *filename,T &to) {
    std::ifstream in(filename);
    if (!in)
        throw std::string("Couldn't read file ") + filename;
    else {
        in >> to;
    }
}


template <class Value,class Set,class Ch, class Tr>
inline bool parse_range_as(std::basic_istream<Ch,Tr> &in,Set &set) {
    char c;
//    typedef typename Set::value_type Value;    
    Value a,b;
    while(in) {
        if (!(in>>c)) break;
        bool remove=false;
        if (c!='^') {            
            if (c!='\\')
                in.unget();
        } else
            remove=true;
        if(!(in>>a)) break;
        if (remove)
            set.erase(a);
        else
            set.insert(a);
        if (!(in>>c)) break;        
        if (c=='-') {
            if (!(in>>b)) break;
            for(;a<=b;++a)
                if (remove)
                    set.erase(a);
                else
                    set.insert(a);
            if (!(in>>c)) break;
        }
    //post: c has a just gotten character that may be a , or the beginning of the next range (or a backslash)
        if (c!=',')
            in.unget();
    }
    return in.eof(); // only ok if we get here after consuming whole input
}

template <class Value,class Set,class Ch, class Tr>
inline bool parse_range_as(const std::basic_string<Ch,Tr> &in,Set &set) {
    std::basic_istringstream<Ch,Tr> instream(in);
    return parse_range_as<Value>(instream,set);
}


template <class Set>
inline bool parse_range(const std::string &range,Set &set) {
    std::istringstream is(range);
    return parse_range_as<typename Set::value_type>(is,set);
}




#define O_INSERTER(decl) std::basic_ostream<Ch,Tr> & operator <<(std::basic_ostream<Ch,Tr> & o, decl)

#define FREE_O_INSERTER(decl) template <class Ch,class Tr> inline O_INSERTER(decl)

#define FRIEND_O_INSERTER(decl) template <class Ch,class Tr> friend O_INSERTER(decl)

#define O_print  template <class Ch,class Tr> std::basic_ostream<Ch,Tr> & print(std::basic_ostream<Ch,Tr> & o)

template <char equal,char comma,class Ck,class Cv>
std::ostream& print_parallel_key_val(std::ostream &o,const Ck &K,const Cv &V) 
{
    typename Ck::const_iterator ik=K.begin(),ek=K.end();
    typename Cv::const_iterator iv=V.begin(),ev=V.end();
    o << '(';
    word_spacer_c<comma> sep;
    for(;ik<ek && iv<ev;++ik,++iv) {
        o << sep << *ik << equal << *iv;        
    }
    return o << ')';
}

template <class Ck,class Cv>
std::ostream& print_parallel_key_val(std::ostream &o,const Ck &K,const Cv &V) 
{
    return print_parallel_key_val<'=',',',Ck,Cv>(o,K,V);
}


template <class It,class Ch,class Tr>
inline std::basic_ostream<Ch,Tr> & print_sequence(std::basic_ostream<Ch,Tr> & o,It begin,It end) 
{
    word_spacer_c<' '> sep;
    o << "[";
    for (;begin!=end;++begin)
        o << sep << *begin;
    return o << "]";
}


template <class Cont,class Ch,class Tr>
inline std::basic_ostream<Ch,Tr> & print_sequence(std::basic_ostream<Ch,Tr> & o,const Cont &thing)
{
    return print_sequence(o,thing.begin(),thing.end());
}


template <class Cont,class Ch,class Tr>
inline std::basic_ostream<Ch,Tr> & print_default(std::basic_ostream<Ch,Tr> & o,const Cont &thing,typename
                                                 has_const_iterator<Cont>::type *SFINAE=0)
{
    return print_sequence(o,thing);
}

template <class Cont,class Ch,class Tr>
inline std::basic_ostream<Ch,Tr> & print_default(std::basic_ostream<Ch,Tr> & o,const Cont &thing,typename
                                                 not_has_const_iterator<Cont>::type *SFINAE=0) 
{
    return o << thing;
}

}

template <class V,class Ch,class Tr>
inline typename boost::enable_if<graehl::has_const_iterator<V>, std::basic_ostream<Ch,Tr> & >::type
operator <<(std::basic_ostream<Ch,Tr> & o,const V &thing)
{
    return print_default(o,thing);
}

namespace graehl {

#define USE_PRINT_SEQUENCE(C) \
template <class Ch,class Tr> \
inline std::basic_ostream<Ch,Tr> & operator <<(std::basic_ostream<Ch,Tr> & o,const C &thing) \
{ \
    return print_sequence(o,thing); \
}

#define USE_PRINT_SEQUENCE_TEMPLATE(C) \
template <class V,class Ch,class Tr>                                                         \
inline std::basic_ostream<Ch,Tr> & operator <<(std::basic_ostream<Ch,Tr> & o,const C<V> &thing) \
{ \
    return print_sequence(o,thing); \
}

// why not template?  because i think that we may conflict with other overrides
#define OLD_MAKE_CONTAINER_PRINT_FOR(C,T)                                           \
    inline std::ostream & operator <<(std::ostream &o,const C<T>& t) {     \
        word_spacer_c<> sep; \
        o << '[';                                                                                               \
        for (C<T>::const_iterator i=t.begin(),e=t.end();i!=e;++i) \
            o << sep << *i;                                            \
        return o << ']'; \
    }

//#define MAKE_VECTOR_PRINT_FOR(T) MAKE_CONTAINER_PRINT_FOR(std::vector,T)

//MAKE_VECTOR_PRINT_FOR(int)
//MAKE_VECTOR_PRINT_FOR(unsigned)
//MAKE_VECTOR_PRINT_FOR(std::string)

USE_PRINT_SEQUENCE_TEMPLATE(std::vector)

// header=NULL gives just the string, no newline


inline char hex_digit_char(unsigned char hex) {
    //assert(hex <= 0xF);
    // allowed to work for 0...36
    if (hex <= 9)
        return '0'+hex;
// if (hex > 0xF)
//    return '?';
    return 'A'+(hex-10);
}

// prints 4 chars: \x1B
// returns # of chars printed
template <class Ch, class Tr>
inline unsigned out_char_hex(std::basic_ostream<Ch,Tr> &out, unsigned char c) {
    char ls_hex=c & 0xF;
    char ms_hex=c >> 4; // unsigned; no sign extension
    out << "\\x" << hex_digit_char(ms_hex) << hex_digit_char(ls_hex);
    return 4;
}



// prints ^@ through ^_ for 0x0->0x1F, ^? for DEL (0x7F) and \x9A for high chars (>0x7F)
// returns number of chars print
template <class Ch, class Tr>
inline unsigned out_char_ascii(std::basic_ostream<Ch,Tr> &out, unsigned char c) {
    if (c < 0x20) {
        out << '^' << (c+'@');
        return 2;
    } else if (c < 0x7F) {
        out << c;
        return 1;
    } else if (c == 0x7F) {
        out << "^?";
        return 2;
    } else {
        return out_char_hex(out,c);
    }
}

// if you want custom actions/parsing while reading labels, make a functor with this signature and pass it as an argument to read_tree (or read):
template <class Label>
struct DefaultReader
{
  typedef Label value_type;
  template <class Ch, class Tr>
        std::basic_istream<Ch,Tr>&
         operator()(std::basic_istream<Ch,Tr>& in,value_type &l) const {
          return in >> l;
         }
};

template <class String=std::string>
struct getline_reader
{
    typedef String value_type;
    template <class Ch, class Tr>
    void operator()(std::basic_istream<Ch,Tr>& in,value_type &read_into) const {
        std::getline(read_into);
    }
};

    

struct DefaultWriter
{
  template <class Ch, class Tr,class value_type>
        std::basic_ostream<Ch,Tr>&
         operator()(std::basic_ostream<Ch,Tr>& o,const value_type &l) const {
          return o << l;
         }
};

struct LineWriter
{
  template <class Ch, class Tr,class Label>
        std::basic_ostream<Ch,Tr>&
         operator()(std::basic_ostream<Ch,Tr>& o,const Label &l) const {
      return o << l << std::endl;
         }
};

template <class F,class R>
struct ReaderCallback : public R
{
    F f;
    ReaderCallback(const R& reader,const F &func) : f(func),R(reader) {}
   template <class Ch, class Tr>
    std::basic_istream<Ch,Tr>&
   operator()(std::basic_istream<Ch,Tr>& in,typename R::value_type &l) const {
       deref(f)();
       std::basic_istream<Ch,Tr>& ret=R::operator()(in,l);
       return ret;
   }
 };

template <class Label,class F>
struct ProgressReader
{
    F tick;
    ProgressReader(const F &f) : tick(f) {}
    ProgressReader(const ProgressReader &p) : tick(p.tick) {}
    typedef Label value_type;
    template <class Ch, class Tr>
    std::basic_istream<Ch,Tr>&
    operator()(std::basic_istream<Ch,Tr>& in,value_type &l) const {
        deref(tick)();
        return in >> l;
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
        template <class Ch, class Tr>
        std::basic_istream<Ch,Tr>&
        operator()(std::basic_istream<Ch,Tr>& in,value_type &v) const {
            in >> v;
            if (max < v)
                max = v;
        }
    };


// can only be passed to class that itself reads things with a Reader read method.
template <class R>
struct IndirectReader
{
    IndirectReader() {}
    R reader;
    IndirectReader(const R& r) : reader(r) {}
    typedef typename R::value_type value_type;

    template <class Target,class Ch, class Tr>
        std::basic_istream<Ch,Tr>&
         operator()(std::basic_istream<Ch,Tr>& in,Target &l) const {
        return gen_extractor(in,l,reader);
    }
};

  template <class Ch, class Tr, class T,class Writer> inline
  std::ios_base::iostate range_print_iostate(std::basic_ostream<Ch,Tr>& o,T begin, T end,Writer writer,bool multiline=false,bool parens=true)
  {
      static const char *const MULTILINE_SEP="\n";
      const char space=' ';
      if (parens) {
          o << '(';
          if (multiline)
              o << MULTILINE_SEP;
      }
      if (multiline) {
          for (;begin!=end;++begin) {
              o << space;
              deref(writer)(o,*begin);
              o << MULTILINE_SEP;
          }
      } else {
          word_spacer_c<space> sep;
          for (;begin!=end;++begin) {
              o << sep;
              deref(writer)(o,*begin);
          }
      }
      if (parens) {
          o << ')';
          if (multiline)
              o << MULTILINE_SEP;
      }
      return GENIOGOOD;
}

template <class Ch, class Tr, class T,class Writer> inline
std::basic_ostream<Ch,Tr> & range_print(std::basic_ostream<Ch,Tr>& o,T begin, T end,Writer writer,bool multiline=false,bool parens=true)
{
    range_print_iostate(o,begin,end,writer,multiline,parens);
    return o;
}


template <class Ch, class Tr, class T>
inline  std::ios_base::iostate print_range(std::basic_ostream<Ch,Tr>& o,T begin, T end,bool multiline=false,bool parens=true) {
   return  range_print_iostate(o,begin,end,DefaultWriter(),multiline,parens);
}

  // modifies out iterator.  if returns GENIOBAD then elements might be left partially extracted.  (clear them yourself if you want)
template <class Ch, class Tr, class Reader, class T>
std::ios_base::iostate range_read(std::basic_istream<Ch,Tr>& in,T &out,Reader read)
{
    char c;
    EXPECTI_COMMENT_FIRST(in>>c);
    if (c=='(') {
        for(;;) {
//            EXPECTI_COMMENT(in>>c);
            if (!(in >> c))
                goto done;
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
            //FIXME:
            // doesn't work for back inserter for some reason
# define IFBADREAD \
                        if (!deref(read)(in,*&(*out++)).good())

#endif
            IFBADREAD goto fail;
            EXPECTI_COMMENT(in>>c);
            if (c != ',') in.unget();
        }
        goto done;
    } else {
        in.unget();
        for(;;) {
            IFBADREAD {
                if (in.eof())
                    goto done;
                else
                    goto fail;
            }
        }
    }
fail:
  return GENIOBAD;
done:
  return GENIOGOOD;
}
#undef IFBADREAD

// note: may attempt to read MORE than [begin,end) - looks for closing paren, fails if not found after no more than end-begin elements (throwing an exception on failure)
template <class Ch, class Tr,class T>
T read_range(std::basic_istream<Ch,Tr>& in,T begin,T end) {
#if 1
    bounded_iterator<T> o(begin,end);
    if (range_read(in,o,DefaultReader<typename std::iterator_traits<T>::value_type >()) != GENIOGOOD)
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
    const int waiting_i=0,seen_i=1,seen_id=2,scan_number=3; // can't get enum to work in gcc-4    
    int state=waiting_i;
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
        out.put(c);
    }
    if (state == scan_number) // file ends without newline.
                OUTN;
#undef OUTN
}

template <class Func>
inline void split_noquote(
    const std::string &csv,
    Func f,
    const std::string &delim=","
    )
{
    using namespace std;
    string::size_type pos=0,nextpos;
    string::size_type delim_len=delim.length();
//    DBP2(delim,delim_len);
    while((nextpos=csv.find(delim,pos)) != string::npos) {
//        DBP4(csv,pos,nextpos,string(csv,pos,nextpos-pos));        
        if (! f(string(csv,pos,nextpos-pos)) )
            return;
        pos=nextpos+delim_len;
    }
    if (pos!=0) {
//        DBP4(csv,pos,csv.length(),string(csv,pos,csv.length()-pos));
        f(string(csv,pos,csv.length()-pos));
    }    
}

#ifdef TEST
char *split_strs[]={"",",a","",0};
char *seps[]={";",";;",",,","   ","=,",",=",0};

BOOST_AUTO_UNIT_TEST( TEST_io )
{
    using namespace std;
    {
        split_noquote(";,a;",make_expect_visitor(split_strs),";");
        for (char **p=seps;*p;++p) {
            string s;
            for (char **q=split_strs;*q;++q)
                s.append(*q);
            split_noquote(s,make_expect_visitor(split_strs),*p);
        }
    }
    
}
#endif

} //graehl
#endif
