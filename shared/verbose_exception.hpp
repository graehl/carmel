#ifndef _VERBOSE_EXCEPTION_HPP
#define _VERBOSE_EXCEPTION_HPP

#include <exception>
#include <iostream>
#include <sstream>

struct VerboseException : public std::exception
{
        const char *file;
        const char *function;
        unsigned line;
    std::string message;
    VerboseException(const char *fun,const char *fil,unsigned lin) : file(fil),function(fun),line(lin) {
        std::stringstream mbuf;
        mbuf << function << "() [" << file << ":" << line << "].";
        message=mbuf.str();
    }

    template <class M1>
    VerboseException(const char *fun,const char *fil,unsigned lin,const M1 &m1) : file(fil),function(fun),line(lin) {
        std::stringstream mbuf;
        mbuf << function << "() [" << file << ":" << line << "]: " << m1 << ".";
        message=mbuf.str();
    }
    template <class M1,class M2>
    VerboseException(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2) : file(fil),function(fun),line(lin) {
        std::stringstream mbuf;
        mbuf << function << "() [" << file << ":" << line << "]: " << m1 << m2 << ".";
        message=mbuf.str();
    }
    template <class M1,class M2,class M3>
    VerboseException(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2,const M3 &m3) : file(fil),function(fun),line(lin) {
        std::stringstream mbuf;
        mbuf << function << "() [" << file << ":" << line << "]: " << m1 << m2 << m3 << ".";
        message=mbuf.str();
    }
    virtual const char* what() const throw()
    {
        return message.c_str();
    }
};


#define VTHROW_A(type)  throw type(__FUNCTION__,__FILE__,__LINE__)
#define VTHROW_A_1(type,arg)  throw type(__FUNCTION__,__FILE__,__LINE__,arg)
#define VTHROW_A_2(type,arg,arg2)  throw type(__FUNCTION__,__FILE__,__LINE__,arg,arg2)

#define VTHROW_A(type)  throw type(__FUNCTION__,__FILE__,__LINE__)
#define VTHROW_A_1(type,arg)  throw type(__FUNCTION__,__FILE__,__LINE__,arg)
#define VTHROW_A_2(type,arg,arg2)  throw type(__FUNCTION__,__FILE__,__LINE__,arg,arg2)

#define VTHROW VTHROW_A(VerboseException)
#define VTHROW_1 VTHROW_A_1(VerboseException)
#define VTHROW_2 VTHROW_A_2(VerboseException)

#define VERBOSE_EXCEPTION_WRAP(type) \
    type(const char *fun,const char *fil,unsigned lin,const M1 &m1) : VerboseException(fun,fil,lin,#type " ") {}                       \
    template <class M1> type(const char *fun,const char *fil,unsigned lin,const M1 &m1) : VerboseException(fun,fil,lin,#type " ",m1) {}                     \
    template <class M1,class M2> type(const char *fun,const char *fil,unsigned lin,const M1 &m1,const M2 &m2) : VerboseException(fun,fil,lin,#type " ",m1,m2) {}


#endif
