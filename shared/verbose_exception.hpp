// throw exceptions holding a message, and function/file/line info
#ifndef VERBOSE_EXCEPTION_HPP
#define VERBOSE_EXCEPTION_HPP

#include <exception>
#include <iostream>
#include <sstream>

namespace graehl {

struct verbose_exception : public std::exception
{
        const char *file;
        const char *function;
        unsigned line;
    std::string message;
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
        mbuf << function << "() [" << file << ":" << line << "]: " << m1 << m2 << ".";
        message=mbuf.str();
    }
    template <class M1,class M2,class M3>
    verbose_exception(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3) : file(fil),function(fun),line(lin) {
        std::stringstream mbuf;
        mbuf << function << "() [" << file << ":" << line << "]: " << m1 << m2 << m3 << ".";
        message=mbuf.str();
    }
    template <class M1,class M2,class M3,class M4>
    verbose_exception(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3,const M4 &m4) : file(fil),function(fun),line(lin) {
        std::stringstream mbuf;
        mbuf << function << "() [" << file << ":" << line << "]: " << m1 << m2 << m3 << m4 << ".";
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

#define VTHROW VTHROW_A(verbose_exception)
#define VTHROW_1(a1) VTHROW_A_1(verbose_exception,a1)
#define VTHROW_2(a1,a2) VTHROW_A_2(verbose_exception,a1,a2)
#define VTHROW_3(a1,a2,a3) VTHROW_A_3(verbose_exception,a1,a2,a3)

#define VERBOSE_EXCEPTION_WRAP(type) \
    type(const char *fun,const char *fil,unsigned lin) : graehl::verbose_exception(fun,fil,lin,#type ": ") {}                                                 \
    template <class M1> type(const char *fun,const char *fil,unsigned lin,const M1 &m1) : graehl::verbose_exception(fun,fil,lin,#type " ",m1) {}                  \
    template <class M1,class M2> type(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2) : graehl::verbose_exception(fun,fil,lin,#type " ",m1,m2) {}                       \
    template <class M1,class M2,class M3> type(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3) : graehl::verbose_exception(fun,fil,lin,#type " ",m1,m2,m3) {}


#endif
