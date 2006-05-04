#ifndef SHELL_ESCAPE_HPP
#define SHELL_ESCAPE_HPP

#include <sstream>
#include <algorithm>
#include <string>

namespace graehl {

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
    bool no_escape=(find_if(i,end,is_shell_special<char>)==end);
    s.seekg(0,std::ios::beg);    
    if (no_escape)
        std::copy(std::istreambuf_iterator<Ch,Tr>(s),end,std::ostreambuf_iterator<Ch,Tr>(out));
    else {
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

template <class C>
inline std::string shell_quote(const C& data) {
    std::stringstream s;
    out_shell_quote(s,data);
    return s.str();
}

}

#endif
