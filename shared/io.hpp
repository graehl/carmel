// helpful template functions related to input/output.  important concept: Reader and Writer-two argument functors read(in,val) and write(out,val)
#ifndef IO_HPP
#define IO_HPP

#include "genio.h"
#include "funcs.hpp"

#include <locale>
#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include "has_print.hpp"
#include <streambuf>

template <class Ch, class Tr,class Alloc>
inline void rewind(std::basic_stringstream<Ch,Tr,Alloc> &ss) 
{
    ss.clear();
    ss.seekg(0,std::ios::beg);
}

struct false_for_all_chars 
{
    bool operator()(char c) const {
        return false;
    }    
};
    
template <class F>
struct or_true_for_char : public F {
    typedef or_true_for_char<F> self;
    char C;
    or_true_for_char(char thischar,const F &f=F()) : F(f), C(thischar) {}
    or_true_for_char(const self &s) : F(s),C(s.C) {}
    bool operator()(char c) const {
        return c == C || F::operator()(c);
    }
};

template <class F>
struct or_true_for_chars : public F {
    typedef or_true_for_chars<F> self;
    char C1,C2;
    or_true_for_chars(char c1,char c2,const F &f=F()) : F(f),C1(c1),C2(c2) {}
    or_true_for_chars(const self &s) : F(s),C1(s.C1),C2(s.C2) {}
    bool operator()(char c) const {
        return c == C1 || c == C2 || F::operator()(c);
    }
};

// SpecialChar(c) == true or c == escape_char -> escape
template <class SpecialChar,class Input,class Output>
inline Output copy_escaping(Input in,Input in_end,Output out,SpecialChar is_special,char escape_char='\\')
{
    or_true_for_char<SpecialChar> need_quote(escape_char,is_special);
    for (;in!=in_end;++in) {
        char c=*in;
        if (need_quote(c))
            *out++=escape_char;
        *out++=c;
    }
    return out;
}

//todo: template traits - but shouldn't matter for output!
template <class Ch,class Tr=std::char_traits<Ch> >
struct printed_stringstream : public std::basic_stringstream<Ch,Tr>
{
    typedef std::basic_stringstream<Ch,Tr> stream;
    typedef std::istreambuf_iterator<Ch,Tr> iterator;    
    stream& as_stream() 
    {
        return *(stream *)this;
    }
    const stream& as_stream() const
    {
        return *(const stream *)this;
    }
    // note: only one begin() can be iterated concurrently (each begin() destroys all current iterators)
    iterator begin()
    {
        rewind(as_stream());
        return as_stream();
    }
    static iterator end()
    {
        return iterator();
    }    
    template <class Data>
    printed_stringstream(const Data &data)
    {
        as_stream() << data;
    }
    template <class CharP>
    bool has_char(CharP p)  const
    {
        return find_if(begin(),end(),p) != end();
    }
};
    
template <class Data, class Output, class SpecialChar>
inline Output print_escaping(const Data &data,Output out,SpecialChar is_special=SpecialChar(),char escape_char='\\')
{
    printed_stringstream<typename Output::value_type> pr(data);
    copy_escaping(pr.begin(),pr.end(),out,is_special,escape_char);
}

// escape_char and quote_char automatically added to escape_inside_quotes
template <class Data, class Output, class QuoteProtChar,class SpecialChar>
inline Output print_quoting_if(QuoteProtChar quote_if,const Data &data,Output out,char quote_char='"',SpecialChar escape_inside_quotes=SpecialChar(),char escape_char='\\')
{
    printed_stringstream<typename Output::value_type> pr(data);
    if (pr.has_char(quote_if)) {
        or_true_for_char<SpecialChar> quote_or_special(quote_char,escape_inside_quotes);
        *out++=quote_char;
        out=copy_escaping(pr.begin(),pr.end(),out,quote_or_special,escape_char);
        *out++=quote_char;
        return out;
    } else
        return std::copy(pr.begin(),pr.end(),out);
}

