// preprocessor macros for building strings out of ostream << "a" << v << "b" - type expressions
#ifndef GRAEHL__SHARED__MAKESTR_HPP
#define GRAEHL__SHARED__MAKESTR_HPP

/* most likely usage:
string s=MAKESTRE(1<<" "<<c);

equivalent of

string s=boost::lexical_cast<string>(1)+" "+boost::lexical_cast<string>(c)

and likely faster if we use something more C-locale-formatting-constant than ostringstream

*/
#include <sstream>
#include <stdexcept>

/// this is necessary to turn ints into c-strings using preprocessor, e.g. MAKESTR_FILE_LINE
#define MAKESTR_STRINGIZE(str) #str
#define MAKESTR_EXPAND_STRINGIZE(expr) MAKESTR_STRINGIZE(expr)
#define MAKESTR_FILE_LINE "(" __FILE__  ":" MAKESTR_EXPAND_STRINGIZE(__LINE__)  ")"
#define MAKESTR_DATE __DATE__ " " __TIME__

// usage: MAKESTRS(str,os << 1;os << 2) => str="12"
#define MAKESTRS2(string,o,expr) do {std::ostringstream o;expr;if (!o) throw std::runtime_error(MAKESTR_FILE_LINE ": failed to write" #expr);string=o.str();}while(0)
#define MAKESTRS(string,expr) MAKESTRS2(string,o,expr)

// usage: MAKESTRTO(str,1 << 2) => str="12"
#define MAKESTRTO(string,expr) MAKESTRS(string,o << expr)
#define MAKESTRFL(string,expr) MAKESTRTO(string,MAKESTR_FILE_LINE << expr)

// (obviously, no comma/newlines allowed in expr unless you protect them using above COMMA() macro)

//usage: string s=MAKESTRE(1<<" "<<c);
#define MAKESTRE(expr) ((dynamic_cast<std::ostringstream &>(std::ostringstream()<<std::dec<<expr)).str())
// std::dec (or seekp, or another manip) is needed to convert to std::ostream reference.

//usage: string s=OSTR(1<<" "<<c);
#define OSTR(expr) ((dynamic_cast<std::ostringstream &>(std::ostringstream()<<std::dec<<expr)).str())
#define OSTRF(f) ((dynamic_cast<std::ostringstream &>(f(std::ostringstream()<<std::dec))).str())
#define OSTRF1(f,x) ((dynamic_cast<std::ostringstream &>(f(std::ostringstream()<<std::dec,x))).str())
#define OSTRF2(f,x1,x2) ((dynamic_cast<std::ostringstream &>(f(std::ostringstream()<<std::dec,x1,x2))).str())
// std::dec (or seekp, or another manip) is needed to convert to std::ostream reference.

#ifndef COMMA
# define COMMA() ,
#endif


#ifdef GRAEHL_TEST
#include <graehl/shared/test.hpp>

BOOST_AUTO_TEST_CASE( makestring )
{
    std::string s,e;
# line 39
    MAKESTRFL(s,"asdf");BOOST_CHECK(s.find("makestr.hpp:39)asdf")!=std::string::npos);

    e="12";

    MAKESTRTO(s,"12");BOOST_CHECK(s==e);
    MAKESTRTO(s,1 << 2);BOOST_CHECK(s==e);
    MAKESTRTO(s,1;o << 2);BOOST_CHECK(s==e);

    //copy of MAKESTRS
//     BOOST_CHECK(MAKESTRE("12")==e);
//     BOOST_CHECK(MAKESTRE(1 << 2)==e);
//     BOOST_CHECK(MAKECSTRE('1' << '2')==e);
//     BOOST_CHECK(MAKECSTRE(1 << 2)==e);
}
#endif

#endif
