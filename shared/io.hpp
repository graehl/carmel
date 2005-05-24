#ifndef IO_HPP
#define IO_HPP

#include "genio.h"
#include "funcs.hpp"

#include <locale>
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

class ctype_mod_ws: public std::ctype<char>
{
 public:
    enum make_not_anon_14 {
        ADD,REPLACE,REMOVE
    };
    template <class CharPred>
    ctype_mod_ws(CharPred pred,int mode=REMOVE): std::ctype<char>(get_table(pred,mode)) {}
 private:
    static inline void clear_space(std::ctype_base::mask& c) { c &= ~space; }
    static inline void set_space(std::ctype_base::mask& c) { c &= ~space; }
    template <class CharPred>
    static std::ctype_base::mask* get_table(CharPred pred,int mode) {
        static std::ctype_base::mask rc[table_size];
        std::copy(classic_table(), classic_table() + table_size, rc);
        if (mode == REPLACE)
            std::for_each(rc, rc + table_size, clear_space);
        for(unsigned i=0;i<table_size;++i) {
            std::ctype_base::mask &m=rc[i];
            if (pred(i)) {
                if (mode==REMOVE)
                    clear_space(m);
                else
                    set_space(m);
            } else {
                if (mode==REPLACE)
                    clear_space(m);
            }
        }
        return rc;
    }

};

template <char C>
struct true_for_char {
    bool operator()(char c) const {
        return c == C;
    }
};

//note: you must free the new in locale's ctype! (or minor memory leak)
template <class CharPred,class charT, class Traits>
inline void change_ws(std::basic_istream<charT,Traits> &in, CharPred pred, int mode=ctype_mod_ws::ADD)
{
    std::locale l;
    ctype_mod_ws *new_traits=new ctype_mod_ws(pred,mode);
    std::locale new_l(l, new_traits);
    in.imbue(new_l);
}
template <char C,class charT, class Traits>
inline void add_ws(std::basic_istream<charT,Traits> &in, int mode=ctype_mod_ws::ADD)
{
    change_ws(in,true_for_char<C>(),mode);
}
template <char C,class charT, class Traits>
inline void replace_ws(std::basic_istream<charT,Traits> &in, int mode=ctype_mod_ws::REPLACE)
{
    change_ws(in,true_for_char<C>(),mode);
}
template <char C,class charT, class Traits>
inline void remove_ws(std::basic_istream<charT,Traits> &in, int mode=ctype_mod_ws::REMOVE)
{
    change_ws(in,true_for_char<C>(),mode);
}

