#ifndef GRAEHL__SHARED__WORD_SPACER_HPP
#define GRAEHL__SHARED__WORD_SPACER_HPP

#include <sstream>
#include <string>

namespace graehl {
/// print spaces between each word (none before or after)
///
///usage: word_spacer sp; while(...) o << sp << ...;
///
/// alternative: word_spacer_sp; while(...) o << get_string() << ...;
struct word_spacer {
    bool first;
    char space_string[2];
    word_spacer(char space=' ') : first(true) { space_string[0]=space;space_string[1]=0;}
    const char *empty() const
    {
        return space_string+1;
    }
    const char *space() const
    {
        return space_string;
    }
    const char *get_string() 
    {
        if (first) {
            first=false;
            return empty();
        } else {
            return space();
        }
    }
    void reset() 
    {
        first=true;
    }
    template <class O>
    void print(O &o)
    {
        if (first)
            first=false;
        else
            o << space();
    }    
    template <class O> friend inline O& operator<<(O& o,word_spacer &me) 
    {
        me.print(o);
        return o;
    }
};

inline std::string space_sep_words(const std::string &sentence,char space=' ')
{
    std::stringstream o;
    std::string word;
    std::istringstream i(sentence);
    word_spacer sep(space);
    while (i >> word) {
        o << sep;
        o << word; //FIXME: why is this not calling operator <<, but trying to print string OR file_arg?
    }
    return o.str();
}

inline int compare_space_normalized(const std::string &a, const std::string &b)
{
    return space_sep_words(a).compare(space_sep_words(b));
}


//!< print before word.
template <char sep=' '>
struct word_spacer_c {
    bool first;
    word_spacer_c() : first(true) {}
    void reset() 
    {
        first=true;
    }
    template <class O>
    void print(O&o) 
    {
        if (first)
            first=false;
        else
            o << sep;
    }
    typedef word_spacer_c<sep> Self;
    static const char seperator=sep;
    template <class O> friend inline O& operator<<(O& o,Self &me) 
    {
        me.print(o);
        return o;
    }
};

}
#endif
