#ifndef MEMORY_STREAM_HPP
#define MEMORY_STREAM_HPP

// (output only so far) stream that allocates its own variable sized buffer,
// like stringstream, but exposes the buffer to the user, like strstream ... the
// str() method returns a string by value (copy), though, just like
// stringstream.

#include <streambuf>
#include <ostream>
#include <string>
#include <utility>

namespace graehl {

template <typename cT,
          typename traits = std::char_traits<cT> >
class basic_memory_streambuf:
    public std::basic_streambuf<cT, traits>
{
 public:
    typedef cT value_type;
    typedef value_type *iterator;     //NOTE: you can count on iterator being a pointer
    typedef std::basic_streambuf<cT, traits> base;
    typedef typename base::int_type int_type;
    typedef std::streamsize size_type;
    
    using base::in_avail;
    using base::pptr;
    using base::pbump;
    using base::epptr;
    using base::gptr;
    using base::gbump;
    iterator begin() const 
    {
        return buffer;
    }
    iterator end() const
    {
        return pptr();
    }
    size_type size() const 
    {
        return end()-begin();
    }
    size_type capacity() const
    {
        return epptr()-begin();
    }
    bool full() const
    {
        return false;
    }
    
    const static size_type DEFAULT_BUFSIZE=
#ifdef GRAEHL_TEST
        1;
#else 
        64*1024;
#endif 
    const static unsigned GROW_RATIO=2; // could make float = 1.5
    basic_memory_streambuf() : buffer(new cT[DEFAULT_BUFSIZE])
    {
        setp(buffer, buffer + DEFAULT_BUFSIZE);
    }
    ~basic_memory_streambuf() { delete[] buffer; }
    value_type &back() const 
    {
        return *(end()-1);
    }    
    std::basic_string<cT, traits> str() const {
        return std::basic_string<cT, traits>(begin(), end());
    }
    cT *c_str()
    {
        if (*end()!=traits::eos()) {
            if (needsgrow())
                grow();
            *end()=traits::eos();
        }
        return begin();
    }
    void reset()
    {
        setp(buffer,epptr());
    }
 private:
    bool needsgrow() const
    {
        return end()==epptr();
    }
    void grow() 
    {
        size_type sz = capacity();
        size_type sz_bigger=GROW_RATIO*sz;
        cT* newbuffer = new cT[sz_bigger];
        traits::copy(newbuffer, begin(), size());
        safe_replace_buffer(newbuffer);
        setp(buffer+sz, buffer + sz_bigger);
    }
    
    int_type overflow(int_type c) {
        if (pptr() == epptr() && !traits::eq_int_type(c, traits::eof())) {
            grow();
            *end()=c;
            pbump(1);
        }
        return traits::not_eof(c);
    }
    void safe_replace_buffer(cT *newbuffer) 
    {
#if 1
        cT *oldbuffer=buffer;
        buffer=newbuffer; // assign this before deleting in case of exception
        delete[] oldbuffer;
#else 
        delete[] buffer; //if this throws, we have inconsistent state: buffer points at illegal place (and doesn't match with streambuf::setp
        buffer=newbuffer;
#endif        
    }
    
    cT* buffer;
};

typedef basic_memory_streambuf<char> memory_streambuf;
typedef basic_memory_streambuf<wchar_t> wmemory_streambuf;

template <typename cT,
          typename traits = std::char_traits<cT> >
class basic_memory_stream:
    public std::basic_ostream<cT, traits> {
    typedef basic_memory_streambuf<cT, traits> Sbuf;
 public:
    typedef std::basic_ostream<cT, traits> Base;
    typedef std::streamsize size_type;
    basic_memory_stream():
        std::basic_ostream<cT, traits>(&m_sbuf) {
    }
    std::basic_string<cT, traits> str() const {
        return m_sbuf.str();
    }
    
    typedef cT value_type;
    typedef value_type *iterator; //NOTE: you can count on iterator being a pointer
    iterator begin() const 
    {
        return m_sbuf.begin();
    }
    iterator end() const
    {
        return m_sbuf.end();
    }
    size_type size() const 
    {
        return m_sbuf.size();
    }
    void reset()
    {
        return m_sbuf.reset();
        Base::clear();
    }
    Sbuf &buf() 
    {
        return m_sbuf;
    }
 private:
    Sbuf m_sbuf;
};

typedef basic_memory_stream<char>    memory_stream;
typedef basic_memory_stream<wchar_t> wmemory_stream; 

#ifdef GRAEHL_TEST
# include "array_stream.hpp"
BOOST_AUTO_TEST_CASE( TEST_memory_stream )
{
    const unsigned N=memory_streambuf::DEFAULT_BUFSIZE*2+4;
    char buf[N];
    memory_stream o1;
//    TEST_check_memory_stream(o1,buf,N);    
}

#endif

}

#endif
