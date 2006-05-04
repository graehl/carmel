#ifndef GRAEHL__SHARED__INDENT_LEVEL_HPP
#define GRAEHL__SHARED__INDENT_LEVEL_HPP

namespace graehl {

template <char sep=' '>
struct indent_level {
    unsigned indent;
    indent_level() { reset(); }
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
    template <class O>
    O& newline(O &o) {
        o << std::endl;
        for (unsigned i=0;i<indent;++i)
            o << sep;
        return o;
    }
};

}//graehl

#endif
