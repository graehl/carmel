#ifndef GRAEHL_SHARED__PRINTLINES_HPP
#define GRAEHL_SHARED__PRINTLINES_HPP

namespace graehl {

template <class I,class O>
void printlines(O &o,I i,I end,const char *endl) 
{
    for (;i!=end;++i)
        o << *i << endl;
}

template <class I,class O>
void printlines(O &o,I const& i,const char *endl="\n") 
{
    printlines(o,i.begin(),i.end(),endl);
}

}

#endif
