// preprocessor macros for building strings out of ostream << "a" << v << "b" - type expressions
#ifndef MAKESTRING_HPP
#define MAKESTRING_HPP

#include <sstream>
#include <stdexcept>


#define MAKESTR_STRINGIZE(str) #str
#define MAKESTR_EXPAND_STRINGIZE(expr) MAKESTR_STRINGIZE(expr)
#define MAKESTR_FILE_LINE "(" __FILE__  ":" MAKESTR_EXPAND_STRINGIZE(__LINE__)  ")"
#define MAKESTR_DATE __DATE__ " " __TIME__

// __FILE__  ":"  __LINE__  "): failed to write " #expr
// usage: MAKESTRS(str,os << 1;os << 2) => str="12"
#define MAKESTRS(string,expr) do {std::ostringstream os;expr;if (!os) throw std::runtime_error(MAKESTR_FILE_LINE ": failed to write" #expr);string=os.str();}while(0)
// usage: MAKESTR(str,1 << 2) => str="12"
#define MAKESTR(string,expr) MAKESTRS(string,os << expr)
#define MAKESTRFL(string,expr) MAKESTR(string,MAKESTR_FILE_LINE << expr)

//DOESN'T WORK!
//#define MAKESTRE(expr) static_cast<std::ostringstream &>((*(std::ostringstream *)&std::ostringstream()) << "" <<  expr).str()
//#define MAKECSTRE(expr) MAKESTRE(expr).c_str()

// (obviously, no comma/newlines allowed in expr)


#ifdef TEST
#include "test.hpp"
#endif

#ifdef TEST_MAIN


BOOST_AUTO_UNIT_TEST( makestring )
{
    std::string s,e;
    e="12";
    MAKESTRS(s,os << "12");BOOST_CHECK(s==e);
    MAKESTRS(s,os << 1 << 2);BOOST_CHECK(s==e);
    MAKESTRS(s,os << 1;os << 2);BOOST_CHECK(s==e);

    //copy of MAKESTRS
    MAKESTR(s,"12");BOOST_CHECK(s==e);
    MAKESTR(s,1 << 2);BOOST_CHECK(s==e);
    MAKESTR(s,1;os << 2);BOOST_CHECK(s==e);

    //copy of MAKESTRS
//     BOOST_CHECK(MAKESTRE("12")==e);
//     BOOST_CHECK(MAKESTRE(1 << 2)==e);
//     BOOST_CHECK(MAKECSTRE('1' << '2')==e);
//     BOOST_CHECK(MAKECSTRE(1 << 2)==e);
   
#line 42
    MAKESTRFL(s,"asdf");BOOST_CHECK(s=="(makestr.hpp:42)asdf");
    
}
#endif

#endif
