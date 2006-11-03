#ifndef GRAEHL__SHARED__INDENT_LEVEL_HPP
#define GRAEHL__SHARED__INDENT_LEVEL_HPP

namespace graehl {

struct indent_level {
    char tab;
    char const* bullet;
    unsigned indent;
    indent_level(char tab_=' ',char const* bullet_="") { reset(); }
    void reset(char tab_=' ',char const* bullet_="")
    {
        indent=0;
        tab=tab_;
        bullet=bullet_;
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
    template <class O>
    O& newline(O &o) {
        o << std::endl;
        for (unsigned i=0;i<indent;++i)
            o << tab;
        o << bullet;
        return o;
    }
};

}//graehl

#endif