// escape_char and quote_char automatically added to is_special
template <class Data, class Output, class SpecialChar>
inline Output print_maybe_quoting(const Data &data,Output out,char quote_char='"',SpecialChar is_special=SpecialChar(),char escape_char='\\')
{
    return print_quoting_if(or_true_for_chars<SpecialChar>(escape_char,quote_char,is_special),data,out,quote_char,is_special,escape_char);
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
template <class CharPred,class Ch, class Tr>
inline void change_ws(std::basic_istream<Ch,Tr> &in, CharPred pred, int mode=ctype_mod_ws::ADD)
{
    std::locale l;
    ctype_mod_ws *new_traits=new ctype_mod_ws(pred,mode);
    std::locale new_l(l, new_traits);
    in.imbue(new_l);
}
template <char C,class Ch, class Tr>
inline void add_ws(std::basic_istream<Ch,Tr> &in, int mode=ctype_mod_ws::ADD)
{
    change_ws(in,true_for_char<C>(),mode);
}
template <char C,class Ch, class Tr>
inline void replace_ws(std::basic_istream<Ch,Tr> &in, int mode=ctype_mod_ws::REPLACE)
{
    change_ws(in,true_for_char<C>(),mode);
}
template <char C,class Ch, class Tr>
inline void remove_ws(std::basic_istream<Ch,Tr> &in, int mode=ctype_mod_ws::REMOVE)
{
    change_ws(in,true_for_char<C>(),mode);
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

//!< print before word.
template <char sep=' '>
struct WordSeparator {
    bool first;
    WordSeparator() : first(true) {}
    void reset() 
    {
        first=true;
    }
    O_print 
    {
        if (first)
            first=false;
        else
            o << sep;
        return o;
    }
    typedef WordSeparator<sep> Self;
    static const char seperator=sep;
    FRIEND_O_INSERTER(Self &me) 
    {
        return me.print(o);
    }
};

template <char sep=' '>
struct IndentLevel {
    unsigned indent;
    IndentLevel() { reset(); }
    void reset() 
    {
        indent=0;
    }
    void in() 
    {
        ++indent;
    }
    void out()
    {
        //assert(indent!=0);
        --indent;
    }
    void operator ++() 
    {
        in();
    }
    void operator --()
    {
        out();
    }    
    std::ostream & newline(std::ostream &o) {
        o << std::endl;
        for (unsigned i=0;i<indent;++i)
            o << sep;
        return o;
    }
};

template <char equal,char comma,class Ck,class Cv>
std::ostream& print_parallel_key_val(std::ostream &o,const Ck &K,const Cv &V) 
{
    typename Ck::const_iterator ik=K.begin(),ek=K.end();
    typename Cv::const_iterator iv=V.begin(),ev=V.end();
    o << '(';
    WordSeparator<comma> sep;
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
    WordSeparator<' '> sep;
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

template <class V,class Ch,class Tr>
inline typename boost::enable_if<has_const_iterator<V>, std::basic_ostream<Ch,Tr> & >::type
operator <<(std::basic_ostream<Ch,Tr> & o,const V &thing)
{
    return print_default(o,thing);
}

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
        WordSeparator<> sep; \
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


template <class C,class Ch, class Tr>
inline std::basic_ostream<Ch,Tr> & out_shell_quote(std::basic_ostream<Ch,Tr> &out, const C& data) {
    std::stringstream s;
    s << data;
    char c;
    std::istreambuf_iterator<Ch,Tr> i(s),end;
    if (find_if(i,end,is_shell_special<char>)==end) {
        rewind(s);
        std::copy(std::istreambuf_iterator<Ch,Tr>(s),end,std::ostreambuf_iterator<Ch,Tr>(out));
    } else {
        out << '"';
        rewind(s);
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
template <class Ch, class Tr>
inline std::basic_ostream<Ch,Tr> & print_command_line(std::basic_ostream<Ch,Tr> &out, int argc, char *argv[], const char *header="COMMAND LINE:\n") {
    if (header)
        out << header;
    WordSeparator<' '> sep;
    for (int i=0;i<argc;++i) {
        out << sep;
        out_shell_quote(out,argv[i]);
    }
    if (header)
        out << std::endl;
    return out;
}

inline std::string get_command_line( int argc, char *argv[], const char *header="COMMAND LINE:\n") {
    std::ostringstream os;
    print_command_line(os,argc,argv,header);
    return os.str();
}

struct argc_argv : private std::stringbuf
{
    typedef char *arg_t;
    typedef arg_t *argv_t;
    
    std::vector<char *> argvptrs;
    int argc() const
    {
        return argvptrs.size();
    }
    argv_t argv() const
    {
        return argc() ? (char**)&(argvptrs[0]) : NULL;
    }
    void parse(const std::string &cmdline) 
    {
        argvptrs.clear();
        argvptrs.push_back("ARGV");
#if 1
        str(cmdline+" ");  // we'll need space for terminating final arg.
#else
        str(cmdline);
        seekoff(0,ios_base::end,ios_base::out);        
        sputc((char)' ');
#endif
        char *i=gptr(),*end=egptr();

        char *o=i;
        char terminator;
    next_arg:
        while(i!=end && *i==' ') ++i;  // [ ]*
        if (i==end) return;

        if (*i=='"' || *i=='\'') {
            terminator=*i;
            ++i;
        } else {
            terminator=' ';
        }
        
        argvptrs.push_back(o);

        while(i!=end) {
            if (*i=='\\') {
                ++i;
            } else if (*i==terminator) {
                *o++=0;
                ++i;
                goto next_arg;
            }
            *o++=*i++;
        }
        *o++=0;
    }
    argc_argv() {}
    explicit argc_argv(const std::string &cmdline) 
    {
        parse(cmdline);
    }
};


template <class C,class Ch, class Tr>
inline void out_always_quote(std::basic_ostream<Ch,Tr> &out, const C& data) {
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

template <class C,class Ch, class Tr>
inline void out_ensure_quote(std::basic_ostream<Ch,Tr> &out, const C& data) {
    typedef std::basic_string<Ch,Tr> String;
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

template <class C,class Ch, class Tr>
inline void out_quote(std::basic_ostream<Ch,Tr> &out, const C& data) {
    std::basic_stringstream<Ch,Tr> s;
    s << data;
    or_true_for_char<true_for_char<'\\'> > special('"');
    typedef std::istreambuf_iterator<Ch,Tr> i_iter;
    typedef std::ostream_iterator<Ch,Tr> o_iter;
    i_iter i(s),end;
    bool quote=(std::find_if(i,end,special) != end);
    rewind(s);
    if (quote) {
        out << '"';
        for (i_iter i(s);i!=end;++i) {
            Ch c=*i;
            if (c == '"' || c== '\\')
                out.put('\\');
            out.put(c);
        }
        out << '"';        
    } else {
//        std::copy(i_iter(s),end,o_iter(out));
        /*        
        for (i_iter i(s);i!=end;++i)
            out.put(*i);
        */
        Ch c;
        while(s.get(c))
            out.put(c);
    }
}

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

#ifndef ERROR_CONTEXT_CHARS
#define ERROR_CONTEXT_CHARS 50
#endif
#ifndef ERROR_PRETEXT_CHARS
#define ERROR_PRETEXT_CHARS 20
#endif

template <class C>
inline C scrunch_char(C c,char with='/') 
{
    switch(c) {
    case '\n':case '\t':
        return with;
    default:
        return c;
    }   
}

template <class O,class C>
inline void output_n(O &o,const C &c,unsigned n) 
{
    for (unsigned i=0;i<n;++i)
        o << c;
}

template <class Ic,class It,class Oc,class Ot>
inline void show_error_context(std::basic_istream<Ic,It>  &in,std::basic_ostream<Oc,Ot> &out,unsigned prechars=ERROR_PRETEXT_CHARS,unsigned postchars=ERROR_CONTEXT_CHARS) {
    char c;
    unsigned actual_pretext_chars=0;
    typedef std::basic_ifstream<Ic,It> fstrm;
//    if (fstrm * fs = dynamic_cast<fstrm *>(&in)) {
    bool ineof=in.eof();
    in.clear();
    in.unget();
    in.clear();
    std::streamoff before(in.tellg());
//    DBP(before);
    if (before>0) {        
        in.seekg(-(int)prechars,std::ios_base::cur);
        std::streamoff after(in.tellg());
        if (!in) {
            in.clear();
            in.seekg(after=0);
        }
        actual_pretext_chars=before-after;
    } 
        
    
//    } else {
//        actual_pretext_chars=0;
//    }
    in.clear();
    out << "INPUT ERROR: ";
    out << " reading byte #" << before+1;
    if (ineof)
        out << " (at EOF)";
    out << " (^ marks the read position):\n";
    const unsigned indent=3;
    output_n(out,'.',indent);
    unsigned ip,ip_lastline=0;
    for(ip=0;ip<actual_pretext_chars;++ip) {
        if (in.get(c)) {
            ++ip_lastline;
            out << scrunch_char(c);
/*        out << c;
            if (c=='\n') {
                ip_lastline=0;
            }
*/
        } else
            break;
    }
    
    if (ip!=actual_pretext_chars)
        out << "<<<WARNING: COULD NOT READ " << prechars << " pre-error characters (only got " << ip << ")>>>";
    for(unsigned i=0;i<ERROR_CONTEXT_CHARS;++i)
        if (in.get(c))
            out << scrunch_char(c);
        else
            break;

    out << std::endl;
    output_n(out,' ',indent);
    for(unsigned i=0;i<ip_lastline;++i)
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


// can only be passed to class that itself reads things with a Reader get_from method.
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
std::ios_base::iostate range_get_from(std::basic_istream<Ch,Tr>& in,T &out,Reader read)
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

inline std::string subspan(const std::string &s,std::string::size_type begin,std::string::size_type end) 
{
    return std::string(s,begin,end-begin);
}

    
template <class Func>
inline void     split_noquote(
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
#include "test.hpp"
#include <cstring>
#endif

#ifdef TEST
char *test_strs[]={"ARGV","ba","a","b c","d"," e f ","123",0};
char *split_strs[]={"",",a","",0};
char *seps[]={";",";;",",,","   ","=,",",=",0};

BOOST_AUTO_UNIT_TEST( TEST_io )
{
    using namespace std;
    {        
        string opts="ba a \"b c\" 'd' ' e f ' 123";
        argc_argv args(opts);
        BOOST_CHECK_EQUAL(args.argc(),7);        
        for (unsigned i=1;i<args.argc();++i) {
            CHECK_EQUAL_STRING(test_strs[i],args.argv()[i]);
        }    
    }
    {        
        string opts=" ba a \"\\b c\" 'd' ' e f '123 ";
        argc_argv args(opts);
        BOOST_CHECK_EQUAL(args.argc(),7);
        for (unsigned i=1;i<args.argc();++i) {
            CHECK_EQUAL_STRING(test_strs[i],args.argv()[i]);
        }    
    }
    {
        argc_argv args("");
        BOOST_CHECK_EQUAL(args.argc(),1);
    }
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

#endif
