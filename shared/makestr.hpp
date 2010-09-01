// preprocessor macros for building strings out of ostream << "a" << v << "b" - type expressions
#ifndef GRAEHL__SHARED__MAKESTR_HPP
#define GRAEHL__SHARED__MAKESTR_HPP

#include <sstream>
#include <stdexcept>

struct DefaultWriter
{
  template <class Ch, class Tr,class value_type>
        std::basic_ostream<Ch,Tr>&
         operator()(std::basic_ostream<Ch,Tr>& o,const value_type &l) const {
          return o << l;
         }
};

struct LineWriter
{
  template <class Ch, class Tr,class Label>
        std::basic_ostream<Ch,Tr>&
         operator()(std::basic_ostream<Ch,Tr>& o,const Label &l) const {
      return o << l << std::endl;
         }
};

// expr object that can participate in ostream exprs
template <class Ib,Ie>
struct range_printer {
};
template <class Ib,Ie>
range_printer<Ib,Ie> range_format(T begin,T end,

  template <class O, class T,class Writer> inline
  std::ios_base::iostate range_print_iostate(O& o,T begin, T end,Writer writer,bool multiline=false,bool parens=true,char open_paren='(',char close_paren=')')
  {
      static const char *const MULTILINE_SEP="\n";
      const char space=' ';
      if (parens) {
          o << open_paren;
          if (multiline)
              o << MULTILINE_SEP;
      }
      if (multiline) {
          for (;begin!=end;++begin) {
              o << space;
              deref(writer)(o,*begin);
              o << MULTILINE_SEP;
          }
      } else {
          word_spacer_c<space> sep;
          for (;begin!=end;++begin) {
              o << sep;
              deref(writer)(o,*begin);
          }
      }
      if (parens) {
          o << close_paren;
          if (multiline)
              o << MULTILINE_SEP;
      }
      return GENIOGOOD;
}

template <class Ch, class Tr, class T,class Writer> inline
std::basic_ostream<Ch,Tr> & range_print(std::basic_ostream<Ch,Tr>& o,T begin, T end,Writer writer,bool multiline=false,bool parens=true,char open_paren='(',char close_paren=')')
{
    range_print_iostate(o,begin,end,writer,multiline,parens,open_paren,close_paren);
    return o;
}

#define MAKESTR_STRINGIZE(str) #str
#define MAKESTR_EXPAND_STRINGIZE(expr) MAKESTR_STRINGIZE(expr)
#define MAKESTR_FILE_LINE "(" __FILE__  ":" MAKESTR_EXPAND_STRINGIZE(__LINE__)  ")"
#define MAKESTR_DATE __DATE__ " " __TIME__

// __FILE__  ":"  __LINE__  "): failed to write " #expr
// usage: MAKESTRS(str,os << 1;os << 2) => str="12"
#define MAKESTRS2(string,o,expr) do {std::ostringstream o;expr;if (!o) throw std::runtime_error(MAKESTR_FILE_LINE ": failed to write" #expr);string=o.str();}while(0)
#define MAKESTRS(string,expr) MAKESTRS2(string,o,expr)
// usage: MAKESTR(str,1 << 2) => str="12"
#define MAKESTR(string,expr) MAKESTRS(string,o,o << expr)
#define MAKESTRFL(string,expr) MAKESTR(string,MAKESTR_FILE_LINE << expr)

#define OSTR(expr) ((dynamic_cast<ostringstream &>(ostringstream()<<std::dec<<expr)).str())
#define OSTRF(f) ((dynamic_cast<ostringstream &>(f(ostringstream()<<std::dec))).str())
#define OSTRF1(f,x) ((dynamic_cast<ostringstream &>(f(ostringstream()<<std::dec,x))).str())
#define OSTRF2(f,x1,x2) ((dynamic_cast<ostringstream &>(f(ostringstream()<<std::dec,x1,x2))).str())

//DOESN'T WORK!
//#define MAKESTRE(expr) static_cast<std::ostringstream &>((*(std::ostringstream *)&std::ostringstream()) << "" <<  expr).str()
//TEST THIS BEFORE USING (note: MAKECSTRE is from a temporary object, so probably shouldn't even be used as a function argument)
#define MAKESTRE(expr) ( ((std::ostringstream&)(std::ostringstream() << expr)).str() )
#define MAKECSTRE(expr) MAKESTRE(expr).c_str()


// (obviously, no comma/newlines allowed in expr)


#ifdef TEST
#include <graehl/shared/test.hpp>

BOOST_AUTO_TEST_CASE( makestring )
{
    std::string s,e;
# line 39
    MAKESTRFL(s,"asdf");BOOST_CHECK(s.find("makestr.hpp:39)asdf")!=std::string::npos);

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
}
#endif

#endif