template <class Value,class Set,class charT, class Traits>
inline bool parse_range_as(std::basic_istream<charT,Traits> &in,Set &set) {
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

template <class Value,class Set,class charT, class Traits>
inline bool parse_range_as(const std::basic_string<charT,Traits> &in,Set &set) {
    std::basic_istringstream<charT,Traits> instream(in);
    return parse_range_as<Value>(instream,set);
}


template <class Set>
inline bool parse_range(const std::string &range,Set &set) {
    std::istringstream is(range);
    return parse_range_as<typename Set::value_type>(is,set);
}


//!< print before word.
template <char sep=' '>
struct WordSeparator {
    bool first;
    WordSeparator() : first(true) {}
    std::ostream & print(std::ostream &o) {
        if (first)
            first=false;
        else
            o << sep;
        return o;
    }
};

template <char sep>
inline
std::ostream & operator <<(std::ostream &o, WordSeparator<sep> &separator) {
    return separator.print(o);
}

template <class Ck,class Cv>
std::ostream& print_parallel_key_val(std::ostream &o,const Ck &K,const Cv &V) 
{
    typename Ck::const_iterator ik=K.begin(),ek=K.end();
    typename Cv::const_iterator iv=V.begin(),ev=V.end();
    o << '(';
    WordSeparator<','> sep;
    for(;ik<ek && iv<ev;++ik,++iv) {
        o << sep << *ik << '=' << *iv;        
    }
    return o << ')';
}


// why not template?  because i think that we may conflict with other overrides
#define MAKE_CONTAINER_PRINT_FOR(C,T)                                           \
    inline std::ostream & operator <<(std::ostream &o,const C<T>& t) {     \
        WordSeparator<> sep; \
        o << '(';                                                                                               \
        for (C<T>::const_iterator i=t.begin(),e=t.end();i!=e;++i) \
            o << sep << *i;                                            \
        return o << ')'; \
    }

#define MAKE_VECTOR_PRINT_FOR(T) MAKE_CONTAINER_PRINT_FOR(std::vector,T)

MAKE_VECTOR_PRINT_FOR(int)
MAKE_VECTOR_PRINT_FOR(unsigned)
MAKE_VECTOR_PRINT_FOR(std::string)


template <class C>
inline bool is_shell_special(C c) {
    switch(c) {
    case ' ':case '\t':case '\n':
    case '\\':
    case '>':case '<':case '|':
    case '&':case ';':
    case '"':case '\'':case '`':
    case '~':case '*':case '?':case '{':case '}':
    case '$':case '!':case '(':case ')':
        return true;
    default:
        return false;
    }
}

template <class C>
inline bool needs_shell_escape_in_quotes(C c) {
    switch(c) {
    case '\\':case '"':case '$':case '`':case '!':
        return true;
    default:
        return false;
    }
}

template <class C,class charT, class Traits>
inline std::basic_ostream<charT,Traits> & out_shell_quote(std::basic_ostream<charT,Traits> &out, const C& data) {
    std::stringstream s;
    s << data;
    std::string str=s.str();

    char c;
    if (find_if(str.begin(),str.end(),is_shell_special<char>)==str.end()) {
        out << str;
    } else {
        out << '"';
        while (s.get(c)) {
            if (needs_shell_escape_in_quotes(c))
                out.put('\\');
            out.put(c);
        }
        out << '"';
    }
    return out;
}


// header=NULL gives just the string, no newline
template <class charT, class Traits>
inline std::basic_ostream<charT,Traits> & print_command_line(std::basic_ostream<charT,Traits> &out, int argc, char *argv[], const char *header="COMMAND LINE:\n") {
    if (header)
        out << header;
    WordSeparator<' '> sep;
    for (int i=0;i<argc;++i)
        out_shell_quote(out << sep,argv[i]);
    if (header)
        out << std::endl;
    return out;
}

inline std::string get_command_line( int argc, char *argv[], const char *header="COMMAND LINE:\n") {
    std::ostringstream os;
    print_command_line(os,argc,argv,header);
    return os.str();
}


template <class C,class charT, class Traits>
inline void out_always_quote(std::basic_ostream<charT,Traits> &out, const C& data) {
    std::stringstream s;
    s << data;
    char c;
    out << '"';
    while (s.get(c)) {
        if (c == '"' || c== '\\')
            out.put('\\');
        out.put(c);
    }
    out << '"';
}

template <class C,class charT, class Traits>
inline void out_ensure_quote(std::basic_ostream<charT,Traits> &out, const C& data) {
    typedef std::basic_string<charT,Traits> String;
    String s=boost::lexical_cast<String>(data);
    if (s[0] == '"')
        out << s;
    else {
        out << '"';
        for (typename String::iterator i=s.begin(),e=s.end();i!=e;++i) {
            char c=*i;
            if (c == '"' || c== '\\')
                out.put('\\');
            out.put(c);
        }
        out << '"';
    }
}

template <class C,class charT, class Traits>
inline void out_quote(std::basic_ostream<charT,Traits> &out, const C& data) {
    typedef std::basic_string<charT,Traits> String;
    String s=boost::lexical_cast<String>(data);
    if (s.find('"') == s.find('\\')) // == String::npos
        out << s;
    else {
        out << '"';
        for (typename String::iterator i=s.begin(),e=s.end();i!=e;++i) {
            char c=*i;
            if (c == '"' || c== '\\')
                out.put('\\');
            out.put(c);
        }
        out << '"';
    }
}

inline char hex_digit_char(unsigned char hex) {
    //assert(hex <= 0xF);
    if (hex <= 9)
        return '0'+hex;
// if (hex > 0xF)
//    return '?';
    return 'A'+(0xF & hex);
}

// prints 4 chars: \x1B
// returns # of chars printed
template <class charT, class Traits>
inline unsigned out_char_hex(std::basic_ostream<charT,Traits> &out, unsigned char c) {
    char ls_hex=c & 0xF;
    char ms_hex=c >> 4; // unsigned; no sign extension
    out << "\\x" << hex_digit_char(ms_hex) << hex_digit_char(ls_hex);
    return 4;
}



// prints ^@ through ^_ for 0x0->0x1F, ^? for DEL (0x7F) and \x9A for high chars (>0x7F)
// returns number of chars print
template <class charT, class Traits>
inline unsigned out_char_ascii(std::basic_ostream<charT,Traits> &out, unsigned char c) {
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

#ifndef ERROR_CONTEXT_CHARS
#define ERROR_CONTEXT_CHARS 50
#endif
#ifndef ERROR_PRETEXT_CHARS
#define ERROR_PRETEXT_CHARS 20
#endif

template <class Ic,class It,class Oc,class Ot>
inline void show_error_context(std::basic_istream<Ic,It>  &in,std::basic_ostream<Oc,Ot> &out) {
    char c;
    unsigned pretext_chars;
    typedef std::basic_ifstream<Ic,It> fstrm;
//    if (fstrm * fs = dynamic_cast<fstrm *>(&in)) {
    in.clear();
    in.unget();
     in.clear();
     std::streamoff before(in.tellg());
        in.seekg(-ERROR_PRETEXT_CHARS,std::ios_base::cur);
        std::streamoff after(in.tellg());
        if (!in) {
            in.clear();
            in.seekg(after=0);
        }
        pretext_chars=before-after;
//    } else {
//        pretext_chars=0;
//    }
    in.clear();
    out << "INPUT ERROR: ";
    out << " reading byte #" << before+1;
    out << " (^ marks the read position):\n...";
    unsigned ip;
    for(ip=0;ip<pretext_chars;++ip)
        if (in.get(c))
            out << c;
        else
            break;
    if (ip!=pretext_chars)
        out << "<<<WARNING: COULD NOT READ " << pretext_chars << " pre-error characters (wanted " << pretext_chars << ")>>>";
    for(unsigned i=0;i<ERROR_CONTEXT_CHARS;++i)
        if (in.get(c) && c!='\n')
            out << c;
        else
            break;

    out << std::endl << "   ";
    for(unsigned i=0;i<ip;++i)
        out << ' ';
    out << '^' << std::endl;
}

template <class Ic,class It>
void throw_input_error(std::basic_istream<Ic,It>  &in,const char *error="",const char *item="input",unsigned number=0) {
    std::ostringstream err;
    err << "Error reading " << item << " # " << number << ": " << error << std::endl;
    std::streamoff where(in.tellg());
    show_error_context(in,err);
//    err << "(file position " <<  where << ")" << std::endl;
    throw std::runtime_error(err.str());
}

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
  template <class charT, class Traits,class value_type>
        std::basic_ostream<charT,Traits>&
         operator()(std::basic_ostream<charT,Traits>& o,const value_type &l) const {
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

template <class F,class R>
struct ReaderCallback : public R
{
    F f;
    ReaderCallback(const R& reader,const F &func) : f(func),R(reader) {}
   template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
   operator()(std::basic_istream<charT,Traits>& in,typename R::value_type &l) const {
       deref(f)();
       std::basic_istream<charT,Traits>& ret=R::operator()(in,l);
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
    template <class charT, class Traits>
    std::basic_istream<charT,Traits>&
    operator()(std::basic_istream<charT,Traits>& in,value_type &l) const {
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
    typedef typename R::value_type value_type;

    template <class Target,class charT, class Traits>
        std::basic_istream<charT,Traits>&
         operator()(std::basic_istream<charT,Traits>& in,Target &l) const {
        return gen_extractor(in,l,reader);
    }
};

  template <class charT, class Traits, class T,class Writer>
  std::ios_base::iostate range_print_on(std::basic_ostream<charT,Traits>& o,T begin, T end,Writer writer,bool multiline=false,bool parens=true)
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
          bool first=true;
          WordSeparator<space> sep;
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

  template <class charT, class Traits, class T>
inline  std::ios_base::iostate print_range(std::basic_ostream<charT,Traits>& o,T begin, T end,bool multiline=false,bool parens=true) {
      return range_print_on(o,begin,end,DefaultWriter(),multiline,parens);
  }

  // modifies out iterator.  if returns GENIOBAD then elements might be left partially extracted.  (clear them yourself if you want)
template <class charT, class Traits, class Reader, class T>
std::ios_base::iostate range_get_from(std::basic_istream<charT,Traits>& in,T &out,Reader read)

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


#endif
