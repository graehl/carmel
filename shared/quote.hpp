#ifndef GRAEHL__SHARED__QUOTE_HPP
#define GRAEHL__SHARED__QUOTE_HPP

#include <algorithm>
#include <locale>
#include <algorithm>
#include <streambuf>
#include <sstream>
#include <iterator>
#include <boost/config.hpp>
#include <boost/lexical_cast.hpp>
#include <graehl/shared/char_predicate.hpp>

//#include <boost/integer_traits.hpp>

namespace graehl {

/// convenient usual " \ escaping conventions

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

template <class S,class Ch, class Tr>
inline void out_string_always_quote(std::basic_ostream<Ch,Tr> &out, S const& s)
{
    out << '"';
    for (typename S::const_iterator i=s.begin(),e=s.end();i!=e;++i) {
        char c=*i;
        if (c == '"' || c== '\\')
            out.put('\\');
        out.put(c);
    }
    out << '"';
}


template <class C,class Ch, class Tr>
inline void out_ensure_quote(std::basic_ostream<Ch,Tr> &out, const C& data) {
    typedef std::basic_string<Ch,Tr> S;
    S s=boost::lexical_cast<S>(data);
    if (s[0] == '"')
        out << s;
    else
        out_string_always_quote(out,s);
}

template <class C,class Ch, class Tr,class IsSpecial>
inline void out_quote(std::basic_ostream<Ch,Tr> &out, const C& data,IsSpecial is_special=IsSpecial()) {
    std::basic_stringstream<Ch,Tr> s;
    s << data;
    or_true_for_char<true_for_char<'\\'> > needs_quote('"',is_special);
    typedef std::istreambuf_iterator<Ch,Tr> i_iter;
    typedef std::ostream_iterator<Ch,Tr> o_iter;
    i_iter i(s),end;
    bool quote=(std::find_if(i,end,needs_quote) != end);
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

//////// more general/complicated: specify predicate for what to escape, backslash, and any output iter.

//static const std::size_t char_lookup_size=std::ctype<char>::table_size;

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

template <class Ch, class Tr,class Alloc>
inline void rewind(std::basic_stringstream<Ch,Tr,Alloc> &ss) 
{
    ss.clear();
    ss.seekg(0,std::ios::beg);
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

/// Out: std::ostream_iterator or other output iter
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




}//graehl

#endif
