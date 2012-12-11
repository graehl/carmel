// robustly handle the ISI file header (first line of file="$$$...")
#ifndef FILEHEADER_HPP
#define FILEHEADER_HPP

#include <string>
#include <graehl/shared/funcs.hpp>

namespace graehl {

const std::string default_file_header_prefix="$$$";

template <class String> bool is_file_header(const String &s)
{
    return match_begin(s.begin(),s.end(),default_file_header_prefix.begin(),default_file_header_prefix.end());
}

// consumes input from i.  and compares it to s.
// return iterator to last char matched (s.end if all matched)
template <class Istream,class String>
inline typename String::const_iterator  match_input(Istream &i,const String &s) {
    typename String::const_iterator p=s.begin(),e=s.end();
    typedef typename String::value_type Char;
    Char c;
    while (i.get(c) && p!=e && c==*p)
        ++p;
    i.unget();
    return p;
}

//same as copy header but with null ostream
inline std::string ignore_header(std::istream &i,const std::string &header_string=default_file_header_prefix)
{
    std::string::const_iterator matched_until=match_input(i,header_string);
    char c;
    if (matched_until==header_string.end()) {
        while (i.get(c) && c!='\n') ;
        return std::string();
    } else
        return std::string(header_string.begin(),matched_until);
}

inline std::ostream & write_header(std::ostream &o,const std::string &header_string=default_file_header_prefix)
{
    return o << header_string;
}

// create a header line or append to an existing one, no matter what
//
//!< if i starts with the first character of the header string, copy rest of
//line (but not newline) to o; otherwise, write the header string to o
//if header not matched at all, restore i (unget), otherwise, return partially matched portion.
inline std::string copy_header(std::istream &i, std::ostream &o,
                          const std::string & print_if_new_header,
                          const std::string &header_string=default_file_header_prefix) {
    std::string::const_iterator matched_until=match_input(i,header_string);
//    DBP(matched_until-header_string.begin());
    o << header_string;
    if (matched_until==header_string.end()) {
//        DBPC("found header");
        char c;
        while (i.get(c) && c!='\n')
            o.put(c);
        return std::string();
    } else {
        o << print_if_new_header;
        return std::string(header_string.begin(),matched_until);
    }
}
}
#endif
