#ifndef GRAEHL__SHARED__INPUT_ERROR_HPP
#define GRAEHL__SHARED__INPUT_ERROR_HPP

#ifndef INPUT_ERROR_TELLG
# define INPUT_ERROR_TELLG 0
#endif

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>

namespace graehl {

#ifndef GRAEHL__ERROR_CONTEXT_CHARS
#define GRAEHL__ERROR_CONTEXT_CHARS 50
#endif
#ifndef GRAEHL__ERROR_PRETEXT_CHARS
#define GRAEHL__ERROR_PRETEXT_CHARS 20
#endif

inline void
throw_parse_error(std::string const& context,
                  std::string const& complaint="parse error",
                  std::string::size_type pos=std::string::npos)
{
    using namespace std;
    ostringstream o;
    o << complaint << ": ";
    if (pos==string::npos)
        o << context;
    else
        o << string(context,0,pos)<<"<ERROR HERE>"<<string(context,pos);
    throw runtime_error(o.str());
}

template <class C>
inline
C scrunch_char(C c,char with='/')
{
    switch(c) {
    case '\n':case '\t':
        return with;
    default:
        return c;
    }
}

template <class O,class C> inline
void output_n(O &o,const C &c,unsigned n)
{
    for (unsigned i=0;i<n;++i)
        o << c;
}

template <class Ic,class It,class Oc,class Ot> inline
void show_error_context(std::basic_istream<Ic,It>  &in,std::basic_ostream<Oc,Ot> &out,unsigned prechars=GRAEHL__ERROR_PRETEXT_CHARS,unsigned postchars=GRAEHL__ERROR_CONTEXT_CHARS) {
    char c;
    std::streamoff actual_pretext_chars=0;
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
    for(unsigned i=0;i<GRAEHL__ERROR_CONTEXT_CHARS;++i)
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

template <class Ic,class It> inline
void throw_input_error(std::basic_istream<Ic,It> &in,std::string const& error="",const char *item="input",unsigned number=0) {
    std::ostringstream err;
    err << "Error reading";
    if (item)
        err << ' ' << item << " # " << number;
    err << ": " << error << std::endl;
#if INPUT_ERROR_TELLG
    std::streamoff where(in.tellg());
#endif
    show_error_context(in,err);
     err
#if INPUT_ERROR_TELLG
   << "(file position " <<  where << ")"
#endif
         << std::endl;
    throw std::runtime_error(err.str());
}

}//ns


#endif
