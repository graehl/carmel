/** \file

    throw exceptions holding a message, and function/file/line info.
*/

#ifndef GRAEHL_SHARED__VERBOSE_EXCEPTION_HPP
#define GRAEHL_SHARED__VERBOSE_EXCEPTION_HPP

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <string>

namespace graehl {

// no line numbers etc. init w/ a string. if you want non-string, try THROW_MSG(type,a<<b)
#define SIMPLE_EXCEPTION_PREFIX(ExceptionClass,msgPre) \
struct ExceptionClass : public std::exception \
{ \
  std::string message; \
  ExceptionClass(std::string const& m="error") : message(msgPre+m) {} \
  ExceptionClass(ExceptionClass const& o) : message(o.message) {} \
  ~ExceptionClass() throw() {} \
  const char* what() const throw() { return message.c_str(); } \
}

#define SIMPLE_EXCEPTION_CLASS(ExceptionClass) SIMPLE_EXCEPTION_PREFIX(ExceptionClass,#ExceptionClass ": ")

struct verbose_exception : public std::exception
{
  const char *file;
  const char *function;
  unsigned line;
  std::string message;
  verbose_exception() : file(""),function(""),line(),message("unspecified verbose_exception") {}
  verbose_exception(char const* m) : file(""),function(""),line(),message(m) {}
  verbose_exception(std::string const& m) : file(""),function(""),line(),message(m) {}
  verbose_exception(verbose_exception const& o) : file(o.file),function(o.function),line(o.line),message(o.message) {}
#if 0
# ifndef WIN32
  // causes problems in VS 2010
  template <class M1>
  verbose_exception(M1 const& m) : file(""),function(""),line()
  {
    std::stringstream mbuf;
    mbuf<<m;
    message=mbuf.str();
  }
# endif
#endif
  verbose_exception(const char *fun,const char *fil,unsigned lin) : file(fil),function(fun),line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "].";
    message=mbuf.str();
  }
  template <class M1>
  verbose_exception(const char *fun,const char *fil,unsigned lin,const M1 &m1) : file(fil),function(fun),line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ".";
    message=mbuf.str();
  }
  template <class M1,class M2>
  verbose_exception(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2) : file(fil),function(fun),line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ".";
    message=mbuf.str();
  }
  template <class M1,class M2,class M3>
  verbose_exception(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3) : file(fil),function(fun),line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ' ' << m3 << ".";
    message=mbuf.str();
  }
  template <class M1,class M2,class M3,class M4>
  verbose_exception(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3,const M4 &m4) : file(fil),function(fun),line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ' ' << m3 << ' ' << m4 << ".";
    message=mbuf.str();
  }
  template <class M1,class M2,class M3,class M4,class M5>
  verbose_exception(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3,const M4 &m4,const M5 &m5) : file(fil),function(fun),line(lin) {
    std::stringstream mbuf;
    mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ' ' << m2 << ' ' << m3 << ' ' << m4 << ' ' << m5 << ".";
    message=mbuf.str();
  }
  ~verbose_exception() throw()
  {
  }
  const char* what() const throw()
  {
    return message.c_str();
  }
};

}


#define VTHROW_A(type)  throw type(__FUNCTION__,__FILE__,__LINE__)
#define VTHROW_A_1(type,arg)  throw type(__FUNCTION__,__FILE__,__LINE__,arg)
#define VTHROW_A_2(type,arg,arg2)  throw type(__FUNCTION__,__FILE__,__LINE__,arg,arg2)
#define VTHROW_A_3(type,arg,arg2,arg3)  throw type(__FUNCTION__,__FILE__,__LINE__,arg,arg2,arg3)
#define VTHROW_A_4(type,arg,arg2,arg3,arg4)  throw type(__FUNCTION__,__FILE__,__LINE__,arg,arg2,arg3,arg4)

#define VTHROW VTHROW_A(graehl::verbose_exception)
#define VTHROW_1(a1) VTHROW_A_1(graehl::verbose_exception,a1)
#define VTHROW_2(a1,a2) VTHROW_A_2(graehl::verbose_exception,a1,a2)
#define VTHROW_3(a1,a2,a3) VTHROW_A_3(graehl::verbose_exception,a1,a2,a3)
#define VTHROW_4(a1,a2,a3,a4) VTHROW_A_4(graehl::verbose_exception,a1,a2,a3,a4)

#define THROW_MSG(type,msg) do { std::stringstream out; out<<msg; throw type(out.str()); } while(0)
#define VTHROW_A_MSG(type,msg) do { std::stringstream out; out<<msg; throw type(__FUNCTION__,__FILE__,__LINE__,out.str()); } while(0)
#define VTHROW_MSG(msg) VTHROW_A_MSG(graehl::verbose_exception,msg)

#define VERBOSE_EXCEPTION_WRAP(etype)                                                                            \
  etype(std::string const& msg=#etype)                                                                            \
    : graehl::verbose_exception(msg) {}                                                                         \
  etype(const char *fun,const char *fil,unsigned lin)                                                            \
    : graehl::verbose_exception(fun,fil,lin,#etype ": ") {}                                                      \
  template <class M1> etype(const char *fun,const char *fil,unsigned lin,const M1 &m1)                           \
    : graehl::verbose_exception(fun,fil,lin,#etype " ",m1) {}                                                    \
  template <class M1,class M2> etype(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2)     \
    : graehl::verbose_exception(fun,fil,lin,#etype " ",m1,m2) {}                                                 \
  template <class M1,class M2,class M3>                                                                         \
  etype(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3)                     \
    : graehl::verbose_exception(fun,fil,lin,#etype " ",m1,m2,m3) {}                                              \
  template <class M1,class M2,class M3,class M4>                                                                \
  etype(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3,const M4 &m4)        \
    : graehl::verbose_exception(fun,fil,lin,#etype " ",m1,m2,m3,m4) {}

#define VERBOSE_EXCEPTION_DECLARE(etype) struct etype : graehl::verbose_exception { VERBOSE_EXCEPTION_WRAP(etype) };


#endif
