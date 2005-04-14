#ifndef FILEHEADER_HPP
#define FILEHEADER_HPP

const char *file_header_prefix="$$$";

inline void ignore_header(std::istream &i,const char *header_string=file_header_prefix)
{
    char c;
    if (i.get(c))
        if (c==header_string[0])
            while(i.get(c) && c!='\n');
        else
            i.unget();
}

inline std::ostream & write_header(std::ostream &o,const char *header_string=file_header_prefix)
{
    return o << header_string;
}

//!< if i starts with the first character of the header string, copy rest of
//line (but not newline) to o; otherwise, write the header string to o
inline void copy_header(std::istream &i, std::ostream &o,const char *header_string=file_header_prefix) {
    char c;
    if (i.get(c)) {
        if (c==header_string[0]) {
            do {
                o.put(c);
            } while(i.get(c) && c!='\n');
            return;
        } else
            i.unget();
    }
    o << header_string;
}

#endif
